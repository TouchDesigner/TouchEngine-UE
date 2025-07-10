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

#include "Windows/AllowWindowsPlatformTypes.h"
#include "d3d11.h"
#include "D3D11TouchUtils.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "ITouchEngineModule.h"
#include "TouchEngine/TED3D11.h"
#include "TouchEngine/TouchObject.h"

#include "TouchTextureExporterD3D11.h"
#include "TouchTextureImporterD3D11.h"
#include "Algo/AnyOf.h"
#include "Rendering/TouchResourceProvider.h"
#include "Util/FutureSyncPoint.h"

namespace UE::TouchEngine::D3DX11
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

			return HandleTypes.Contains(TED3DHandleTypeD3D11Global)
				|| HandleTypes.Contains(TED3DHandleTypeD3D11NT);
		}
	}
	
	/** */
	class FTouchEngineD3X11ResourceProvider : public FTouchResourceProvider
	{
	public:
		
		FTouchEngineD3X11ResourceProvider(TouchObject<TED3D11Context> TEContext, ID3D11DeviceContext& DeviceContext);

		virtual TEGraphicsContext* GetContext() const override;
		virtual FTouchLoadInstanceResult ValidateLoadedTouchEngine() override;
		virtual TSet<EPixelFormat> GetExportablePixelTypes(TEInstance& InInstance) override;
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks_GameThread() override;

	protected:
		virtual FTouchTextureImporter& GetTextureImporter() override { return TextureImporter.Get(); }
		virtual FTouchTextureExporter& GetTextureExporter() override { return TextureExporter.Get(); }

	private:
		TouchObject<TED3D11Context>	TEContext;
		/** Util for exporting, i.e. ExportTextureToTouchEngine_AnyThread */
		TSharedRef<FTouchTextureExporterD3D11> TextureExporter;
		/** Util for importing, i.e. LinkTexture */
		TSharedRef<FTouchTextureImporterD3D11> TextureImporter;
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

		TouchObject<TED3D11Context> TEContext = nullptr;
		const TEResult Res = TED3D11ContextCreate(Device, TEContext.take());
		if (Res != TEResultSuccess)
		{
			InitArgs.ResultCallback(Res, TEXT("Unable to create TouchEngine Context"));
			return nullptr;
		}
    
		return MakeShared<FTouchEngineD3X11ResourceProvider>(MoveTemp(TEContext), *DeviceContext);
	}

	FTouchEngineD3X11ResourceProvider::FTouchEngineD3X11ResourceProvider(
		TouchObject<TED3D11Context> InTEContext,
		ID3D11DeviceContext& DeviceContext)
		: TEContext(MoveTemp(InTEContext))
		, TextureExporter(MakeShared<FTouchTextureExporterD3D11>())
 		, TextureImporter(MakeShared<FTouchTextureImporterD3D11>(TEContext, DeviceContext))
	{}
    
    TEGraphicsContext* FTouchEngineD3X11ResourceProvider::GetContext() const
    {
     	return TEContext;
    }
	
	FTouchLoadInstanceResult FTouchEngineD3X11ResourceProvider::ValidateLoadedTouchEngine()
	{
		if (!Private::SupportsNeededTextureTypes(GetInstance().get()))
		{
			return FTouchLoadInstanceResult::MakeFailure(TEXT("Texture type TETextureTypeD3DShared is not supported by this TouchEngine instance."));
		}
		
		if (!Private::SupportsNeededHandleTypes(GetInstance().get()))
		{
			return FTouchLoadInstanceResult::MakeFailure(TEXT("Handle type TED3DHandleTypeD3D11Global and TED3DHandleTypeD3D11NT are not supported by this TouchEngine instance."));
		}
		
		return FTouchLoadInstanceResult::MakeSuccess();
	}
	
	TSet<EPixelFormat> FTouchEngineD3X11ResourceProvider::GetExportablePixelTypes(TEInstance& InInstance)
	{
		int32 Count = 0;
		const TEResult ResultGettingCount = TEInstanceGetSupportedD3DFormats(&InInstance, nullptr, &Count);
		if (ResultGettingCount != TEResultInsufficientMemory)
		{
			return {};
		}

		TArray<DXGI_FORMAT> SupportedTypes;
		SupportedTypes.SetNumZeroed(Count);
		const TEResult ResultGettingTypes = TEInstanceGetSupportedD3DFormats(&InInstance, SupportedTypes.GetData(), &Count);
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
	
	TFuture<FTouchSuspendResult> FTouchEngineD3X11ResourceProvider::SuspendAsyncTasks_GameThread()
	{
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
		
		TArray<TFuture<FTouchSuspendResult>> Futures;
		Futures.Emplace(TextureExporter->SuspendAsyncTasks());
		Futures.Emplace(TextureImporter->SuspendAsyncTasks());
		FFutureSyncPoint::SyncFutureCompletion<FTouchSuspendResult>(Futures, [Promise = MoveTemp(Promise)]() mutable
		{
			Promise.SetValue(FTouchSuspendResult{});
		});

		return Future;
	}
}
