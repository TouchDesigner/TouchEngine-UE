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

/* Unfortunately, this section can not be alphabetically ordered.
 * D3D11Resources is not IWYU compliant and we must include the other files
 * before it to fix its incomplete type dependencies. */
// #include "Windows/AllowWindowsPlatformTypes.h"
// #include <d3d11.h>
// #include "Windows/HideWindowsPlatformTypes.h"

#include "TouchEngineD3X11ResourceProvider.h"

#include "D3D11RHIPrivate.h"
// #include "D3D11Util.h"
// #include "D3D11State.h"
// #include "D3D11Resources.h"


#include "ITouchEngineModule.h"
#include "Engine/TouchEngine.h"

#include "TouchEngine/TED3D11.h"
#include "TouchEngine/TouchObject.h"

#include <deque>

#include "TouchTextureLinkerD3D11.h"
#include "Rendering/TouchResourceProvider.h"
#include "Rendering/TouchResourceProviderProxy.h"

 static DXGI_FORMAT toTypedDXGIFormat(EPixelFormat Format)
{
 	switch (Format)
 	{
 	case PF_DXT1:
	case PF_DXT3:
	case PF_DXT5:
	default:
		return DXGI_FORMAT_UNKNOWN;
 	case PF_G8:
 		return DXGI_FORMAT_R8_UNORM;
 	case PF_G16:
 		return DXGI_FORMAT_R16_UNORM;
 	case PF_A32B32G32R32F:
 		return DXGI_FORMAT_R32G32B32A32_FLOAT;
 	case PF_B8G8R8A8:
 		return DXGI_FORMAT_B8G8R8A8_UNORM;
 	case PF_FloatRGB:
 	case PF_FloatRGBA:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
 	case PF_R32_FLOAT:
 		return DXGI_FORMAT_R32_FLOAT;
 	case PF_G16R16:
 		return DXGI_FORMAT_R16G16_UNORM;
 	case PF_G16R16F:
 		return DXGI_FORMAT_R16G16_FLOAT;
 	case PF_G32R32F:
 		return DXGI_FORMAT_R32G32_FLOAT;
 	case PF_A2B10G10R10:
 		return DXGI_FORMAT_R10G10B10A2_UNORM;
 	case PF_A16B16G16R16:
 		return DXGI_FORMAT_R16G16B16A16_UNORM;
 	case PF_R16F:
 		return DXGI_FORMAT_R16_FLOAT;
 	case PF_FloatR11G11B10:
 		return DXGI_FORMAT_R11G11B10_FLOAT;
 	case PF_R8G8B8A8:
 		return DXGI_FORMAT_R8G8B8A8_UNORM;
 	case PF_R8G8:
 		return DXGI_FORMAT_R8G8_UNORM;
 	case PF_R16G16B16A16_UNORM:
 		return DXGI_FORMAT_R16G16B16A16_UNORM;
 	}
}

static bool IsTypeless(DXGI_FORMAT Format)
{
 	switch (Format)
 	{
 	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC7_TYPELESS:
 		return true;
 	default:
 		return false;
 	}
}

namespace UE::TouchEngine::D3DX11
{
	class TexCleanup
    {
    public:
    	ID3D11Query*		Query = nullptr;
    	TED3D11Texture*		Texture = nullptr;
    };

 	struct FTextureLinkData
 	{
 		UTexture2D* Texture;
 	};
	
	/** */
	class FTouchEngineD3X11ResourceProvider : public FTouchResourceProvider
	{
	public:
		
		FTouchEngineD3X11ResourceProvider(TED3D11Context& TEContext, ID3D11Device& Device, ID3D11DeviceContext& DeviceContext);
		void Release_RenderThread();

