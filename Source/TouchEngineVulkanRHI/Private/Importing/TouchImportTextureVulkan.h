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

#pragma once

#include "CoreMinimal.h"
#include "RHI.h"
#include "TextureResource.h"
#include "RHICommandList.h"
#include "Async/Future.h"
#include "Rendering/Importing/TouchImportTexture_AcquireOnRenderThread.h"

#include "TouchEngine/TouchObject.h"

THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END

#include "Util/SemaphoreVulkanUtils.h"

namespace UE::TouchEngine::Vulkan
{
	class FVulkanSharedResourceSecurityAttributes;
	struct FVulkanPointers;
	struct FVulkanContext;
	
	class FTouchImportTextureVulkan : public ITouchImportTexture, public TSharedFromThis<FTouchImportTextureVulkan>
	{
		using Super = FTouchImportTexture_AcquireOnRenderThread;
		friend struct FRHICommandCopyTouchToUnreal;
	public:

		using FHandle = void*;
		
		static TSharedPtr<FTouchImportTextureVulkan> CreateTexture(const TouchObject<TEVulkanTexture_>& SharedOutputTexture, TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes);

		FTouchImportTextureVulkan(
			TSharedPtr<VkImage> ImageHandle,
			TSharedPtr<VkDeviceMemory> ImportedTextureMemoryOwnership,
			TouchObject<TEVulkanTexture_> InSharedOutputTexture,
			TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes
		);
		virtual ~FTouchImportTextureVulkan() override;
		
		//~ Begin ITouchPlatformTexture Interface
		virtual FTextureMetaData GetTextureMetaData() const override;
		virtual bool IsCurrentCopyDone() override;
		virtual ECopyTouchToUnrealResult CopyNativeToUnrealRHI_RenderThread(const FTouchCopyTextureArgs& CopyArgs, TSharedRef<FTouchTextureImporter> Importer) override;
		//~ End ITouchPlatformTexture Interface

		TEVulkanTexture_* GetSharedTexture() const { return WeakSharedOutputTextureReference; }
		const TSharedPtr<VkCommandBuffer>& GetCommandBuffer() const { return CommandBuffer; }

		const TSharedPtr<VkCommandBuffer>& EnsureCommandBufferInitialized(FRHICommandListBase& RHICmdList);

	private:

		/** Whether to use Windows NT handles for exporting */
		const bool bUseNTHandles = true;
		
		/** Manages VkImage handle. Calls vkDestroyImage when destroyed. */
		TSharedPtr<VkImage> ImageHandle;
		/** Manages memory of ImageHandle. Calls vkFreeMemory when destroyed. */
		TSharedPtr<VkDeviceMemory> ImportedTextureMemoryOwnership;
		/** Our own command buffer because Unreal's upload buffer API is too constrained. Destroys the command buffer when destroyed. */
		TSharedPtr<VkCommandBuffer> CommandBuffer;
		
		TEVulkanTexture_* WeakSharedOutputTextureReference;
		TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes;

		/** Cached data for the semaphore on which we need to wait */
		struct FWaitSemaphoreData
		{
			HANDLE Handle;
			TouchObject<TEVulkanSemaphore> TouchSemaphore;
			TSharedPtr<VkSemaphore> VulkanSemaphore;

			FWaitSemaphoreData(HANDLE Handle, TouchObject<TEVulkanSemaphore> TouchSemaphore, TSharedPtr<VkSemaphore> VulkanSemaphore)
				: Handle(Handle)
				, TouchSemaphore(MoveTemp(TouchSemaphore))
				, VulkanSemaphore(MoveTemp(VulkanSemaphore))
			{}
		};
		/**
		 * Holds data to the semaphore that was created by Touch Designer.
		 * Touch Designer internally reuses semaphores (so the handles will be the same).
		 * This data is updated whenever the handle changes.
		 */
		TOptional<FWaitSemaphoreData> WaitSemaphoreData;

		TOptional<FTouchVulkanSemaphoreExport> SignalSemaphoreData;
		uint64 CurrentSemaphoreValue = 0;
		
		static void OnWaitVulkanSemaphoreUsageChanged(void* Semaphore, TEObjectEvent Event, void* Info);
	};
}
