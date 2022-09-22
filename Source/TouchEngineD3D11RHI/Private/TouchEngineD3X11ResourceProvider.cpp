 // Copyright © Derivative Inc. 2022

#include "TouchEngineD3X11ResourceProvider.h"

#include "ITouchEngineModule.h"
#include "Engine/TouchEngine.h"

/* Unfortunately, this section can not be alphabetically ordered.
 * D3D11Resources is not IWYU compliant and we must include the other files
 * before it to fix its incomplete type dependencies. */
// #include "Windows/AllowWindowsPlatformTypes.h"
// #include <d3d11.h>
// #include "Windows/HideWindowsPlatformTypes.h"
#include "D3D11RHIPrivate.h"
// #include "D3D11Util.h"
// #include "D3D11State.h"
// #include "D3D11Resources.h"

#include "TouchEngine/TED3D11.h"
#include "TouchEngine/TouchObject.h"

#include <deque>

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
	
	/** */
	class FTouchEngineD3X11ResourceProvider : public FTouchResourceProvider
	{
	public:
		
		FTouchEngineD3X11ResourceProvider(TouchObject<TED3D11Context> TEContext, ID3D11Device& Device, ID3D11DeviceContext& DeviceContext);
		void Release_RenderThread();

		virtual TEGraphicsContext* GetContext() const override;
		virtual TEResult CreateContext(FTouchEngineDevice* InDevice, TEGraphicsContext*& InContext) override;
		virtual TEResult CreateTexture(const TETexture* InSrc, TETexture*& InDst) override;
		virtual TETexture* CreateTexture(FRHITexture2D* InTexture, TETextureOrigin InOrigin, TETextureComponentMap InMap) override;
		virtual TETexture* CreateTextureWithFormat(FRHITexture2D* InTexture, TETextureOrigin InOrigin, TETextureComponentMap InMap, EPixelFormat InFormat) override;
		virtual void ReleaseTexture(const FString& InName, TETexture* InTexture) override;
		virtual void ReleaseTexture(FTouchTOP& InTexture) override;
		virtual void ReleaseTextures_RenderThread(bool bIsFinalClean = false) override;
		virtual void QueueTextureRelease_RenderThread(TETexture* InTexture) override;
		virtual FTexture2DResource* GetTexture(const TETexture* InTexture) override;
		//virtual void CopyResource()

		virtual bool IsSupportedFormat(EPixelFormat InFormat) override;

	private:
		
		TouchObject<TED3D11Context>	TEContext = nullptr;
		ID3D11Device* Device;
		ID3D11DeviceContext* DeviceContext;
		std::deque<TexCleanup> TextureCleanups;
		
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

		TouchObject<TED3D11Context> TEContext;
		const TEResult Res = TED3D11ContextCreate(Device, TEContext.take());
		InitArgs.ResultCallback(Res, TEXT("Unable to create TouchEngine Context"));
		if (Res != TEResultSuccess)
		{
			return nullptr;
		}
    
		const TSharedRef<FTouchEngineD3X11ResourceProvider> Impl = MakeShared<FTouchEngineD3X11ResourceProvider>(TEContext, *Device, *DeviceContext);
		return MakeShared<FTouchResourceProviderProxy>(Impl, FRenderThreadCallback::CreateSP(Impl, &FTouchEngineD3X11ResourceProvider::Release_RenderThread));
	}

	FTouchEngineD3X11ResourceProvider::FTouchEngineD3X11ResourceProvider(
		TouchObject<TED3D11Context> TEContext,
		ID3D11Device& Device,
		ID3D11DeviceContext& DeviceContext)
		: TEContext(MoveTemp(TEContext))
		, Device(&Device)
		, DeviceContext(&DeviceContext)
    {}

    void FTouchEngineD3X11ResourceProvider::Release_RenderThread()
    {
     	ensure(IsInRenderingThread());
    
     	ReleaseTextures_RenderThread();
     	//CleanupTextures(MyImmediateContext, &MyTexCleanups, FinalClean::True);
    
     	if (TextureCleanups.size())
     		TextureCleanups.clear();
     	DeviceContext = nullptr;
     	Device = nullptr;
    }
    
    TEGraphicsContext* FTouchEngineD3X11ResourceProvider::GetContext() const
    {
     	return TEContext;
    }
    
    TEResult FTouchEngineD3X11ResourceProvider::CreateContext(FTouchEngineDevice* InDevice, TEGraphicsContext*& InContext)
    {
    	return TED3D11ContextCreate(static_cast<ID3D11Device*>(InDevice), static_cast<TED3D11Context**>(InContext));
    }
    
    TEResult FTouchEngineD3X11ResourceProvider::CreateTexture(const TETexture* InSrc, TETexture*& InDst)
    {
    	return TED3D11ContextCreateTexture(TEContext.get(), static_cast<TEDXGITexture*>(const_cast<void*>(InSrc)), static_cast<TED3D11Texture**>(InDst));
    }
    
    TETexture* FTouchEngineD3X11ResourceProvider::CreateTexture(FRHITexture2D* InTexture, TETextureOrigin InOrigin, TETextureComponentMap InMap)
    {
    	ID3D11Texture2D* D3D11Texture = static_cast<ID3D11Texture2D*>(GetD3D11TextureFromRHITexture(InTexture)->GetResource());
    	return TED3D11TextureCreate(D3D11Texture, InOrigin, InMap, nullptr, nullptr);
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
    
    void FTouchEngineD3X11ResourceProvider::ReleaseTexture(const FString& InName, TETexture* InTexture)
    {
    
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
    	ID3D11Texture2D* D3D11Texture = TED3D11TextureGetTexture(static_cast<const TED3D11Texture*>(InTexture));
    	return (FTexture2DResource*)D3D11Texture;
    }
    
    bool FTouchEngineD3X11ResourceProvider::IsSupportedFormat(EPixelFormat InFormat)
    {
    	return toTypedDXGIFormat(InFormat) != DXGI_FORMAT_UNKNOWN;
    }
}
