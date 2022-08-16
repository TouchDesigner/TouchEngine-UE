// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"
#include "TouchEngine/TEGraphicsContext.h"
#include "TouchEngine/TETexture.h"
#include "TouchEngine/TouchObject.h"

typedef void FTouchEngineDevice;

/** Common interface for rendering API implementations */
class FTouchEngineResourceProvider 
{
public:
	virtual ~FTouchEngineResourceProvider() = default;

	virtual bool Initialize(TFunctionRef<void(const FString&)> LoadErrorCallback, TFunctionRef<void(const TEResult, const FString&)> ResultErrorCallback) = 0;
	virtual void Release() = 0;

	virtual TEGraphicsContext* GetContext() const = 0;
	virtual TEResult CreateContext(FTouchEngineDevice* Device, TEGraphicsContext*& Context) = 0;
	virtual TEResult CreateTexture(TouchObject<TEGraphicsContext>& Context, const TETexture* Src, TETexture*& Dst) = 0;
	virtual TETexture* CreateTexture(FRHITexture2D* Texture, TETextureOrigin Origin, TETextureComponentMap Map) = 0;
	virtual void ReleaseTexture(const FString& Name, TETexture* Texture) = 0;
	virtual void ReleaseTextures(const bool& bIsFinalClean = false) = 0;
	virtual void QueueTextureRelease(TETexture* Texture) = 0;

	/** Returns the texture resource in it's native format. */
	virtual FTexture2DResource* GetTexture(const TETexture* Texture) = 0;
};
