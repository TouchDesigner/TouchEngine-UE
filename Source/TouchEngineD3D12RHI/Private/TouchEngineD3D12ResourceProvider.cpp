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
#include "D3D12TouchUtils.h"
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "Exporting/TouchTextureExporterD3D12.h"
#include "ITouchEngineModule.h"
#include "Algo/AnyOf.h"
#include "Importing/TouchTextureImporterD3D12.h"
#include "Rendering/TouchResourceProvider.h"
#include "Util/TouchFenceCache.h"

#include "TouchEngine/TED3D12.h"
#include "Util/FutureSyncPoint.h"

namespace UE::TouchEngine::D3DX12
{
	namespace Private
	{
		static bool SupportsNeededTextureTypes(TEInstance* Instance)
		{
			int32_t Count = 0;
			TEInstanceGetSupportedTextureTypes(Instance, nullptr, &Count);
			TArray<TETextureType> TextureTypes;
			TextureTypes.SetNumUninitialized(Count);
			TEInstanceGetSupportedTextureTypes(Instance, TextureTypes.GetData(), &Count);

			return TextureTypes.Contains(TETextureTypeD3DShared);
		}

		static bool SupportsNeededHandleTypes(TEInstance* Instance)
		{
			int32_t Count = 0;
			TEInstanceGetSupportedD3DHandleTypes(Instance, nullptr, &Count);
			TArray<TED3DHandleType> HandleTypes;
			HandleTypes.SetNumUninitialized(Count);
			TEInstanceGetSupportedD3DHandleTypes(Instance, HandleTypes.GetData(), &Count);

			return HandleTypes.Contains(TED3DHandleTypeD3D12ResourceNT );
		}
	}
	
	/** */
	class FTouchEngineD3X12ResourceProvider : public FTouchResourceProvider
	{
	public:
		
		FTouchEngineD3X12ResourceProvider(ID3D12Device* Device, TouchObject<TED3D12Context> TEContext, TSharedRef<FTouchFenceCache> FenceCache, TSharedRef<FTouchTextureExporterD3D12> TextureExporter);

		virtual void ConfigureInstance(const TouchObject<TEInstance>& Instance) override {}
		virtual TEGraphicsContext* GetContext() const override;
		virtual FTouchLoadInstanceResult ValidateLoadedTouchEngine(TEInstance& Instance) override;
		virtual TSet<EPixelFormat> GetExportablePixelTypes(TEInstance& Instance) override;
		virtual void PrepareForExportToTouchEngine_AnyThread() override;
		virtual TouchObject<TETexture> ExportTextureToTouchEngineInternal_AnyThread(const FTouchExportParameters& Params) override;
		virtual void FinalizeExportToTouchEngine_AnyThread() override;
		// virtual TFuture<FTouchTextureImportResult> ImportTextureToUnrealEngine_AnyThread(const FTouchImportParameters& LinkParams, TSharedPtr<FTouchFrameCooker> FrameCooker) override;
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() override;
		
	protected:
		virtual FTouchTextureImporter& GetImporter() override { return TextureLinker.Get(); }

	private:

		TouchObject<TED3D12Context> TEContext;
		TSharedRef<FTouchFenceCache> FenceCache;
		TSharedRef<FTouchTextureExporterD3D12> TextureExporter;
		TSharedRef<FTouchTextureImporterD3D12> TextureLinker;
	};

	TSharedPtr<FTouchResourceProvider> MakeD3DX12ResourceProvider(const FResourceProviderInitArgs& InitArgs)
	{
		ID3D12Device* Device = (ID3D12Device*)GDynamicRHI->RHIGetNativeDevice();
		if (!Device)
		{
			InitArgs.LoadErrorCallback(TEXT("Unable to obtain DX12 Device."));
			return nullptr;
		}

		TouchObject<TED3D12Context> TEContext = nullptr;
		const TEResult Res = TED3D12ContextCreate(Device, TEContext.take());
		if (Res != TEResultSuccess)
		{
			InitArgs.ResultCallback(Res, TEXT("Unable to create TouchEngine Context"));
			return nullptr;
		}

		TSharedRef<FTouchFenceCache> FenceCache = MakeShared<FTouchFenceCache>(Device);
		const TSharedPtr<FTouchTextureExporterD3D12> TextureExporter = FTouchTextureExporterD3D12::Create(FenceCache);
		if (!TextureExporter)
		{
			InitArgs.ResultCallback(Res, TEXT("Unable to create FTouchTextureExporterD3D12"));
			return nullptr;
		}
    
		return MakeShared<FTouchEngineD3X12ResourceProvider>(Device, MoveTemp(TEContext), FenceCache, TextureExporter.ToSharedRef());
	}

