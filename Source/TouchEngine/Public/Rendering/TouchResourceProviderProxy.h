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
		virtual TFuture<FTouchImportResult> ImportTextureToUnrealEngine(const FTouchImportParameters& LinkParams) override { return Implementation->ImportTextureToUnrealEngine(LinkParams); }

	private:

		TSharedRef<FTouchResourceProvider> Implementation;
		/** Called on the render thread when the proxy is destroyed */
		FRenderThreadCallback CleanOnRenderThread;
	};
}
