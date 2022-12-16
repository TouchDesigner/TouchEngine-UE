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

#include "RhiIncludeHelper.h"
#include "VulkanRHIPrivate.h"
#include "VulkanTouchUtils.h"

#include "vulkan/vulkan_core.h"

#include "TouchEngine/TEVulkan.h"

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
		virtual TFuture<FTouchExportResult> ExportTextureToTouchEngineInternal(const FTouchExportParameters& Params) override;
		virtual TFuture<FTouchImportResult> ImportTextureToUnrealEngine(const FTouchImportParameters& LinkParams) override;
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() override;

	private:

		TouchObject<TEVulkanContext> TEContext;
#if PLATFORM_WINDOWS
		TSharedRef<FVulkanSharedResourceSecurityAttributes> SharedSecurityAttributes;
#endif
		TSharedRef<FTouchTextureExporterVulkan> TextureExporter;
		TSharedRef<FTouchTextureImporterVulkan> TextureLinker;
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
		, TextureLinker(MakeShared<FTouchTextureImporterVulkan>(SharedSecurityAttributes))
#else
	static_assert("Update Vulkan code for non-Windows platforms")
#endif
	{}

	void FTouchEngineVulkanResourceProvider::ConfigureInstance(const TouchObject<TEInstance>& Instance)
	{
		TextureLinker->ConfigureInstance(Instance);
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

		TSet<EPixelFormat> Formats;
		Formats.Reserve(SupportedTypes.Num());
		for (VkFormat Format : SupportedTypes)
		{
			const EPixelFormat PixelFormat = VulkanToUnrealTextureFormat(Format);
			if (PixelFormat != PF_Unknown)
			{
				Formats.Add(PixelFormat);
			}
		}
		return Formats;
	}
	
	TFuture<FTouchExportResult> FTouchEngineVulkanResourceProvider::ExportTextureToTouchEngineInternal(const FTouchExportParameters& Params)
	{
		return TextureExporter->ExportTextureToTouchEngine(Params);
	}

	TFuture<FTouchImportResult> FTouchEngineVulkanResourceProvider::ImportTextureToUnrealEngine(const FTouchImportParameters& LinkParams)
	{
		return TextureLinker->ImportTexture(LinkParams);
	}

	TFuture<FTouchSuspendResult> FTouchEngineVulkanResourceProvider::SuspendAsyncTasks()
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
