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
#include "Engine/Util/TouchFrameCooker.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
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
		const FTextureMetaData LinkMetaData = GetTextureMetaData(LinkParams.TETexture);

		// 1. Create the SharedTETexture to copy from

		UTexture2D* UEDestinationTexture = nullptr;
		bool AccessRHIViaReferenceTexture = false;
		{
			FScopeLock Lock(&LinkDataMutex);
			if (!ensure(LinkMetaData.PixelFormat != PF_Unknown))
			{
				UE_LOG(LogTouchEngine, Error, TEXT("[FTouchTextureImporter::ExecuteLinkTextureRequest_AnyThread[%s]] The PlatformMetadata has an unknown Pixel format `%s` for parameter `%s` for frame `%lld`"),
					   *GetCurrentThreadStr(), GetPixelFormatString(LinkMetaData.PixelFormat), *Identifier.ToString(), LinkParams.FrameData.FrameID);
			}
			else if (UTexture2D* PoolTexture = FindPoolTextureMatchingMetadata(LinkMetaData, LinkParams.FrameData)) // if the UTexture and the TE Texture matches size and format, copy straight into the UTexture resource
			{
				UEDestinationTexture = PoolTexture;
				AccessRHIViaReferenceTexture = true;
			}
			else // otherwise we need to create a new resource
			{
				UE_LOG(LogTouchEngine, Log, TEXT("[FTouchTextureImporter::ExecuteLinkTextureRequest_AnyThread[%s]] Need to create new UTexture for parameter `%s`: %dx%d [%s] for frame `%lld`"),
					   *GetCurrentThreadStr(), *Identifier.ToString(), LinkMetaData.SizeX, LinkMetaData.SizeY, GetPixelFormatString(LinkMetaData.PixelFormat), LinkParams.FrameData.FrameID);
				{
					DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.1.1 [AT] Link Texture Import - Create UTexture"), STAT_TE_III_A_1_1, STATGROUP_TouchEngine);
					const FString Name = FString::Printf(TEXT("%s [%lld:%f]"), *Identifier.ToString(), LinkParams.FrameData.FrameID, FPlatformTime::Seconds() - GStartTime);
					const FName UniqueName = MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), FName(Name));
					UEDestinationTexture = UTexture2D::CreateTransient(LinkMetaData.SizeX, LinkMetaData.SizeY, LinkMetaData.PixelFormat, UniqueName);
					UEDestinationTexture->NeverStream = true;
					UEDestinationTexture->SRGB = LinkMetaData.IsSRGB;
					UEDestinationTexture->AddToRoot();
				}
				{
					DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.1.2 [AT] Link Texture Import - Update Resource"), STAT_TE_III_A_1_2, STATGROUP_TouchEngine);
					// Hack to allow us to call UpdateResource from this thread. This should be safe because we just created it, and we are returning it after it has been initialised
					ETaskTag PreviousTagScope = FTaskTagScope::SwapTag(ETaskTag::ENone);
					{
						FOptionalTaskTagScope Scope(ETaskTag::EParallelGameThread);
						UEDestinationTexture->UpdateResource();
					}
					FTaskTagScope::SwapTag(PreviousTagScope);
				}
				AccessRHIViaReferenceTexture = false;
				// Here, accessing the reference texture will not give the expected result as it is not yet pointing to the right RHI
				// so we access directly Texture->GetResource()->TextureRHI which is the RHI that will be referenced, as seen in FStreamableTextureResource::InitRHI()
			}
		}

		ENQUEUE_RENDER_COMMAND(CopyRHI)([WeakThis = AsWeak(), LinkParams, UEDestinationTexture, AccessRHIViaReferenceTexture](FRHICommandListImmediate& RHICmdList) mutable
		{
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ThisPin || ThisPin->TaskSuspender.IsSuspended())
			{
				return;
			}
			
			const FName& Identifier = LinkParams.Identifier;
			TSharedPtr<ITouchImportTexture> PlatformTexture;
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.2 [RT] Link Texture Import - CreateSharedTETexture"), STAT_TE_III_A_2, STATGROUP_TouchEngine);
			   // 1. We get the source texture sent by TouchEngine
			   PlatformTexture = ThisPin->CreatePlatformTexture_RenderThread(LinkParams.Instance, LinkParams.TETexture);
			}

			TRefCountPtr<FRHITexture> UEDestinationTextureRHI;
			if (IsValid(UEDestinationTexture))
			{
				UEDestinationTextureRHI = AccessRHIViaReferenceTexture ?
					UEDestinationTexture->TextureReference.TextureReferenceRHI->GetReferencedTexture() :
					UEDestinationTexture->GetResource()->TextureRHI; //todo: check if always work
			}

			if (PlatformTexture && UEDestinationTextureRHI)
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.3 [RT] Link Texture Import - CopyRHI"), STAT_TE_III_A_3, STATGROUP_TouchEngine);
				// 2. We create a destination UTexture RHI if we don't have one already
				const FTouchCopyTextureArgs CopyArgs { LinkParams, RHICmdList, nullptr, UEDestinationTextureRHI}; //,  DestUETextureRHI};
				ThisPin->CopyNativeToUnreal_RenderThread(PlatformTexture, CopyArgs);

				{
					FScopeLock Lock(&ThisPin->LinkDataMutex);
					FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData.FindOrAdd(LinkParams.Identifier);
					TextureLinkData.UnrealTexture = UEDestinationTexture;
					TextureLinkData.bIsInProgress = false;
				}
			}

		}); // ~ENQUEUE_RENDER_COMMAND(CopyRHI)

		const bool bFailure = !ensure(IsValid(UEDestinationTexture));
		if (bFailure)
		{
			Promise.SetValue(FTouchTextureImportResult::MakeFailure());
			return;
		}
		
		// Here, we want to make sure the previous texture would be put back in the pool.
		TSharedPtr<TPromise<UTexture2D*>> PreviousTextureToBePooledPromise = MakeShared<TPromise<UTexture2D*>>();
		TFuture<UTexture2D*> PreviousTextureToBePooledResult = PreviousTextureToBePooledPromise->GetFuture();

		Promise.SetValue( FTouchTextureImportResult::MakeSuccessful(UEDestinationTexture, MoveTemp(PreviousTextureToBePooledPromise)));
			
		PreviousTextureToBePooledResult.Next([WeakThis = AsWeak(), LinkParams](UTexture2D* PreviousTextureToBePooled)
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

	void FTouchTextureImporter::CopyNativeToUnreal_RenderThread(const TSharedPtr<ITouchImportTexture>& TETexture, const FTouchCopyTextureArgs& CopyArgs)
	{
		const ECopyTouchToUnrealResult Result = TETexture->CopyNativeToUnrealRHI_RenderThread(CopyArgs, AsShared());
		
		const bool bSuccessfulCopy = Result == ECopyTouchToUnrealResult::Success;
		ETouchLinkErrorCode ErrorCode = bSuccessfulCopy ? ETouchLinkErrorCode::Success : ETouchLinkErrorCode::FailedToCopyResources;

		UE_CLOG(bSuccessfulCopy, LogTouchEngine, Log, TEXT("   [FTouchTextureImporter::CopyTexture_AnyThread] Successfully copied Texture to Unreal Engine for parameter [%s] for frame `%lld`"),*CopyArgs.RequestParams.Identifier.ToString(), CopyArgs.RequestParams.FrameData.FrameID)
		UE_CLOG(!bSuccessfulCopy, LogTouchEngine, Error, TEXT("   [FTouchTextureImporter::CopyTexture_AnyThread] UNSUCCESSFULLY copied Texture to Unreal Engine for parameter [%s] for frame `%lld`"),*CopyArgs.RequestParams.Identifier.ToString(), CopyArgs.RequestParams.FrameData.FrameID)
	}
}
