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
		const TPair<TEResult, TETexture*> CopiedTextureInfo = CopyTexture(LinkParams.Texture);
		if (CopiedTextureInfo.Key != TEResultSuccess)
		{
			return MakeFulfilledPromise<FTouchLinkResult>(FTouchLinkResult::MakeFailure()).GetFuture();
		}
		
		FTouchTextureLinkJob LinkJob { LinkParams };
		LinkJob.CopiedTextureData.set(CopiedTextureInfo.Value);
		{
			FScopeLock Lock(&QueueTaskSection);
			FTouchTextureLinkData& TextureLinkData = LinkData.FindOrAdd(LinkParams.ParameterName);
			if (TextureLinkData.bIsInProgress)
			{
				return EnqueueLinkTextureRequest(TextureLinkData, LinkJob);
			}
			
			TextureLinkData.bIsInProgress = true;
		}

		TPromise<FTouchLinkResult> Promise;
		TFuture<FTouchLinkResult> Result = Promise.GetFuture();
		ExecuteLinkTextureRequest(MoveTemp(Promise), LinkJob);
		return Result;
	}

	TFuture<FTouchLinkResult> FTouchTextureLinker::EnqueueLinkTextureRequest(FTouchTextureLinkData& TextureLinkData, const FTouchTextureLinkJob& LinkParams)
	{
		if (TextureLinkData.ExecuteNext.IsSet())
		{
			TextureLinkData.ExecuteNext->SetValue(FTouchLinkResult::MakeCancelled());
		}
		TextureLinkData.ExecuteNextParams = LinkParams;
		TextureLinkData.ExecuteNext = TPromise<FTouchLinkResult>();
		return TextureLinkData.ExecuteNext->GetFuture();
	}

	void FTouchTextureLinker::ExecuteLinkTextureRequest(TPromise<FTouchLinkResult>&& Promise, const FTouchTextureLinkJob& LinkParams)
	{
		TFuture<TLinkStep<UTexture2D*>> TextureCreationOperation = GetOrAllocateTexture(LinkParams);
		TFuture<TLinkStep<UTexture2D*>> TextureCopyOperation = CopyTexture(MoveTemp(TextureCreationOperation));
		
		TextureCopyOperation.Next([this, Promise = MoveTemp(Promise)](TLinkStep<UTexture2D*> Texture) mutable
			{
				TOptional<TPromise<FTouchLinkResult>> ExecuteNext;
				FTouchTextureLinkJob ExecuteNextParams;
				{
					FScopeLock Lock(&QueueTaskSection);
					FTouchTextureLinkData& TextureLinkData = LinkData.FindOrAdd(Texture.Value.ParameterName);
					if (TextureLinkData.ExecuteNext.IsSet())
					{
						ExecuteNext = MoveTemp(TextureLinkData.ExecuteNext);
						TextureLinkData.ExecuteNext.Reset();
						ExecuteNextParams = TextureLinkData.ExecuteNextParams;
					}
					TextureLinkData.bIsInProgress = false;
				}

				const bool bFailure = Texture.Key == nullptr;
				Promise.SetValue(bFailure ? FTouchLinkResult::MakeFailure() : FTouchLinkResult::MakeSuccessful(Texture.Key));

				if (ExecuteNext.IsSet())
				{
					ExecuteLinkTextureRequest(MoveTemp(*ExecuteNext), ExecuteNextParams);
				}
			});
	}

	TFuture<TLinkStep<UTexture2D*>> FTouchTextureLinker::GetOrAllocateTexture(const FTouchTextureLinkJob& LinkParams)
	{
		FTouchTextureLinkData& TextureLinkData = LinkData[LinkParams.ParameterName];
		if (TextureLinkData.UnrealTexture)
		{
			return MakeFulfilledPromise<TLinkStep<UTexture2D*>>(LinkData[LinkParams.ParameterName].UnrealTexture, LinkParams).GetFuture();
		}
		
		return ExecuteOnGameThread<TLinkStep<UTexture2D*>>([this, LinkParams]() -> TLinkStep<UTexture2D*>
		{
			const int32 SizeX = GetSharedTextureWidth(LinkParams.Texture);
			const int32 SizeY = GetSharedTextureHeight(LinkParams.Texture);
			const EPixelFormat PixelFormat = GetSharedTexturePixelFormat(LinkParams.Texture);
			if (!ensure(PixelFormat != PF_Unknown))
			{
				return TLinkStep<UTexture2D*>{ nullptr, LinkParams };
			}
			
			UTexture2D* Texture = UTexture2D::CreateTransient(SizeX, SizeY, PixelFormat);
			Texture->UpdateResource();
			LinkData[LinkParams.ParameterName].UnrealTexture = Texture;
			return TLinkStep<UTexture2D*>{ Texture, LinkParams };
		});
	}

	TFuture<TLinkStep<UTexture2D*>> FTouchTextureLinker::CopyTexture(TFuture<TLinkStep<UTexture2D*>>&& ContinueFrom)
	{
		TPromise<TLinkStep<UTexture2D*>> Promise;
		TFuture<TLinkStep<UTexture2D*>> Result = Promise.GetFuture();
		ContinueFrom.Next([this, Promise = MoveTemp(Promise)](TLinkStep<UTexture2D*> IntermediateResult) mutable
		{
			ENQUEUE_RENDER_COMMAND(CopyTexture)([this, Promise = MoveTemp(Promise), IntermediateResult](FRHICommandListImmediate& RHICmdList) mutable
			{
				FTexture2DRHIRef TextureRef = MakeRHITextureFrom(IntermediateResult.Value.CopiedTextureData, IntermediateResult.Key->GetPixelFormat());
				FRHITexture* Source = TextureRef->GetTexture2D();
				FRHITexture* Destination = IntermediateResult.Key->GetResource()->GetTexture2DResource()->TextureRHI; 
				RHICmdList.CopyTexture(Source, Destination, FRHICopyTextureInfo());
				Promise.SetValue(IntermediateResult);
			});
		});
		return Result;
	}
}
