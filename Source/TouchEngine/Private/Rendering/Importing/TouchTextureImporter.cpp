/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "Rendering/Importing/TouchTextureImporter.h"
#include "RenderingThread.h"
#include "Logging.h"
#include "Rendering/Importing/ITouchImportTexture.h"
#include "Rendering/TouchResourceProvider.h"

#include "Algo/ForEach.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/Util/TouchFrameCooker.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/TouchHelpers.h"

namespace UE::TouchEngine
{
	FTouchTextureImporter::~FTouchTextureImporter()
	{
		UE_LOG(LogTouchEngine, Verbose, TEXT("Shutting down ~FTouchTextureLinker"));

		// We don't need to care about removing textures from root set in this situation (in fact scheduling a game thread task will not work)
		if (IsEngineExitRequested())
		{
			return;
		}
		
		TArray<UTexture*> TexturesToCleanUp;
		{
			FScopeLock Lock(&LinkDataMutex);
			for (const TPair<FName, FTouchTextureLinkData>& Data : LinkData)
			{
				TexturesToCleanUp.Add(Data.Value.UnrealTexture);
			}
		}
		{
			FScopeLock PoolLock(&TexturePoolMutex);
			for (FImportedTexturePoolData& Data : TexturePool)
			{
				TexturesToCleanUp.Add(Data.UETexture);
			}
		}

		ExecuteOnGameThread<void>([TexturesToCleanUp]()
		{
			Algo::ForEach(TexturesToCleanUp, [](UTexture* Texture)
			{
				if (IsValid(Texture))
				{
					Texture->RemoveFromRoot();
				}
			});
		});
	}
	
