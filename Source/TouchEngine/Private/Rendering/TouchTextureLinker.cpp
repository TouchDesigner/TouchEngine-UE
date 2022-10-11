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

#include "Rendering/TouchResourceProvider.h"

namespace UE::TouchEngine
{
	FTouchTextureLinker::~FTouchTextureLinker()
	{
		check(IsInGameThread());
		
		for (TPair<FName, FTouchTextureLinkData>& Data : LinkData)
		{
			if (Data.Value.ExecuteNext)
			{
				Data.Value.ExecuteNext->SetValue(FTouchLinkResult{ ELinkResultType::Cancelled });
				Data.Value.ExecuteNext.Reset();
			}
			
			if (ensure(IsValid(Data.Value.UnrealTexture)))
			{
				Data.Value.UnrealTexture->RemoveFromRoot();
			}
		}

		// Finishes all pending tasks
		FlushRenderingCommands();
	}
	
	TFuture<FTouchLinkResult> FTouchTextureLinker::LinkTexture(const FTouchLinkParameters& LinkParams)
	{
		{
			FScopeLock Lock(&QueueTaskSection);
			FTouchTextureLinkData& TextureLinkData = LinkData.FindOrAdd(LinkParams.ParameterName);
			if (TextureLinkData.bIsInProgress)
			{
				return EnqueueLinkTextureRequest(TextureLinkData, LinkParams);
			}
			
			TextureLinkData.bIsInProgress = true;
		}

		TPromise<FTouchLinkResult> Promise;
		TFuture<FTouchLinkResult> Result = Promise.GetFuture();
		ExecuteLinkTextureRequest(MoveTemp(Promise), LinkParams);
		return Result;
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
		TFuture<FTouchTextureLinkJob> CreateJobOperation = CreateJob(LinkParams);
		TFuture<FTouchTextureLinkJob> TextureCreationOperation = GetOrAllocateUnrealTexture(MoveTemp(CreateJobOperation));
		TFuture<FTouchTextureLinkJob> TextureCopyOperation = CopyTexture(MoveTemp(TextureCreationOperation));
		
		TextureCopyOperation.Next([WeakThis = TWeakPtr<FTouchTextureLinker>(SharedThis(this)), Promise = MoveTemp(Promise)](FTouchTextureLinkJob LinkJob) mutable
			{
				const TSharedPtr<FTouchTextureLinker> ThisPin = WeakThis.Pin();
				if (!ThisPin)
				{
					return;
				}
			
				TOptional<TPromise<FTouchLinkResult>> ExecuteNext;
				FTouchLinkParameters ExecuteNextParams;
				{
					FScopeLock Lock(&ThisPin->QueueTaskSection);
					FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData.FindOrAdd(LinkJob.ParameterName);
					if (TextureLinkData.ExecuteNext.IsSet())
					{
						ExecuteNext = MoveTemp(TextureLinkData.ExecuteNext);
						TextureLinkData.ExecuteNext.Reset();
						ExecuteNextParams = TextureLinkData.ExecuteNextParams;
					}
					TextureLinkData.bIsInProgress = false;
				}

				const bool bFailure = LinkJob.ErrorCode != ETouchLinkErrorCode::Success && !ensure(LinkJob.UnrealTexture != nullptr);
				Promise.SetValue(bFailure ? FTouchLinkResult::MakeFailure() : FTouchLinkResult::MakeSuccessful(LinkJob.UnrealTexture));

				if (ExecuteNext.IsSet())
				{
					ThisPin->ExecuteLinkTextureRequest(MoveTemp(*ExecuteNext), ExecuteNextParams);
				}
			});
	}

	TFuture<FTouchTextureLinkJob> FTouchTextureLinker::CreateJob(const FTouchLinkParameters& LinkParams)
	{
		TPromise<FTouchTextureLinkJob> Promise;
		TFuture<FTouchTextureLinkJob> Result = Promise.GetFuture();
		AcquireSharedAndCreatePlatformTexture(LinkParams.Instance, LinkParams.Texture)
			.Next([LinkParams, Promise = MoveTemp(Promise)](TMutexLifecyclePtr<FNativeTextureHandle> Texture) mutable
			{
				FTouchTextureLinkJob LinkJob { LinkParams.ParameterName, Texture };
				if (!Texture)
				{
					LinkJob.ErrorCode = ETouchLinkErrorCode::FailedToCreatePlatformTexture;
				}
				
				Promise.SetValue(LinkJob);
			});
		return Result;
	}