		virtual TEGraphicsContext* GetContext() const override;
		virtual TEResult CreateTexture(TETexture* InSrc, TETexture*& InDst) override;
		virtual TFuture<FTouchLinkResult> LinkTexture(const FTouchLinkParameters& LinkParams) override;
		virtual TETexture* CreateTextureWithFormat(FRHITexture2D* InTexture, TETextureOrigin InOrigin, TETextureComponentMap InMap, EPixelFormat InFormat) override;
		virtual void ReleaseTexture(FTouchTOP& InTexture) override;
		virtual void ReleaseTextures_RenderThread(bool bIsFinalClean = false) override;
		virtual void QueueTextureRelease_RenderThread(TETexture* InTexture) override;
		virtual FTexture2DResource* GetTexture(const TETexture* InTexture) override;

		virtual bool IsSupportedFormat(EPixelFormat InFormat) override;

	private:
		
		TED3D11Context*	TEContext = nullptr;
		ID3D11Device* Device;
		ID3D11DeviceContext* DeviceContext;
		std::deque<TexCleanup> TextureCleanups;

		/** Implements LinkTexture */
		FTouchTextureLinkerD3D11 TextureLinker;
	};

	TSharedPtr<FTouchResourceProvider> MakeD3DX11ResourceProvider(const FResourceProviderInitArgs& InitArgs)
	{
		ID3D11Device* Device = (ID3D11Device*)GDynamicRHI->RHIGetNativeDevice();
		if (!Device)
		{
			InitArgs.LoadErrorCallback(TEXT("Unable to obtain DX11 Device."));
			return nullptr;
		}

		ID3D11DeviceContext* DeviceContext;
		Device->GetImmediateContext(&DeviceContext);
		if (!DeviceContext)
		{
			InitArgs.LoadErrorCallback(TEXT("Unable to obtain DX11 Context."));
			return nullptr;
		}

		TED3D11Context* TEContext = nullptr;
		const TEResult Res = TED3D11ContextCreate(Device, &TEContext);
		if (Res != TEResultSuccess)
		{
			InitArgs.ResultCallback(Res, TEXT("Unable to create TouchEngine Context"));
			return nullptr;
		}
    
		const TSharedRef<FTouchEngineD3X11ResourceProvider> Impl = MakeShared<FTouchEngineD3X11ResourceProvider>(*TEContext, *Device, *DeviceContext);
		return MakeShared<FTouchResourceProviderProxy>(Impl, FRenderThreadCallback::CreateSP(Impl, &FTouchEngineD3X11ResourceProvider::Release_RenderThread));
	}

	FTouchEngineD3X11ResourceProvider::FTouchEngineD3X11ResourceProvider(
		TED3D11Context& TEContext,
		ID3D11Device& Device,
		ID3D11DeviceContext& DeviceContext)
		: TEContext(&TEContext)
		, Device(&Device)
		, DeviceContext(&DeviceContext)
 		, TextureLinker(TEContext)
    {}

    void FTouchEngineD3X11ResourceProvider::Release_RenderThread()
    {
     	ensure(IsInRenderingThread());
    
     	ReleaseTextures_RenderThread();
     	if (TextureCleanups.size())
     	{
     		TextureCleanups.clear();
     	}
     	DeviceContext = nullptr;
     	Device = nullptr;
    }
    
    TEGraphicsContext* FTouchEngineD3X11ResourceProvider::GetContext() const
    {
     	return TEContext;
    }
    
    TEResult FTouchEngineD3X11ResourceProvider::CreateTexture(TETexture* InSrc, TETexture*& InDst)
    {
		TED3DSharedTexture* TextureDXGI = static_cast<TED3DSharedTexture*>(InSrc);
		TED3D11Texture* Output = nullptr;
    	const TEResult Result = TED3D11ContextCreateTexture(TEContext, TextureDXGI, &Output);
		if (Output)
		{
			InDst = Output;
		}
		return Result;
    }

 	TFuture<FTouchLinkResult> FTouchEngineD3X11ResourceProvider::LinkTexture(const FTouchLinkParameters& LinkParams)
	{
		return TextureLinker.LinkTexture(LinkParams);
	}
    
