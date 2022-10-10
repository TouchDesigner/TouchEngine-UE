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

#include "TouchEngineD3D12ResourceProvider.h"

#include "ITouchEngineModule.h"
#include "Rendering/TouchResourceProvider.h"

#include "Windows/PreWindowsApi.h"
#include "d3d12.h"
#include "TouchTextureLinkerD3D12.h"
#include "TouchEngine/TED3D12.h"
#include "Windows/PostWindowsApi.h"

namespace UE::TouchEngine::D3DX12
{
	/** */
	class FTouchEngineD3X12ResourceProvider : public FTouchResourceProvider
	{
	public:
		
		FTouchEngineD3X12ResourceProvider(TouchObject<TED3D12Context> TEContext);

		virtual TEGraphicsContext* GetContext() const override;
		virtual TFuture<FTouchExportResult> ExportTextureToTouchEngine(const FTouchExportParameters& Params) override;
		virtual TFuture<FTouchLinkResult> LinkTexture(const FTouchLinkParameters& LinkParams) override;

	private:

		TouchObject<TED3D12Context> TEContext;
		TSharedRef<FTouchTextureLinkerD3D12> TextureLinker;
	};

	TSharedPtr<FTouchResourceProvider> MakeD3DX12ResourceProvider(const FResourceProviderInitArgs& InitArgs)
	{
		ID3D12Device* Device = (ID3D12Device*)GDynamicRHI->RHIGetNativeDevice();
		if (!Device)
		{
			InitArgs.LoadErrorCallback(TEXT("Unable to obtain DX11 Device."));
			return nullptr;
		}

		TouchObject<TED3D12Context> TEContext = nullptr;
		const TEResult Res = TED3D12ContextCreate(Device, TEContext.take());
		if (Res != TEResultSuccess)
		{
			InitArgs.ResultCallback(Res, TEXT("Unable to create TouchEngine Context"));
			return nullptr;
		}
    
		return MakeShared<FTouchEngineD3X12ResourceProvider>(MoveTemp(TEContext));
	
	}

	FTouchEngineD3X12ResourceProvider::FTouchEngineD3X12ResourceProvider(TouchObject<TED3D12Context> TEContext)
		: TEContext(MoveTemp(TEContext))
		, TextureLinker(MakeShared<FTouchTextureLinkerD3D12>())
	{}

	TEGraphicsContext* FTouchEngineD3X12ResourceProvider::GetContext() const
	{
		return TEContext;
	}

	TFuture<FTouchExportResult> FTouchEngineD3X12ResourceProvider::ExportTextureToTouchEngine(const FTouchExportParameters& Params)
	{
		return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::UnsupportedOperation }).GetFuture();
	}

	TFuture<FTouchLinkResult> FTouchEngineD3X12ResourceProvider::LinkTexture(const FTouchLinkParameters& LinkParams)
	{
		return TextureLinker->LinkTexture(LinkParams);
	}
}
