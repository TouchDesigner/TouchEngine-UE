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

#include "RHICommandCopyTouchToUnreal.h"
#include "RHI.h"
#include "TextureResource.h"

#include "Logging.h"
#include "TouchImportTextureVulkan.h"
#include "Rendering/Importing/ITouchImportTexture.h"
#include "Rendering/Importing/TouchImportParams.h"
#include "Util/TextureShareVulkanPlatformWindows.h"
#include "Util/VulkanGetterUtils.h"

#include "TEVulkanInclude.h"
#include "Rendering/Importing/TouchTextureImporter.h"
#include "Util/SemaphoreVulkanUtils.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/VulkanCommandBuilder.h"

THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END
#include "WindowsVulkanPlatformDefines.h"
#include "VulkanRHIPrivate.h"
#include "VulkanContext.h"

namespace UE::TouchEngine::Vulkan
{
	FRHICOMMAND_MACRO(FRHICommandCopyTouchToUnreal)
	{
		TWeakPtr<UE::TouchEngine::FTouchTextureImporter> Importer;
		const TSharedPtr<FTouchImportTextureVulkan> SharedTexture;
		
		// Note that this keeps the output texture alive for the duration of the command (through FTouchImportParameters::Texture)
		FTouchImportParameters RequestParams;
		const FTextureRHIRef Target;

		// Vulkan related
		FVulkanPointers VulkanPointers;
		
		FRHICommandCopyTouchToUnreal(TWeakPtr<UE::TouchEngine::FTouchTextureImporter> InImporter, TSharedPtr<FTouchImportTextureVulkan> InSharedTexture, const FTouchImportParameters& RequestParams, const FTextureRHIRef& Target)
			: Importer(MoveTemp(InImporter))
			, SharedTexture(MoveTemp(InSharedTexture))
			, RequestParams(RequestParams)
			, Target(Target)
		{
			check(SharedTexture->WeakSharedOutputTextureReference == RequestParams.TETexture);
		}

		void Execute(FRHICommandListBase& CmdList)
		{
			const TSharedPtr<UE::TouchEngine::FTouchTextureImporter> ImporterPin = Importer.Pin();
			if (!ImporterPin || ImporterPin->IsSuspended())
			{
				return;
			}
			
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      III.A.4.a [RHI] Link Texture Import - RHI Import Copy"), STAT_TE_III_A_4_a_Vulkan, STATGROUP_TouchEngine);

			FVulkanCommandBuilder CommandBuilder = *SharedTexture->EnsureCommandBufferInitialized(CmdList).Get();
			CommandBuilder.BeginCommands();
			const bool bSuccess = AcquireMutex(CmdList, CommandBuilder);
			if (bSuccess && Target->GetFormat() != PF_Unknown)
			{
				CopyTexture();
				ReleaseMutex(CommandBuilder);
				CommandBuilder.Submit(CmdList);
			}
			else
			{
				if (bSuccess && Target->GetFormat() == PF_Unknown)
				{
					UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Target->GetFormat() returned `PF_Unknown`"))
				}
			}
		}

		VkCommandBuffer GetCommandBuffer() const { return *SharedTexture->GetCommandBuffer().Get(); }
		
		bool AcquireMutex(FRHICommandListBase& CmdList, FVulkanCommandBuilder& CommandBuilder);
		bool AllocateWaitSemaphore(const TouchObject<TEVulkanSemaphore>& SemaphoreTE);
		void CopyTexture() const;
		void ReleaseMutex(FVulkanCommandBuilder& CommandBuilder) const;
	};

	bool FRHICommandCopyTouchToUnreal::AcquireMutex(FRHICommandListBase& CmdList, FVulkanCommandBuilder& CommandBuilder)
	{
		TouchObject<TEVulkanSemaphore> VulkanSemaphoreTE;
		VulkanSemaphoreTE.set(static_cast<TEVulkanSemaphore*>(RequestParams.TETextureTransfer.Semaphore.get()));
		AllocateWaitSemaphore(VulkanSemaphoreTE);
		if (!SharedTexture->WaitSemaphoreData.IsSet())
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Failed to copy Vulkan semaphore."))
			return false;
		}
		
