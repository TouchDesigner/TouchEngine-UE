// Copyright © Derivative Inc. 2022

#include "TouchEngineD3D12ResourceProvider.h"

#include "ITouchEngineModule.h"
#include "Rendering/TouchResourceProvider.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "d3d12.h"
#include "TouchEngine/TED3D12.h"

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
		return MakeFulfilledPromise<FTouchLinkResult>(FTouchLinkResult{ ELinkResultType::UnsupportedOperation }).GetFuture();
	}
}
