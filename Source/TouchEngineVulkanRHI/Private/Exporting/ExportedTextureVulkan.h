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
THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END
#include "Rendering/Exporting/ExportedTouchTexture.h"
#include "Util/SemaphoreVulkanUtils.h"

namespace UE::TouchEngine::Vulkan
{
	class FVulkanSharedResourceSecurityAttributes;

	class FExportedTextureVulkan : public FExportedTouchTexture
	{
		template <typename ObjectType, ESPMode Mode>
		friend class SharedPointerInternals::TIntrusiveReferenceController;
		friend struct FRHICommandCopyUnrealToTouch;
		friend class FTouchTextureExporterVulkan;
	public:

		static TSharedPtr<FExportedTextureVulkan> Create(const FRHITexture2D& SourceRHI, const TSharedRef<FVulkanSharedResourceSecurityAttributes>& SecurityAttributes);
		
		//~ Begin FExportedTouchTexture Interface
		virtual bool CanFitTexture(const FRHITexture* TextureToFit) const override;
		//~ End FExportedTouchTexture Interface

		EPixelFormat GetPixelFormat() const { return PixelFormat; }
		FIntPoint GetResolution() const { return Resolution; }
		
		const TSharedRef<VkImage>& GetImageOwnership() const { return ImageOwnership; }
		const TSharedRef<VkDeviceMemory>& GetTextureMemoryOwnership() const { return TextureMemoryOwnership; }
		const TSharedPtr<VkCommandBuffer>& GetCommandBuffer() const { return CommandBuffer; }

		const TSharedPtr<VkCommandBuffer>& EnsureCommandBufferInitialized(FRHICommandListBase& RHICmdList);

		void LogCompletedValue(const FString& Prefix) const //todo: look at removing once Sync is fully working
		{
			const uint64& SavedValue = CurrentSemaphoreValue;
			if (!SignalSemaphoreData)
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[LogCompletedValue] %s No Semaphore Data for texture `%s`. CurrentSemaphoreValue: `%lld`"), *Prefix, *DebugName, SavedValue)
				return;
			}
			const uint64 CounterValue = SignalSemaphoreData->GetCompletedSemaphoreValue();

			if(CounterValue == SavedValue)
			{
				UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("[LogCompletedValue] %s Semaphore `%s` for texture `%s`: CurrentSemaphoreValue: `%lld` (vkGetSemaphoreCounterValue: `%lld`)"),
					*Prefix, *SignalSemaphoreData->DebugName, *DebugName, SavedValue, CounterValue)
			}
		}
	protected:
		virtual void RemoveTextureCallback() override;
	private:

		const EPixelFormat PixelFormat;
		const FIntPoint Resolution;
		const bool bIsSRGB;

		const TSharedRef<VkImage> ImageOwnership;
		const TSharedRef<VkDeviceMemory> TextureMemoryOwnership;
		TSharedPtr<VkCommandBuffer> CommandBuffer;

		TOptional<FTouchVulkanSemaphoreImport> WaitSemaphoreData;
		TOptional<FTouchVulkanSemaphoreExport> SignalSemaphoreData;
		uint64 CurrentSemaphoreValue = 0;
		
		FExportedTextureVulkan(
			TouchObject<TEVulkanTexture> SharedTexture,
			EPixelFormat PixelFormat,
			const FIntPoint& Resolution,
			bool bInIsSRGB,
			TSharedRef<VkImage> ImageOwnership,
			TSharedRef<VkDeviceMemory> TextureMemoryOwnership
			);
		
		static void TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info);
		static void OnWaitVulkanSemaphoreUsageChanged(void* Semaphore, TEObjectEvent Event, void* Info);
	};
}