		CommandBuilder.AddWaitSemaphore({ *SharedTexture->WaitSemaphoreData->VulkanSemaphore.Get(),
			RequestParams.TETextureTransfer.WaitValue, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });

		const VkImageLayout AcquireOldLayout = static_cast<VkImageLayout>(RequestParams.TETextureTransfer.VulkanOldLayout);
		const VkImageLayout AcquireNewLayout = static_cast<VkImageLayout>(RequestParams.TETextureTransfer.VulkanNewLayout);
		
		ensureMsgf(AcquireNewLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXT("TEInstanceSetVulkanOutputAcquireImageLayout was called with VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL but TE did not transfer correctly."));
		VkImageMemoryBarrier ImageBarriers[2] = { { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER }, { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER } };
		VkImageMemoryBarrier& SourceImageBarrier = ImageBarriers[0];
		SourceImageBarrier.pNext = nullptr;
		SourceImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(AcquireOldLayout);
		SourceImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(AcquireNewLayout);
		SourceImageBarrier.oldLayout = AcquireOldLayout;
		SourceImageBarrier.newLayout = AcquireNewLayout;
		SourceImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.image = *SharedTexture->ImageHandle.Get();
		
		const FTextureRHIRef TargetTexture = Target; // Target->GetResource()->TextureRHI->GetTexture2D();
		const FVulkanTexture* Dest = static_cast<FVulkanTexture*>(TargetTexture->GetTextureBaseRHI());
		
		const VkImageLayout CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		
		VkImageMemoryBarrier& DestImageBarrier = ImageBarriers[1];
		DestImageBarrier.pNext = nullptr;
		DestImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(CurrentLayout);
		DestImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(AcquireNewLayout);
		DestImageBarrier.oldLayout = CurrentLayout;
		DestImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		DestImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.image = Dest->Image;
		
		VulkanRHI::vkCmdPipelineBarrier(
			GetCommandBuffer(),
			GetVkStageFlagsForLayout(AcquireOldLayout),
			GetVkStageFlagsForLayout(AcquireNewLayout),
			0,
			0,
			nullptr,
			0,
			nullptr,
			2,
			ImageBarriers
		);
		