	FTouchEngineD3X12ResourceProvider::FTouchEngineD3X12ResourceProvider(ID3D12Device* Device, TouchObject<TED3D12Context> TEContext, TSharedRef<FTouchFenceCache> FenceCache, TSharedRef<FTouchTextureExporterD3D12> TextureExporter)
		: TEContext(MoveTemp(TEContext))
		, FenceCache(MoveTemp(FenceCache))
		, TextureExporter(MoveTemp(TextureExporter))
		, TextureLinker(MakeShared<FTouchTextureImporterD3D12>(Device, FenceCache))
	{}

	TEGraphicsContext* FTouchEngineD3X12ResourceProvider::GetContext() const
	{
		return TEContext;
	}
	
	FTouchLoadInstanceResult FTouchEngineD3X12ResourceProvider::ValidateLoadedTouchEngine(TEInstance& Instance)
	{
		if (!Private::SupportsNeededTextureTypes(&Instance))
		{
			return FTouchLoadInstanceResult::MakeFailure(TEXT("Texture type TETextureTypeD3DShared is not supported by this TouchEngine instance."));
		}
		
		if (!Private::SupportsNeededHandleTypes(&Instance))
		{
			return FTouchLoadInstanceResult::MakeFailure(TEXT("DirectX 12 shared textures are not supported. This may be a limitation of the hardware or the version of TouchDesigner installed"));
		}
		
		return FTouchLoadInstanceResult::MakeSuccess();
	}
	
	TSet<EPixelFormat> FTouchEngineD3X12ResourceProvider::GetExportablePixelTypes(TEInstance& Instance)
	{
		int32 Count = 0;
		const TEResult ResultGettingCount = TEInstanceGetSupportedD3DFormats(&Instance, nullptr, &Count);
		if (ResultGettingCount != TEResultInsufficientMemory)
		{
			return {};
		}

		TArray<DXGI_FORMAT> SupportedTypes;
		SupportedTypes.SetNumZeroed(Count);
		const TEResult ResultGettingTypes = TEInstanceGetSupportedD3DFormats(&Instance, SupportedTypes.GetData(), &Count);
		if (ResultGettingTypes != TEResultSuccess)
		{
			return {};
		}

		TSet<EPixelFormat> Formats;
		Formats.Reserve(SupportedTypes.Num());
		for (DXGI_FORMAT Format : SupportedTypes)
		{
			const EPixelFormat PixelFormat = ConvertD3FormatToPixelFormat(Format);
			if (PixelFormat != PF_Unknown)
			{
				Formats.Add(PixelFormat);
			}
		}
		return Formats;
	}

	void FTouchEngineD3X12ResourceProvider::PrepareForExportToTouchEngine_AnyThread()
	{
		TextureExporter->PrepareForExportToTouchEngine_AnyThread();
	}

	TouchObject<TETexture> FTouchEngineD3X12ResourceProvider::ExportTextureToTouchEngineInternal_AnyThread(const FTouchExportParameters& Params) //needs to be in this class as it links the both subclasses
	{
		return TextureExporter->ExportTextureToTouchEngine_AnyThread(Params, GetContext());
	}

	void FTouchEngineD3X12ResourceProvider::FinalizeExportToTouchEngine_AnyThread()
	{
		TextureExporter->FinalizeExportToTouchEngine_AnyThread();
	}

	// TFuture<FTouchTextureImportResult> FTouchEngineD3X12ResourceProvider::ImportTextureToUnrealEngine_AnyThread(const FTouchImportParameters& LinkParams, TSharedPtr<FTouchFrameCooker> FrameCooker)
	// {
	// 	return TextureLinker->ImportTexture_AnyThread(LinkParams, FrameCooker);
	// }

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
