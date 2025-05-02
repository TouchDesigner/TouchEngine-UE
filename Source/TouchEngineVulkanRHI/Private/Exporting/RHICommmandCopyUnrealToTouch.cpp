// Fill out your copyright notice in the Description page of Project Settings.


#include "RHICommmandCopyUnrealToTouch.h"

#include "Importing/VulkanImportUtils.h"
#include "Rendering/TouchResourceProvider.h"

THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END
#include "HAL/Platform.h"
#if PLATFORM_WINDOWS
#include "WindowsVulkanPlatformDefines.h"
#endif
#include "VulkanRHIPrivate.h"
#include "VulkanContext.h"

#include "ExportedTextureVulkan.h"
#include "Logging.h"
#include "Engine/TEDebug.h"
#include "Rendering/Exporting/TouchExportParams.h"
#include "TEVulkanInclude.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/VulkanCommandBuilder.h"
#include "Util/VulkanGetterUtils.h"
#include "Util/TouchHelpers.h"

namespace UE::TouchEngine::Vulkan
{
	FRHICOMMAND_MACRO(FRHICommandCopyUnrealToTouch)
	{
		bool bFulfilledPromise = false;

		TouchObject<TEInstance> Instance;
		FTextureRHIRef SrcTextureStableRHI;
		TSharedRef<FExportedTextureVulkan> SharedTextureResources;

		FRHICommandCopyUnrealToTouch(TouchObject<TEInstance> InInstance, const FTextureRHIRef& InSrcTextureStableRHI, const TSharedRef<FExportedTextureVulkan>& InDestTexture)
			: Instance(InInstance), SrcTextureStableRHI(InSrcTextureStableRHI), SharedTextureResources(InDestTexture)
		{
		}

		FTextureRHIRef GetSourceTexture() const { return SrcTextureStableRHI;}
		FVulkanTexture* GetSourceVulkanTexture() const
		{
			return static_cast<FVulkanTexture*>(GetSourceTexture().GetReference());
		}

		VkImage GetDestinationTexture() const { return *SharedTextureResources->GetImageOwnership_RenderThread(); }
		FVulkanCommandBuilder CommandBuilder{{}};

		void Execute(FRHICommandListBase& CmdList)
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    I.B.4 [RHI] Cook Frame - RHI Export Copy"), STAT_TE_I_B_4_Vulkan, STATGROUP_TouchEngine);
			SharedTextureResources->LogCompletedValue(FString("1. Start of `FRHICommandCopyUnrealToTouch::Execute`:"));

			TSharedPtr<VkCommandBuffer> CommandBuffer = SharedTextureResources->EnsureCommandBufferInitialized_RenderThread(CmdList); // SharedTextureResources->GetCommandBuffer().Get();
			CommandBuilder = {*CommandBuffer.Get()};
			
			{
				bool bTransferred = false;
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.a [RHI] Cook Frame - RHI - Wait for Read Access"), STAT_TE_I_B_4_a_Vulkan, STATGROUP_TouchEngine);
				// 1. If TE still has ownership of it, schedule a wait operation
				const bool bNeedsOwnershipTransfer = SharedTextureResources->WasEverUsedByTouchEngine();
				if (bNeedsOwnershipTransfer)
				{
					// FTouchTextureTransfer TETextureTransfer;
					// TETextureTransfer.Result = TEResultNoMatchingEntity;
					// if (SharedTextureResources->GetTouchRepresentation_RenderThread() && TEInstanceHasTextureTransfer(Instance, SharedTextureResources->GetTouchRepresentation_RenderThread())) // If this is a pre-existing texture
					// {
					// 	// Here we can use a regular TEInstanceGetTextureTransfer even for Vulkan because the contents of the texture can be discarded
					// 	// as noted https://github.com/TouchDesigner/TouchEngine-Windows#vulkan
					// 	TETextureTransfer.Result = TEInstanceGetTextureTransfer(Instance, SharedTextureResources->GetTouchRepresentation_RenderThread(), TETextureTransfer.Semaphore.take(), &TETextureTransfer.WaitValue); // request an ownership transfer from TE to UE, will be processed below
					// 	if (TETextureTransfer.Result != TEResultSuccess && TETextureTransfer.Result != TEResultNoMatchingEntity) //TEResultNoMatchingEntity would be raised if there is no texture transfer waiting
					// 	{
					// 		UE_LOG(LogTouchEngine, Error, TEXT("[ExportTextureToTE_AnyThread[%s]] TEInstanceGetTextureTransfer returned `%s`."), *GetCurrentThreadStr(), *TEResultToString(TETextureTransfer.Result));
					// 		return;
					// 	}
					// }

					if (SharedTextureResources->GetTETextureTransferBackToUE().Semaphore)
					{
						if (SharedTextureResources->GetTETextureTransferBackToUE().Result == TEResultSuccess)
						{
							CommandBuilder.BeginCommands();
							WaitForReadAccess(SharedTextureResources->GetTETextureTransferBackToUE().Semaphore, SharedTextureResources->GetTETextureTransferBackToUE().WaitValue);
							TransferFromTouch(CmdList);
							bTransferred = true;
						}
						else if (SharedTextureResources->GetTETextureTransferBackToUE().Result != TEResultNoMatchingEntity) //TEResultNoMatchingEntity would be raised if there is no texture transfer waiting
						{
							UE_LOG(LogTouchEngine, Error, TEXT("[ExportTextureToTE_AnyThread[%s]] TEInstanceGetTextureTransfer returned `%s`."), *GetCurrentThreadStr(), *TEResultToString(SharedTextureResources->GetTETextureTransferBackToUE().Result));
							return;
						}
					}
				}
				
				if (!bTransferred)
				{
					CommandBuilder.BeginCommands();
					TransferFromInitialState(CmdList);
				}
			}

			SharedTextureResources->LogCompletedValue(FString("2. After Read Access"));
			{
				CopyTexture();
			}

			{
				// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.c [RHI] Cook Frame - RHI - Enqueue Signal"), STAT_TE_I_B_4_c_Vulkan, STATGROUP_TouchEngine);
				// 3.
				ReturnToTouchEngine();
			}
			SharedTextureResources->LogCompletedValue(FString("3. After ReturnToTouchEngine"));
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.d [RHI] Cook Frame - RHI - Execute Command List"), STAT_TE_I_B_4_d_Vulkan, STATGROUP_TouchEngine);
				CommandBuilder.Submit(CmdList);
			}
			SharedTextureResources->LogCompletedValue(FString("4. After Submitting Command Builder"));
			bFulfilledPromise = true;
		}

		void WaitForReadAccess(const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue);
		bool AllocateWaitSemaphore(const TouchObject<TESemaphore>& Semaphore);
		void TransferFromTouch(FRHICommandListBase& CmdList) const;
		void TransferFromInitialState(FRHICommandListBase& CmdList) const;
		
		void CopyTexture() const;
		void ReturnToTouchEngine();
	};

	void FRHICommandCopyUnrealToTouch::WaitForReadAccess(const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
		AllocateWaitSemaphore(Semaphore);

		if (ensure(SharedTextureResources->WaitSemaphoreData))
		{
			CommandBuilder.AddWaitSemaphore({ *SharedTextureResources->WaitSemaphoreData->VulkanSemaphore.Get(), WaitValue, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });
			UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("   [FRHICommandCopyUnrealToTouch[%s]] '%s' Enqueuing Wait for TE Semaphore to reach `%llu`"), *GetCurrentThreadStr(), *SharedTextureResources->DebugName, WaitValue)
		}
	}

	bool FRHICommandCopyUnrealToTouch::AllocateWaitSemaphore(const TouchObject<TESemaphore>& Semaphore)
	{
		// return true;
		TouchObject<TEVulkanSemaphore> VulkanSemaphoreTE;
		VulkanSemaphoreTE.set(static_cast<TEVulkanSemaphore*>(Semaphore.get()));
		
		const HANDLE SharedHandle = TEVulkanSemaphoreGetHandle(VulkanSemaphoreTE);
		const bool bIsValidHandle = SharedHandle != nullptr;
		const bool bIsOutdated = !SharedTextureResources->WaitSemaphoreData.IsSet() || SharedTextureResources->WaitSemaphoreData->Handle != SharedHandle;
		
		UE_CLOG(!bIsValidHandle, LogTouchEngineVulkanRHI, Warning, TEXT("Invalid semaphore handle received from TouchEngine"));
		if (bIsValidHandle && bIsOutdated)
		{
			const TOptional<FTouchVulkanSemaphoreImport> SemaphoreImport = ImportTouchSemaphore(VulkanSemaphoreTE, &FExportedTextureVulkan::OnWaitVulkanSemaphoreUsageChanged, this);
			if (!SemaphoreImport)
			{
				SharedTextureResources->WaitSemaphoreData.Reset();
				return false;
			}
			
			SharedTextureResources->WaitSemaphoreData = *SemaphoreImport;
		}
		
		return bIsValidHandle;
	}
	
	void FRHICommandCopyUnrealToTouch::TransferFromTouch(FRHICommandListBase& CmdList) const
	{
		const FVulkanTexture* SourceVulkanTexture = GetSourceVulkanTexture();
		FVulkanCommandListContext& VulkanContext = static_cast<FVulkanCommandListContext&>(CmdList.GetContext());
		FVulkanCmdBuffer* LayoutManager = VulkanContext.GetCommandBufferManager()->GetActiveCmdBuffer();
		const FVulkanImageLayout* UnrealLayoutData = LayoutManager->GetLayoutManager().GetFullLayout(SourceVulkanTexture->Image);
		const VkImageLayout CurrentLayout = UnrealLayoutData->MainLayout;
		
		VkImageMemoryBarrier ImageBarriers[2] = { { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER }, { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER } };
		VkImageMemoryBarrier& SourceImageBarrier = ImageBarriers[0];
		SourceImageBarrier.pNext = nullptr;
		SourceImageBarrier.oldLayout = CurrentLayout;
		SourceImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		SourceImageBarrier.srcAccessMask = GetVkAccessMaskForLayout(SourceImageBarrier.oldLayout);;
		SourceImageBarrier.dstAccessMask = GetVkAccessMaskForLayout(SourceImageBarrier.newLayout);;
		SourceImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.image = SourceVulkanTexture->Image;
		SourceImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		SourceImageBarrier.subresourceRange.levelCount = 1;
		SourceImageBarrier.subresourceRange.layerCount = 1;
		
		VkImageMemoryBarrier& DestImageBarrier = ImageBarriers[1];
		DestImageBarrier.pNext = nullptr;
		DestImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DestImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		DestImageBarrier.srcAccessMask = GetVkAccessMaskForLayout(DestImageBarrier.oldLayout);;
		DestImageBarrier.dstAccessMask = GetVkAccessMaskForLayout(DestImageBarrier.newLayout);;
		DestImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.image = GetDestinationTexture();
		DestImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		DestImageBarrier.subresourceRange.levelCount = 1;
		DestImageBarrier.subresourceRange.layerCount = 1;
		
		VulkanRHI::vkCmdPipelineBarrier(
			CommandBuilder.GetCommandBuffer(), //			GetCommandBuffer(),
			GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_UNDEFINED),
			GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
			0,
			0,
			nullptr,
			0,
			nullptr,
			2,
			ImageBarriers
		);
	}

	void FRHICommandCopyUnrealToTouch::TransferFromInitialState(FRHICommandListBase& CmdList) const
	{
		const FVulkanTexture* SourceVulkanTexture = GetSourceVulkanTexture();
		FVulkanCommandListContext& VulkanContext = static_cast<FVulkanCommandListContext&>(CmdList.GetContext());
		FVulkanCmdBuffer* LayoutManager = VulkanContext.GetCommandBufferManager()->GetActiveCmdBuffer();
		const FVulkanImageLayout* UnrealLayoutData = LayoutManager->GetLayoutManager().GetFullLayout(SourceVulkanTexture->Image);
		const VkImageLayout CurrentLayout = UnrealLayoutData->MainLayout;
		
		VkImageMemoryBarrier ImageBarriers[2] = { { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER }, { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER } };
		VkImageMemoryBarrier& SourceImageBarrier = ImageBarriers[0];
		SourceImageBarrier.pNext = nullptr;
		SourceImageBarrier.oldLayout = CurrentLayout;
		SourceImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		SourceImageBarrier.srcAccessMask = GetVkAccessMaskForLayout(SourceImageBarrier.oldLayout);
		SourceImageBarrier.dstAccessMask = GetVkAccessMaskForLayout(SourceImageBarrier.newLayout);
		SourceImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.image = SourceVulkanTexture->Image;
		SourceImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		SourceImageBarrier.subresourceRange.levelCount = 1;
		SourceImageBarrier.subresourceRange.layerCount = 1;
		
		VulkanRHI::vkCmdPipelineBarrier(
			CommandBuilder.GetCommandBuffer(), // GetCommandBuffer(),
			GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_UNDEFINED),
			GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			ImageBarriers
		);
	}
	
	void FRHICommandCopyUnrealToTouch::CopyTexture() const
	{
		const FVulkanTexture* SourceVulkanTexture = GetSourceVulkanTexture();

		VkImageCopy Region;
		FMemory::Memzero(Region);
		const FPixelFormatInfo& PixelFormatInfo = GPixelFormats[GetSourceTexture()->GetFormat()];
		ensure(SourceVulkanTexture->GetDesc().Extent.X <= SharedTextureResources->GetResolution_RenderThread().X
			&& SourceVulkanTexture->GetDesc().Extent.Y <= SharedTextureResources->GetResolution_RenderThread().Y);
		Region.extent.width = FMath::Max<uint32>(PixelFormatInfo.BlockSizeX, SharedTextureResources->GetResolution_RenderThread().X);
		Region.extent.height = FMath::Max<uint32>(PixelFormatInfo.BlockSizeY, SharedTextureResources->GetResolution_RenderThread().Y);
		Region.extent.depth = 1;
		// FVulkanSurface constructor sets aspectMask like this so let's do the same for now
		Region.srcSubresource.aspectMask = SourceVulkanTexture->GetFullAspectMask();
		Region.srcSubresource.layerCount = 1;
		Region.dstSubresource.aspectMask = VulkanRHI::GetAspectMaskFromUEFormat(SharedTextureResources->GetPixelFormat_RenderThread(), true, true);
		Region.dstSubresource.layerCount = 1;
		
		VulkanRHI::vkCmdCopyImage(CommandBuilder.GetCommandBuffer(), SourceVulkanTexture->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *SharedTextureResources->GetImageOwnership_RenderThread(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
		UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("   [FRHICommandCopyUnrealToTouch[%s]] '%s' Texture copy enqueued to render thread."), *GetCurrentThreadStr(), *SharedTextureResources->DebugName)
	}

	void FRHICommandCopyUnrealToTouch::ReturnToTouchEngine()
	{
		constexpr VkImageLayout OldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		const VkImageLayout NewLayout = TEInstanceGetVulkanInputReleaseImageLayout(Instance.get());
		VkImageMemoryBarrier DestImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		DestImageBarrier.oldLayout = OldLayout;
		DestImageBarrier.newLayout = NewLayout;
		DestImageBarrier.srcAccessMask = GetVkAccessMaskForLayout(OldLayout);
		DestImageBarrier.dstAccessMask = GetVkAccessMaskForLayout(NewLayout);
		DestImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.image = GetDestinationTexture();
		DestImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		DestImageBarrier.subresourceRange.levelCount = 1;
		DestImageBarrier.subresourceRange.layerCount = 1;
		
		VulkanRHI::vkCmdPipelineBarrier(
			CommandBuilder.GetCommandBuffer(),
			GetVkStageFlagsForLayout(OldLayout),
			GetVkStageFlagsForLayout(NewLayout),
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&DestImageBarrier
		);
		
		CommandBuilder.AddSignalSemaphore({ *SharedTextureResources->SignalSemaphoreData->VulkanSemaphore.Get(), SharedTextureResources->CurrentSemaphoreValue});
		UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("   [FRHICommandCopyUnrealToTouch[%s]] '%s' Enqueuing Fence change to `%llu`"), *GetCurrentThreadStr(), *SharedTextureResources->DebugName, SharedTextureResources->CurrentSemaphoreValue)
	}

	bool CopyUnrealToTouchRHICommand(FRHICommandListImmediate& RHICmdList, const TouchObject<TEInstance>& Instance, const FTextureRHIRef& InSrcTextureStableRHI, const TSharedRef<FExportedTextureVulkan>& InDestTexture)
	{
		ALLOC_COMMAND_CL(RHICmdList, FRHICommandCopyUnrealToTouch)(Instance, InSrcTextureStableRHI, InDestTexture);
		return true;
	}
}
