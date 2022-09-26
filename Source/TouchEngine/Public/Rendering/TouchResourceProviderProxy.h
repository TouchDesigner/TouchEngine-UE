// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"
#include "TouchResourceProvider.h"

namespace UE::TouchEngine
{
	DECLARE_DELEGATE(FRenderThreadCallback);
	
	/**
	 * Util class to make sure rendering resources are released on the render thread.
	 * When the proxy object is destroyed, it keeps the real resource provider alive until the cleanup callback has been called.
	 */
	class FTouchResourceProviderProxy final : public FTouchResourceProvider
	{
	public:
		
		FTouchResourceProviderProxy(TSharedRef<FTouchResourceProvider> Implementation, FRenderThreadCallback CleanOnRenderThread)
			: Implementation(MoveTemp(Implementation))
			, CleanOnRenderThread(MoveTemp(CleanOnRenderThread))
		{}

		virtual ~FTouchResourceProviderProxy() override
		{
			if (IsInRenderingThread()) // Unlikely but implemented for future proofing
			{
				CleanOnRenderThread.ExecuteIfBound();
			}
			else
			{
				ENQUEUE_RENDER_COMMAND(SetTOPOutput)(
					[Impl = MoveTemp(Implementation), CleanDelegate = MoveTemp(CleanOnRenderThread)](FRHICommandListImmediate& RHICmdList)
					{
						CleanDelegate.ExecuteIfBound();
						// Note that Impl is kept alive until this point and is now destroyed because its last instance goes out of scope
					});
			}
		}
		
		virtual TEGraphicsContext* GetContext() const override { return Implementation->GetContext(); }
		virtual TEResult CreateTexture(TETexture* Src, TETexture*& Dst) override { return Implementation->CreateTexture(Src, Dst); }
		virtual TFuture<FTouchLinkResult> LinkTexture(const FTouchLinkParameters& LinkParams) override { return Implementation->LinkTexture(LinkParams); }
		virtual TETexture* CreateTextureWithFormat(FRHITexture2D* Texture, TETextureOrigin Origin, TETextureComponentMap Map, EPixelFormat Format) override { return Implementation->CreateTextureWithFormat(Texture, Origin, Map, Format); }

		virtual void ReleaseTexture(FTouchTOP& Texture) override { Implementation->ReleaseTexture(Texture); }
		virtual void ReleaseTextures_RenderThread(bool bIsFinalClean = false) override { Implementation->ReleaseTextures_RenderThread(bIsFinalClean); }
		virtual void QueueTextureRelease_RenderThread(TETexture* Texture) override { Implementation->QueueTextureRelease_RenderThread(Texture);}

		/** Returns the texture resource in it's native format. */
		virtual FTexture2DResource* GetTexture(const TETexture* Texture) override { return Implementation->GetTexture(Texture); }

		virtual bool IsSupportedFormat(EPixelFormat Format) override { return Implementation->IsSupportedFormat(Format); }

	private:

		TSharedRef<FTouchResourceProvider> Implementation;
		/** Called on the render thread when the proxy is destroyed */
		FRenderThreadCallback CleanOnRenderThread;
	};
}
