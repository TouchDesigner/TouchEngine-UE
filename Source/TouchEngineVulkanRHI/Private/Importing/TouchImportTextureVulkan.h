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
#include "Async/Future.h"
#include "Rendering/Importing/TouchImportTexture_AcquireOnRenderThread.h"

#include "TouchEngine/TouchObject.h"
#include "TouchEngine/TESemaphore.h"

#include "vulkan_core.h"
#include "VulkanRHIPrivate.h"
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
		
		static TSharedPtr<FTouchImportTextureVulkan> CreateTexture(FRHICommandListBase& RHICmdList, const TouchObject<TEVulkanTexture_>& SharedOutputTexture, TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes);

		FTouchImportTextureVulkan(
			TSharedPtr<VkImage> ImageHandle,
			TSharedPtr<VkDeviceMemory> ImportedTextureMemoryOwnership,
			TSharedPtr<VkCommandBuffer> CommandBuffer,
			TouchObject<TEVulkanTexture_> InSharedOutputTexture,
			TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes
			);
		virtual ~FTouchImportTextureVulkan() override;
		
		//~ Begin ITouchPlatformTexture Interface
		virtual FTextureMetaData GetTextureMetaData() const override;
		virtual TFuture<ECopyTouchToUnrealResult> CopyNativeToUnreal_RenderThread(const FTouchCopyTextureArgs& CopyArgs) override;
		//~ End ITouchPlatformTexture Interface

		TEVulkanTexture_* GetSharedTexture() const { return WeakSharedOutputTextureReference; }

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
