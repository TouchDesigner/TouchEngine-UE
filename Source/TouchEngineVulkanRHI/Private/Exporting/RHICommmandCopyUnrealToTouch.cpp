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
#include "TEVulkanInclude.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/VulkanCommandBuilder.h"
#include "Util/VulkanGetterUtils.h"
#include "Util/TouchHelpers.h"

namespace UE::TouchEngine::Vulkan
{
	FRHICOMMAND_MACRO(FRHICommandCopyUnrealToTouch)
	{
		TouchObject<TEInstance> Instance;
		VkSemaphore WaitForTransitionSemaphoreHandle;
		FTextureRHIRef SrcTextureStableRHI;
		TSharedRef<FExportedTextureVulkan> DestTexture;

		FRHICommandCopyUnrealToTouch(const TouchObject<TEInstance>& InInstance, const FTextureRHIRef& InSrcTextureStableRHI, const TSharedRef<FExportedTextureVulkan>& InDestTexture, VkSemaphore InWaitForTransitionSemaphore)
			: Instance(InInstance), WaitForTransitionSemaphoreHandle(InWaitForTransitionSemaphore), SrcTextureStableRHI(InSrcTextureStableRHI), DestTexture(InDestTexture)
		{
		}

		FTextureRHIRef GetSourceTexture() const { return SrcTextureStableRHI;}
		FVulkanTexture* GetSourceVulkanTexture() const
		{
			return static_cast<FVulkanTexture*>(GetSourceTexture().GetReference());
		}

		VkImage GetDestinationTexture() const { return *DestTexture->GetImageOwnership_RenderThread(); }
		FVulkanCommandBuilder CommandBuilder{{}};

		void Execute(FRHICommandListBase& CmdList)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("[FRHICommandCopyUnrealToTouch::Execute[%s]] Creating VkCommandBuffer to copy texture '%s' to '%s'"), *GetCurrentThreadStr(), *SrcTextureStableRHI->GetName().ToString(), *DestTexture->DebugName)

			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    I.B.4 [RHI] Cook Frame - RHI Export Copy"), STAT_TE_I_B_4_Vulkan, STATGROUP_TouchEngine);
			DestTexture->LogCompletedValue(FString("1. Start of `FRHICommandCopyUnrealToTouch::Execute`:"));

			TSharedPtr<VkCommandBuffer> CommandBuffer = DestTexture->EnsureCommandBufferInitialized_RenderThread(CmdList); // SharedTextureResources->GetCommandBuffer().Get();
			CommandBuilder = {*CommandBuffer.Get()};

			// todo: on 5.6 we do not have access anymore to the signalling semaphore so we flushed instead. Check for 5.7
			// if (ensure(WaitForTransitionSemaphoreHandle))
			// {
			// 	CommandBuilder.AddWaitSemaphore({ WaitForTransitionSemaphoreHandle, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });
			// 	const uint64 NewValue = GetCompletedSemaphoreValue(&WaitForTransitionSemaphoreHandle, TEXT("AFTER RHIEndTransitions:  Wait for any work to be done on the texture"));
			// 	UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("   [FRHICommandCopyUnrealToTouch[%s]] '%s' AFTER RHIEndTransitions for UE Texture Semaphore '%p' to reach WaitValue `%d`  (Current: %lld)"), *GetCurrentThreadStr(), *GetSourceTexture()->GetName().ToString(), &WaitForTransitionSemaphoreHandle, 1, NewValue)
			// }

			
			{
				bool bTransferred = false;
				// 1. If TE still has ownership of it, schedule a wait operation
				const bool bNeedsOwnershipTransfer = DestTexture->WasEverUsedByTouchEngine() && DestTexture->GetTETextureTransferBackToUE().Semaphore;
				if (bNeedsOwnershipTransfer)
				{
					if (DestTexture->GetTETextureTransferBackToUE().Result == TEResultSuccess)
					{
						CommandBuilder.BeginCommands();
						UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("[FRHICommandCopyUnrealToTouch::Execute[%s]] Enqueuing wait for Texture Transfer back to UE with Semaphore '%p' and WaitValue '%lld' for texture '%s'"), *GetCurrentThreadStr(), DestTexture->GetTETextureTransferBackToUE().Semaphore.get(), DestTexture->GetTETextureTransferBackToUE().WaitValue, *DestTexture->DebugName);
						WaitForReadAccess(DestTexture->GetTETextureTransferBackToUE().Semaphore, DestTexture->GetTETextureTransferBackToUE().WaitValue);
						TransferFromTouch(CmdList);
						bTransferred = true;
					}
					else if (DestTexture->GetTETextureTransferBackToUE().Result != TEResultNoMatchingEntity) //TEResultNoMatchingEntity would be raised if there is no texture transfer waiting
					{
						UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[FRHICommandCopyUnrealToTouch[%s]] TEInstanceGetTextureTransfer returned `%s`."), *GetCurrentThreadStr(), *TEResultToString(DestTexture->GetTETextureTransferBackToUE().Result));
						return;
					}
				}
				
