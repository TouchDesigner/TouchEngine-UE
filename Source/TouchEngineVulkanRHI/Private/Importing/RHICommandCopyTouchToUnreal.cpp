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

#include "Logging.h"
#include "TouchImportTextureVulkan.h"
#include "Rendering/Importing/ITouchImportTexture.h"
#include "Rendering/Importing/TouchImportParams.h"
#include "Util/TextureShareVulkanPlatformWindows.h"
#include "Util/VulkanGetterUtils.h"
#include "Util/VulkanWindowsFunctions.h"

#include "TouchEngine/TEVulkan.h"
#include "Util/SemaphoreVulkanUtils.h"
#include "Util/VulkanCommandBuilder.h"

namespace UE::TouchEngine::Vulkan
{
	FRHICOMMAND_MACRO(FRHICommandCopyTouchToUnreal)
	{
		const TSharedPtr<FTouchImportTextureVulkan> SharedState;
		
		TPromise<ECopyTouchToUnrealResult> Promise;
		// Note that this keeps the output texture alive for the duration of the command (through FTouchImportParameters::Texture)
		FTouchImportParameters RequestParams;
		const UTexture* Target;

		// Vulkan related
		FVulkanPointers VulkanPointers;
		FVulkanCommandBuilder CommandBuilder;

		// Received from TE
		const VkImageLayout AcquireOldLayout;
		const VkImageLayout AcquireNewLayout;
		const TouchObject<TESemaphore> Semaphore;
		const uint64 WaitValue;

		bool bHasPromiseValue = false;
		
		FRHICommandCopyTouchToUnreal(TSharedPtr<FTouchImportTextureVulkan> InSharedState, TPromise<ECopyTouchToUnrealResult> InPromise, FTouchImportParameters RequestParams, UTexture* Target, VkImageLayout AcquireOldLayout, VkImageLayout AcquireNewLayout, TouchObject<TESemaphore> Semaphore, uint64 WaitValue)
			: SharedState(MoveTemp(InSharedState))
			, Promise(MoveTemp(InPromise))
			, RequestParams(RequestParams)
			, Target(Target)
			, CommandBuilder(*SharedState->CommandBuffer.Get())
			, AcquireOldLayout(AcquireOldLayout)
			, AcquireNewLayout(AcquireNewLayout)
			, Semaphore(MoveTemp(Semaphore))
			, WaitValue(WaitValue)
		{
			check(SharedState->WeakSharedOutputTextureReference == RequestParams.Texture);
		}

		~FRHICommandCopyTouchToUnreal()
		{
			if (!ensureMsgf(bHasPromiseValue, TEXT("Investigate broken promise")))
			{
				Promise.EmplaceValue(ECopyTouchToUnrealResult::Failure);
			}
		}

		void Execute(FRHICommandListBase& CmdList)
		{
			CommandBuilder.BeginCommands();
			const bool bSuccess = AcquireMutex(CmdList);
			if (bSuccess)
			{
				CopyTexture();
				ReleaseMutex();
				CommandBuilder.Submit(CmdList);
				Promise.SetValue(ECopyTouchToUnrealResult::Success);
			}
			else
			{
				Promise.SetValue(ECopyTouchToUnrealResult::Failure);
			}
			
			bHasPromiseValue = true;
		}

		VkCommandBuffer GetCommandBuffer() const { return CommandBuilder.GetCommandBuffer(); }
		
		bool AcquireMutex(FRHICommandListBase& CmdList);
		bool AllocateWaitSemaphore(const TouchObject<TEVulkanSemaphore>& SemaphoreTE);
		void CopyTexture();
		void ReleaseMutex();
	};

	bool FRHICommandCopyTouchToUnreal::AcquireMutex(FRHICommandListBase& CmdList)
	{
		TouchObject<TEVulkanSemaphore> VulkanSemaphoreTE;
		VulkanSemaphoreTE.set(static_cast<TEVulkanSemaphore*>(Semaphore.get()));
		AllocateWaitSemaphore(VulkanSemaphoreTE);
		if (!SharedState->WaitSemaphoreData.IsSet())
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Failed to copy Vulkan semaphore."))
			return false;
		}
		
		CommandBuilder.AddWaitSemaphore({ *SharedState->WaitSemaphoreData->VulkanSemaphore.Get(), WaitValue, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });
		
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
		SourceImageBarrier.image = *SharedState->ImageHandle.Get();
		
		const FTexture2DRHIRef TargetTexture = Target->GetResource()->TextureRHI->GetTexture2D();
		FVulkanTexture* Dest = static_cast<FVulkanTexture*>(TargetTexture->GetTextureBaseRHI());
		
		FVulkanCommandListContext& VulkanContext = static_cast<FVulkanCommandListContext&>(CmdList.GetContext());
		FVulkanImageLayout& UnrealLayoutData = VulkanContext.GetLayoutManager().GetFullLayoutChecked(Dest->Image);
		
		VkImageMemoryBarrier& DestImageBarrier = ImageBarriers[1];
		DestImageBarrier.pNext = nullptr;
		DestImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(UnrealLayoutData.MainLayout);
		DestImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(AcquireNewLayout);
		DestImageBarrier.oldLayout = UnrealLayoutData.MainLayout;
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
		const bool bIsOutdated = !SharedState->WaitSemaphoreData.IsSet() || SharedState->WaitSemaphoreData->Handle != SharedHandle;

