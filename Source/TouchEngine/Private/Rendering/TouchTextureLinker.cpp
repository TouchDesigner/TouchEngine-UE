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
		GetOrAllocateTexture(LinkParams)
			.Next([](TLinkJob<UTexture2D*> Texture)
			{
				return Texture;
			})
			.Next([this, Promise = MoveTemp(Promise)](TLinkJob<UTexture2D*> Texture) mutable
			{
				TOptional<TPromise<FTouchLinkResult>> ExecuteNext;
				FTouchLinkParameters ExecuteNextParams;
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

	TFuture<TLinkJob<UTexture2D*>> FTouchTextureLinker::GetOrAllocateTexture(const FTouchLinkParameters& LinkParams)
	{
		if (LinkData[LinkParams.ParameterName].Texture)
		{
			return MakeFulfilledPromise<TLinkJob<UTexture2D*>>(LinkData[LinkParams.ParameterName].Texture, LinkParams).GetFuture();
		}
		
		return ExecuteOnGameThread<TLinkJob<UTexture2D*>>([this, LinkParams]() -> TLinkJob<UTexture2D*>
		{
			const int32 SizeX = GetSharedTextureWidth(LinkParams.Texture);
			const int32 SizeY = GetSharedTextureHeight(LinkParams.Texture);
			const EPixelFormat PixelFormat = GetSharedTexturePixelFormat(LinkParams.Texture);
			if (!ensure(PixelFormat != PF_Unknown))
			{
				return TLinkJob<UTexture2D*>{ nullptr, LinkParams };
			}
			
			UTexture2D* Texture = UTexture2D::CreateTransient(SizeX, SizeY, PixelFormat);
			LinkData[LinkParams.ParameterName].Texture = Texture;
			return TLinkJob<UTexture2D*>{ Texture, LinkParams };
		});
	}
}
