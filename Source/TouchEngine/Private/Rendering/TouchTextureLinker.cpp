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

#include "Rendering/TouchTextureLinker.h"

#include "Logging.h"
#include "Rendering/ITouchPlatformTexture.h"
#include "Rendering/TouchResourceProvider.h"

#include "Algo/ForEach.h"

namespace UE::TouchEngine
{
	FTouchTextureLinker::~FTouchTextureLinker()
	{
		UE_LOG(LogTouchEngine, Verbose, TEXT("Shutting down ~FTouchTextureLinker"));

		// We don't need to care about removing textures from root set in this situation (in fact scheduling a game thread task will not work)
		if (IsEngineExitRequested())
		{
			return;
		}
		
		TArray<UTexture*> TexturesToCleanUp;
		for (TPair<FName, FTouchTextureLinkData>& Data : LinkData)
		{
			if (IsValid(Data.Value.UnrealTexture))
			{
				TexturesToCleanUp.Add(Data.Value.UnrealTexture);
			}
		}

		ExecuteOnGameThread<void>([TexturesToCleanUp]()
		{
			Algo::ForEach(TexturesToCleanUp, [](UTexture* Texture){ Texture->RemoveFromRoot(); });
		});
	}
	
	TFuture<FTouchLinkResult> FTouchTextureLinker::LinkTexture(const FTouchLinkParameters& LinkParams)
	{
		{
			FScopeLock Lock(&QueueTaskSection);
			if (TaskSuspender.IsSuspended())
			{
				UE_LOG(LogTouchEngine, Warning, TEXT("FTouchTextureLinker is suspended. Your task will be ignored."));
				return MakeFulfilledPromise<FTouchLinkResult>(FTouchLinkResult{ ELinkResultType::Cancelled }).GetFuture();
			}
			
			FTouchTextureLinkData& TextureLinkData = LinkData.FindOrAdd(LinkParams.ParameterName);
			if (TextureLinkData.bIsInProgress)
			{
				return EnqueueLinkTextureRequest(TextureLinkData, LinkParams)
					.Next([TaskToken = TaskSuspender.StartTask()](auto Result){ return Result; });
			}
			
			TextureLinkData.bIsInProgress = true;
		}

		TPromise<FTouchLinkResult> Promise;
		TFuture<FTouchLinkResult> Future = Promise.GetFuture();
		ExecuteLinkTextureRequest(MoveTemp(Promise), LinkParams);
		
		return Future.Next([TaskToken = TaskSuspender.StartTask()](auto Result) { return Result; });
	}

	TFuture<FTouchSuspendResult> FTouchTextureLinker::SuspendAsyncTasks()
	{
		FScopeLock Lock(&QueueTaskSection);
		for (TPair<FName, FTouchTextureLinkData>& Data : LinkData)
		{
			if (Data.Value.ExecuteNext)
			{
				Data.Value.ExecuteNext->SetValue(FTouchLinkResult{ ELinkResultType::Cancelled });
				Data.Value.ExecuteNext.Reset();
			}
		}
		
		return TaskSuspender.Suspend();
	}

	TFuture<FTouchLinkResult> FTouchTextureLinker::EnqueueLinkTextureRequest(FTouchTextureLinkData& TextureLinkData, const FTouchLinkParameters& LinkParams)
	{
		if (TextureLinkData.ExecuteNext.IsSet())
		{
			TextureLinkData.ExecuteNext->SetValue(FTouchLinkResult::MakeCancelled());
		}
		TextureLinkData.ExecuteNextParams = LinkParams;
		TextureLinkData.ExecuteNext = TPromise<FTouchLinkResult>();
		return TextureLinkData.ExecuteNext->GetFuture();
	}