		UE_CLOG(!bIsValidHandle, LogTouchEngineVulkanRHI, Warning, TEXT("Invalid semaphore handle received from TouchEngine"));
		if (bIsValidHandle && bIsOutdated)
		{
			const TOptional<FTouchVulkanSemaphoreImport> SemaphoreImport = ImportTouchSemaphore(SemaphoreTE, &FTouchImportTextureVulkan::OnWaitVulkanSemaphoreUsageChanged, this);
			if (!SemaphoreImport)
			{
				return false;
			}
			
			TouchObject<TEVulkanSemaphore> TouchSemaphore;
			TouchSemaphore.set(SemaphoreTE);
			SharedState->WaitSemaphoreData.Emplace(SharedHandle, TouchSemaphore, SemaphoreImport->VulkanSemaphore);
		}

		return bIsValidHandle;
	}

	void FRHICommandCopyTouchToUnreal::CopyTexture() 
	{
		const FTexture2DRHIRef TargetTexture = Target->GetResource()->TextureRHI->GetTexture2D();
		
		FVulkanTexture* Dest = static_cast<FVulkanTexture*>(TargetTexture->GetTextureBaseRHI());

		VkImageCopy Region;
		FMemory::Memzero(Region);
		const FPixelFormatInfo& PixelFormatInfo = GPixelFormats[TargetTexture->GetFormat()];
		const FTextureMetaData SrcInfo = SharedState->GetTextureMetaData();
		ensure(SharedState->CanCopyInto(Target));
		
		Region.extent.width = FMath::Max<uint32>(PixelFormatInfo.BlockSizeX, SrcInfo.SizeX);
		Region.extent.height = FMath::Max<uint32>(PixelFormatInfo.BlockSizeY, SrcInfo.SizeY);
		Region.extent.depth = 1;
		// FVulkanSurface constructor sets aspectMask like this so let's do the same for now
		Region.srcSubresource.aspectMask = GetAspectMaskFromUEFormat(SrcInfo.PixelFormat, true, true);
		Region.srcSubresource.layerCount = 1;
		Region.dstSubresource.aspectMask = Dest->GetFullAspectMask();
		Region.dstSubresource.layerCount = 1;
		
		VulkanRHI::vkCmdCopyImage(GetCommandBuffer(), *SharedState->ImageHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Dest->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
	}

	void FRHICommandCopyTouchToUnreal::ReleaseMutex()
	{
		if (!SharedState->SignalSemaphoreData.IsSet())
		{
			SharedState->SignalSemaphoreData = CreateAndExportSemaphore(SharedState->SecurityAttributes->Get(), SharedState->CurrentSemaphoreValue);
		}
		
		++SharedState->CurrentSemaphoreValue;
		CommandBuilder.AddSignalSemaphore({ *SharedState->SignalSemaphoreData->VulkanSemaphore.Get(), SharedState->CurrentSemaphoreValue });
		// The contents of the texture can be discarded so use TEInstanceAddTextureTransfer instead of TEInstanceAddVulkanTextureTransfer
		TEInstanceAddTextureTransfer(RequestParams.Instance, RequestParams.Texture, SharedState->SignalSemaphoreData->TouchSemaphore, SharedState->CurrentSemaphoreValue);
	}
	
	TFuture<ECopyTouchToUnrealResult> DispatchCopyTouchToUnrealRHICommand(const FTouchCopyTextureArgs& CopyArgs, TSharedRef<FTouchImportTextureVulkan> SharedState)
	{
		if (!ensureMsgf(SharedState->CanCopyInto(CopyArgs.Target), TEXT("Caller was supposed to make sure that the target texture is compatible!")))
		{
			return MakeFulfilledPromise<ECopyTouchToUnrealResult>(ECopyTouchToUnrealResult::Failure).GetFuture();
		}
		
		const TouchObject<TEInstance> Instance = CopyArgs.RequestParams.Instance;
		const TouchObject<TETexture> TextureToCopy = CopyArgs.RequestParams.Texture;
		
		if (TextureToCopy && TEInstanceHasVulkanTextureTransfer(Instance, TextureToCopy))
		{
			VkImageLayout AcquireOldLayout;
			VkImageLayout AcquireNewLayout;
			
			TouchObject<TESemaphore> Semaphore;
			uint64 WaitValue;
			const TEResult ResultCode = TEInstanceGetVulkanTextureTransfer(Instance, TextureToCopy, &AcquireOldLayout, &AcquireNewLayout, Semaphore.take(), &WaitValue);
			if (ResultCode != TEResultSuccess)
			{
				return MakeFulfilledPromise<ECopyTouchToUnrealResult>(ECopyTouchToUnrealResult::Failure).GetFuture();
			}

			TPromise<ECopyTouchToUnrealResult> Promise;
			TFuture<ECopyTouchToUnrealResult> Future = Promise.GetFuture();
			ALLOC_COMMAND_CL(CopyArgs.RHICmdList, FRHICommandCopyTouchToUnreal)(SharedState, MoveTemp(Promise), CopyArgs.RequestParams, CopyArgs.Target, AcquireOldLayout, AcquireNewLayout, Semaphore, WaitValue);
			return Future;
		}

		return MakeFulfilledPromise<ECopyTouchToUnrealResult>(ECopyTouchToUnrealResult::Failure).GetFuture();
	}
}