		return true;
	}

	bool FRHICommandCopyTouchToUnreal::AllocateWaitSemaphore(const TouchObject<TEVulkanSemaphore>& SemaphoreTE)
	{
		const HANDLE SharedHandle = TEVulkanSemaphoreGetHandle(SemaphoreTE);
		const bool bIsValidHandle = SharedHandle != nullptr;
		const bool bIsOutdated = !SharedTexture->WaitSemaphoreData.IsSet() || SharedTexture->WaitSemaphoreData->Handle != SharedHandle;

		UE_CLOG(!bIsValidHandle, LogTouchEngineVulkanRHI, Warning, TEXT("Invalid semaphore handle received from TouchEngine"));
		if (bIsValidHandle && bIsOutdated)
		{
			const TOptional<FTouchVulkanSemaphoreImport> SemaphoreImport = ImportTouchSemaphore(SemaphoreTE);
			if (!SemaphoreImport)
			{
				return false;
			}
			
			TouchObject<TEVulkanSemaphore> TouchSemaphore;
			TouchSemaphore.set(SemaphoreTE);
			SharedTexture->WaitSemaphoreData.Emplace(SharedHandle, TouchSemaphore, SemaphoreImport->VulkanSemaphore);
		}

		return bIsValidHandle;
	}

	void FRHICommandCopyTouchToUnreal::CopyTexture() const
	{
		const FTextureRHIRef TargetTexture = Target;

		const FVulkanTexture* Dest = static_cast<FVulkanTexture*>(TargetTexture->GetTextureBaseRHI());

		VkImageCopy Region;
		FMemory::Memzero(Region);
		const FPixelFormatInfo& PixelFormatInfo = GPixelFormats[TargetTexture->GetFormat()];
		const FTextureMetaData SrcInfo = SharedTexture->GetTextureMetaData();
		ensure(SharedTexture->CanCopyInto(Target));
		
		Region.extent.width = FMath::Max<uint32>(PixelFormatInfo.BlockSizeX, SrcInfo.SizeX);
		Region.extent.height = FMath::Max<uint32>(PixelFormatInfo.BlockSizeY, SrcInfo.SizeY);
		Region.extent.depth = 1;
		// FVulkanSurface constructor sets aspectMask like this so let's do the same for now
		Region.srcSubresource.aspectMask = VulkanRHI::GetAspectMaskFromUEFormat(SrcInfo.PixelFormat, true, true);
		Region.srcSubresource.layerCount = 1;
		Region.dstSubresource.aspectMask = Dest->GetFullAspectMask();
		Region.dstSubresource.layerCount = 1;
		
		VulkanRHI::vkCmdCopyImage(GetCommandBuffer(), *SharedTexture->ImageHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Dest->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
	}

	void FRHICommandCopyTouchToUnreal::ReleaseMutex(FVulkanCommandBuilder& CommandBuilder) const
	{
		if (!SharedTexture->SignalSemaphoreData.IsSet())
		{
			SharedTexture->SignalSemaphoreData = CreateAndExportSignalSemaphore(SharedTexture->SecurityAttributes->Get(), SharedTexture->CurrentSemaphoreValue, FString(TEXT("Semaphore")));
		}
		
		++SharedTexture->CurrentSemaphoreValue;
		CommandBuilder.AddSignalSemaphore({ *SharedTexture->SignalSemaphoreData->VulkanSemaphore.Get(), SharedTexture->CurrentSemaphoreValue });
		// The contents of the texture can be discarded so use TEInstanceAddTextureTransfer instead of TEInstanceAddVulkanTextureTransfer
		TEInstanceAddTextureTransfer(RequestParams.Instance, RequestParams.TETexture, SharedTexture->SignalSemaphoreData->TouchSemaphore, SharedTexture->CurrentSemaphoreValue);
	}
	
	ECopyTouchToUnrealResult CopyTouchToUnrealRHICommand(const FTouchCopyTextureArgs& CopyArgs, const TSharedRef<FTouchImportTextureVulkan>& SharedTexture, const TSharedRef<UE::TouchEngine::FTouchTextureImporter>& Importer)
	{
		if (!ensureMsgf(SharedTexture->CanCopyInto(CopyArgs.TargetRHI), TEXT("Caller was supposed to make sure that the target texture is compatible!")))
		{
			return ECopyTouchToUnrealResult::Failure;
		}
		
		// const TouchObject<TEInstance> Instance = CopyArgs.RequestParams.Instance;
		// const TouchObject<TETexture> TextureToCopy = CopyArgs.RequestParams.TETexture;
		
		if (CopyArgs.RequestParams.TETexture)
		{
			if (CopyArgs.RequestParams.TETextureTransfer.Result != TEResultSuccess && CopyArgs.RequestParams.TETextureTransfer.Result != TEResultNoMatchingEntity)
			{
				return ECopyTouchToUnrealResult::Failure;
			}
			
			ALLOC_COMMAND_CL(CopyArgs.RHICmdList, FRHICommandCopyTouchToUnreal)(Importer.ToSharedPtr()->AsWeak(), SharedTexture, CopyArgs.RequestParams, CopyArgs.TargetRHI);
			return ECopyTouchToUnrealResult::Success;
		}

		return ECopyTouchToUnrealResult::Failure;
	}
}