	TFuture<FTouchTextureImportResult> FTouchTextureImporter::ImportTexture_AnyThread(const FTouchImportParameters& LinkParams, const TSharedPtr<FTouchFrameCooker>& FrameCooker)
	{
		if (TaskSuspender.IsSuspended())
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("FTouchTextureLinker is suspended. Your task will be ignored."));
			return MakeFulfilledPromise<FTouchTextureImportResult>(FTouchTextureImportResult{ EImportResultType::Cancelled }).GetFuture();
		}

		// Here we just want to start the texture transfer, but this will be processed later on in the render thread
		LinkParams.GetTextureTransferResult = GetTextureTransfer(LinkParams);
		
		FTaskSuspender::FTaskTracker TaskToken = TaskSuspender.StartTask();
		TPromise<FTouchTextureImportResult> Promise;
		TFuture<FTouchTextureImportResult> Future = Promise.GetFuture()
			.Next([TaskToken](auto Result)
			{
				return Result;
			});

		bool bIsInProgress;
		{
			FScopeLock Lock(&LinkDataMutex);
			const FTouchTextureLinkData& TextureLinkData = LinkData.FindOrAdd(LinkParams.Identifier);
			bIsInProgress = TextureLinkData.bIsInProgress;
		}
		
		if (ensure(!bIsInProgress))
		{
			// Lock.Unlock();
			ExecuteLinkTextureRequest_AnyThread(MoveTemp(Promise), LinkParams, FrameCooker);
		}
		else // with the way the flow has been refactored, this is not supposed to happen anymore
		{
			// EnqueueLinkTextureRequest(TextureLinkData, MoveTemp(Promise), LinkParams);
			Promise.SetValue(FTouchTextureImportResult::MakeCancelled());
			// Lock.Unlock();
		}
		
		return Future;
	}

	TFuture<FTouchSuspendResult> FTouchTextureImporter::SuspendAsyncTasks()
	{
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();

		ENQUEUE_RENDER_COMMAND(FinishRemainingTasks)([ThisPin = SharedThis(this), Promise = MoveTemp(Promise)](FRHICommandListImmediate& RHICmdList) mutable
		{
			// {
			// 	FScopeLock Lock(&ThisPin->LinkDataMutex);
			// 	for (TPair<FName, FTouchTextureLinkData>& Data : ThisPin->LinkData)
			// 	{
			// 		if (Data.Value.ExecuteNext)
			// 		{
			// 			Data.Value.ExecuteNext->SetValue(FTouchTextureImportResult{ EImportResultType::Cancelled });
			// 			Data.Value.ExecuteNext.Reset();
			// 		}
			// 	}
			// }
			ThisPin->TaskSuspender.Suspend().Next([Promise = MoveTemp(Promise)](auto) mutable
			{
				Promise.SetValue({});
			});
		});
		return Future; ;
	}

	void FTouchTextureImporter::TexturePoolMaintenance(const FTouchEngineInputFrameData& FrameData)
	{
		FScopeLock PoolLock(&TexturePoolMutex);
		while (TexturePool.Num() > PoolSize)
		{
			FImportedTexturePoolData& TextureData = TexturePool[0]; // we would remove from the front as they have been here the longest
			if (IsValid(TextureData.UETexture))
			{
				if (TextureData.PooledFrameID == FrameData.FrameID)
				{
					break; //if we reach a texture added this frame, we know the next textures will also have been added this frame so we stop removing from the pool
				}
				// as we might create a lot of textures and the GC might take some time to kick in, we expedite some of the cleaning
				TextureData.UETexture->RemoveFromRoot();
				TextureData.UETexture->TextureReference.TextureReferenceRHI.SafeRelease();
				TextureData.UETexture->ReleaseResource(); 
				TextureData.UETexture->ConditionalBeginDestroy();
			}
			TexturePool.RemoveAt(0);
		}
		SET_DWORD_STAT(STAT_TE_ImportedTexturePool_NbTexturesPool, TexturePool.Num())
	}

	bool FTouchTextureImporter::RemoveUTextureFromPool(UTexture2D* Texture)
	{
		if (!IsValid(Texture))
		{
			return false;
		}
		
		FScopeLock Lock(&LinkDataMutex);
		for(TTuple<FName, FTouchTextureLinkData>& Data : LinkData)
		{
			if (Data.Value.UnrealTexture == Texture)
			{
				Texture->RemoveFromRoot(); // this is enough to ensure it will not be put back in the pool, as we are checking for this
				return true;
			}
		}

		return false;
	}

	TEResult FTouchTextureImporter::GetTextureTransfer(const FTouchImportParameters& ImportParams)
	{
		ImportParams.GetTextureTransferResult = TEInstanceGetTextureTransfer(ImportParams.Instance, ImportParams.TETexture, ImportParams.GetTextureTransferSemaphore.take(), &ImportParams.GetTextureTransferWaitValue);
		return ImportParams.GetTextureTransferResult;
	}
	
	void FTouchTextureImporter::ExecuteLinkTextureRequest_AnyThread(TPromise<FTouchTextureImportResult>&& Promise, const FTouchImportParameters& LinkParams, const TSharedPtr<FTouchFrameCooker>& FrameCooker)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.1 [AT] Link Texture Import"), STAT_TE_III_A_1, STATGROUP_TouchEngine);
		// At this point, we are neither on the GameThread nor on the RenderThread, we are on a parallel thread.
		// A UTexture2D can only be created on the GameThread (especially calling UTexture2D::UpdateResource) so if we need to create one, we will need to send the necessary information to the GameThread.
		// In synchronised mode though, we are already waiting on the cook promise at this point, so we cannot create it right now.
		// Once the TEEventFrameDidFinish occurs, the GameThread will restart and we will create them at that point and update the  before being able to call GetOutputs.

		// 2. Check if we already have a UTexture that could hold the data from TouchEngine, or enqueue one to be created on GameThread
		const FName Identifier = LinkParams.Identifier;
		FTextureCreationFormat TextureFormat;
		TextureFormat.Identifier = Identifier;
		TextureFormat.OnTextureCreated = MakeShared<TPromise<UTexture2D*>>(); // This promise will be set from the GameThread, unless we have a UTexture available or if we have an issue
		TFuture<UTexture2D*> OnTextureCreated = TextureFormat.OnTextureCreated->GetFuture(); // Creating this Future as TextureFormat might be moved

		const FTextureMetaData LinkMetaData = GetTextureMetaData(LinkParams.TETexture);
		TextureFormat.SizeX = LinkMetaData.SizeX;
		TextureFormat.SizeY = LinkMetaData.SizeY;
		TextureFormat.PixelFormat = LinkMetaData.PixelFormat;

		// 1. Create the SharedTETexture to copy from
		TPromise<TSharedPtr<ITouchImportTexture>> CreateSharedTETexturePromise;
		TFuture<TSharedPtr<ITouchImportTexture>> CreateSharedTETextureResult = CreateSharedTETexturePromise.GetFuture();
		ENQUEUE_RENDER_COMMAND(CreatePlatformTexture)([WeakThis = AsWeak(), LinkParams, CreateSharedTETexturePromise = MoveTemp(CreateSharedTETexturePromise)](FRHICommandListImmediate& RHICmdList) mutable
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.2 [RT] Link Texture Import - CreateSharedTETexture"), STAT_TE_III_A_2, STATGROUP_TouchEngine);
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ThisPin || ThisPin->TaskSuspender.IsSuspended())
			{
				CreateSharedTETexturePromise.SetValue(nullptr);
				return;
			}
			// 1. We get the source texture sent by TouchEngine
			const TSharedPtr<ITouchImportTexture> SharedTETexture = ThisPin->CreatePlatformTexture_RenderThread(LinkParams.Instance, LinkParams.TETexture);
			CreateSharedTETexturePromise.SetValue(SharedTETexture);
		}); // ~ENQUEUE_RENDER_COMMAND(CreateSharedTETexture)

		TPromise<FTexture2DRHIRef> CreateUTextureRHIPromise;
		TFuture<FTexture2DRHIRef> CreateUTextureRHIResult = CreateUTextureRHIPromise.GetFuture();
		
		bool bNeedUTextureResourceSwap = false;
		{
			FScopeLock Lock(&LinkDataMutex);
			// const FTouchTextureLinkData& TextureLinkData = LinkData[Identifier];
			if (!ensure(LinkMetaData.PixelFormat != PF_Unknown))
			{
				UE_LOG(LogTouchEngine, Error, TEXT("[FTouchTextureImporter::ExecuteLinkTextureRequest_AnyThread[%s]] The PlatformMetadata has an unknown Pixel format `%s` for parameter `%s` for frame `%lld`"),
					   *GetCurrentThreadStr(), GetPixelFormatString(LinkMetaData.PixelFormat), *Identifier.ToString(), LinkParams.FrameData.FrameID);

				// PlatformTextureLinkJob.ErrorCode = ETouchLinkErrorCode::FailedToCreatePlatformTexture;
				CreateUTextureRHIPromise.SetValue(nullptr);
				TextureFormat.OnTextureCreated->SetValue(nullptr);
			}
			else if (UTexture2D* PoolTexture = FindPoolTextureMatchingMetadata(LinkMetaData, LinkParams.FrameData)) // if the UTexture and the TE Texture matches size and format, copy straight into the UTexture resource
			{
				ENQUEUE_RENDER_COMMAND(GetUTextureRHI)([PoolTexture, CreateUTextureRHIPromise = MoveTemp(CreateUTextureRHIPromise)](FRHICommandListImmediate& RHICmdList) mutable
				{
					const FTextureReferenceRHIRef RHI = IsValid(PoolTexture) ? PoolTexture->TextureReference.TextureReferenceRHI : nullptr;
					CreateUTextureRHIPromise.SetValue(static_cast<TRefCountPtr<FRHITexture>>(RHI));
				}); // ~ENQUEUE_RENDER_COMMAND(GetUTextureRHI)
				TextureFormat.OnTextureCreated->SetValue(PoolTexture);
			}
			// else if (CanCopyIntoUTexture(LinkMetaData, TextureLinkData.UnrealTexture)) // if the UTexture and the TE Texture matches size and format, copy straight into the UTexture resource
			// {
			// 	UE_LOG(LogTouchEngine, Log, TEXT("[FTouchTextureImporter::ExecuteLinkTextureRequest_AnyThread[%s]] Reusing existing UTexture `%s` for parameter `%s`: %dx%d [%s] for frame `%lld`"),
			// 		*GetCurrentThreadStr(), *TextureLinkData.UnrealTexture->GetName(), *Identifier.ToString(), LinkMetaData.SizeX, LinkMetaData.SizeY, GetPixelFormatString(LinkMetaData.PixelFormat), LinkParams.FrameData.FrameID);
			//
			// 	// We cannot access the RHI of the UTexture via GetResource because we might be in an unknown thread. We end up creating a new one and linking it on GameThread
			// 	ENQUEUE_RENDER_COMMAND(GetUTextureRHI)([Texture = TextureLinkData.UnrealTexture, CreateUTextureRHIPromise = MoveTemp(CreateUTextureRHIPromise)](FRHICommandListImmediate& RHICmdList) mutable
			// 	{
			// 		// CreateUTextureRHIPromise.SetValue(IsValid(Texture) ? Texture->GetResource()->TextureRHI->GetTexture2D() : nullptr);
			// 		const FTextureReferenceRHIRef RHI = IsValid(Texture) ? Texture->TextureReference.TextureReferenceRHI : nullptr;
			// 		CreateUTextureRHIPromise.SetValue(static_cast<TRefCountPtr<FRHITexture>>(RHI));
			// 	}); // ~ENQUEUE_RENDER_COMMAND(GetUTextureRHI)
			// 	TextureFormat.OnTextureCreated->SetValue(TextureLinkData.UnrealTexture);
			// }
			else // otherwise we need to create a new resource
			{
				UE_LOG(LogTouchEngine, Log, TEXT("[FTouchTextureImporter::ExecuteLinkTextureRequest_AnyThread[%s]] Need to create new UTexture for parameter `%s`: %dx%d [%s] for frame `%lld`"),
					   *GetCurrentThreadStr(), *Identifier.ToString(), LinkMetaData.SizeX, LinkMetaData.SizeY, GetPixelFormatString(LinkMetaData.PixelFormat), LinkParams.FrameData.FrameID);

				ENQUEUE_RENDER_COMMAND(CreateUTextureRHI)([CreateUTextureRHIPromise = MoveTemp(CreateUTextureRHIPromise), LinkMetaData](FRHICommandListImmediate& RHICmdList) mutable
				{
					DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.3 [RT] Link Texture Import - CreateUTextureRHI"), STAT_TE_III_A_3, STATGROUP_TouchEngine);

					const FRHITextureCreateDesc TextureDesc =
							FRHITextureCreateDesc::Create2D(TEXT("RHIAsyncCreateTexture2D"))
							.SetExtent(FIntPoint(LinkMetaData.SizeX, LinkMetaData.SizeY))
							.SetFormat(LinkMetaData.PixelFormat)
							// .SetFlags(TexCreate_ShaderResource)
							.SetNumMips(1)
							.SetInitialState(ERHIAccess::SRVMask);
					const FTexture2DRHIRef SharedTextureRHI = RHICreateTexture(TextureDesc); //todo: look at Dx12 and maybe use RHICreateTexture2DFromResource
				
					CreateUTextureRHIPromise.SetValue(SharedTextureRHI.IsValid() ? SharedTextureRHI : nullptr);
				}); // ~ENQUEUE_RENDER_COMMAND(CreateUTextureRHI)

				bNeedUTextureResourceSwap = true;
				// if (TextureLinkData.UnrealTexture) // if we already have a UTexture, we will just replace its render resource
				// {
				// 	TextureFormat.OnTextureCreated->SetValue(TextureLinkData.UnrealTexture);
				// }
				// else // otherwise we need one created on the GameThread
				// {
					FrameCooker->AddTextureToCreateOnGameThread(MoveTemp(TextureFormat));
				// }
			}
		}

		TPromise<FTexture2DRHIRef> CopyTexturePromise;
		TFuture<FTexture2DRHIRef> CopyTextureResult = CopyTexturePromise.GetFuture();
		ENQUEUE_RENDER_COMMAND(CopyRHI)([WeakThis = AsWeak(), CreateSharedTETextureResult = MoveTemp(CreateSharedTETextureResult),
			CreateUTextureRHIResult = MoveTemp(CreateUTextureRHIResult), CopyTexturePromise = MoveTemp(CopyTexturePromise), LinkParams](FRHICommandListImmediate& RHICmdList) mutable
		{
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ThisPin || ThisPin->TaskSuspender.IsSuspended())
			{
				return;
			}
			
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.4 [RT] Link Texture Import - CopyRHI"), STAT_TE_III_A_4, STATGROUP_TouchEngine);

			{
				// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("III.A.4.a [RT] Link Texture Import - WaitSharedTETexture"), STAT_TE_III_A_4_a, STATGROUP_TouchEngine);
				CreateSharedTETextureResult.Wait();
			}
			{
				// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("III.A.4.b [RT] Link Texture Import - WaitUTexture"), STAT_TE_III_A_4_b, STATGROUP_TouchEngine);
				CreateUTextureRHIResult.Wait();
			}
			const TSharedPtr<ITouchImportTexture> PlatformTexture = CreateSharedTETextureResult.Get();
			const TRefCountPtr<FRHITexture> UTextureRHI = CreateUTextureRHIResult.Get();
			if (!PlatformTexture || !UTextureRHI)
			{
				CopyTexturePromise.SetValue(nullptr);
				return;
			}
			// 2. We create a destination UTexture RHI if we don't have one already
			const FTouchCopyTextureArgs CopyArgs { LinkParams, RHICmdList, nullptr, UTextureRHI}; //,  DestUETextureRHI};

			ThisPin->CopyNativeToUnreal_RenderThread(PlatformTexture, CopyArgs, LinkParams);
			CopyTexturePromise.SetValue(UTextureRHI);
		}); // ~ENQUEUE_RENDER_COMMAND(CopyRHI)
		
		TFuture<UTexture2D*> TextureCreationOperation = OnTextureCreated.Next(
		[WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), LinkParams, bNeedUTextureResourceSwap, CopyTextureResult = MoveTemp(CopyTextureResult)](UTexture2D* Texture) mutable
		{
			const FName& Identifier = LinkParams.Identifier;
			const FTouchEngineInputFrameData& FrameData = LinkParams.FrameData;
			
			UE_LOG(LogTouchEngine, Verbose, TEXT("[FTouchTextureImporter::ExecuteLinkTextureRequest_AnyThread->OnTextureCreated[%s]] UTexture %screated for parameter `%s` for frame `%lld`"),
				*GetCurrentThreadStr(), Texture ? TEXT("") : TEXT("NOT "), *Identifier.ToString(), FrameData.FrameID);

			if (Texture && bNeedUTextureResourceSwap)
			{
				// And we update the Texture Reference to the given Texture2DRHI on the render thread
				ENQUEUE_RENDER_COMMAND(CopyTexture)([WeakThis, LinkParams, Texture, CopyTextureResult = MoveTemp(CopyTextureResult)](FRHICommandListImmediate& RHICmdList)
				{
					CopyTextureResult.Wait();
					DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.5 [RT] Link Texture Import - RHIUpdateTextureReference"), STAT_TE_III_A_5, STATGROUP_TouchEngine);
					
					const FTexture2DRHIRef NewRHITexture = CopyTextureResult.Get();
					RHIUpdateTextureReference(Texture->TextureReference.TextureReferenceRHI, NewRHITexture);
					//todo: the below might need updating if we start having to handle multiple mips
					Texture->GetPlatformData()->SizeX = NewRHITexture ? NewRHITexture->GetDesc().Extent.X : 0;
					Texture->GetPlatformData()->SizeY = NewRHITexture ? NewRHITexture->GetDesc().Extent.Y : 0;
					if(!Texture->GetPlatformData()->Mips.IsEmpty())
					{
						FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
						Mip.SizeX = Texture->GetPlatformData()->SizeX;
						Mip.SizeY = Texture->GetPlatformData()->SizeY;
					}
				}); // ~ENQUEUE_RENDER_COMMAND(CopyTexture)
			}
			
			if (const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin())
			{
				FScopeLock Lock(&ThisPin->LinkDataMutex);
				FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData[Identifier];
				// if (UTexture2D* PreviousTexture = TextureLinkData.UnrealTexture) // if we have a previous texture we generated for this parameter, we can add it to the pool
				// {
				// 	if (PreviousTexture != Texture)
				// 	{
				// 		// PreviousTexture->RemoveFromRoot();
				// 		{
				// 			FScopeLock PoolLock(&ThisPin->TexturePoolMutex);
				// 			ThisPin->TexturePool.Add({FrameData.FrameID, PreviousTexture});
				// 		}
				// 		TextureLinkData.UnrealTexture = Texture;
				// 	}
				// }
				// else
				// {
					TextureLinkData.UnrealTexture = Texture;
				// }
				// SET_DWORD_STAT(STAT_TENoTexture2d, ThisPin->AllImportedTextures.Num())
				// INC_DWORD_STAT_BY(STAT_TENoTexture2dRemoved, NoRemoved)
			}
			return Texture;
		});
	
		// 5. Then we start the next one if there is one enqueued. //todo: when does this happen?
		TextureCreationOperation.Next([WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), Promise = MoveTemp(Promise), LinkParams, FrameCooker](UTexture2D* UnrealTexture) mutable
		{
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ThisPin || ThisPin->TaskSuspender.IsSuspended())
			{
				Promise.SetValue(FTouchTextureImportResult::MakeCancelled());
				return;
			}
			// start the next enqueued copy. todo: when does this happen?
			// TOptional<TPromise<FTouchTextureImportResult>> ExecuteNext;
			// FTouchImportParameters ExecuteNextParams;
			{
				FScopeLock Lock(&ThisPin->LinkDataMutex);
				FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData.FindOrAdd(LinkParams.Identifier);
			   // if (TextureLinkData.ExecuteNext.IsSet())
			   // {
				  //  ExecuteNext = MoveTemp(TextureLinkData.ExecuteNext);
				  //  TextureLinkData.ExecuteNext.Reset();
				  //  ExecuteNextParams = TextureLinkData.ExecuteNextParams;
			   // }
			   TextureLinkData.bIsInProgress = false;
			}

			// if (ExecuteNext.IsSet()) //todo: when does this happen?
			// {
			// 	ThisPin->ExecuteLinkTextureRequest_AnyThread(MoveTemp(*ExecuteNext), ExecuteNextParams, FrameCooker);
			// }

			const bool bFailure = !ensure(UnrealTexture != nullptr);
			if (bFailure)
			{
				Promise.SetValue(FTouchTextureImportResult::MakeFailure());
			}
			else
			{
				// Here, we want to make sure the previous texture would be put back in the pool.
				TSharedPtr<TPromise<UTexture2D*>> PreviousTextureToBePooledPromise = MakeShared<TPromise<UTexture2D*>>();
				TFuture<UTexture2D*> PreviousTextureToBePooledResult = PreviousTextureToBePooledPromise->GetFuture();

				Promise.SetValue( FTouchTextureImportResult::MakeSuccessful(UnrealTexture, MoveTemp(PreviousTextureToBePooledPromise)));
				
				PreviousTextureToBePooledResult.Next([WeakThis, LinkParams](UTexture2D* PreviousTextureToBePooled)
				{
					if (!IsValid(PreviousTextureToBePooled))
					{
						return;
					}
					
					const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
					if (ThisPin && !ThisPin->TaskSuspender.IsSuspended())
					{
						if (PreviousTextureToBePooled->IsRooted()) // if the texture is not rooted, we have been asked to remove it from the set, see RemoveUTextureFromPool
						{
							FScopeLock PoolLock(&ThisPin->TexturePoolMutex);
							ThisPin->TexturePool.Add({LinkParams.FrameData.FrameID, PreviousTextureToBePooled});
						}
					}
					else
					{
						PreviousTextureToBePooled->RemoveFromRoot();
					}
				});
			}
		});
	}

	UTexture2D* FTouchTextureImporter::FindPoolTextureMatchingMetadata(const FTextureMetaData& TETextureMetadata, const FTouchEngineInputFrameData& FrameData)
	{
		UTexture2D* PooledTexture = nullptr;
		
		FScopeLock PoolLock(&TexturePoolMutex);
		for (int i = 0; i < TexturePool.Num(); ++i)
		{
			FImportedTexturePoolData& TextureData = TexturePool[i];
			if (TextureData.PooledFrameID >= FrameData.FrameID || !IsValid(TextureData.UETexture))
			{
				// if the texture was pooled this frame, we do not return it as it could still be in use
				continue;
			}
			if (CanCopyIntoUTexture(TETextureMetadata, TextureData.UETexture))
			{
				PooledTexture = TextureData.UETexture;
				TexturePool.RemoveAt(i);
				break;
			}
		}

		return PooledTexture;
	}

	void FTouchTextureImporter::CopyNativeToUnreal_RenderThread(const TSharedPtr<ITouchImportTexture>& TETexture, const FTouchCopyTextureArgs& CopyArgs, const FTouchImportParameters& LinkParams)
	{
		const ECopyTouchToUnrealResult Result = TETexture->CopyNativeToUnrealRHI_RenderThread(CopyArgs, AsShared());
		
		const bool bSuccessfulCopy = Result == ECopyTouchToUnrealResult::Success;
		ETouchLinkErrorCode ErrorCode = bSuccessfulCopy ? ETouchLinkErrorCode::Success : ETouchLinkErrorCode::FailedToCopyResources;
		if (bSuccessfulCopy)
		{
			UE_LOG(LogTouchEngine, Log, TEXT("   [FTouchTextureImporter::CopyTexture_AnyThread] Successfully copied Texture to Unreal Engine for parameter [%s] for frame `%lld`"),*LinkParams.Identifier.ToString(), LinkParams.FrameData.FrameID)
		}
		else
		{
			UE_LOG(LogTouchEngine, Error, TEXT("   [FTouchTextureImporter::CopyTexture_AnyThread] UNSUCCESSFULLY copied Texture to Unreal Engine for parameter [%s] for frame `%lld`"),*LinkParams.Identifier.ToString(), LinkParams.FrameData.FrameID)
		}	
	}

	// void FTouchTextureImporter::EnqueueLinkTextureRequest(FTouchTextureLinkData& TextureLinkData, TPromise<FTouchTextureImportResult>&& NewPromise, const FTouchImportParameters& LinkParams)
	// {
	// 	if (TextureLinkData.ExecuteNext.IsSet())
	// 	{
	// 		TextureLinkData.ExecuteNext->SetValue(FTouchTextureImportResult::MakeCancelled());
	// 	}
	// 	TextureLinkData.ExecuteNextParams = LinkParams;
	// 	TextureLinkData.ExecuteNext = MoveTemp(NewPromise);
	// }

	// void FTouchTextureImporter::EnqueueTextureCopy_AnyThread(FTouchTextureLinkJob&& PlatformTextureLinkJob, FTexture2DRHIRef DestUETextureRHI)
	// {
	// 	ENQUEUE_RENDER_COMMAND(CopyRHITexture)([WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), LinkJob = MoveTemp(PlatformTextureLinkJob), DestUETextureRHI](FRHICommandListImmediate& RHICmdList) mutable
	// 	{
	// 		check(LinkJob.ErrorCode == ETouchLinkErrorCode::Success);
	// 		
	// 		const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
	// 		if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks"))
	// 			|| ThisPin->TaskSuspender.IsSuspended())
	// 		{
	// 			LinkJob.ErrorCode = ETouchLinkErrorCode::Cancelled;
	// 			return;
	// 		}
	//
	// 		const FTouchCopyTextureArgs CopyArgs { LinkJob.RequestParams, RHICmdList, nullptr,  DestUETextureRHI};
	// 		
	// 		ECopyTouchToUnrealResult Result = LinkJob.PlatformTexture->CopyNativeToUnrealRHI_RenderThread(CopyArgs, ThisPin.ToSharedRef());
	// 		
	// 		const bool bSuccessfulCopy = Result == ECopyTouchToUnrealResult::Success;
	// 		ETouchLinkErrorCode ErrorCode = bSuccessfulCopy ? ETouchLinkErrorCode::Success : ETouchLinkErrorCode::FailedToCopyResources;
	// 		if (bSuccessfulCopy)
	// 		{
	// 			UE_LOG(LogTouchEngine, Log, TEXT("   [FTouchTextureImporter::CopyTexture_AnyThread] Successfully copied Texture to Unreal Engine for parameter [%s] for frame `%lld`"),*LinkJob.RequestParams.Identifier.ToString(), LinkJob.RequestParams.FrameData.FrameID)
	// 		}
	// 		else
	// 		{
	// 			UE_LOG(LogTouchEngine, Error, TEXT("   [FTouchTextureImporter::CopyTexture_AnyThread] UNSUCCESSFULLY copied Texture to Unreal Engine for parameter [%s] for frame `%lld`"),*LinkJob.RequestParams.Identifier.ToString(), LinkJob.RequestParams.FrameData.FrameID)
	// 		}
	// 	});
	// }
	//
	// TFuture<FTouchTextureLinkJob> FTouchTextureImporter::CopyTexture_AnyThread(TFuture<FTouchTextureLinkJob>&& ContinueFrom)
	// {
	// 	//todo: this could be simplified
	// 	TPromise<FTouchTextureLinkJob> Promise;
	// 	TFuture<FTouchTextureLinkJob> Result = Promise.GetFuture();
	// 	ContinueFrom.Next([WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
	// 	{
	// 		const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
	// 		if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks")) //todo: how does this work?
	// 			|| ThisPin->TaskSuspender.IsSuspended()
	// 			|| IntermediateResult.ErrorCode != ETouchLinkErrorCode::Success
	// 			|| !ensureMsgf(IntermediateResult.PlatformTexture && IsValid(IntermediateResult.UnrealTexture), TEXT("Should be valid if no error code was set")))
	// 		{
	// 			Promise.SetValue(IntermediateResult);
	// 			return;
	// 		}
	// 		
	// 		ENQUEUE_RENDER_COMMAND(CopyTexture)([WeakThis, IntermediateResult](FRHICommandListImmediate& RHICmdList) mutable// , Promise = MoveTemp(Promise)
	// 		{
	// 			check(IntermediateResult.ErrorCode == ETouchLinkErrorCode::Success);
	// 			
	// 			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
	// 			if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks"))
	// 				|| ThisPin->TaskSuspender.IsSuspended())
	// 			{
	// 				IntermediateResult.ErrorCode = ETouchLinkErrorCode::Cancelled;
	// 				return;
	// 			}
	//
	// 			const FTouchCopyTextureArgs CopyArgs { IntermediateResult.RequestParams, RHICmdList, IntermediateResult.UnrealTexture };
	// 			IntermediateResult.PlatformTexture->CopyNativeToUnreal_RenderThread(CopyArgs, ThisPin.ToSharedRef())
	// 				.Next([IntermediateResult](ECopyTouchToUnrealResult Result) mutable
	// 				{
	// 					const bool bSuccessfulCopy = Result == ECopyTouchToUnrealResult::Success;
	// 					IntermediateResult.ErrorCode = bSuccessfulCopy
	// 						? ETouchLinkErrorCode::Success
	// 						: ETouchLinkErrorCode::FailedToCopyResources;
	// 					if (bSuccessfulCopy)
	// 					{
	// 						UE_LOG(LogTouchEngine, Log, TEXT("   [FTouchTextureImporter::CopyTexture_AnyThread] Successfully copied Texture to Unreal Engine for parameter [%s] for frame `%lld`"),*IntermediateResult.RequestParams.Identifier.ToString(), IntermediateResult.RequestParams.FrameData.FrameID)
	// 					}
	// 					else
	// 					{
	// 						UE_LOG(LogTouchEngine, Error, TEXT("   [FTouchTextureImporter::CopyTexture_AnyThread] UNSUCCESSFULLY copied Texture to Unreal Engine for parameter [%s] for frame `%lld`"),*IntermediateResult.RequestParams.Identifier.ToString(), IntermediateResult.RequestParams.FrameData.FrameID)
	// 					}
	// 				});
	// 		});
	// 		Promise.SetValue(IntermediateResult); // we can enqueue the command and return directly at this point, as the texture is ready
	// 	});
	// 	return Result;
	// }
}
