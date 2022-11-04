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

#include "TouchTextureExporterVulkan.h"

#include "vulkan_core.h"
#include "VulkanRHIPrivate.h"

#include "Logging.h"
#include "Rendering/Exporting/TouchExportParams.h"
#include "TouchEngine/TEVulkan.h"
#include "Util/SemaphoreVulkanUtils.h"
#include "Util/TextureShareVulkanPlatformWindows.h"
#include "Util/VulkanCommandBuilder.h"
#include "Util/VulkanGetterUtils.h"

namespace UE::TouchEngine::Vulkan
{
	FRHICOMMAND_MACRO(FRHICommandCopyUnrealToTouch)
	{
		TPromise<FTouchExportResult> Promise;
		bool bFulfilledPromise = false;

		const FTouchExportParameters ExportParameters;
		const TSharedPtr<FExportedTextureVulkan> SharedTextureResources;
		const TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes;
		FVulkanCommandBuilder CommandBuilder;
		
		FRHICommandCopyUnrealToTouch(TPromise<FTouchExportResult> Promise, FTouchExportParameters ExportParameters, TSharedPtr<FExportedTextureVulkan> InSharedTextureResources, TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes)
			: Promise(MoveTemp(Promise))
			, ExportParameters(MoveTemp(ExportParameters))
			, SharedTextureResources(MoveTemp(InSharedTextureResources))
			, SecurityAttributes(MoveTemp(SecurityAttributes))
			, CommandBuilder(SharedTextureResources->GetCommandBuffer().Get())
		{}

		~FRHICommandCopyUnrealToTouch()
		{
			if (!ensureMsgf(bFulfilledPromise, TEXT("Investigate broken promise")))
			{
				Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::UnknownFailure });
			}
		}

		VkCommandBuffer GetCommandBuffer() const { return SharedTextureResources->GetCommandBuffer().Get(); }
		
		FTexture2DRHIRef GetSourceTexture() const { return ExportParameters.Texture->GetResource()->TextureRHI->GetTexture2D(); }
		FVulkanSurface& GetSourceSurface() const
		{
			const FTexture2DRHIRef SourceUnrealTexture = GetSourceTexture();
			FVulkanTextureBase* Src = static_cast<FVulkanTextureBase*>(SourceUnrealTexture->GetTextureBaseRHI());
			return Src->Surface;
		}

		VkImage GetDestinationTexture() const { return *SharedTextureResources->GetImageOwnership(); }
		

		void Execute(FRHICommandListBase& CmdList)
		{
			CommandBuilder.BeginCommands();

			// 1. If TE is using it, schedule a wait operation
			if (TEInstanceHasVulkanTextureTransfer(ExportParameters.Instance, SharedTextureResources->GetTouchRepresentation()))
			{
				VkImageLayout AcquireOldLayout;
				VkImageLayout AcquireNewLayout;
				TouchObject<TESemaphore> Semaphore;
				uint64 WaitValue;
				const TEResult ResultCode = TEInstanceGetVulkanTextureTransfer(ExportParameters.Instance, SharedTextureResources->GetTouchRepresentation(), &AcquireOldLayout, &AcquireNewLayout, Semaphore.take(), &WaitValue);
				// Will be false the very first time the texture is created because TE has never received the texture
				if (ResultCode == TEResultSuccess)
				{
					WaitForReadAccess(Semaphore, WaitValue);
					TransferFromTouch(AcquireOldLayout, AcquireNewLayout);
				}
			}
			else
			{
				TransferFromInitialState();
			}

			// 2. Copy texture
			CopyTexture();

			// 3. 
			ReturnToTouchEngine();
			
			CommandBuilder.Submit(CmdList);
			bFulfilledPromise = true;
			Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::Success, SharedTextureResources->GetTouchRepresentation() });
		}

		void WaitForReadAccess(const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue);
		bool AllocateWaitSemaphore(const TouchObject<TESemaphore>& Semaphore);
		void TransferFromTouch(VkImageLayout AcquireOldLayout, VkImageLayout AcquireNewLayout);
		void TransferFromInitialState();
		
		void CopyTexture();
		void ReturnToTouchEngine();
	};

	void FRHICommandCopyUnrealToTouch::WaitForReadAccess(const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
		AllocateWaitSemaphore(Semaphore);

		if (ensure(SharedTextureResources->WaitSemaphoreData))
		{
			CommandBuilder.AddWaitSemaphore({ *SharedTextureResources->WaitSemaphoreData->VulkanSemaphore.Get(), WaitValue, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });
		}
	}

	bool FRHICommandCopyUnrealToTouch::AllocateWaitSemaphore(const TouchObject<TESemaphore>& Semaphore)
	{
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
	
	void FRHICommandCopyUnrealToTouch::TransferFromTouch(VkImageLayout AcquireOldLayout, VkImageLayout AcquireNewLayout)
	{
		VkImageMemoryBarrier ImageBarriers[2] = { { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER }, { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER } };
		VkImageMemoryBarrier& SourceImageBarrier = ImageBarriers[0];
		SourceImageBarrier.pNext = nullptr;
		SourceImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_UNDEFINED);
		SourceImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		SourceImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		SourceImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		SourceImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.image = GetSourceSurface().Image;

		ensureMsgf(AcquireNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXT("We called TEInstanceSetVulkanOutputAcquireImageLayout with VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL. TE did not transfer properly."));
		VkImageMemoryBarrier& DestImageBarrier = ImageBarriers[1];
		DestImageBarrier.pNext = nullptr;
		DestImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(AcquireOldLayout);
		DestImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		DestImageBarrier.oldLayout = AcquireOldLayout;
		DestImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		DestImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.image = GetDestinationTexture();

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

	void FRHICommandCopyUnrealToTouch::TransferFromInitialState()
	{
		VkImageMemoryBarrier ImageBarriers[2] = { { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER }, { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER } };
		VkImageMemoryBarrier& SourceImageBarrier = ImageBarriers[0];
		SourceImageBarrier.pNext = nullptr;
		SourceImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_UNDEFINED);
		SourceImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		SourceImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		SourceImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		SourceImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SourceImageBarrier.image = GetSourceSurface().Image;

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
	
	void FRHICommandCopyUnrealToTouch::CopyTexture() 
	{
		FVulkanSurface& SrcSurface = GetSourceSurface();

		VkImageCopy Region;
		FMemory::Memzero(Region);
		const FPixelFormatInfo& PixelFormatInfo = GPixelFormats[GetSourceTexture()->GetFormat()];
		ensure(SrcSurface.Width <= static_cast<uint32>(SharedTextureResources->GetResolution().X) && SrcSurface.Height <= static_cast<uint32>(SharedTextureResources->GetResolution().Y));
		Region.extent.width = FMath::Max<uint32>(PixelFormatInfo.BlockSizeX, SharedTextureResources->GetResolution().X);
		Region.extent.height = FMath::Max<uint32>(PixelFormatInfo.BlockSizeY, SharedTextureResources->GetResolution().Y);
		Region.extent.depth = 1;
		// FVulkanSurface constructor sets aspectMask like this so let's do the same for now
		Region.srcSubresource.aspectMask = SrcSurface.GetFullAspectMask();
		Region.srcSubresource.layerCount = 1;
		Region.dstSubresource.aspectMask = GetAspectMaskFromUEFormat(SharedTextureResources->GetPixelFormat(), true, true);
		Region.dstSubresource.layerCount = 1;
		
		VulkanRHI::vkCmdCopyImage(GetCommandBuffer(), SrcSurface.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *SharedTextureResources->GetImageOwnership(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
	}

	void FRHICommandCopyUnrealToTouch::ReturnToTouchEngine()
	{
		if (!SharedTextureResources->SignalSemaphoreData.IsSet())
		{
			SharedTextureResources->SignalSemaphoreData = CreateAndExportSemaphore(SecurityAttributes->Get(), SharedTextureResources->CurrentSemaphoreValue);
		}

		constexpr VkImageLayout OldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		const VkImageLayout NewLayout = TEInstanceGetVulkanInputReleaseImageLayout(ExportParameters.Instance.get());
		VkImageMemoryBarrier DestImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		DestImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(OldLayout);
		DestImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(NewLayout);
		DestImageBarrier.oldLayout = OldLayout;
		DestImageBarrier.newLayout = NewLayout;
		DestImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DestImageBarrier.image = GetDestinationTexture();
		VulkanRHI::vkCmdPipelineBarrier(
			GetCommandBuffer(),
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
		
		++SharedTextureResources->CurrentSemaphoreValue;
		CommandBuilder.AddSignalSemaphore({ *SharedTextureResources->SignalSemaphoreData->VulkanSemaphore.Get(), SharedTextureResources->CurrentSemaphoreValue });
		// The contents of the texture can be discarded so use TEInstanceAddTextureTransfer instead of TEInstanceAddVulkanTextureTransfer
		TEInstanceAddVulkanTextureTransfer(
			ExportParameters.Instance,
			SharedTextureResources->GetTouchRepresentation(),
			OldLayout,
			NewLayout,
			SharedTextureResources->SignalSemaphoreData->TouchSemaphore,
			SharedTextureResources->CurrentSemaphoreValue);
	}
	
	FTouchTextureExporterVulkan::FTouchTextureExporterVulkan(TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes)
		: SecurityAttributes(MoveTemp(SecurityAttributes))
	{}

	TFuture<FTouchSuspendResult> FTouchTextureExporterVulkan::SuspendAsyncTasks()
	{
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
		
		TFuture<FTouchSuspendResult> FinishRenderingTasks = FTouchTextureExporter::SuspendAsyncTasks();
		// Once all the rendering tasks have finished using the copying textures, they can be released.
		FinishRenderingTasks.Next([this, Promise = MoveTemp(Promise)](auto) mutable
		{
			ReleaseTextures().Next([Promise = MoveTemp(Promise)](auto) mutable
			{
				Promise.SetValue({});
			});
		});

		return Future;
	}

	TSharedPtr<FExportedTextureVulkan> FTouchTextureExporterVulkan::CreateTexture(const FTextureCreationArgs& Params)
	{
		const FRHITexture2D* SourceRHI = GetRHIFromTexture(Params.Texture);
		return FExportedTextureVulkan::Create(*SourceRHI, Params.CmdList, SecurityAttributes);
	}

	TFuture<FTouchExportResult> FTouchTextureExporterVulkan::ExportTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTouchExportParameters& Params)
	{
		const TSharedPtr<FExportedTextureVulkan> SharedTextureResources = GetOrTryCreateTexture(MakeTextureCreationArgs(Params, { RHICmdList }));
		if (!SharedTextureResources)
		{
			return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::InternalGraphicsDriverError }).GetFuture();
		}
		
		TPromise<FTouchExportResult> Promise; 
		TFuture<FTouchExportResult> Future = Promise.GetFuture();

		ALLOC_COMMAND_CL(RHICmdList, FRHICommandCopyUnrealToTouch)(MoveTemp(Promise), Params, SharedTextureResources, SecurityAttributes);
		return Future;
	}
}
