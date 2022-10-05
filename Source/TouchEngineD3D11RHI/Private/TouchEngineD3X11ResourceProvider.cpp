﻿/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
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
#include "ITouchEngineModule.h"

#include "TouchEngine/TED3D11.h"
#include "TouchEngine/TouchObject.h"

#include "D3D11TouchUtils.h"
#include "TouchTextureExporterD3D11.h"
#include "TouchTextureLinkerD3D11.h"
#include "Rendering/TouchResourceProvider.h"
#include "Rendering/TouchResourceProviderProxy.h"

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
		virtual TFuture<FTouchExportResult> ExportTextureToTouchEngine(const FTouchExportParameters& Params) override;
		virtual TFuture<FTouchLinkResult> LinkTexture(const FTouchLinkParameters& LinkParams) override;

	private:
		
		TED3D11Context*	TEContext = nullptr;
		ID3D11Device* Device;
		ID3D11DeviceContext* DeviceContext;

		/** Util for exporting, i.e. ExportTextureToTouchEngine */
		FTouchTextureExporterD3D11 TextureExporter;
		/** Util for importing, i.e. LinkTexture */
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
 		, TextureLinker(TEContext, DeviceContext)
    {}

    void FTouchEngineD3X11ResourceProvider::Release_RenderThread()
    {
     	ensure(IsInRenderingThread());
    
     	DeviceContext = nullptr;
     	Device = nullptr;
    }
    
    TEGraphicsContext* FTouchEngineD3X11ResourceProvider::GetContext() const
    {
     	return TEContext;
    }

 	TFuture<FTouchExportResult> FTouchEngineD3X11ResourceProvider::ExportTextureToTouchEngine(const FTouchExportParameters& Params)
	{
		return TextureExporter.ExportTextureToTouchEngine(Params);
	}

 	TFuture<FTouchLinkResult> FTouchEngineD3X11ResourceProvider::LinkTexture(const FTouchLinkParameters& LinkParams)
	{
		return TextureLinker.LinkTexture(LinkParams);
	}
}