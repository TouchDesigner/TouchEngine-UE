// Fill out your copyright notice in the Description page of Project Settings.


#include "RHICommmandCopyUnrealToTouch.h"

#include "Importing/VulkanImportUtils.h"

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
		FTextureResource* SrcTextureResource;
		TSharedRef<FExportedTextureVulkan> DestTextureResources;

		FRHICommandCopyUnrealToTouch(FTextureResource* InSrcTextureResource, const TSharedRef<FExportedTextureVulkan>& InDestTexture)
			: SrcTextureResource(InSrcTextureResource), DestTextureResources(InDestTexture)
		{
		}

		VkCommandBuffer GetCommandBuffer() const { return *CommandBuffer.Get(); }
		
		const FTextureRHIRef& GetSourceTexture() const { return SrcTextureResource->GetTextureRHI(); }
		FVulkanTexture* GetSourceVulkanTexture() const
		{
			return static_cast<FVulkanTexture*>(SrcTextureResource->GetTextureRHI()->GetTextureBaseRHI());
		}

		VkImage GetDestinationTexture() const { return *DestTextureResources->GetImageOwnership_RenderThread(); }
		TSharedPtr<VkCommandBuffer> CommandBuffer;

		void Execute(FRHICommandListBase& RHICmdList)
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    I.B.4 [RHI] RHI Export Copy"), STAT_TE_I_B_4_Vulkan, STATGROUP_TouchEngine);
			DestTextureResources->LogCompletedValue(FString("1. Start of `FRHICommandCopyUnrealToTouch::Execute`:"));

			// FVulkanCommandBuilder CommandBuilder = *DestTextureResources->EnsureCommandBufferInitialized_RenderThread(CmdList).Get(); // SharedTextureResources->GetCommandBuffer().Get();
			CommandBuffer = CreateCommandBuffer(RHICmdList);
			FVulkanCommandBuilder CommandBuilder {*CommandBuffer.Get()};
			
			{
				bool bBeganCommands = false;
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.a [RHI] Cook Frame - RHI - Wait for Read Access"), STAT_TE_I_B_4_a_Vulkan, STATGROUP_TouchEngine);
				// 1. If TE still has ownership of it, schedule a wait operation
				const bool bNeedsOwnershipTransfer = DestTextureResources->WasEverUsedByTouchEngine();
				if (ensure(!bNeedsOwnershipTransfer || !DestTextureResources->IsInUseByTouchEngine())) // todo: We are not supposed to have texture in use at that point, but this is still hit
				{
					// if (ExportParameters.TETextureTransfer.Result == TEResultSuccess)
					// {
					// 	CommandBuilder.BeginCommands();
					// 	WaitForReadAccess(CommandBuilder, ExportParameters.TETextureTransfer.Semaphore, ExportParameters.TETextureTransfer.WaitValue);
					// 	TransferFromTouch(CmdList);
					// 	bBeganCommands = true;
					// }
					// else if (ExportParameters.TETextureTransfer.Result != TEResultNoMatchingEntity) // TE does not have ownership
					// {
					// 	UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to transfer ownership of pooled texture back from TouchEngine"));
					// 	return;
					// }
				}
				
				if (!bBeganCommands)
				{
					CommandBuilder.BeginCommands();
					TransferFromTouch(RHICmdList);
				}
			}

			DestTextureResources->LogCompletedValue(FString("2. After Read Access"));
			{
				// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.b [RHI] Cook Frame - RHI - Enqueue Copy"), STAT_TE_I_B_4_b_Vulkan, STATGROUP_TouchEngine);
				// 2. Copy texture
				CopyTexture();
			}

			DestTextureResources->LogCompletedValue(FString("3. After ReturnToTouchEngine"));
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.d [RHI] Cook Frame - RHI - Execute Command List"), STAT_TE_I_B_4_d_Vulkan, STATGROUP_TouchEngine);
				CommandBuilder.Submit(RHICmdList);
			}
			DestTextureResources->LogCompletedValue(FString("4. After Submitting Command Builder"));
		}

		void WaitForReadAccess(FVulkanCommandBuilder& CommandBuilder, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue);
		bool AllocateWaitSemaphore(const TouchObject<TESemaphore>& Semaphore);
		void TransferFromTouch(FRHICommandListBase& CmdList) const;
		void TransferFromInitialState(FRHICommandListBase& CmdList) const;
		
		void CopyTexture() const;
	};

	void FRHICommandCopyUnrealToTouch::WaitForReadAccess(FVulkanCommandBuilder& CommandBuilder, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
		AllocateWaitSemaphore(Semaphore);

		if (ensure(DestTextureResources->WaitSemaphoreData))
		{
			CommandBuilder.AddWaitSemaphore({ *DestTextureResources->WaitSemaphoreData->VulkanSemaphore.Get(), WaitValue, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });
		}
	}

	bool FRHICommandCopyUnrealToTouch::AllocateWaitSemaphore(const TouchObject<TESemaphore>& Semaphore)
	{
		TouchObject<TEVulkanSemaphore> VulkanSemaphoreTE;
		VulkanSemaphoreTE.set(static_cast<TEVulkanSemaphore*>(Semaphore.get()));
		
		const HANDLE SharedHandle = TEVulkanSemaphoreGetHandle(VulkanSemaphoreTE);
		const bool bIsValidHandle = SharedHandle != nullptr;
		const bool bIsOutdated = !DestTextureResources->WaitSemaphoreData.IsSet() || DestTextureResources->WaitSemaphoreData->Handle != SharedHandle;

		UE_CLOG(!bIsValidHandle, LogTouchEngineVulkanRHI, Warning, TEXT("Invalid semaphore handle received from TouchEngine"));
		if (bIsValidHandle && bIsOutdated)
		{
			const TOptional<FTouchVulkanSemaphoreImport> SemaphoreImport = ImportTouchSemaphore(VulkanSemaphoreTE, &FExportedTextureVulkan::OnWaitVulkanSemaphoreUsageChanged, this);
			if (!SemaphoreImport)
			{
				DestTextureResources->WaitSemaphoreData.Reset();
				return false;
			}
			
			DestTextureResources->WaitSemaphoreData = *SemaphoreImport;
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
		SourceImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(CurrentLayout);
		SourceImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		SourceImageBarrier.oldLayout = CurrentLayout;
		SourceImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		SourceImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.image = SourceVulkanTexture->Image;
		SourceImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		
		VkImageMemoryBarrier& DestImageBarrier = ImageBarriers[1];
		DestImageBarrier.pNext = nullptr;
		DestImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_UNDEFINED); 
		DestImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		DestImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // VK_IMAGE_LAYOUT_UNDEFINED tells GPU that we can override old texture data
		DestImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		DestImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.image = GetDestinationTexture();
		DestImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		
		VulkanRHI::vkCmdPipelineBarrier(
			GetCommandBuffer(),
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
		SourceImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(CurrentLayout);
		SourceImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		SourceImageBarrier.oldLayout = CurrentLayout;
		SourceImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		SourceImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.image = SourceVulkanTexture->Image;
		SourceImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		
		VulkanRHI::vkCmdPipelineBarrier(
			GetCommandBuffer(),
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
		ensure(SourceVulkanTexture->GetDesc().Extent.X <= DestTextureResources->GetResolution_RenderThread().X
			&& SourceVulkanTexture->GetDesc().Extent.Y <= DestTextureResources->GetResolution_RenderThread().Y);
		Region.extent.width = FMath::Max<uint32>(PixelFormatInfo.BlockSizeX, DestTextureResources->GetResolution_RenderThread().X);
		Region.extent.height = FMath::Max<uint32>(PixelFormatInfo.BlockSizeY, DestTextureResources->GetResolution_RenderThread().Y);
		Region.extent.depth = 1;
		// FVulkanSurface constructor sets aspectMask like this so let's do the same for now
		Region.srcSubresource.aspectMask = SourceVulkanTexture->GetFullAspectMask();
		Region.srcSubresource.layerCount = 1;
		Region.dstSubresource.aspectMask = VulkanRHI::GetAspectMaskFromUEFormat(DestTextureResources->GetPixelFormat_RenderThread(), true, true);
		Region.dstSubresource.layerCount = 1;
		
		VulkanRHI::vkCmdCopyImage(GetCommandBuffer(), SourceVulkanTexture->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *DestTextureResources->GetImageOwnership_RenderThread(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
		UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("   [FRHICommandCopyUnrealToTouch[%s]] Texture copy enqueued to render thread."),	*GetCurrentThreadStr())
	}


	
	FRHICOMMAND_MACRO(FRHICommandSignalCopyUnrealToTouch)
	{
		TouchObject<TEInstance> Instance;
		TSharedRef<FExportedTextureVulkan> DestTextureResources;

		FRHICommandSignalCopyUnrealToTouch(TouchObject<TEInstance> InInstance, const TSharedRef<FExportedTextureVulkan>& InDestTexture)
			: Instance(InInstance)
			, DestTextureResources(InDestTexture)
		{
		}

		VkImage GetDestinationTexture() const { return *DestTextureResources->GetImageOwnership_RenderThread(); }

		void Execute(FRHICommandListBase& RHICmdList)
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    I.B.4 [RHI] RHI Export Signal"), STAT_TE_I_B_4b_Vulkan, STATGROUP_TouchEngine);
			DestTextureResources->LogCompletedValue(FString("1. Start of `FRHICommandSignalCopyUnrealToTouch::Execute`:"));

			const TSharedPtr<VkCommandBuffer> CommandBuffer = CreateCommandBuffer(RHICmdList);
			FVulkanCommandBuilder CommandBuilder {*CommandBuffer.Get()}; // *DestTextureResources->EnsureCommandBufferInitialized_RenderThread(CmdList).Get(); // SharedTextureResources->GetCommandBuffer().Get();
			CommandBuilder.BeginCommands();
			
			{
				// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.c [RHI] Cook Frame - RHI - Enqueue Signal"), STAT_TE_I_B_4_c_Vulkan, STATGROUP_TouchEngine);
				// 3. 
				ReturnToTouchEngine(CommandBuilder);
			}
			DestTextureResources->LogCompletedValue(FString("2. After ReturnToTouchEngine"));
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.d [RHI] RHI Export Signal - Execute Command List"), STAT_TE_I_B_4b_d_Vulkan, STATGROUP_TouchEngine);
				CommandBuilder.Submit(RHICmdList);
			}
			DestTextureResources->LogCompletedValue(FString("3. After Submitting Command Builder"));
		}

		void ReturnToTouchEngine(FVulkanCommandBuilder& CommandBuilder) const;
	};

	void FRHICommandSignalCopyUnrealToTouch::ReturnToTouchEngine(FVulkanCommandBuilder& CommandBuilder) const
	{
		if (!DestTextureResources->SignalSemaphoreData.IsSet()) // todo: this could be done earlier
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[FRHICommandCopyUnrealToTouch::ReturnToTouchEngine] `SignalSemaphoreData` is not set!"));
		}

		constexpr VkImageLayout OldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		const VkImageLayout NewLayout =  TEInstanceGetVulkanInputReleaseImageLayout(Instance.get()); //VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //
		VkImageMemoryBarrier DestImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		DestImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(OldLayout);
		DestImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(NewLayout);
		DestImageBarrier.oldLayout = OldLayout;
		DestImageBarrier.newLayout = NewLayout;
		DestImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.image = GetDestinationTexture();
		DestImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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
		
		++DestTextureResources->CurrentSemaphoreValue;
		CommandBuilder.AddSignalSemaphore({ *DestTextureResources->SignalSemaphoreData->VulkanSemaphore.Get(), DestTextureResources->CurrentSemaphoreValue});
		UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("   [FRHICommandCopyUnrealToTouch[%s]] Enqueuing Fence change to `%llu`"), *GetCurrentThreadStr(), DestTextureResources->CurrentSemaphoreValue)
	}


	bool CopyUnrealToTouchRHICommand(FRHICommandListImmediate& RHICmdList, FTextureResource* InSrcTextureResource, const TSharedRef<FExportedTextureVulkan>& InDestTexture)
	{
		ALLOC_COMMAND_CL(RHICmdList, FRHICommandCopyUnrealToTouch)(InSrcTextureResource, InDestTexture);
		return true;
	}

	bool SignalCopyFromUnrealToTouchRHICommand(FRHICommandListImmediate& RHICmdList, const TouchObject<TEInstance>& Instance, const TSharedRef<FExportedTextureVulkan>& InDestTexture)
	{
		if (Instance)
		{
			ALLOC_COMMAND_CL(RHICmdList, FRHICommandSignalCopyUnrealToTouch)(Instance, InDestTexture);
			return true;
		}
		return false;
	}
}
