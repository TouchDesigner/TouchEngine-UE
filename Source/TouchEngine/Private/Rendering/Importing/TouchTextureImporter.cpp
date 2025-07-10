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
#include "Logging.h"
#include "RenderingThread.h"
#include "Engine/TEDebug.h"
#include "Engine/Util/TouchFrameCooker.h"
#include "Rendering/TouchResourceProvider.h"
#include "Rendering/Importing/ITouchImportTexture.h"
#include "Tasks/Task.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
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

		ExecuteOnGameThread<void>([TexturesToCleanUp = MoveTemp(TexturesToCleanUp)]()
		{
			for(UTexture* Texture : TexturesToCleanUp)
			{
				if (IsValid(Texture))
				{
					Texture->RemoveFromRoot();
				}
			};
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
		LinkParams.TETextureTransfer = GetTextureTransfer(LinkParams);
		
		// FTaskSuspender::FTaskTracker TaskToken = TaskSuspender.StartTask();
		TPromise<FTouchTextureImportResult> Promise;
		TFuture<FTouchTextureImportResult> Future = Promise.GetFuture()
			.Next([TaskToken = TaskSuspender.StartTask()](auto Result) // The task token will be auto deleted after this promise is done
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
			ExecuteLinkTextureRequest_AnyThread(MoveTemp(Promise), LinkParams, FrameCooker);
		}
		else // with the way the flow has been refactored, this is not supposed to happen anymore
		{
			Promise.SetValue(FTouchTextureImportResult::MakeCancelled());
		}
		
		return Future;
	}

	TFuture<FTouchSuspendResult> FTouchTextureImporter::SuspendAsyncTasks()
	{
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
		TFuture<FTouchSuspendResult> TaskSuspenderFuture = TaskSuspender.Suspend();

		ENQUEUE_RENDER_COMMAND(FinishRemainingTasks)([ThisPin = SharedThis(this), Promise = MoveTemp(Promise), TaskSuspenderFuture = MoveTemp(TaskSuspenderFuture)](FRHICommandListImmediate& RHICmdList) mutable
		{
			// We only wait in the render thread to be sure that all the previously enqueued render copies are started or cancelled
			TaskSuspenderFuture.Next([ThisPin, Promise = MoveTemp(Promise)](auto) mutable
			{
				// At this point, all the copies - if any - are started and enqueued on the RHI thread (they might be done already).
				// The variable KeepTexturesAliveForCopy holds all the remaining texture pending to be copied, so we now need to wait for them to be all done by checking if their fences have been signaled
				FScopeLock Lock(&ThisPin->KeepTexturesAliveMutex);
				ThisPin->RemoveUnusedAliveTextures();
				if (ThisPin->KeepTexturesAliveForCopy.IsEmpty())
				{
					Promise.SetValue({});
				}
				else
				{
					UE_LOG(LogTouchEngine, Log, TEXT("[FTouchTextureImporter::SuspendAsyncTasks] About to start waiting for %d texture copies..."), ThisPin->KeepTexturesAliveForCopy.Num())
					// If we are waiting on an export, start a background task to wait for it
					UE::Tasks::Launch(UE_SOURCE_LOCATION, [ThisPin, Promise = MoveTemp(Promise)]() mutable
					{
						double StartTime = FPlatformTime::Seconds(); 
						FPlatformProcess::ConditionalSleep([ThisPin, StartTime]()
						{
							if (FPlatformTime::Seconds() < StartTime + 5.0) // 5s should be more than enough time, we would be expecting this next tick
							{
								FScopeLock Lock(&ThisPin->KeepTexturesAliveMutex);
								ThisPin->RemoveUnusedAliveTextures();
								return ThisPin->KeepTexturesAliveForCopy.IsEmpty();
							}
							return true; // stop sleeping if any of those is invalid or if we timed out
						}, 0.1f);
						{
							FScopeLock Lock(&ThisPin->KeepTexturesAliveMutex);
							UE_LOG(LogTouchEngine, Log, TEXT("[FTouchTextureImporter::SuspendAsyncTasks::ConditionalSleep] Done waiting. Remaining copies: %d"), ThisPin->KeepTexturesAliveForCopy.Num());
							ThisPin->KeepTexturesAliveForCopy.Empty(); //to be sure we clear them, if ended up with a timeout
							Promise.SetValue({});
						}
					}, LowLevelTasks::ETaskPriority::BackgroundLow);
				}
			});
		});
		return Future; ;
	}

	bool FTouchTextureImporter::CanCopyIntoUTexture(const FTextureMetaData& Source, const UTexture* Target)
	{
		if (IsValid(Target))
		{
			const FTextureRHIRef TargetRHI = FTouchResourceProvider::GetStableRHIFromTexture(Target);
			if (TargetRHI) // this can fail often when UE is in the background
			{
				const FRHITextureDesc Desc = TargetRHI->GetDesc();
				return Source.SizeX == Desc.Extent.X
					&& Source.SizeY == Desc.Extent.Y
					&& Source.PixelFormat == Desc.Format
					&& Source.IsSRGB == Target->SRGB;
			}
		}
		return false;
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
		for(const TTuple<FName, FTouchTextureLinkData>& Data : LinkData)
		{
			if (Data.Value.UnrealTexture == Texture)
			{
				Texture->RemoveFromRoot(); // this is enough to ensure it will not be put back in the pool, as we are checking for this
				return true;
			}
		}

		return false;
	}

	FTouchTextureTransfer FTouchTextureImporter::GetTextureTransfer(const FTouchImportParameters& ImportParams)
	{
		FTouchTextureTransfer Transfer;
		Transfer.Result = TEInstanceGetTextureTransfer(ImportParams.Instance, ImportParams.TETexture, Transfer.Semaphore.take(), &Transfer.WaitValue);
		UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceGetTextureTransfer(TEInstance: '%p', texture: '%p', semaphore&: '%p', waitValue&: '%lld') [Thread: '%s', Identifier: '%s']  =>  Returned '%s'"),
			ImportParams.Instance.get(),
			ImportParams.TETexture.get(),
			Transfer.Semaphore.get(),
			Transfer.WaitValue,
			*GetCurrentThreadStr(),
			*ImportParams.Identifier.ToString(),
			*TEResultToString(Transfer.Result)
		)

		return Transfer;
	}
	
	void FTouchTextureImporter::ExecuteLinkTextureRequest_AnyThread(TPromise<FTouchTextureImportResult>&& Promise, const FTouchImportParameters& LinkParams, const TSharedPtr<FTouchFrameCooker>& FrameCooker)
	{
		if (TaskSuspender.IsSuspended())
		{
			Promise.SetValue(FTouchTextureImportResult::MakeFailure());
			return;
		}
		if (GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan) // todo: For UE 5.6, Vulkan Textures are not supported
		{
			Promise.SetValue(FTouchTextureImportResult::MakeFailure());
			return;
		}

		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.1 [AT] Link Texture Import"), STAT_TE_III_A_1, STATGROUP_TouchEngine);
		// At this point, we are neither on the GameThread nor on the RenderThread, we are on a parallel thread.
		// A UTexture2D can be created on any thread but the call to UTexture2D::UpdateResource need to be on GameThread.
		// As we are creating the texture here, we use an FTaskTagScope to allow us to call UTexture2D::UpdateResource from this thread.
		// There should be no issues as the texture is not used anywhere else at this point.
		
		const FTextureMetaData TETextureMetadata = GetTextureMetaData(LinkParams.TETexture);
		
		// 1. Check if we already have a UTexture that could hold the data from TouchEngine, or create one
		bool bAccessRHIViaReferenceTexture = false;
		UTexture2D* UEDestinationTexture = GetOrCreateUTextureMatchingMetaData(TETextureMetadata, LinkParams, bAccessRHIViaReferenceTexture);
		if (!ensure(IsValid(UEDestinationTexture)))
		{
			Promise.SetValue(FTouchTextureImportResult::MakeFailure());
			return;
		}

		// 2. Enqueue the copy of the Texture
		ENQUEUE_RENDER_COMMAND(CopyRHI)([WeakThis = AsWeak(), LinkParams, UEDestinationTexture, bAccessRHIViaReferenceTexture](FRHICommandListImmediate& RHICmdList) mutable
		{
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ThisPin || ThisPin->TaskSuspender.IsSuspended())
			{
				return;
			}
			
			TSharedPtr<ITouchImportTexture> PlatformTexture;
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.2 [RT] Link Texture Import - CreateSharedTETexture"), STAT_TE_III_A_2, STATGROUP_TouchEngine);
			   // 1. We get the source texture sent by TouchEngine
			   PlatformTexture = ThisPin->CreatePlatformTexture_RenderThread(LinkParams.Instance, LinkParams.TETexture);
			}

			TRefCountPtr<FRHITexture> UEDestinationTextureRHI;
			if (IsValid(UEDestinationTexture))
			{
				UEDestinationTextureRHI = bAccessRHIViaReferenceTexture && UEDestinationTexture->TextureReference.TextureReferenceRHI ?
					FTextureRHIRef{UEDestinationTexture->TextureReference.TextureReferenceRHI->GetReferencedTexture()} :
					UEDestinationTexture->GetResource() ? UEDestinationTexture->GetResource()->TextureRHI : nullptr;
			}

			if (PlatformTexture && UEDestinationTextureRHI)
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.3 [RT] Link Texture Import - CopyRHI"), STAT_TE_III_A_3, STATGROUP_TouchEngine);
				// 2. We create a destination UTexture RHI if we don't have one already
				const FTouchCopyTextureArgs CopyArgs { LinkParams, RHICmdList, UEDestinationTextureRHI};
				ThisPin->CopyNativeToUnreal_RenderThread(PlatformTexture, CopyArgs);

				{
					FScopeLock Lock(&ThisPin->LinkDataMutex);
					FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData.FindOrAdd(LinkParams.Identifier);
					TextureLinkData.UnrealTexture = UEDestinationTexture;
					TextureLinkData.bIsInProgress = false;
				}
			}
		}); // ~ENQUEUE_RENDER_COMMAND(CopyRHI)
		
		//3. Here, we want to make sure the previous texture would be put back in the pool, so we create a promise to be filled
		TSharedPtr<TPromise<UTexture2D*>> PreviousTextureToBePooledPromise = MakeShared<TPromise<UTexture2D*>>();
		TFuture<UTexture2D*> PreviousTextureToBePooledResult = PreviousTextureToBePooledPromise->GetFuture();
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

		Promise.SetValue( FTouchTextureImportResult::MakeSuccessful(UEDestinationTexture, MoveTemp(PreviousTextureToBePooledPromise)));
	}
	
	UTexture2D* FTouchTextureImporter::GetOrCreateUTextureMatchingMetaData(const FTextureMetaData& TETextureMetadata, const FTouchImportParameters& LinkParams, bool& bOutAccessRHIViaReferenceTexture)
	{
		UTexture2D* UEDestinationTexture = nullptr;
		if (!ensure(TETextureMetadata.PixelFormat != PF_Unknown))
		{
			bOutAccessRHIViaReferenceTexture = false;
			UE_LOG(LogTouchEngine, Error, TEXT("[FTouchTextureImporter::ExecuteLinkTextureRequest_AnyThread[%s]] The PlatformMetadata has an unknown Pixel format `%s` for parameter `%s` for frame `%lld`"),
				   *GetCurrentThreadStr(), GetPixelFormatString(TETextureMetadata.PixelFormat), *LinkParams.Identifier.ToString(), LinkParams.FrameData.FrameID);
		}
		else if (UTexture2D* PoolTexture = FindPoolTextureMatchingMetadata(TETextureMetadata, LinkParams.FrameData)) // if the UTexture and the TE Texture matches size and format, copy straight into the UTexture resource
		{
			bOutAccessRHIViaReferenceTexture = true;
			UEDestinationTexture = PoolTexture;
		}
		else // otherwise we need to create a new resource
		{
			
			UE_LOG(LogTouchEngine, Log, TEXT("[FTouchTextureImporter::ExecuteLinkTextureRequest_AnyThread[%s]] Need to create new UTexture for parameter `%s`: %dx%d [%s] for frame `%lld`"),
				   *GetCurrentThreadStr(), *LinkParams.Identifier.ToString(), TETextureMetadata.SizeX, TETextureMetadata.SizeY, GetPixelFormatString(TETextureMetadata.PixelFormat), LinkParams.FrameData.FrameID);
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.1.1 [AT] Link Texture Import - Create UTexture"), STAT_TE_III_A_1_1, STATGROUP_TouchEngine);
				const FString Name = FString::Printf(TEXT("%s [%lld:%f]"), *LinkParams.Identifier.ToString(), LinkParams.FrameData.FrameID, FPlatformTime::Seconds() - GStartTime);
				const FName UniqueName = MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), FName(Name));
				UEDestinationTexture = UTexture2D::CreateTransient(TETextureMetadata.SizeX, TETextureMetadata.SizeY, TETextureMetadata.PixelFormat, UniqueName);
				UEDestinationTexture->NeverStream = true;
				UEDestinationTexture->SRGB = TETextureMetadata.IsSRGB;
				UEDestinationTexture->AddToRoot();
			}
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.A.1.2 [AT] Link Texture Import - Update Resource"), STAT_TE_III_A_1_2, STATGROUP_TouchEngine);
				// Hack to allow us to call UpdateResource from this thread. This should be safe because we just created it, and we are returning it after it has been initialised
				const ETaskTag PreviousTagScope = FTaskTagScope::SwapTag(ETaskTag::ENone);
				{
					FTaskTagScope Scope(ETaskTag::EParallelGameThread);
					UEDestinationTexture->UpdateResource();
				}
				FTaskTagScope::SwapTag(PreviousTagScope);
			}
			bOutAccessRHIViaReferenceTexture = false;
			// Here, accessing the reference texture will not give the expected result as it is not yet pointing to the right RHI
			// so we access directly Texture->GetResource()->TextureRHI which is the RHI that will be referenced, as seen in FStreamableTextureResource::InitRHI()
		}
		
		return UEDestinationTexture;
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
		UE_CLOG(bSuccessfulCopy, LogTouchEngine, Verbose, TEXT("   [FTouchTextureImporter::CopyTexture_AnyThread] Successfully copied Texture to Unreal Engine for parameter [%s] for frame `%lld`"),*CopyArgs.RequestParams.Identifier.ToString(), CopyArgs.RequestParams.FrameData.FrameID)
		UE_CLOG(!bSuccessfulCopy, LogTouchEngine, Error, TEXT("   [FTouchTextureImporter::CopyTexture_AnyThread] UNSUCCESSFULLY copied Texture to Unreal Engine for parameter [%s] for frame `%lld`"),*CopyArgs.RequestParams.Identifier.ToString(), CopyArgs.RequestParams.FrameData.FrameID)
		if (bSuccessfulCopy)
		{
			FScopeLock Lock(&KeepTexturesAliveMutex);
			KeepTexturesAliveForCopy.Add({TETexture, CopyArgs.TargetRHI});
		}
	}

	void FTouchTextureImporter::RemoveUnusedAliveTextures()
	{
		FScopeLock Lock(&KeepTexturesAliveMutex);
		KeepTexturesAliveForCopy.RemoveAll([](const TPair<TSharedPtr<ITouchImportTexture>, FTexture2DRHIRef>& TexturePair)
		{
			return !TexturePair.Key || TexturePair.Key->IsCurrentCopyDone();
		});
	}
}
