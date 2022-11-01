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
#include "Rendering/Importing/TouchImportTexture_AcquireOnRenderThread.h"

#include "TouchEngine/TouchObject.h"
#include "TouchEngine/TESemaphore.h"

#include "vulkan_core.h"
#include "VulkanRHIPrivate.h"

namespace VulkanRHI
{
	class FSemaphore;
}

namespace UE::TouchEngine::Vulkan
{
	class FVulkanSharedResourceSecurityAttributes;

	namespace Private
	{
		struct FVulkanCopyOperationData;
	}
	
	struct FVulkanPointers;
	struct FVulkanContext;
	
	class FTouchImportTextureVulkan : public ITouchImportTexture
	{
		using Super = FTouchImportTexture_AcquireOnRenderThread;
	public:

		using FHandle = void*;
		
		static TSharedPtr<FTouchImportTextureVulkan> CreateTexture(const TouchObject<TEVulkanTexture_>& Shared, TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes);

		FTouchImportTextureVulkan(TSharedPtr<VkImage> ImageHandle, TSharedPtr<VkDeviceMemory> ImportedTextureMemoryOwnership, TouchObject<TEVulkanTexture_> InSharedTexture, TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes);
		virtual ~FTouchImportTextureVulkan() override;
		
		//~ Begin ITouchPlatformTexture Interface
		virtual FTextureMetaData GetTextureMetaData() const override { return SourceTextureMetaData; }
		virtual TFuture<ECopyTouchToUnrealResult> CopyNativeToUnreal_RenderThread(const FTouchCopyTextureArgs& CopyArgs) override;
		//~ End ITouchPlatformTexture Interface

		TouchObject<TEVulkanTexture_> GetSharedTexture() const { return SharedTexture; }

	private:

		/** Whether to use Windows NT handles for exporting */
		const bool bUseNTHandles = true;
		
		/** Manages VkImage handle. Calls vkDestroyImage when destroyed. */
		TSharedPtr<VkImage> ImageHandle;
		/** Manages memory of ImageHandle. Calls vkFreeMemory when destroyed. */
		TSharedPtr<VkDeviceMemory> ImportedTextureMemoryOwnership;

		FTextureMetaData SourceTextureMetaData;
		
		TouchObject<TEVulkanTexture_> SharedTexture;
		TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes;

		/** Cached data for the semaphore on which we need to wait */
		struct FWaitSemaphoreData
		{
			HANDLE Handle;
			TouchObject<TEVulkanSemaphore> TouchSemaphore;
			TSharedPtr<VkSemaphore> VulkanSemaphore;
			VulkanRHI::FSemaphore UnrealSemaphore;

			FWaitSemaphoreData(HANDLE Handle, TouchObject<TEVulkanSemaphore> TouchSemaphore, TSharedPtr<VkSemaphore> VulkanSemaphore, VulkanRHI::FSemaphore UnrealSemaphore)
				: Handle(Handle)
				, TouchSemaphore(MoveTemp(TouchSemaphore))
				, VulkanSemaphore(MoveTemp(VulkanSemaphore))
				, UnrealSemaphore(UnrealSemaphore)
			{}
		};
		/**
		 * Holds data to the semaphore that was created by Touch Designer.
		 * Touch Designer internally reuses semaphores (so the handles will be the same).
		 * This data is updated whenever the handle changes.
		 */
		TOptional<FWaitSemaphoreData> WaitSemaphoreData;

		/** Cached data for the semaphore we will signal */
		struct FSignalSemaphoreData
		{
			HANDLE ExportedHandle = nullptr;
			TouchObject<TEVulkanSemaphore> TouchSemaphore;
			TSharedPtr<VkSemaphore> VulkanSemaphore;
		};
		TOptional<FSignalSemaphoreData> SignalSemaphoreData;
		uint64 CurrentSemaphoreValue = 0;
		
		bool AcquireMutex(Private::FVulkanCopyOperationData& CopyOperationData, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue, VkImageLayout AcquireOldLayout, VkImageLayout AcquireNewLayout);
		bool AllocateWaitSemaphore(Private::FVulkanCopyOperationData& CopyOperationData, TEVulkanSemaphore* SemaphoreTE);
		void CopyTexture(const Private::FVulkanCopyOperationData& ContextInfo);
		void ReleaseMutex(Private::FVulkanCopyOperationData& ContextInfo);
		
		static void OnWaitVulkanSemaphoreUsageChanged(void* semaphore, TEObjectEvent event, void* info);
	};
}
