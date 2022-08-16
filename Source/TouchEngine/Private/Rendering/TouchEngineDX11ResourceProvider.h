// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"
#include <deque>
#include "TouchEngineResourceProvider.h"

struct ID3D11Query;
struct ID3D11Texture;
struct ID3D11Resource;

namespace TouchDesigner::DX11
{
	class TexCleanup
	{
	public:
		ID3D11Query*		Query = nullptr;
		TED3D11Texture*		Texture = nullptr;
	};
 
	struct FTouchTOP
	{
		UTexture2D*				Texture = nullptr;
		ID3D11Resource*	WrappedResource = nullptr;
	};
}

/** */
class FTouchEngineDX11ResourceProvider : public FTouchEngineResourceProvider
{
public:
	FTouchEngineDX11ResourceProvider();

	virtual bool Initialize(TFunctionRef<void(const FString&)> LoadErrorCallback, TFunctionRef<void(const TEResult, const FString&)> ResultErrorCallback) override;
	virtual void Release() override;

	virtual TEGraphicsContext* GetContext() const override;
	virtual TEResult CreateContext(FTouchEngineDevice* Device, TEGraphicsContext*& Context) override;
	virtual TEResult CreateTexture(TouchObject<TEGraphicsContext>& Context, const TETexture* Src, TETexture*& Dst) override;
	virtual TETexture* CreateTexture(FRHITexture2D* Texture, TETextureOrigin Origin, TETextureComponentMap Map) override;
	virtual void ReleaseTexture(const FString& Name, TETexture* Texture) override;
	virtual void ReleaseTextures(const bool& bIsFinalClean = false) override;
	virtual void QueueTextureRelease(TETexture* Texture) override;
	virtual FTexture2DResource* GetTexture(const TETexture* Texture) override;
	//virtual void CopyResource()

private:
	TouchObject<TED3D11Context>									MyTEContext = nullptr;
	struct ID3D11Device*										MyDevice = nullptr;
	struct ID3D11DeviceContext*									MyImmediateContext = nullptr;
	std::deque<TouchDesigner::DX11::TexCleanup>					MyTexCleanups;
};
