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
		virtual TFuture<FTouchExportResult> ExportTextureToTouchEngine(const FTouchExportParameters& Params) override { return Implementation->ExportTextureToTouchEngine(Params); }
		virtual TFuture<FTouchLinkResult> LinkTexture(const FTouchLinkParameters& LinkParams) override { return Implementation->LinkTexture(LinkParams); }

	private:

		TSharedRef<FTouchResourceProvider> Implementation;
		/** Called on the render thread when the proxy is destroyed */
		FRenderThreadCallback CleanOnRenderThread;
	};
}
