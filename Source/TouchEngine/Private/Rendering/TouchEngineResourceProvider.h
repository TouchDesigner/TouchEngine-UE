// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"
#include "TouchEngine/TEGraphicsContext.h"
#include "TouchEngine/TETexture.h"
#include "TouchEngine/TouchObject.h"

typedef void FTouchEngineDevice;

namespace TouchDesigner
{
	enum class ERHI : uint8
	{
		Invalid,
		DX11,
		DX12,
		Vulkan
	};
}


/** Common interface for rendering API implementations */
class FTouchEngineResourceProvider
{
public:
	virtual ~FTouchEngineResourceProvider() = default;

	virtual bool Initialize(TFunctionRef<void(const FString&)> LoadErrorCallback, TFunctionRef<void(const TEResult, const FString&)> ResultErrorCallback) = 0;
	virtual void Release() = 0;

	virtual TEGraphicsContext* GetContext() const = 0;
	virtual TEResult CreateContext(FTouchEngineDevice* Device, TEGraphicsContext*& Context) = 0;
	virtual TEResult CreateTexture(const TETexture* Src, TETexture*& Dst) = 0;
	virtual TETexture* CreateTexture(FRHITexture2D* Texture, TETextureOrigin Origin, TETextureComponentMap Map) = 0;

	// @todo should this be a named separate, or a another overload of CreateTexture??? - Drakynfly
	virtual TETexture* CreateTextureWithFormat(FRHITexture2D* Texture, TETextureOrigin Origin, TETextureComponentMap Map, EPixelFormat Format) = 0;

	virtual void ReleaseTexture(const FString& Name, TETexture* Texture) = 0;

	// @todo not sure what @George was intending the interaction to be between FTouchTOP and TouchDesigner::DX11::FTouchTOP, so this is probably temporary - Drakynfly
	virtual void ReleaseTexture(struct FTouchTOP& Texture) = 0;
	virtual void ReleaseTextures(const bool& bIsFinalClean = false) = 0;
	virtual void QueueTextureRelease(TETexture* Texture) = 0;

	/** Returns the texture resource in it's native format. */
	virtual FTexture2DResource* GetTexture(const TETexture* Texture) = 0;

	virtual TouchDesigner::ERHI GetRHIType() = 0;
	virtual bool IsSupportedFormat(EPixelFormat Format) = 0;
};
