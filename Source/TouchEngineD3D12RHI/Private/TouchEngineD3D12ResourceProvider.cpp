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

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include "d3d12.h"
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "Exporting/TouchTextureExporterD3D12.h"
#include "ITouchEngineModule.h"
#include "Linking/TouchTextureLinkerD3D12.h"
#include "Rendering/TouchResourceProvider.h"

#include "TouchEngine/TED3D12.h"
#include "Util/FutureSyncPoint.h"

namespace UE::TouchEngine::D3DX12
{
	/** */
	class FTouchEngineD3X12ResourceProvider : public FTouchResourceProvider
	{
	public:
		
		FTouchEngineD3X12ResourceProvider(ID3D12Device* Device, TouchObject<TED3D12Context> TEContext);

		virtual TEGraphicsContext* GetContext() const override;
		virtual TFuture<FTouchExportResult> ExportTextureToTouchEngine(const FTouchExportParameters& Params) override;
		virtual TFuture<FTouchLinkResult> LinkTexture(const FTouchLinkParameters& LinkParams) override;
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() override;

	private:

		TouchObject<TED3D12Context> TEContext;
		TSharedRef<FTouchTextureExporterD3D12> TextureExporter;
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
    
		return MakeShared<FTouchEngineD3X12ResourceProvider>(Device, MoveTemp(TEContext));
	}

	FTouchEngineD3X12ResourceProvider::FTouchEngineD3X12ResourceProvider(ID3D12Device* Device, TouchObject<TED3D12Context> TEContext)
		: TEContext(MoveTemp(TEContext))
		, TextureExporter(MakeShared<FTouchTextureExporterD3D12>())
		, TextureLinker(MakeShared<FTouchTextureLinkerD3D12>(Device))
	{}

	TEGraphicsContext* FTouchEngineD3X12ResourceProvider::GetContext() const
	{
		return TEContext;
	}

	TFuture<FTouchExportResult> FTouchEngineD3X12ResourceProvider::ExportTextureToTouchEngine(const FTouchExportParameters& Params)
	{
		// TODO DP: Currently disable because the code is WIP
		return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::UnsupportedOperation }).GetFuture();
		//return TextureExporter->ExportTextureToTouchEngine(Params);
	}

	TFuture<FTouchLinkResult> FTouchEngineD3X12ResourceProvider::LinkTexture(const FTouchLinkParameters& LinkParams)
	{
		return TextureLinker->LinkTexture(LinkParams);
	}

	TFuture<FTouchSuspendResult> FTouchEngineD3X12ResourceProvider::SuspendAsyncTasks()
	{
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
		
		TArray<TFuture<FTouchSuspendResult>> Futures;
		Futures.Emplace(TextureExporter->SuspendAsyncTasks());
		Futures.Emplace(TextureLinker->SuspendAsyncTasks());
		FFutureSyncPoint::SyncFutureCompletion<FTouchSuspendResult>(Futures, [Promise = MoveTemp(Promise)]() mutable
		{
			Promise.SetValue(FTouchSuspendResult{});
		});
		
		return Future;
	}
}