				if (!bTransferred)
				{
					CommandBuilder.BeginCommands();
					TransferFromInitialState(CmdList);
				}
			}

			CopyTexture();
			ReturnToTouchEngine();

			CommandBuilder.Submit(CmdList);
			
			DestTexture->LogCompletedValue(FString("4. After Submitting Command Builder"));
			UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("[FRHICommandCopyUnrealToTouch::Execute[%s]] Submitted buffer to copy texture '%s' to '%s'"), *GetCurrentThreadStr(), *SrcTextureStableRHI->GetName().ToString(), *DestTexture->DebugName)
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

		if (ensure(DestTexture->WaitSemaphoreData))
		{
			const uint64 CurrentValue = GetCompletedSemaphoreValue(DestTexture->WaitSemaphoreData->VulkanSemaphore.Get(),DestTexture->DebugName);
			CommandBuilder.AddWaitSemaphore({ *DestTexture->WaitSemaphoreData->VulkanSemaphore.Get(), WaitValue, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });
			UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("   [FRHICommandCopyUnrealToTouch[%s]] '%s' Enqueuing Wait for TE Semaphore '%p' to reach WaitValue `%llu`  (Current: %lld)"), *GetCurrentThreadStr(), *DestTexture->DebugName, DestTexture->GetTETextureTransferBackToUE().Semaphore.get(), WaitValue, CurrentValue)
		}
	}

	bool FRHICommandCopyUnrealToTouch::AllocateWaitSemaphore(const TouchObject<TESemaphore>& Semaphore)
	{
		// return true;
		TouchObject<TEVulkanSemaphore> VulkanSemaphoreTE;
		VulkanSemaphoreTE.set(static_cast<TEVulkanSemaphore*>(Semaphore.get()));
		
		const HANDLE SharedHandle = TEVulkanSemaphoreGetHandle(VulkanSemaphoreTE);
		const bool bIsValidHandle = SharedHandle != nullptr;
		const bool bIsOutdated = !DestTexture->WaitSemaphoreData.IsSet() || DestTexture->WaitSemaphoreData->Handle != SharedHandle;
		
		UE_CLOG(!bIsValidHandle, LogTouchEngineVulkanRHI, Warning, TEXT("Invalid semaphore handle received from TouchEngine"));
		if (bIsValidHandle && bIsOutdated)
		{
			const TOptional<FTouchVulkanSemaphoreImport> SemaphoreImport = ImportTouchSemaphore(VulkanSemaphoreTE);
			if (!SemaphoreImport)
			{
				DestTexture->WaitSemaphoreData.Reset();
				return false;
			}
			
			DestTexture->WaitSemaphoreData = *SemaphoreImport;
		}
		
		return bIsValidHandle;
	}
	
	void FRHICommandCopyUnrealToTouch::TransferFromTouch(FRHICommandListBase& CmdList) const
	{
		const FVulkanTexture* SourceVulkanTexture = GetSourceVulkanTexture();
		const VkImageLayout CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		
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
		const VkImageLayout CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		
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
		ensure(SourceVulkanTexture->GetDesc().Extent.X <= DestTexture->GetResolution_RenderThread().X
			&& SourceVulkanTexture->GetDesc().Extent.Y <= DestTexture->GetResolution_RenderThread().Y);
		Region.extent.width = FMath::Max<uint32>(PixelFormatInfo.BlockSizeX, DestTexture->GetResolution_RenderThread().X);
		Region.extent.height = FMath::Max<uint32>(PixelFormatInfo.BlockSizeY, DestTexture->GetResolution_RenderThread().Y);
		Region.extent.depth = 1;
		// FVulkanSurface constructor sets aspectMask like this so let's do the same for now
		Region.srcSubresource.aspectMask = SourceVulkanTexture->GetFullAspectMask();
		Region.srcSubresource.layerCount = 1;
		Region.dstSubresource.aspectMask = VulkanRHI::GetAspectMaskFromUEFormat(DestTexture->GetPixelFormat_RenderThread(), true, true);
		Region.dstSubresource.layerCount = 1;
		
		VulkanRHI::vkCmdCopyImage(CommandBuilder.GetCommandBuffer(), SourceVulkanTexture->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *DestTexture->GetImageOwnership_RenderThread(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
		UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("   [FRHICommandCopyUnrealToTouch[%s]] '%s' Texture copy enqueued to render thread."), *GetCurrentThreadStr(), *DestTexture->DebugName)
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
		
		const uint64 CurrentValue = GetCompletedSemaphoreValue(DestTexture->SignalSemaphoreData->VulkanSemaphore.Get(), DestTexture->SignalSemaphoreData->DebugName);
		CommandBuilder.AddSignalSemaphore({ *DestTexture->SignalSemaphoreData->VulkanSemaphore.Get(), DestTexture->CurrentSemaphoreValue});
		UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("   [FRHICommandCopyUnrealToTouch[%s]] '%s' Enqueuing Fence '%p' [TE: '%p', UE: '%s'] change to `%llu` (Current: %lld)"),
			*GetCurrentThreadStr(),
			*DestTexture->DebugName,
			DestTexture->SignalSemaphoreData->VulkanSemaphore.Get(),
			DestTexture->SignalSemaphoreData->TouchSemaphore.get(),
			*DestTexture->SignalSemaphoreData->DebugName,
			DestTexture->CurrentSemaphoreValue,
			CurrentValue
		)
	}

	bool CopyUnrealToTouchRHICommand(FRHICommandListImmediate& RHICmdList, const TouchObject<TEInstance>& Instance, const FTextureRHIRef& InSrcTextureStableRHI, const TSharedRef<FExportedTextureVulkan>& InDestTexture)
	{
		// This part is a bit of a hack. We basically need to know when UE is done doing any work on the source texture so we are able to copy.
		// There was a case where a material was drawn on a texture in BP, but the copy was happening before the material was drawn.
		// We want to enqueue a semaphore that gets signalled when the current command buffer is submitted, but UE does not expose these functions.
		// The only way we found to add a semaphore to the submit queue is to manually attach one to a Transition, so we start transitioning the textures here.
		VkSemaphore WaitForTransitionSemaphore {};
		// {
		// 	TArray<FRHITransitionInfo> TransitionInfos;
		// 	TransitionInfos.Add(FRHITransitionInfo(InSrcTextureStableRHI, ERHIAccess::Unknown, ERHIAccess::CopySrc));
		// 	const FRHITransition* Transition = RHICreateTransition(FRHITransitionCreateInfo(RHICmdList.GetPipeline(), RHICmdList.GetPipeline(), ERHITransitionCreateFlags::None, TransitionInfos));
		//
		// 	// Here we access the transition barrier data and add our own custom semaphore
		// 	FVulkanPipelineBarrier* Data = const_cast<FVulkanPipelineBarrier*>(Transition->GetPrivateData<FVulkanPipelineBarrier>());
		// 	ensureMsgf(Data->Semaphore == nullptr, TEXT("We are not expecting a Semaphore to have been setup for this transition as it will be overriden"));
		//
		// 	FVulkanPointers VulkanPointers;
		// 	Data->Semaphore = new VulkanRHI::FSemaphore(*VulkanPointers.VulkanDevice);
		// 	WaitForTransitionSemaphore = Data->Semaphore->GetHandle();
		// 	const uint64 CurrentValue = GetCompletedSemaphoreValue(&WaitForTransitionSemaphore, TEXT("BEFORE RHIBeginTransitions:  Wait for any work to be done on the texture"));
		// 	UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("   [CopyUnrealToTouchRHICommand[%s]] '%s' Enqueuing Wait for UE Texture Semaphore '%p' to reach WaitValue `%d`  (Current: %lld)"), *GetCurrentThreadStr(), *InSrcTextureStableRHI->GetName().ToString(), &WaitForTransitionSemaphore, 1, CurrentValue)
		//
		// 	RHICmdList.BeginTransition(Transition);
		// 	RHICmdList.EndTransition(Transition);
		// }

		ALLOC_COMMAND_CL(RHICmdList, FRHICommandCopyUnrealToTouch)(Instance, InSrcTextureStableRHI, InDestTexture, WaitForTransitionSemaphore);
		return true;
	}
}
