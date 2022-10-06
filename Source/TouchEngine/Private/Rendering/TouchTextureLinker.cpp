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
		TArray<UTexture2D*> Textures;
		for (TPair<FName, FTouchTextureLinkData>& Data : LinkData)
		{
			if (Data.Value.ExecuteNext)
			{
				Data.Value.ExecuteNext->SetValue(FTouchLinkResult{ ELinkResultType::Cancelled });
			}
			if (ensure(IsValid(Data.Value.UnrealTexture)))
			{
				Textures.Add(Data.Value.UnrealTexture);
			}
		}
		
		ExecuteOnGameThread<void>([Textures]()
		{
			for (UTexture2D* Texture2D : Textures)
			{
				Texture2D->RemoveFromRoot();
			}
		});
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
		
		TextureCopyOperation.Next([this, Promise = MoveTemp(Promise)](FTouchTextureLinkJob LinkJob) mutable
			{
				TOptional<TPromise<FTouchLinkResult>> ExecuteNext;
				FTouchLinkParameters ExecuteNextParams;
				{
					FScopeLock Lock(&QueueTaskSection);
					FTouchTextureLinkData& TextureLinkData = LinkData.FindOrAdd(LinkJob.ParameterName);
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
					ExecuteLinkTextureRequest(MoveTemp(*ExecuteNext), ExecuteNextParams);
				}
			});
	}

	TFuture<FTouchTextureLinkJob> FTouchTextureLinker::CreateJob(const FTouchLinkParameters& LinkParams)
	{
		TPromise<FTouchTextureLinkJob> Promise;
		TFuture<FTouchTextureLinkJob> Result = Promise.GetFuture();
		AcquireSharedAndCreatePlatformTexture(LinkParams.Instance, LinkParams.Texture)
			.Next([LinkParams, Promise = MoveTemp(Promise)](TMutexLifecyclePtr<TouchObject<TETexture>> Texture) mutable
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
		ContinueFrom.Next([this, Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
		{
			if (IntermediateResult.ErrorCode != ETouchLinkErrorCode::Success)
			{
				Promise.SetValue(IntermediateResult);
				return;
			}
			
			const FName ParameterName = IntermediateResult.ParameterName;

			// Common case: early out and continue operations current thread (should be render thread fyi)
			FTouchTextureLinkData& TextureLinkData = LinkData[ParameterName];
			if (TextureLinkData.UnrealTexture)
			{
				IntermediateResult.UnrealTexture = TextureLinkData.UnrealTexture;
				Promise.SetValue(IntermediateResult);
				return;
			}

			// Typically only happens once
			ExecuteOnGameThread<FTouchTextureLinkJob>([this, IntermediateResult]() mutable -> FTouchTextureLinkJob
			{
				const int32 SizeX = GetPlatformTextureWidth(*IntermediateResult.PlatformTexture);
				const int32 SizeY = GetPlatformTextureHeight(*IntermediateResult.PlatformTexture);
				const EPixelFormat PixelFormat = GetPlatformTexturePixelFormat(*IntermediateResult.PlatformTexture);
				if (!ensure(PixelFormat != PF_Unknown))
				{
					IntermediateResult.ErrorCode = ETouchLinkErrorCode::FailedToCreateUnrealTexture;
					return IntermediateResult;
				}
				
				UTexture2D* Texture = UTexture2D::CreateTransient(SizeX, SizeY, PixelFormat);
				Texture->AddToRoot();
				Texture->UpdateResource();
				LinkData[IntermediateResult.ParameterName].UnrealTexture = Texture;
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
		ContinueFrom.Next([this, Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
		{
			if (IntermediateResult.ErrorCode != ETouchLinkErrorCode::Success
				|| !ensureMsgf(IntermediateResult.PlatformTexture && IsValid(IntermediateResult.UnrealTexture), TEXT("Should be valid if no error code is set")))
			{
				Promise.SetValue(IntermediateResult);
				return;
			}
			
			ENQUEUE_RENDER_COMMAND(CopyTexture)([this, Promise = MoveTemp(Promise), IntermediateResult](FRHICommandListImmediate& RHICmdList) mutable
			{
				// This is here so you can debug using RenderDoc
				RHICmdList.GetComputeContext().RHIPushEvent(TEXT("TouchEngineCopyResources"), FColor::Red);
				{
					TMutexLifecyclePtr<TouchObject<TETexture>> PlatformTexture = MoveTemp(IntermediateResult.PlatformTexture);
					checkf(IntermediateResult.PlatformTexture == nullptr, TEXT("Has the TSharedPtr API changed? We want the mutex to be released at the end of the scope"));
					CopyNativeResources(*PlatformTexture, IntermediateResult.UnrealTexture);
					// ~TAppMutexPtr will now release the mutex, returning the texture back to TE. TE will handle the texture's destruction.
					// It's better to release it before custom user code executes when we call SetValue below
				}
				RHICmdList.GetComputeContext().RHIPopEvent();

				Promise.SetValue(IntermediateResult);
			});
		});
		return Result;
	}
}