	void FTouchTextureLinker::ExecuteLinkTextureRequest(TPromise<FTouchLinkResult>&& Promise, const FTouchLinkParameters& LinkParams)
	{
		FTouchTextureLinkJob CreateJobOperation = CreateJob(LinkParams);
		TFuture<FTouchTextureLinkJob> TextureCreationOperation = GetOrAllocateUnrealTexture(MoveTemp(CreateJobOperation));
		TFuture<FTouchTextureLinkJob> TextureCopyOperation = CopyTexture(MoveTemp(TextureCreationOperation));
		
		TextureCopyOperation.Next([WeakThis = TWeakPtr<FTouchTextureLinker>(SharedThis(this)), Promise = MoveTemp(Promise)](FTouchTextureLinkJob LinkJob) mutable
		{
			const TSharedPtr<FTouchTextureLinker> ThisPin = WeakThis.Pin();
			if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks"))
				|| ThisPin->TaskSuspender.IsSuspended())
			{
				Promise.SetValue(FTouchLinkResult::MakeCancelled());
				return;
			}
		
			TOptional<TPromise<FTouchLinkResult>> ExecuteNext;
			FTouchLinkParameters ExecuteNextParams;
			{
				FScopeLock Lock(&ThisPin->QueueTaskSection);
				FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData.FindOrAdd(LinkJob.RequestParams.ParameterName);
				if (TextureLinkData.ExecuteNext.IsSet())
				{
					ExecuteNext = MoveTemp(TextureLinkData.ExecuteNext);
					TextureLinkData.ExecuteNext.Reset();
					ExecuteNextParams = TextureLinkData.ExecuteNextParams;
				}
				TextureLinkData.bIsInProgress = false;
			}

			const bool bFailure = LinkJob.ErrorCode != ETouchLinkErrorCode::Success && !ensure(LinkJob.UnrealTexture != nullptr);
			const FTouchLinkResult Result = bFailure ? FTouchLinkResult::MakeFailure() : FTouchLinkResult::MakeSuccessful(LinkJob.UnrealTexture);
			Promise.SetValue(Result);

			if (ExecuteNext.IsSet())
			{
				ThisPin->ExecuteLinkTextureRequest(MoveTemp(*ExecuteNext), ExecuteNextParams);
			}
		});
	}

	FTouchTextureLinkJob FTouchTextureLinker::CreateJob(const FTouchLinkParameters& LinkParams)
	{
		const TSharedPtr<ITouchPlatformTexture> PlatformTexture = CreatePlatformTexture(LinkParams.Instance, LinkParams.Texture);
		const FTouchTextureLinkJob LinkJob { LinkParams, PlatformTexture, nullptr, PlatformTexture ? ETouchLinkErrorCode::Success : ETouchLinkErrorCode::FailedToCreatePlatformTexture };
		return LinkJob;
	}

	TFuture<FTouchTextureLinkJob> FTouchTextureLinker::GetOrAllocateUnrealTexture(FTouchTextureLinkJob IntermediateResult)
	{
		if (IntermediateResult.ErrorCode != ETouchLinkErrorCode::Success)
		{
			return MakeFulfilledPromise<FTouchTextureLinkJob>(IntermediateResult).GetFuture();
		}
		
		const FName ParameterName = IntermediateResult.RequestParams.ParameterName;
		// Common case: early out and continue operations current thread (should be render thread fyi)
		FTouchTextureLinkData& TextureLinkData = LinkData[ParameterName];
		if (TextureLinkData.UnrealTexture)
		{
			IntermediateResult.UnrealTexture = TextureLinkData.UnrealTexture;
			return MakeFulfilledPromise<FTouchTextureLinkJob>(IntermediateResult).GetFuture();
		}

		// Typically only happens once
		TPromise<FTouchTextureLinkJob> Promise;
		TFuture<FTouchTextureLinkJob> Result = Promise.GetFuture();
		ExecuteOnGameThread<FTouchTextureLinkJob>([WeakThis = TWeakPtr<FTouchTextureLinker>(SharedThis(this)), IntermediateResult]() mutable -> FTouchTextureLinkJob
		{
			const TSharedPtr<FTouchTextureLinker> ThisPin = WeakThis.Pin();
			if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks"))
				|| ThisPin->TaskSuspender.IsSuspended())
			{
				IntermediateResult.ErrorCode = ETouchLinkErrorCode::Cancelled;
				return IntermediateResult;
			}
			
			const FTextureMetaData TextureData = IntermediateResult.PlatformTexture->GetTextureMetaData();
			if (!ensure(TextureData.PixelFormat != PF_Unknown))
			{
				IntermediateResult.ErrorCode = ETouchLinkErrorCode::FailedToCreateUnrealTexture;
				return IntermediateResult;
			}
			
			UTexture2D* Texture = UTexture2D::CreateTransient(TextureData.SizeX, TextureData.SizeY, TextureData.PixelFormat);
			Texture->AddToRoot();
			Texture->UpdateResource();
			ThisPin->LinkData[IntermediateResult.RequestParams.ParameterName].UnrealTexture = Texture;
			IntermediateResult.UnrealTexture = Texture;
			
			return IntermediateResult;
		})
		.Next([Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
		{
			Promise.SetValue(IntermediateResult);
		});
		
		return Result;
	}

	TFuture<FTouchTextureLinkJob> FTouchTextureLinker::CopyTexture(TFuture<FTouchTextureLinkJob>&& ContinueFrom)
	{
		TPromise<FTouchTextureLinkJob> Promise;
		TFuture<FTouchTextureLinkJob> Result = Promise.GetFuture();
		ContinueFrom.Next([WeakThis = TWeakPtr<FTouchTextureLinker>(SharedThis(this)), Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
		{
			const TSharedPtr<FTouchTextureLinker> ThisPin = WeakThis.Pin();
			if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks"))
				|| ThisPin->TaskSuspender.IsSuspended()
				|| IntermediateResult.ErrorCode != ETouchLinkErrorCode::Success
				|| !ensureMsgf(IntermediateResult.PlatformTexture && IsValid(IntermediateResult.UnrealTexture), TEXT("Should be valid if no error code was set")))
			{
				Promise.SetValue(IntermediateResult);
				return;
			}
			
			ENQUEUE_RENDER_COMMAND(CopyTexture)([WeakThis, Promise = MoveTemp(Promise), IntermediateResult](FRHICommandListImmediate& RHICmdList) mutable
			{
				check(IntermediateResult.ErrorCode == ETouchLinkErrorCode::Success);
				
				const TSharedPtr<FTouchTextureLinker> ThisPin = WeakThis.Pin();
				if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks"))
					|| ThisPin->TaskSuspender.IsSuspended())
				{
					IntermediateResult.ErrorCode = ETouchLinkErrorCode::Cancelled;
					Promise.SetValue(IntermediateResult);
					return;
				}

				const FTouchCopyTextureArgs CopyArgs { IntermediateResult.RequestParams, RHICmdList, IntermediateResult.UnrealTexture };
				const bool bSuccessfulCopy = IntermediateResult.PlatformTexture->CopyNativeToUnreal(CopyArgs);
				IntermediateResult.ErrorCode = bSuccessfulCopy
					? ETouchLinkErrorCode::Success
					: ETouchLinkErrorCode::FailedToCopyResources;
				
				Promise.SetValue(IntermediateResult);
			});
		});
		return Result;
	}
}
