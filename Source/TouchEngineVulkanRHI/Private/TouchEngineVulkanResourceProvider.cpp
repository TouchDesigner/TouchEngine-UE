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

#include "TouchEngineVulkanResourceProvider.h"

#include "ITouchEngineModule.h"
#include "Exporting/TouchTextureExporterVulkan.h"
#include "Importing/TouchTextureImporterVulkan.h"
#include "Rendering/TouchResourceProvider.h"
#include "Util/FutureSyncPoint.h"
#if PLATFORM_WINDOWS
#include "Util/TextureShareVulkanPlatformWindows.h"
#endif

#include "Engine/Texture.h"


THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END
#if PLATFORM_WINDOWS
#include "VulkanPlatformDefines.h"
#endif
#include "VulkanRHIPrivate.h"
#include "VulkanTouchUtils.h"

#include "TEVulkanInclude.h"

namespace UE::TouchEngine::Vulkan
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

			return TextureTypes.Contains(TETextureTypeVulkan);
		}
	}
	
	class FTouchEngineVulkanResourceProvider : public FTouchResourceProvider
	{
	public:
		FTouchEngineVulkanResourceProvider(TouchObject<TEVulkanContext> InTEContext);

		virtual void ConfigureInstance(const TouchObject<TEInstance>& Instance) override;
		virtual TEGraphicsContext* GetContext() const override;
		virtual FTouchLoadInstanceResult ValidateLoadedTouchEngine(TEInstance& Instance) override;
		virtual TSet<EPixelFormat> GetExportablePixelTypes(TEInstance& Instance) override;
		virtual TouchObject<TETexture> ExportTextureToTouchEngineInternal_AnyThread(const FTouchExportParameters& Params) override;
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() override;
		virtual void FinalizeExportsToTouchEngine_AnyThread(const FTouchEngineInputFrameData& FrameData) override;
		virtual bool SetExportedTexturePoolSize(int ExportedTexturePoolSize) override;
		virtual bool SetImportedTexturePoolSize(int ImportedTexturePoolSize) override;

	protected:
		virtual FTouchTextureImporter& GetImporter() override { return TextureImporter.Get(); }
		
	private:
		TouchObject<TEVulkanContext> TEContext;
#if PLATFORM_WINDOWS
		TSharedRef<FVulkanSharedResourceSecurityAttributes> SharedSecurityAttributes;
#endif
		TSharedRef<FTouchTextureExporterVulkan> TextureExporter;
		TSharedRef<FTouchTextureImporterVulkan> TextureImporter;
	};

	TSharedPtr<FTouchResourceProvider> MakeVulkanResourceProvider(const FResourceProviderInitArgs& InitArgs)
	{
		TouchObject<TEVulkanContext> TEContext = nullptr;
		const VkPhysicalDeviceIDPropertiesKHR& vkPhysicalDeviceIDProperties = GVulkanRHI->GetDevice()->GetDeviceIdProperties();
		const TEResult Res = TEVulkanContextCreate(
			vkPhysicalDeviceIDProperties.deviceUUID,
			vkPhysicalDeviceIDProperties.driverUUID,
			vkPhysicalDeviceIDProperties.deviceLUID,
			static_cast<bool>(vkPhysicalDeviceIDProperties.deviceLUIDValid),
			TETextureOriginTopLeft,
			TEContext.take()
		);
		if (Res != TEResultSuccess)
		{
			InitArgs.ResultCallback(Res, TEXT("Unable to create TouchEngine Context"));
			return nullptr;
		}
		
		return MakeShared<FTouchEngineVulkanResourceProvider>(MoveTemp(TEContext));
	}

	FTouchEngineVulkanResourceProvider::FTouchEngineVulkanResourceProvider(TouchObject<TEVulkanContext> InTEContext)
		: TEContext(MoveTemp(InTEContext))
#if PLATFORM_WINDOWS
		, SharedSecurityAttributes(MakeShared<FVulkanSharedResourceSecurityAttributes>())
		, TextureExporter(MakeShared<FTouchTextureExporterVulkan>(SharedSecurityAttributes))
		, TextureImporter(MakeShared<FTouchTextureImporterVulkan>(SharedSecurityAttributes))
#else
	static_assert("Update Vulkan code for non-Windows platforms")
