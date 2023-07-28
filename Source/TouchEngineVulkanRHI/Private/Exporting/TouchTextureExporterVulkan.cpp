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
#include "RHI.h"
#include "TextureResource.h"

THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END
#include "HAL/Platform.h"
#if PLATFORM_WINDOWS
#include "VulkanPlatformDefines.h"
#endif
#include "VulkanRHIPrivate.h"
#include "VulkanContext.h"

#include "Logging.h"
#include "Engine/TEDebug.h"
#include "Rendering/Exporting/TouchExportParams.h"
#include "TouchEngine/TEVulkan.h"
#include "Util/SemaphoreVulkanUtils.h"
#include "Util/TextureShareVulkanPlatformWindows.h"
#include "Util/VulkanCommandBuilder.h"
#include "Util/VulkanGetterUtils.h"

#include "Engine/Texture.h"
#include "Engine/Util/TouchVariableManager.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/TouchHelpers.h"

namespace UE::TouchEngine::Vulkan
{
	FRHICOMMAND_MACRO(FRHICommandCopyUnrealToTouch)
	{
		bool bFulfilledPromise = false;

		TSharedPtr<FTouchTextureExporterVulkan> Exporter;
		const FTouchExportParameters ExportParameters;
		const TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes;

		TSharedRef<FExportedTextureVulkan> SharedTextureResources;
		
		FRHICommandCopyUnrealToTouch(TSharedPtr<FTouchTextureExporterVulkan> Exporter, FTouchExportParameters ExportParameters,
			TSharedRef<FExportedTextureVulkan>& InDestTexture, TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes)
			: Exporter(MoveTemp(Exporter))
			, ExportParameters(MoveTemp(ExportParameters))
			, SecurityAttributes(MoveTemp(SecurityAttributes))
			, SharedTextureResources(InDestTexture)
		{}

		VkCommandBuffer GetCommandBuffer() const { return *SharedTextureResources->GetCommandBuffer().Get(); }
		
		FTexture2DRHIRef GetSourceTexture() const { return ExportParameters.Texture->GetResource()->TextureRHI->GetTexture2D(); }
		FVulkanTexture* GetSourceVulkanTexture() const
		{
			return static_cast<FVulkanTexture*>(SharedTextureResources->GetStableRHIOfTextureToCopy().GetReference());
		}

		VkImage GetDestinationTexture() const { return *SharedTextureResources->GetImageOwnership(); }
		

		void Execute(FRHICommandListBase& CmdList)
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    I.B.4 [RHI] Cook Frame - RHI Export Copy"), STAT_TE_I_B_4_Vulkan, STATGROUP_TouchEngine);
			SharedTextureResources->LogCompletedValue(FString("1. Start of `FRHICommandCopyUnrealToTouch::Execute`:"));

			FVulkanCommandBuilder CommandBuilder = *SharedTextureResources->EnsureCommandBufferInitialized(CmdList).Get(); // SharedTextureResources->GetCommandBuffer().Get();

			{
				bool bBeganCommands = false;
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.a [RHI] Cook Frame - RHI - Wait for Read Access"), STAT_TE_I_B_4_a_Vulkan, STATGROUP_TouchEngine);
				// 1. If TE still has ownership of it, schedule a wait operation
				const bool bNeedsOwnershipTransfer = SharedTextureResources->WasEverUsedByTouchEngine();
				if (bNeedsOwnershipTransfer)
				{
					const TEResult& GetTextureTransferResult = ExportParameters.GetTextureTransferResult;
					if (GetTextureTransferResult == TEResultSuccess)
					{
						CommandBuilder.BeginCommands();
						WaitForReadAccess(CommandBuilder, ExportParameters.GetTextureTransferSemaphore, ExportParameters.GetTextureTransferWaitValue);
						TransferFromTouch(CmdList);
						bBeganCommands = true;
					}
					else if (GetTextureTransferResult != TEResultNoMatchingEntity) // TE does not have ownership
					{
						UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to transfer ownership of pooled texture back from Touch Engine"));
						return;
					}
				}
				
				if (!bBeganCommands)
				{
					CommandBuilder.BeginCommands();
					TransferFromInitialState(CmdList);
				}
			}

			SharedTextureResources->LogCompletedValue(FString("2. After Read Access"));
			{
				// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.b [RHI] Cook Frame - RHI - Enqueue Copy"), STAT_TE_I_B_4_b_Vulkan, STATGROUP_TouchEngine);
				// 2. Copy texture
				CopyTexture();
			}

			{
				// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.c [RHI] Cook Frame - RHI - Enqueue Signal"), STAT_TE_I_B_4_c_Vulkan, STATGROUP_TouchEngine);
				// 3. 
				ReturnToTouchEngine(CommandBuilder);
			}
			SharedTextureResources->LogCompletedValue(FString("3. After ReturnToTouchEngine"));
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.d [RHI] Cook Frame - RHI - Execute Command List"), STAT_TE_I_B_4_d_Vulkan, STATGROUP_TouchEngine);
				CommandBuilder.Submit(CmdList);
			}
			SharedTextureResources->LogCompletedValue(FString("4. After Submitting Command Builder"));
			bFulfilledPromise = true;
		}

		void WaitForReadAccess(FVulkanCommandBuilder& CommandBuilder, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue);
		bool AllocateWaitSemaphore(const TouchObject<TESemaphore>& Semaphore);
		void TransferFromTouch(FRHICommandListBase& CmdList);
		void TransferFromInitialState(FRHICommandListBase& CmdList);
		
		void CopyTexture();
		void ReturnToTouchEngine(FVulkanCommandBuilder& CommandBuilder);
	};

	void FRHICommandCopyUnrealToTouch::WaitForReadAccess(FVulkanCommandBuilder& CommandBuilder, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
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
	
	void FRHICommandCopyUnrealToTouch::TransferFromTouch(FRHICommandListBase& CmdList)
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

		VkImageMemoryBarrier& DestImageBarrier = ImageBarriers[1];
		DestImageBarrier.pNext = nullptr;
		DestImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_UNDEFINED); 
		DestImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		DestImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // VK_IMAGE_LAYOUT_UNDEFINED tells GPU that we can override old texture data
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

	void FRHICommandCopyUnrealToTouch::TransferFromInitialState(FRHICommandListBase& CmdList)
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
		const FVulkanTexture* SourceVulkanTexture = GetSourceVulkanTexture();

		VkImageCopy Region;
		FMemory::Memzero(Region);
		const FPixelFormatInfo& PixelFormatInfo = GPixelFormats[GetSourceTexture()->GetFormat()];
		ensure(SourceVulkanTexture->GetDesc().Extent.X <= SharedTextureResources->GetResolution().X
			&& SourceVulkanTexture->GetDesc().Extent.Y <= SharedTextureResources->GetResolution().Y);
		Region.extent.width = FMath::Max<uint32>(PixelFormatInfo.BlockSizeX, SharedTextureResources->GetResolution().X);
		Region.extent.height = FMath::Max<uint32>(PixelFormatInfo.BlockSizeY, SharedTextureResources->GetResolution().Y);
		Region.extent.depth = 1;
		// FVulkanSurface constructor sets aspectMask like this so let's do the same for now
		Region.srcSubresource.aspectMask = SourceVulkanTexture->GetFullAspectMask();
		Region.srcSubresource.layerCount = 1;
		Region.dstSubresource.aspectMask = GetAspectMaskFromUEFormat(SharedTextureResources->GetPixelFormat(), true, true);
		Region.dstSubresource.layerCount = 1;
		
		VulkanRHI::vkCmdCopyImage(GetCommandBuffer(), SourceVulkanTexture->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *SharedTextureResources->GetImageOwnership(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
		UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("   [FRHICommandCopyUnrealToTouch[%s]] Texture copy enqueued to render thread for param `%s`."),
						*GetCurrentThreadStr(),
						*ExportParameters.ParameterName.ToString())
	}

	void FRHICommandCopyUnrealToTouch::ReturnToTouchEngine(FVulkanCommandBuilder& CommandBuilder)
	{
		if (!SharedTextureResources->SignalSemaphoreData.IsSet())
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[FRHICommandCopyUnrealToTouch::ReturnToTouchEngine] `SignalSemaphoreData` is not set!"));
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
		CommandBuilder.AddSignalSemaphore({ *SharedTextureResources->SignalSemaphoreData->VulkanSemaphore.Get(), SharedTextureResources->CurrentSemaphoreValue});
		UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("   [FRHICommandCopyUnrealToTouch[%s]] Enqueuing Fence change to `%llu`"), *GetCurrentThreadStr(), SharedTextureResources->CurrentSemaphoreValue)
	}

	// ---------------------------------------------------------------------
	// -------------------- FTouchTextureExporterVulkan
	// ---------------------------------------------------------------------
	
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

	TSharedPtr<FExportedTextureVulkan> FTouchTextureExporterVulkan::CreateTexture(const FTouchExportParameters& Params, const FRHITexture2D* ParamTextureRHI) const
	{
		return FExportedTextureVulkan::Create(*ParamTextureRHI, SecurityAttributes);
	}

	void FTouchTextureExporterVulkan::FinalizeExportsToTouchEngine_AnyThread(const FTouchEngineInputFrameData& FrameData)
	{
		TexturePoolMaintenance();
	}

	TEResult FTouchTextureExporterVulkan::AddTETextureTransfer(FTouchExportParameters& Params, const TSharedPtr<FExportedTextureVulkan>& Texture)
	{
		if (!Texture->SignalSemaphoreData.IsSet())
		{
			Texture->SignalSemaphoreData = CreateAndExportSemaphore(SecurityAttributes->Get(), Texture->CurrentSemaphoreValue,
				FString::Printf(TEXT("%s [%lld]"), *Params.ParameterName.ToString(), Params.FrameData.FrameID));
			Texture->LogCompletedValue(FString("After `CreateAndExportSemaphore`:"));
		}
		
		return TEInstanceAddTextureTransfer(Params.Instance, Texture->GetTouchRepresentation(), Texture->SignalSemaphoreData->TouchSemaphore, Texture->CurrentSemaphoreValue + 1);
	}

	void FTouchTextureExporterVulkan::FinaliseExportAndEnqueueCopy_AnyThread(FTouchExportParameters& Params, TSharedPtr<FExportedTextureVulkan>& Texture)
	{
		// For Vulkan, we enqueue the copy on the render thread.
		ENQUEUE_RENDER_COMMAND(AccessTexture)([StrongThis = SharedThis(this), Params = MoveTemp(Params),
				SecurityAttributes = SecurityAttributes, ExportedTextureVulkan = Texture.ToSharedRef()](FRHICommandListImmediate& RHICmdList) mutable
		{
			const bool bBecameInvalidSinceRenderEnqueue = !IsValid(Params.Texture);
			if (bBecameInvalidSinceRenderEnqueue)
			{
				return;
			}

			if (!ensure(ExportedTextureVulkan->CanFitTexture(ExportedTextureVulkan->GetStableRHIOfTextureToCopy())))
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[RT] !TextureData->CanFitTexture(SourceRHI) %s vs %s for `%s` for frame %lld"),
				       *ExportedTextureVulkan->Resolution.ToString(), *ExportedTextureVulkan->GetStableRHIOfTextureToCopy()->GetDesc().GetSize().ToString(), *Params.ParameterName.ToString(), Params.FrameData.FrameID)
			}
			ALLOC_COMMAND_CL(RHICmdList, FRHICommandCopyUnrealToTouch)(StrongThis, Params, ExportedTextureVulkan, SecurityAttributes);
		});
	}
}
