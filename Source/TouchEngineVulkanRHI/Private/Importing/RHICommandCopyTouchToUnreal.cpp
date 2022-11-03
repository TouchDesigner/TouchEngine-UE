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
#include "Util/VulkanCommandBuilder.h"

namespace UE::TouchEngine::Vulkan
{
	FRHICOMMAND_MACRO(FRHICommandCopyTouchToUnreal)
	{
		const TSharedPtr<FTouchImportTextureVulkan> SharedState;
		
		TPromise<ECopyTouchToUnrealResult> Promise;
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
			check(SharedState->SharedOutputTexture == RequestParams.Texture);
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
			const bool bSuccess = AcquireMutex();
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


		bool AcquireMutex();
		bool AllocateWaitSemaphore(TEVulkanSemaphore* SemaphoreTE);
		void CopyTexture();
		void ReleaseMutex();
	};

	bool FRHICommandCopyTouchToUnreal::AcquireMutex()
	{
		TEVulkanSemaphore* SemaphoreTE = static_cast<TEVulkanSemaphore*>(Semaphore.get());
		const VkSemaphoreType SemaphoreType = TEVulkanSemaphoreGetType(SemaphoreTE);
		const bool bIsTimeline = SemaphoreType == VK_SEMAPHORE_TYPE_TIMELINE_KHR || SemaphoreType == VK_SEMAPHORE_TYPE_TIMELINE; 
		if (!bIsTimeline)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Received a binary semaphore but only timeline semaphores are implemented."))
			return false;
		}