	TFuture<FTouchTextureLinkJob> FTouchTextureLinker::GetOrAllocateUnrealTexture(TFuture<FTouchTextureLinkJob>&& ContinueFrom)
	{
		TPromise<FTouchTextureLinkJob> Promise;
		TFuture<FTouchTextureLinkJob> Result = Promise.GetFuture();
		ContinueFrom.Next([WeakThis = TWeakPtr<FTouchTextureLinker>(SharedThis(this)), Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
		{
			if (IntermediateResult.ErrorCode != ETouchLinkErrorCode::Success)
			{
				Promise.SetValue(IntermediateResult);
				return;
			}
			
			const TSharedPtr<FTouchTextureLinker> ThisPin = WeakThis.Pin();
			if (!ThisPin)
			{
				IntermediateResult.ErrorCode = ETouchLinkErrorCode::Cancelled;
				Promise.SetValue(IntermediateResult);
				return;
			}
			
			const FName ParameterName = IntermediateResult.ParameterName;
			// Common case: early out and continue operations current thread (should be render thread fyi)
			FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData[ParameterName];
			if (TextureLinkData.UnrealTexture)
			{
				IntermediateResult.UnrealTexture = TextureLinkData.UnrealTexture;
				Promise.SetValue(IntermediateResult);
				return;
			}

			// Typically only happens once
			ExecuteOnGameThread<FTouchTextureLinkJob>([WeakThis, IntermediateResult]() mutable -> FTouchTextureLinkJob
			{
				const TSharedPtr<FTouchTextureLinker> ThisPin = WeakThis.Pin();
				if (!ThisPin)
				{
					IntermediateResult.ErrorCode = ETouchLinkErrorCode::Cancelled;
					return IntermediateResult;
				}
				
				const int32 SizeX = ThisPin->GetPlatformTextureWidth(*IntermediateResult.PlatformTexture);
				const int32 SizeY = ThisPin->GetPlatformTextureHeight(*IntermediateResult.PlatformTexture);
				const EPixelFormat PixelFormat = ThisPin->GetPlatformTexturePixelFormat(*IntermediateResult.PlatformTexture);
				if (!ensure(PixelFormat != PF_Unknown))
				{
					IntermediateResult.ErrorCode = ETouchLinkErrorCode::FailedToCreateUnrealTexture;
					return IntermediateResult;
				}
				
				UTexture2D* Texture = UTexture2D::CreateTransient(SizeX, SizeY, PixelFormat);
				Texture->AddToRoot();
				Texture->UpdateResource();
				ThisPin->LinkData[IntermediateResult.ParameterName].UnrealTexture = Texture;
				IntermediateResult.UnrealTexture = Texture;
				
				return IntermediateResult;
			})
			.Next([Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
			{
				Promise.SetValue(IntermediateResult);
			});
		});
		return Result;
	}

	TFuture<FTouchTextureLinkJob> FTouchTextureLinker::CopyTexture(TFuture<FTouchTextureLinkJob>&& ContinueFrom)
	{
		TPromise<FTouchTextureLinkJob> Promise;
		TFuture<FTouchTextureLinkJob> Result = Promise.GetFuture();
		ContinueFrom.Next([WeakThis = TWeakPtr<FTouchTextureLinker>(SharedThis(this)), Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
		{
			if (IntermediateResult.ErrorCode != ETouchLinkErrorCode::Success
				|| !ensureMsgf(IntermediateResult.PlatformTexture && IsValid(IntermediateResult.UnrealTexture), TEXT("Should be valid if no error code is set")))
			{
				Promise.SetValue(IntermediateResult);
				return;
			}
			
			ENQUEUE_RENDER_COMMAND(CopyTexture)([WeakThis, Promise = MoveTemp(Promise), IntermediateResult](FRHICommandListImmediate& RHICmdList) mutable
			{
				check(IntermediateResult.ErrorCode == ETouchLinkErrorCode::Success);
				
				const TSharedPtr<FTouchTextureLinker> ThisPin = WeakThis.Pin();
				if (!ThisPin)
				{
					IntermediateResult.ErrorCode = ETouchLinkErrorCode::Cancelled;
					Promise.SetValue(IntermediateResult);
					return;
				}

				bool bSuccessfulCopy;
				// This is here so you can debug using RenderDoc
				RHICmdList.GetComputeContext().RHIPushEvent(TEXT("TouchEngineCopyResources"), FColor::Red);
				{
					TMutexLifecyclePtr<FNativeTextureHandle> PlatformTexture = MoveTemp(IntermediateResult.PlatformTexture);
					checkf(IntermediateResult.PlatformTexture == nullptr, TEXT("Has the TSharedPtr API changed? We want the mutex to be released at the end of the scope"));
					bSuccessfulCopy = ThisPin->CopyNativeToUnreal(RHICmdList, *PlatformTexture, IntermediateResult.UnrealTexture);
					// ~TMutexLifecyclePtr will now release the mutex, returning the texture back to TE. TE will handle the texture's destruction.
					// It's better to release it before custom user code executes when we call SetValue below
				}
				RHICmdList.GetComputeContext().RHIPopEvent();
				
				IntermediateResult.ErrorCode = bSuccessfulCopy
					? ETouchLinkErrorCode::Success
					: ETouchLinkErrorCode::FailedToCopyResources;
				Promise.SetValue(IntermediateResult);
			});
		});
		return Result;
	}
}
