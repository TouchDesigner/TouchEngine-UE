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
#include "d3d12.h"
#include "D3D12TouchUtils.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "Exporting/TouchTextureExporterD3D12.h"
#include "ITouchEngineModule.h"
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

		virtual TEGraphicsContext* GetContext() const override;
		virtual FTouchLoadInstanceResult ValidateLoadedTouchEngine() override;
		virtual TSet<EPixelFormat> GetExportablePixelTypes(TEInstance& InInstance) override;
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks_GameThread() override;

	protected:
		virtual FTouchTextureImporter& GetTextureImporter() override { return TextureImporter.Get(); }
		virtual FTouchTextureExporter& GetTextureExporter() override { return TextureExporter.Get(); }

	private:

		TouchObject<TED3D12Context> TEContext;
		TSharedRef<FTouchFenceCache> FenceCache;
		TSharedRef<FTouchTextureExporterD3D12> TextureExporter;
		TSharedRef<FTouchTextureImporterD3D12> TextureImporter;
	};

	TSharedPtr<FTouchResourceProvider> MakeD3DX12ResourceProvider(const FResourceProviderInitArgs& InitArgs)
	{
		ID3D12Device* Device = static_cast<ID3D12Device*>(GDynamicRHI->RHIGetNativeDevice());
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
		const TSharedPtr<FTouchTextureExporterD3D12> TextureExporter = MakeShared<FTouchTextureExporterD3D12>(FenceCache); // FTouchTextureExporterD3D12::Create(FenceCache);
		if (!TextureExporter)
		{
			InitArgs.ResultCallback(Res, TEXT("Unable to create FTouchTextureExporterD3D12"));
			return nullptr;
		}

		TSharedRef<FTouchEngineD3X12ResourceProvider> Provider = MakeShared<FTouchEngineD3X12ResourceProvider>(Device, MoveTemp(TEContext), FenceCache, TextureExporter.ToSharedRef());
		TextureExporter->Initialize(Provider);
		return Provider;
	}

	FTouchEngineD3X12ResourceProvider::FTouchEngineD3X12ResourceProvider(ID3D12Device* Device, TouchObject<TED3D12Context> TEContext, TSharedRef<FTouchFenceCache> FenceCache, TSharedRef<FTouchTextureExporterD3D12> TextureExporter)
		: TEContext(MoveTemp(TEContext))
		, FenceCache(MoveTemp(FenceCache))
		, TextureExporter(MoveTemp(TextureExporter))
		, TextureImporter(MakeShared<FTouchTextureImporterD3D12>(Device, FenceCache))
	{}

	TEGraphicsContext* FTouchEngineD3X12ResourceProvider::GetContext() const
	{
		return TEContext;
	}
	
	FTouchLoadInstanceResult FTouchEngineD3X12ResourceProvider::ValidateLoadedTouchEngine()
	{
		if (!Private::SupportsNeededTextureTypes(GetInstance().get()))
		{
			return FTouchLoadInstanceResult::MakeFailure(TEXT("Texture type TETextureTypeD3DShared is not supported by this TouchEngine instance."));
		}
		
		if (!Private::SupportsNeededHandleTypes(GetInstance().get()))
		{
			return FTouchLoadInstanceResult::MakeFailure(TEXT("DirectX 12 shared textures are not supported. This may be a limitation of the hardware or the version of TouchDesigner installed"));
		}
		
		return FTouchLoadInstanceResult::MakeSuccess();
	}
	
	TSet<EPixelFormat> FTouchEngineD3X12ResourceProvider::GetExportablePixelTypes(TEInstance& InInstance)
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
		// - Uncomment the below to log the formats supported by TE
		// {
		// 	SupportedTypes.Sort([](const DXGI_FORMAT& A, const DXGI_FORMAT& B) { return (int)A < (int)B; });
		// 	FString SupportedTypesString = TEXT("   ==== DX12 FORMATS ====\n");
		// 	for (const DXGI_FORMAT& SupportedType : SupportedTypes)
		// 	{
		// 		SupportedTypesString += FString::Printf(TEXT("[%d] %s\n"),
		// 			 SupportedType, GetD3D12TextureFormatString(SupportedType));
		// 	}
		// 	UE_LOG(LogTemp, Log, TEXT("Formats Supported by TE\n%s"), *SupportedTypesString);
		//
		// 	TArray<FPixelFormatInfo> LocalPixelFormats {GPixelFormats, EPixelFormat::PF_MAX};
		// 	LocalPixelFormats.Sort([](const FPixelFormatInfo& A, const FPixelFormatInfo& B) { return A.Name < B.Name; });
		// 	FString UEFormatString = TEXT("   ==== UE FORMATS TO DX12 FORMATS ====\n");
		// 	FString DXFormatString = TEXT("   ==== DX12 FORMATS TO UE FORMATS ====\n");
		// 	for (const FPixelFormatInfo& GlobalPixelFormat : GPixelFormats)
		// 	{
		// 		UEFormatString += FString::Printf(TEXT("[%d] %s		=>		[%d] %s\n"),
		// 			GlobalPixelFormat.UnrealFormat, GetPixelFormatString(GlobalPixelFormat.UnrealFormat),
		// 			GlobalPixelFormat.PlatformFormat, GetD3D12TextureFormatString(static_cast<DXGI_FORMAT>(GlobalPixelFormat.PlatformFormat)));
		// 		DXFormatString += FString::Printf(TEXT("[%d] %s		=>		[%d] %s\n"),
		// 			GlobalPixelFormat.PlatformFormat, GetD3D12TextureFormatString(static_cast<DXGI_FORMAT>(GlobalPixelFormat.PlatformFormat)), 
		// 			GlobalPixelFormat.UnrealFormat, GetPixelFormatString(GlobalPixelFormat.UnrealFormat));
		// 	}
		// 	UE_LOG(LogTemp, Log, TEXT("%s"), *UEFormatString);
		// 	UE_LOG(LogTemp, Log, TEXT("%s"), *DXFormatString);
		// }
				
		TSet<EPixelFormat> Formats;
		Formats.Reserve(SupportedTypes.Num());
		for (const DXGI_FORMAT Format : SupportedTypes)
		{
			bool IsSRGB;
			const EPixelFormat PixelFormat = ConvertD3FormatToPixelFormat(Format, IsSRGB);
			if (PixelFormat != PF_Unknown)
			{
				Formats.Add(PixelFormat);
			}
		}
		return Formats;
	}
	
	TFuture<FTouchSuspendResult> FTouchEngineD3X12ResourceProvider::SuspendAsyncTasks_GameThread()
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