#endif
	{}

	void FTouchEngineVulkanResourceProvider::ConfigureInstance(const TouchObject<TEInstance>& Instance)
	{
		TextureImporter->ConfigureInstance(Instance);
	}
	
	TEGraphicsContext* FTouchEngineVulkanResourceProvider::GetContext() const
	{
		return TEContext;
	}

	FTouchLoadInstanceResult FTouchEngineVulkanResourceProvider::ValidateLoadedTouchEngine(TEInstance& Instance)
	{
		if (!Private::SupportsNeededTextureTypes(&Instance))
		{
			return FTouchLoadInstanceResult::MakeFailure(TEXT("Texture type TETextureTypeVulkan is not supported by this TouchEngine instance."));
		}

		return FTouchLoadInstanceResult::MakeSuccess();
	}

	TSet<EPixelFormat> FTouchEngineVulkanResourceProvider::GetExportablePixelTypes(TEInstance& Instance)
	{
		int32 Count = 0;
		const TEResult ResultGettingCount = TEInstanceGetSupportedVkFormats(&Instance, nullptr, &Count);
		if (ResultGettingCount != TEResultInsufficientMemory)
		{
			return {};
		}

		TArray<VkFormat> SupportedTypes;
		SupportedTypes.SetNumZeroed(Count);
		const TEResult ResultGettingTypes = TEInstanceGetSupportedVkFormats(&Instance, SupportedTypes.GetData(), &Count);
		if (ResultGettingTypes != TEResultSuccess)
		{
			return {};
		}
		// - Uncomment the below to log the formats supported by TE
		// {
		// 	SupportedTypes.Sort([](const VkFormat& A, const VkFormat& B) { return (int)A < (int)B; });
		// 	FString SupportedTypesString = TEXT("   ==== VULKAN FORMATS ====\n");
		// 	for (const VkFormat& SupportedType : SupportedTypes)
		// 	{
		// 		SupportedTypesString += FString::Printf(TEXT("[%d] %hs\n"),
		// 			 SupportedType, string_VkFormat(SupportedType));
		// 	}
		// 	UE_LOG(LogTemp, Log, TEXT("Formats Supported by TE\n%s"), *SupportedTypesString);
		//
		// 	TArray<FPixelFormatInfo> LocalPixelFormats {GPixelFormats, EPixelFormat::PF_MAX};
		// 	LocalPixelFormats.Sort([](const FPixelFormatInfo& A, const FPixelFormatInfo& B) { return A.Name < B.Name; });
		// 	FString UEFormatString = TEXT("   ==== UE FORMATS TO VULKAN FORMATS ====\n");
		// 	FString VKFormatString = TEXT("   ==== VULKAN FORMATS TO UE FORMATS ====\n");
		// 	for (const FPixelFormatInfo& GlobalPixelFormat : GPixelFormats)
		// 	{
		// 		UEFormatString += FString::Printf(TEXT("[%d] %s		=>		[%d] %hs\n"),
		// 			GlobalPixelFormat.UnrealFormat, GetPixelFormatString(GlobalPixelFormat.UnrealFormat),
		// 			GlobalPixelFormat.PlatformFormat, string_VkFormat(static_cast<VkFormat>(GlobalPixelFormat.PlatformFormat)));
		// 		VKFormatString += FString::Printf(TEXT("[%d] %hs		=>		[%d] %s\n"),
		// 			GlobalPixelFormat.PlatformFormat, string_VkFormat(static_cast<VkFormat>(GlobalPixelFormat.PlatformFormat)), 
		// 			GlobalPixelFormat.UnrealFormat, GetPixelFormatString(GlobalPixelFormat.UnrealFormat));
		// 	}
		// 	UE_LOG(LogTemp, Error, TEXT("%s"), *UEFormatString);
		// 	UE_LOG(LogTemp, Error, TEXT("%s"), *VKFormatString);
		// }

		TSet<EPixelFormat> Formats;
		Formats.Reserve(SupportedTypes.Num());
		for (const VkFormat Format : SupportedTypes)
		{
			bool bIsSRGB;
			const EPixelFormat PixelFormat = VulkanToUnrealTextureFormat(Format, bIsSRGB);
			if (PixelFormat != PF_Unknown)
			{
				Formats.Add(PixelFormat);
			}
		}
		return Formats;
	}

	TouchObject<TETexture> FTouchEngineVulkanResourceProvider::ExportTextureToTouchEngineInternal_AnyThread(const FTouchExportParameters& Params)
	{
		return TextureExporter->ExportTextureToTouchEngine_AnyThread(Params, GetContext());
	}
	
	void FTouchEngineVulkanResourceProvider::FinalizeExportsToTouchEngine_AnyThread(const FTouchEngineInputFrameData& FrameData)
	{
		TextureExporter->FinalizeExportsToTouchEngine_AnyThread(FrameData);
	}

	bool FTouchEngineVulkanResourceProvider::SetExportedTexturePoolSize(int ExportedTexturePoolSize)
	{
		TextureExporter->PoolSize = FMath::Max(ExportedTexturePoolSize, 0);
		return true;
	}

	bool FTouchEngineVulkanResourceProvider::SetImportedTexturePoolSize(int ImportedTexturePoolSize)
	{
		TextureImporter->PoolSize = FMath::Max(ImportedTexturePoolSize, 0);
		return true;
	}

	TFuture<FTouchSuspendResult> FTouchEngineVulkanResourceProvider::SuspendAsyncTasks()
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
