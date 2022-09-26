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
	class FTouchResourceProvider
	{
	public:

		virtual TEGraphicsContext* GetContext() const = 0;
		virtual TEResult CreateContext(FTouchEngineDevice* Device, TEGraphicsContext*& Context) = 0;
		virtual TEResult CreateTexture(TETexture* Src, TETexture*& Dst) = 0;
		virtual TETexture* CreateTexture(FRHITexture2D* Texture, TETextureOrigin Origin, TETextureComponentMap Map) = 0;

		virtual TETexture* CreateTextureWithFormat(FRHITexture2D* Texture, TETextureOrigin Origin, TETextureComponentMap Map, EPixelFormat Format) = 0;

		virtual void ReleaseTexture(const FString& Name, TETexture* Texture) = 0;

		virtual void ReleaseTexture(FTouchTOP& Texture) = 0;
		virtual void ReleaseTextures_RenderThread(bool bIsFinalClean = false) = 0;
		virtual void QueueTextureRelease_RenderThread(TETexture* Texture) = 0;

		/** Returns the texture resource in it's native format. */
		virtual FTexture2DResource* GetTexture(const TETexture* Texture) = 0;

		virtual bool IsSupportedFormat(EPixelFormat Format) = 0;
		
		virtual ~FTouchResourceProvider() = default;
	};
}