    TETexture* FTouchEngineD3X11ResourceProvider::CreateTextureWithFormat(FRHITexture2D* InTexture, TETextureOrigin InOrigin, TETextureComponentMap InMap, EPixelFormat InFormat)
    {
    	ID3D11Texture2D* D3D11Texture = static_cast<ID3D11Texture2D*>(GetD3D11TextureFromRHITexture(InTexture)->GetResource());
    	const DXGI_FORMAT TypedDXGIFormat = (toTypedDXGIFormat(InFormat));
    
    	D3D11_TEXTURE2D_DESC Desc;
    	D3D11Texture->GetDesc(&Desc);
    	if (IsTypeless(Desc.Format))
    	{
    		return TED3D11TextureCreateTypeless(D3D11Texture, InOrigin, InMap, TypedDXGIFormat, nullptr, nullptr);
    	}
    	else
    	{
    		return TED3D11TextureCreate(D3D11Texture, InOrigin, InMap, nullptr, nullptr);
    	}
    }
    
    void FTouchEngineD3X11ResourceProvider::ReleaseTexture(FTouchTOP& InTexture)
    {
    	if (InTexture.WrappedResource)
    	{
    		auto Res = static_cast<ID3D11Resource*>(InTexture.WrappedResource);
    		Res->Release();
    		InTexture.WrappedResource = nullptr;
    	}
    
    	InTexture.Texture = nullptr;
    }
    
    void FTouchEngineD3X11ResourceProvider::ReleaseTextures_RenderThread(bool bIsFinalClean)
    {
    	checkf(IsInRenderingThread(),
    		 TEXT("ReleaseTextures must run on the rendering thread, otherwise"
    			 " the chance for data corruption due to the RenderThread and another"
    			 " one accessing the same textures is high."))
    
    	while (!TextureCleanups.empty())
    	{
    		 TexCleanup& Cleanup = TextureCleanups.front();
    
    
    	 	 if (!Cleanup.Query || !Cleanup.Texture)
    	 	 {
    	 		TextureCleanups.pop_front();
    	 		continue;
    	 	 }
    
    	 	bool Result = false;
    
    	 	DeviceContext->GetData(Cleanup.Query, &Result, sizeof(Result), 0);
    	 	if (Result)
    	 	{
    	 		TERelease(&Cleanup.Texture);
    	 		Cleanup.Query->Release();
    	 		TextureCleanups.pop_front();
    	 	}
    	 	else
    	 	{
    	 		if (bIsFinalClean)
    	 		{
    	 			FPlatformProcess::Sleep(1);
    	 		}
    	 		else
    	 		{
    	 			break;
    	 		}
    	 	}
    	 }
    }
    
    void FTouchEngineD3X11ResourceProvider::QueueTextureRelease_RenderThread(TETexture* InTexture)
    {
    	check(IsInRenderingThread());
    
    	TexCleanup Cleanup;
    	D3D11_QUERY_DESC QueryDesc = {};
    	QueryDesc.Query = D3D11_QUERY_EVENT;
    	HRESULT hresult = Device->CreateQuery(&QueryDesc, &Cleanup.Query);
    	if (hresult == 0)
    	{
    		DeviceContext->End(Cleanup.Query);
    		if (InTexture)
    		{
    			Cleanup.Texture = static_cast<TED3D11Texture*>(InTexture);
    			TextureCleanups.push_back(Cleanup);
    		}
    	}
    }
    
    FTexture2DResource* FTouchEngineD3X11ResourceProvider::GetTexture(const TETexture* InTexture)
    {
		TETextureType Type = TETextureGetType(InTexture);
		if (!ensure(Type == TETextureTypeD3D))
		{
			return nullptr;
		}

    	ID3D11Texture2D* D3D11Texture = TED3D11TextureGetTexture(static_cast<const TED3D11Texture*>(InTexture));
		
		FD3D11DynamicRHI* DynamicRHI = static_cast<FD3D11DynamicRHI*>(GDynamicRHI);
		//DynamicRHI->RHICreateTexture2DFromResource()
		
    	return (FTexture2DResource*)D3D11Texture;
    }
    
    bool FTouchEngineD3X11ResourceProvider::IsSupportedFormat(EPixelFormat InFormat)
    {
    	return toTypedDXGIFormat(InFormat) != DXGI_FORMAT_UNKNOWN;
    }
}
