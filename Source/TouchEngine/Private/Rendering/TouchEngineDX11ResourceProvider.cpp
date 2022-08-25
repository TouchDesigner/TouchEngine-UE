 // Copyright © Derivative Inc. 2022

#include "TouchEngineDX11ResourceProvider.h"

// @todo This is only included for its FTouchTOP, so maybe temp
#include "UTouchEngine.h"

#if PLATFORM_WINDOWS

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

FTouchEngineDX11ResourceProvider::FTouchEngineDX11ResourceProvider()
{

}

bool FTouchEngineDX11ResourceProvider::Initialize(
 	TFunctionRef<void(const FString&)> LoadErrorCallback,
 	TFunctionRef<void(const TEResult, const FString&)> ResultErrorCallback)
{
 	MyDevice = (ID3D11Device*)GDynamicRHI->RHIGetNativeDevice();
 	if (!MyDevice)
 	{
 		LoadErrorCallback(TEXT("Unable to obtain DX11 Device."));
 		return false;
 	}

 	MyDevice->GetImmediateContext(&MyImmediateContext);
 	if (!MyImmediateContext)
 	{
 		LoadErrorCallback(TEXT("Unable to obtain DX11 Context."));
 		return false;
 	}

    const TEResult Res = TED3D11ContextCreate(MyDevice, MyTEContext.take());
	ResultErrorCallback(Res, TEXT("Unable to create TouchEngine Context"));

	if (Res != TEResultSuccess)
	{
		return false;
	}

 	return true;
}

void FTouchEngineDX11ResourceProvider::Release()
{
 	ensure(IsInRenderingThread());

 	ReleaseTextures();
 	//CleanupTextures(MyImmediateContext, &MyTexCleanups, FinalClean::True);

 	if (MyTexCleanups.size())
 		MyTexCleanups.clear();
 	MyImmediateContext = nullptr;
 	MyDevice = nullptr;
}

TEGraphicsContext* FTouchEngineDX11ResourceProvider::GetContext() const
{
 	return MyTEContext;
}

TEResult FTouchEngineDX11ResourceProvider::CreateContext(FTouchEngineDevice* Device, TEGraphicsContext*& Context)
{
	return TED3D11ContextCreate(static_cast<ID3D11Device*>(Device), static_cast<TED3D11Context**>(Context));
}

TEResult FTouchEngineDX11ResourceProvider::CreateTexture(const TETexture* Src, TETexture*& Dst)
{
	return TED3D11ContextCreateTexture(MyTEContext.get(), static_cast<TEDXGITexture*>(const_cast<void*>(Src)), static_cast<TED3D11Texture**>(Dst));
}

TETexture* FTouchEngineDX11ResourceProvider::CreateTexture(FRHITexture2D* Texture, TETextureOrigin Origin, TETextureComponentMap Map)
{
	ID3D11Texture2D* D3D11Texture = static_cast<ID3D11Texture2D*>(GetD3D11TextureFromRHITexture(Texture)->GetResource());
	return TED3D11TextureCreate(D3D11Texture, Origin, Map, nullptr, nullptr);
}

TETexture* FTouchEngineDX11ResourceProvider::CreateTextureWithFormat(FRHITexture2D* Texture, TETextureOrigin Origin, TETextureComponentMap Map, EPixelFormat Format)
{
	ID3D11Texture2D* D3D11Texture = static_cast<ID3D11Texture2D*>(GetD3D11TextureFromRHITexture(Texture)->GetResource());
	const DXGI_FORMAT TypedDXGIFormat = (toTypedDXGIFormat(Format));

	D3D11_TEXTURE2D_DESC Desc;
	D3D11Texture->GetDesc(&Desc);
	if (IsTypeless(Desc.Format))
	{
		return TED3D11TextureCreateTypeless(D3D11Texture, Origin, Map, TypedDXGIFormat, nullptr, nullptr);
	}
	else
	{
		return TED3D11TextureCreate(D3D11Texture, Origin, Map, nullptr, nullptr);
	}
}

void FTouchEngineDX11ResourceProvider::ReleaseTexture(const FString& Name, TETexture* Texture)
{

}

void FTouchEngineDX11ResourceProvider::ReleaseTexture(FTouchTOP& Texture)
{
	if (Texture.WrappedResource)
	{
		auto Res = static_cast<ID3D11Resource*>(Texture.WrappedResource);
		Res->Release();
		Texture.WrappedResource = nullptr;
	}

	Texture.Texture = nullptr;
}

void FTouchEngineDX11ResourceProvider::ReleaseTextures(const bool& bIsFinalClean)
{
	checkf(IsInRenderingThread(),
		 TEXT("ReleaseTextures must run on the rendering thread, otherwise"
			 " the chance for data corruption due to the RenderThread and another"
			 " one accessing the same textures is high."))

	while (!MyTexCleanups.empty())
	{
		 TouchDesigner::DX11::TexCleanup& Cleanup = MyTexCleanups.front();


	 	 if (!Cleanup.Query || !Cleanup.Texture)
	 	 {
	 		MyTexCleanups.pop_front();
	 		continue;
	 	 }

	 	bool Result = false;

	 	MyImmediateContext->GetData(Cleanup.Query, &Result, sizeof(Result), 0);
	 	if (Result)
	 	{
	 		TERelease(&Cleanup.Texture);
	 		Cleanup.Query->Release();
	 		MyTexCleanups.pop_front();
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

void FTouchEngineDX11ResourceProvider::QueueTextureRelease(TETexture* Texture)
{
	ensure(IsInRenderingThread());

	TouchDesigner::DX11::TexCleanup Cleanup;
	D3D11_QUERY_DESC QueryDesc = {};
	QueryDesc.Query = D3D11_QUERY_EVENT;
	HRESULT hresult = MyDevice->CreateQuery(&QueryDesc, &Cleanup.Query);
	if (hresult == 0)
	{
		MyImmediateContext->End(Cleanup.Query);
		if (Texture)
		{
			Cleanup.Texture = static_cast<TED3D11Texture*>(Texture);

			MyTexCleanups.push_back(Cleanup);
		}
	}
}

FTexture2DResource* FTouchEngineDX11ResourceProvider::GetTexture(const TETexture* Texture)
{
	ID3D11Texture2D* D3D11Texture = TED3D11TextureGetTexture(static_cast<const TED3D11Texture*>(Texture));
	return (FTexture2DResource*)D3D11Texture;
}

bool FTouchEngineDX11ResourceProvider::IsSupportedFormat(EPixelFormat Format)
{
	return toTypedDXGIFormat(Format) != DXGI_FORMAT_UNKNOWN;
}

#endif
