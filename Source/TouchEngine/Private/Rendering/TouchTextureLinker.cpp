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

#include "Rendering/Texture2DResource.h"
#include "Rendering/TouchResourceProvider.h"

namespace UE::TouchEngine
{
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
		TFuture<FTouchTextureLinkJob> TextureCreationOperation = GetOrAllocateTexture(MoveTemp(CreateJobOperation));
		TFuture<FTouchTextureLinkJob> TextureCopyOperation = CopyTexture(MoveTemp(TextureCreationOperation));
		
		TextureCopyOperation.Next([this, Promise = MoveTemp(Promise)](FTouchTextureLinkJob Texture) mutable
			{
				TOptional<TPromise<FTouchLinkResult>> ExecuteNext;
				FTouchLinkParameters ExecuteNextParams;
				{
					FScopeLock Lock(&QueueTaskSection);
					FTouchTextureLinkData& TextureLinkData = LinkData.FindOrAdd(Texture.ParameterName);
					if (TextureLinkData.ExecuteNext.IsSet())
					{
						ExecuteNext = MoveTemp(TextureLinkData.ExecuteNext);
						TextureLinkData.ExecuteNext.Reset();
						ExecuteNextParams = TextureLinkData.ExecuteNextParams;
					}
					TextureLinkData.bIsInProgress = false;
				}

				const bool bFailure = Texture.UnrealTexture == nullptr;
				Promise.SetValue(bFailure ? FTouchLinkResult::MakeFailure() : FTouchLinkResult::MakeSuccessful(Texture.UnrealTexture));

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
		ENQUEUE_RENDER_COMMAND(CreateJob)([this, Promise = MoveTemp(Promise), LinkParams](FRHICommandListImmediate& RHICmdList) mutable
		{
			// This is here so you can debug using RenderDoc
			RHICmdList.GetComputeContext().RHIPushEvent(TEXT("TouchEngineAllocatePlatformTexture"), FColor::Red);
			{
				const FTouchTextureLinkJob LinkJob { LinkParams.ParameterName, CreatePlatformTextureFromShared(LinkParams.Texture) };
				Promise.SetValue(LinkJob);
			}
			RHICmdList.GetComputeContext().RHIPopEvent();
		});
		return Result;
	}

	TFuture<FTouchTextureLinkJob> FTouchTextureLinker::GetOrAllocateTexture(TFuture<FTouchTextureLinkJob>&& ContinueFrom)
	{
		TPromise<FTouchTextureLinkJob> Promise;
		TFuture<FTouchTextureLinkJob> Result = Promise.GetFuture();
		ContinueFrom.Next([this, Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
		{
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
				const int32 SizeX = GetPlatformTextureWidth(IntermediateResult.PlatformTexture);
				const int32 SizeY = GetPlatformTextureHeight(IntermediateResult.PlatformTexture);
				const EPixelFormat PixelFormat = GetPlatformTexturePixelFormat(IntermediateResult.PlatformTexture);
				if (!ensure(PixelFormat != PF_Unknown))
				{
					IntermediateResult.PlatformTexture = nullptr;
					return IntermediateResult;
				}
				
				UTexture2D* Texture = UTexture2D::CreateTransient(SizeX, SizeY, PixelFormat);
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
			if (!ensure(IntermediateResult.UnrealTexture))
			{
				return;
			}
			
			ENQUEUE_RENDER_COMMAND(CopyTexture)([this, Promise = MoveTemp(Promise), IntermediateResult](FRHICommandListImmediate& RHICmdList) mutable
			{
				// This is here so you can debug using RenderDoc
				RHICmdList.GetComputeContext().RHIPushEvent(TEXT("TouchEngineCopyResources"), FColor::Red);
				{
					CopyNativeResources(IntermediateResult.PlatformTexture, IntermediateResult.UnrealTexture);
					Promise.SetValue(IntermediateResult);
				}
				RHICmdList.GetComputeContext().RHIPopEvent();
			});
		});
		return Result;
	}
}