		AllocateWaitSemaphore(SemaphoreTE);
		if (!SharedState->WaitSemaphoreData.IsSet())
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Failed to copy Vulkan semaphore."))
			return false;
		}
		
		CommandBuilder.AddWaitSemaphore({ *SharedState->WaitSemaphoreData->VulkanSemaphore.Get(), WaitValue, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });

		ensureMsgf(AcquireNewLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXT("Texture copying requires VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL format. Check that we called TEInstanceSetVulkanAcquireImageLayout correctly."));
		VkImageMemoryBarrier ImageBarriers[2] = { { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER }, { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER } };
		VkImageMemoryBarrier& SourceImageBarrier = ImageBarriers[0];
		SourceImageBarrier.pNext = nullptr;
		SourceImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(AcquireOldLayout);
		SourceImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(AcquireNewLayout);
		SourceImageBarrier.oldLayout = AcquireOldLayout;
		SourceImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		SourceImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.image = *SharedState->ImageHandle.Get();
		
		const FTexture2DRHIRef TargetTexture = Target->GetResource()->TextureRHI->GetTexture2D();
		FVulkanTextureBase* Dest = static_cast<FVulkanTextureBase*>(TargetTexture->GetTextureBaseRHI());
		FVulkanSurface& DstSurface = Dest->Surface;
		VkImageMemoryBarrier& DestImageBarrier = ImageBarriers[1];
		DestImageBarrier.pNext = nullptr;
		DestImageBarrier.srcAccessMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		DestImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(AcquireNewLayout);
		DestImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DestImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		DestImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.image = DstSurface.Image;
		
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

	bool FRHICommandCopyTouchToUnreal::AllocateWaitSemaphore(TEVulkanSemaphore* SemaphoreTE)
	{
		const HANDLE SharedHandle = TEVulkanSemaphoreGetHandle(SemaphoreTE);
		const bool bIsValidHandle = SharedHandle != nullptr;
		const bool bIsOutdated = !SharedState->WaitSemaphoreData.IsSet() || SharedState->WaitSemaphoreData->Handle != SharedHandle;

		UE_CLOG(!bIsValidHandle, LogTouchEngineVulkanRHI, Warning, TEXT("Invalid semaphore handle received from TouchEngine"));
		if (bIsValidHandle && bIsOutdated)
		{
			const VkExternalSemaphoreHandleTypeFlagBits SemaphoreHandleType = TEVulkanSemaphoreGetHandleType(SemaphoreTE);
			// Other types not allowed for vkImportSemaphoreWin32HandleKHR
			if (SemaphoreHandleType != VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT && SemaphoreHandleType != VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT)
			{
				UE_LOG(LogVulkanRHI, Error, TEXT("Unexpected VkExternalSemaphoreHandleTypeFlagBits for Vulkan semaphore exported from Touch Designer (VkExternalSemaphoreHandleTypeFlagBits = %d)"), static_cast<int32>(SemaphoreHandleType))
				return false;
			}

			VkSemaphoreTypeCreateInfo SemTypeCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
			SemTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
			VkSemaphoreCreateInfo SemCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &SemTypeCreateInfo };
			VkSemaphore VulkanSemaphore;
			VERIFYVULKANRESULT(VulkanRHI::vkCreateSemaphore(VulkanPointers.VulkanDeviceHandle, &SemCreateInfo, NULL, &VulkanSemaphore));

			TouchObject<TEVulkanSemaphore> TouchSemaphore;
			TouchSemaphore.set(SemaphoreTE);
			TSharedPtr<VkSemaphore> SharedVulkanSemaphore = MakeShareable<VkSemaphore>(new VkSemaphore(VulkanSemaphore), [](VkSemaphore* VulkanSemaphore)
			{
				const FVulkanPointers VulkanPointers;
				if (VulkanPointers.VulkanDevice)
				{
					VulkanRHI::vkDestroySemaphore(VulkanPointers.VulkanDeviceHandle, *VulkanSemaphore, nullptr);
				}

				delete VulkanSemaphore;
			});
			
			VkImportSemaphoreWin32HandleInfoKHR ImportSemWin32Info = { VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR };
			ImportSemWin32Info.semaphore = VulkanSemaphore;
			ImportSemWin32Info.handleType = SemaphoreHandleType;
			ImportSemWin32Info.handle = SharedHandle;

			VERIFYVULKANRESULT(vkImportSemaphoreWin32HandleKHR(VulkanPointers.VulkanDeviceHandle, &ImportSemWin32Info));
			TEVulkanSemaphoreSetCallback(SemaphoreTE, &SharedState->OnWaitVulkanSemaphoreUsageChanged, this);
			
			SharedState->WaitSemaphoreData.Emplace(SharedHandle, TouchSemaphore, SharedVulkanSemaphore);
		}

		return bIsValidHandle;
	}

	void FRHICommandCopyTouchToUnreal::CopyTexture() 
	{
		const FTexture2DRHIRef TargetTexture = Target->GetResource()->TextureRHI->GetTexture2D();
		
		FVulkanTextureBase* Dest = static_cast<FVulkanTextureBase*>(TargetTexture->GetTextureBaseRHI());
		FVulkanSurface& DstSurface = Dest->Surface;

		VkImageCopy Region;
		FMemory::Memzero(Region);
		const FPixelFormatInfo& PixelFormatInfo = GPixelFormats[TargetTexture->GetFormat()];
		ensure(SharedState->SourceTextureMetaData.SizeX <= DstSurface.Width && SharedState->SourceTextureMetaData.SizeY <= DstSurface.Height);
		Region.extent.width = FMath::Max<uint32>(PixelFormatInfo.BlockSizeX, SharedState->SourceTextureMetaData.SizeX);
		Region.extent.height = FMath::Max<uint32>(PixelFormatInfo.BlockSizeY, SharedState->SourceTextureMetaData.SizeY);
		Region.extent.depth = 1;
		// FVulkanSurface constructor sets aspectMask like this so let's do the same for now
		Region.srcSubresource.aspectMask = GetAspectMaskFromUEFormat(SharedState->SourceTextureMetaData.PixelFormat, true, true);
		Region.srcSubresource.layerCount = 1;
		Region.dstSubresource.aspectMask = DstSurface.GetFullAspectMask();
		Region.dstSubresource.layerCount = 1;
		
		VulkanRHI::vkCmdCopyImage(GetCommandBuffer(), *SharedState->ImageHandle.Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstSurface.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
	}

	void FRHICommandCopyTouchToUnreal::ReleaseMutex()
	{
		if (!SharedState->SignalSemaphoreData.IsSet())
		{
			FTouchImportTextureVulkan::FSignalSemaphoreData NewSemaphoreData;

			VkExportSemaphoreCreateInfo ExportSemInfo = { VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO };
			ExportSemInfo.handleTypes = SharedState->bUseNTHandles ? VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT : VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
			VkExportSemaphoreWin32HandleInfoKHR ExportSemWin32Info = { VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR };
			if (SharedState->bUseNTHandles)
			{
				ExportSemWin32Info.dwAccess = STANDARD_RIGHTS_ALL | SEMAPHORE_MODIFY_STATE;
				ExportSemWin32Info.pAttributes = SharedState->SecurityAttributes->Get();
				ExportSemInfo.pNext = &ExportSemWin32Info;
			}

			VkSemaphoreTypeCreateInfo SemaphoreTypeCreateInfo { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, &ExportSemInfo };
			SemaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
			SemaphoreTypeCreateInfo.initialValue = SharedState->CurrentSemaphoreValue;
			
			VkSemaphoreCreateInfo SemCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &SemaphoreTypeCreateInfo };
			VkSemaphore VulkanSemaphore;
			VERIFYVULKANRESULT(VulkanRHI::vkCreateSemaphore(VulkanPointers.VulkanDeviceHandle, &SemCreateInfo, NULL, &VulkanSemaphore));
			NewSemaphoreData.VulkanSemaphore = MakeShareable<VkSemaphore>(new VkSemaphore(VulkanSemaphore), [](VkSemaphore* VulkanSemaphore)
			{
				const FVulkanPointers VulkanPointers;
				if (VulkanPointers.VulkanDevice)
				{
					VulkanRHI::vkDestroySemaphore(VulkanPointers.VulkanDeviceHandle, *VulkanSemaphore, nullptr);
				}

				delete VulkanSemaphore;
			});
			
			VkSemaphoreGetWin32HandleInfoKHR semaphoreGetWin32HandleInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR };
			semaphoreGetWin32HandleInfo.semaphore = VulkanSemaphore;
			semaphoreGetWin32HandleInfo.handleType = (VkExternalSemaphoreHandleTypeFlagBits)ExportSemInfo.handleTypes;
			VERIFYVULKANRESULT(vkGetSemaphoreWin32HandleKHR(VulkanPointers.VulkanDeviceHandle, &semaphoreGetWin32HandleInfo, &NewSemaphoreData.ExportedHandle));

			TEVulkanSemaphore* TouchSemaphore = TEVulkanSemaphoreCreate(SemaphoreTypeCreateInfo.semaphoreType, NewSemaphoreData.ExportedHandle, (VkExternalSemaphoreHandleTypeFlagBits)ExportSemInfo.handleTypes, nullptr, nullptr);
			NewSemaphoreData.TouchSemaphore.set(TouchSemaphore);

			SharedState->SignalSemaphoreData = NewSemaphoreData;
		}
		
		++SharedState->CurrentSemaphoreValue;
		CommandBuilder.AddSignalSemaphore({ *SharedState->SignalSemaphoreData->VulkanSemaphore.Get(), SharedState->CurrentSemaphoreValue });
		// The contents of the texture can be discarded so use TEInstanceAddTextureTransfer instead of TEInstanceAddVulkanTextureTransfer
		TEInstanceAddTextureTransfer(RequestParams.Instance, RequestParams.Texture, SharedState->SignalSemaphoreData->TouchSemaphore, SharedState->CurrentSemaphoreValue);
	}
	
	TFuture<ECopyTouchToUnrealResult> DispatchCopyTouchToUnrealRHICommand(const FTouchCopyTextureArgs& CopyArgs, TSharedRef<FTouchImportTextureVulkan> SharedState)
	{
		const TouchObject<TEInstance> Instance = CopyArgs.RequestParams.Instance;
		const TouchObject<TETexture> TextureToCopy = CopyArgs.RequestParams.Texture;
		
		if (TextureToCopy && TEInstanceHasVulkanTextureTransfer(Instance, TextureToCopy))
		{
			VkImageLayout AcquireOldLayout;
			VkImageLayout AcquireNewLayout; // Will be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL because we called TEInstanceSetVulkanAcquireImageLayout with it
			
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
