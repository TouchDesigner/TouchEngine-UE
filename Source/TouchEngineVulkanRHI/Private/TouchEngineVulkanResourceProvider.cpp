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

#include "VulkanRHIPrivate.h"

#include "vulkan/vulkan_core.h"

#include "TouchEngine/TEVulkan.h"

namespace UE::TouchEngine::Vulkan
{
	class FTouchEngineVulkanResourceProvider : public FTouchResourceProvider
	{
	public:
		
		FTouchEngineVulkanResourceProvider(TouchObject<TEVulkanContext> TEContext);

		virtual TEGraphicsContext* GetContext() const override;
		virtual TFuture<FTouchExportResult> ExportTextureToTouchEngine(const FTouchExportParameters& Params) override;
		virtual TFuture<FTouchImportResult> ImportTextureToUnrealEngine(const FTouchImportParameters& LinkParams) override;
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() override;

	private:

		TouchObject<TEVulkanContext> TEContext;
		TSharedRef<FTouchTextureExporterVulkan> TextureExporter;
		TSharedRef<FTouchTextureImporterVulkan> TextureLinker;
	};

	TSharedPtr<FTouchResourceProvider> MakeVulkanResourceProvider(const FResourceProviderInitArgs& InitArgs)
	{
		// TODO Vulkan
		VkDevice Device = (VkDevice)GDynamicRHI->RHIGetNativeDevice();
		if (!Device)
		{
			InitArgs.LoadErrorCallback(TEXT("Unable to obtain DX12 Device."));
			return nullptr;
		}

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

	FTouchEngineVulkanResourceProvider::FTouchEngineVulkanResourceProvider(TouchObject<TEVulkanContext> TEContext)
		: TEContext(MoveTemp(TEContext))
		, TextureExporter(MakeShared<FTouchTextureExporterVulkan>())
		, TextureLinker(MakeShared<FTouchTextureImporterVulkan>())
	{}

	TEGraphicsContext* FTouchEngineVulkanResourceProvider::GetContext() const
	{
		return TEContext;
	}

	TFuture<FTouchExportResult> FTouchEngineVulkanResourceProvider::ExportTextureToTouchEngine(const FTouchExportParameters& Params)
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
