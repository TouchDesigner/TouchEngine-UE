// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"
#include "PixelFormat.h"
#include "TouchEngine/TEGraphicsContext.h"
#include "TouchEngine/TETexture.h"
#include "TouchEngine/TouchObject.h"

class FTexture2DResource;
class FRHITexture2D;
struct FTouchTOP;
typedef void FTouchEngineDevice;

namespace UE::TouchEngine
{
	/** Common interface for rendering API implementations */
	class FTouchEngineResourceProvider
	{
	public:
		
		/** Called on render thread to release any render thread resources. */
		virtual void Release_RenderThread() = 0;

		virtual TEGraphicsContext* GetContext() const = 0;
		virtual TEResult CreateContext(FTouchEngineDevice* Device, TEGraphicsContext*& Context) = 0;
		virtual TEResult CreateTexture(const TETexture* Src, TETexture*& Dst) = 0;
		virtual TETexture* CreateTexture(FRHITexture2D* Texture, TETextureOrigin Origin, TETextureComponentMap Map) = 0;

		// @todo should this be a named separate, or a another overload of CreateTexture??? - Drakynfly
		virtual TETexture* CreateTextureWithFormat(FRHITexture2D* Texture, TETextureOrigin Origin, TETextureComponentMap Map, EPixelFormat Format) = 0;

		virtual void ReleaseTexture(const FString& Name, TETexture* Texture) = 0;

		// @todo not sure what @George was intending the interaction to be between FTouchTOP and TouchDesigner::DX11::FTouchTOP, so this is probably temporary - Drakynfly
		virtual void ReleaseTexture(FTouchTOP& Texture) = 0;
		virtual void ReleaseTextures_RenderThread(bool bIsFinalClean = false) = 0;
		virtual void QueueTextureRelease(TETexture* Texture) = 0;

		/** Returns the texture resource in it's native format. */
		virtual FTexture2DResource* GetTexture(const TETexture* Texture) = 0;

		virtual bool IsSupportedFormat(EPixelFormat Format) = 0;
		
		virtual ~FTouchEngineResourceProvider() = default;
	};
}
