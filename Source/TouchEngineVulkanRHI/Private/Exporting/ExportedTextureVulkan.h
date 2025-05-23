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
		friend struct FRHICommandForceSignalWaitValues;
		friend class FTouchTextureExporterVulkan;
	public:
		struct FOutputVulkanTextureData
		{
			TSharedPtr<VkImage> ImageOwnership;
			TSharedPtr<VkDeviceMemory> TextureMemoryOwnership;
			
			HANDLE VulkanSharedHandle = nullptr;
			VkExternalMemoryHandleTypeFlagBits MemoryHandleFlags;
		};

		static TSharedPtr<FExportedTextureVulkan> Create(const TSharedRef<FTouchTextureExporterVulkan>& InExporter, UTexture* InTexture, const TSharedRef<FVulkanSharedResourceSecurityAttributes>& SecurityAttributes);
		
		EPixelFormat GetPixelFormat_RenderThread() const
		{
			return GetSharedTextureRHI_RenderThread() ? GetSharedTextureRHI_RenderThread()->GetFormat() : EPixelFormat::PF_Unknown;
		}
		FIntPoint GetResolution_RenderThread() const
		{
			return GetSharedTextureRHI_RenderThread() ? GetSharedTextureRHI_RenderThread()->GetSizeXY() : FIntPoint{};
		}
		bool GetIsSRGB() const
		{
			return GetSharedTextureRHI_RenderThread() ? EnumHasAnyFlags( GetSharedTextureRHI_RenderThread()->GetFlags(), ETextureCreateFlags::SRGB) : false;
		}
		bool IsCreated_RenderThread() const { return VulkanTextureData.IsSet() && VulkanTextureData->ImageOwnership.IsValid(); }
		bool CanBeShared_RenderThread() const { return IsCreated_RenderThread() && VulkanTextureData->VulkanSharedHandle == nullptr; }
		bool IsShared_RenderThread() const { return IsCreated_RenderThread() && VulkanTextureData->VulkanSharedHandle != nullptr; }
		const TSharedPtr<VkImage>& GetImageOwnership_RenderThread() const
		{
			static const TSharedPtr<VkImage> EmptyImage = nullptr;
			return IsCreated_RenderThread() ? VulkanTextureData->ImageOwnership : EmptyImage;
		}
		const TSharedPtr<VkDeviceMemory>& GetTextureMemoryOwnership_RenderThread() const
		{
			static const TSharedPtr<VkDeviceMemory> EmptyMemory = nullptr;
			return IsShared_RenderThread() ? VulkanTextureData->TextureMemoryOwnership: EmptyMemory;
		}

		const TSharedPtr<VkCommandBuffer>& EnsureCommandBufferInitialized_RenderThread(FRHICommandListBase& RHICmdList);

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

		void SetVulkanTexture_RenderThread(const FTextureRHIRef& VulkanRHI, const FOutputVulkanTextureData& InOutputVulkanTextureData);
		bool ShareTexture_RenderThread();

		virtual bool CanFitTexture(UTexture* TextureToFit) const override;
		virtual bool EnqueueTextureCopy(UTexture* SrcTexture) override;

	protected:
		virtual void SetSemaphoreCallbackForTextureTransferFromTE(TouchObject<TESemaphore> Semaphore) override;

	private:
		FExportedTextureVulkan(const TSharedRef<FTouchTextureExporterVulkan>& InExporter, const TSharedRef<FVulkanSharedResourceSecurityAttributes>& InSharedSecurityAttributes)
			: WeakExporter(InExporter), SecurityAttributes(InSharedSecurityAttributes)
		{}
		
		TWeakPtr<FTouchTextureExporterVulkan> WeakExporter;
		const TSharedRef<FVulkanSharedResourceSecurityAttributes>& SecurityAttributes;
		
		TSharedPtr<VkCommandBuffer> CommandBuffer;

		TOptional<FOutputVulkanTextureData> VulkanTextureData;

		TOptional<FTouchVulkanSemaphoreImport> WaitSemaphoreData;
		TOptional<FTouchVulkanSemaphoreExport> SignalSemaphoreData;
		uint64 CurrentSemaphoreValue = 0;
		
		static void TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info);
	};
}

