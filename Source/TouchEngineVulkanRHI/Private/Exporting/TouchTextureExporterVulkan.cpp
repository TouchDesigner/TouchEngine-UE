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

THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END
#include "VulkanPlatformDefines.h"
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
		FRHITexture2D* SourceRHI; //The RHI from the UTexture, that we retrieved previously to ensure the same Mip is used
		
		FRHICommandCopyUnrealToTouch(TSharedPtr<FTouchTextureExporterVulkan> Exporter, FTouchExportParameters ExportParameters,
			TSharedRef<FExportedTextureVulkan>& InDestTexture, TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes, FRHITexture2D* InSourceRHI)
			: Exporter(MoveTemp(Exporter))
			, ExportParameters(MoveTemp(ExportParameters))
			, SecurityAttributes(MoveTemp(SecurityAttributes))
			, SharedTextureResources(InDestTexture)
			, SourceRHI(InSourceRHI)
		{}

		VkCommandBuffer GetCommandBuffer() const { return *SharedTextureResources->GetCommandBuffer().Get(); }
		
		FTexture2DRHIRef GetSourceTexture() const { return ExportParameters.Texture->GetResource()->TextureRHI->GetTexture2D(); }
		FVulkanTexture* GetSourceVulkanTexture() const
		{
			// const FTexture2DRHIRef SourceUnrealTexture = GetSourceTexture();
			// return static_cast<FVulkanTexture*>(SourceUnrealTexture->GetTextureBaseRHI());
			return static_cast<FVulkanTexture*>(SourceRHI->GetTextureBaseRHI());
		}

		VkImage GetDestinationTexture() const { return *SharedTextureResources->GetImageOwnership(); }
		

		void Execute(FRHICommandListBase& CmdList)
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("RHI Export Copy"), STAT_RHIExportCopy, STATGROUP_TouchEngine);
			SharedTextureResources->LogCompletedValue(FString("1. Start of `FRHICommandCopyUnrealToTouch::Execute`:"));

			FVulkanCommandBuilder CommandBuilder = *SharedTextureResources->EnsureCommandBufferInitialized(CmdList).Get(); // SharedTextureResources->GetCommandBuffer().Get();

			{
				bool bBeganCommands = false;
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("RHI Export Copy - 1. Wait for Read Access"), STAT_RHIExportCopyWait, STATGROUP_TouchEngine);
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
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("RHI Export Copy - 2. Enqueue Copy"), STAT_RHIExportCopyEnqueue, STATGROUP_TouchEngine);
				// 2. Copy texture
				CopyTexture();
			}

			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("RHI Export Copy - 3. Enqueue Signal"), STAT_RHIExportCopyWait, STATGROUP_TouchEngine);
				// 3. 
				ReturnToTouchEngine(CommandBuilder);
			}
			SharedTextureResources->LogCompletedValue(FString("3. After ReturnToTouchEngine"));
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("RHI Export Copy - 4. Execute Command List"), STAT_RHIExportCopyExecuteCommandList, STATGROUP_TouchEngine);
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

	TSharedPtr<FExportedTextureVulkan> FTouchTextureExporterVulkan::CreateTexture(const FTouchExportParameters& Params) const
	{
		const FRHITexture2D* SourceRHI = GetRHIFromTexture(Params.Texture);
		return FExportedTextureVulkan::Create(*SourceRHI, SecurityAttributes);
	}

	TouchObject<TETexture> FTouchTextureExporterVulkan::ExportTexture_AnyThread(const FTouchExportParameters& ParamsConst, TEGraphicsContext* GraphicContext)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Export Texture"), STAT_TEExportTexture, STATGROUP_TouchEngine);
		//todo: code really close to FTouchTextureExporterD3D12::ExportTexture_AnyThread, any way to simplify?
		// 1. We get a Vulkan Texture to copy onto
		bool bIsNewTexture = false;
		TSharedPtr<FExportedTextureVulkan> TextureData;
		FRHITexture2D* SourceRHI;
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Export Texture - 1. Get or Create Pool Texture"), STAT_TEExportTexturePool, STATGROUP_TouchEngine);
			TextureData = GetNextOrAllocPooledTexture(ParamsConst, bIsNewTexture);
			if (!TextureData)
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[ExportTexture_AnyThread[%s]] ETouchExportErrorCode::InternalGraphicsDriverError for parameter `%s` for frame %lld"), *UE::TouchEngine::GetCurrentThreadStr(), *ParamsConst.ParameterName.ToString(), ParamsConst.FrameData.FrameID);
				return nullptr;
			}
			// We need to get the RHI now as this is the RHI of the current Mip, and the current Mip might have changed at render time which would change the size of the texture and fail the copy
			SourceRHI = GetRHIFromTexture(ParamsConst.Texture);
			if (!TextureData->CanFitTexture(SourceRHI) || !TextureData->CanFitTexture(ParamsConst))
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[ExportTexture_AnyThread[%s]] Mips are not currently supported. Make sure the texture `%s` set for parameter `%s` does not have mips"),
					*UE::TouchEngine::GetCurrentThreadStr(), *GetNameSafe(ParamsConst.Texture), *ParamsConst.ParameterName.ToString())
				return nullptr;
			}
		}
		
		UE_LOG(LogTouchEngineVulkanRHI, Display, TEXT("[ExportTexture_AnyThread[%s]] GetNextOrAllocPooledTexture returned %s `%s` for parameter `%s` for frame %lld"),
			   *UE::TouchEngine::GetCurrentThreadStr(), bIsNewTexture ? TEXT("a NEW texture") : TEXT("the EXISTING texture"), *TextureData->DebugName, *ParamsConst.ParameterName.ToString(), ParamsConst.FrameData.FrameID);

		const TouchObject<TETexture>& TouchTexture = TextureData->GetTouchRepresentation();
		FTouchExportParameters Params{ParamsConst};
		Params.GetTextureTransferSemaphore = nullptr;
		Params.GetTextureTransferWaitValue = 0;
		Params.GetTextureTransferResult = TEResultNoMatchingEntity;
		
		// constexpr VkImageLayout OldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		// const VkImageLayout NewLayout = TEInstanceGetVulkanInputReleaseImageLayout(Params.Instance);
		
		// 2. If this is not a new texture, transfer ownership if needed
		if (!bIsNewTexture) // If this is a pre-existing texture
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Export Texture - 2. GetTextureTransfer"), STAT_TEExportTextureGetTransfer, STATGROUP_TouchEngine);
			// if (Params.bReuseExistingTexture)
			// {
			// 	UE_LOG(LogTouchEngineVulkanRHI, Warning, TEXT("[ExportTexture_AnyThread[%s]] Reusing existing texture `%s` for `%s` as Params.bReuseExistingTexture was true for frame %lld"), *GetCurrentThreadStr(), *TextureData->DebugName, *Params.ParameterName.ToString(), ParamsConst.FrameData.FrameID);
			// 	return TouchTexture;
			// }

			const TouchObject<TEInstance>& Instance = Params.Instance;
			

			if (TEInstanceHasTextureTransfer(Params.Instance, TouchTexture))
			{
				// Here we can use a regular TEInstanceGetTextureTransfer because the contents of the texture can be discarded
				// as noted https://github.com/TouchDesigner/TouchEngine-Windows#vulkan
				Params.GetTextureTransferResult = TEInstanceGetTextureTransfer(Instance, TouchTexture, Params.GetTextureTransferSemaphore.take(), &Params.GetTextureTransferWaitValue); // request an ownership transfer from TE to UE, will be processed below
				// Params.GetTextureTransferResult = TEInstanceGetVulkanTextureTransfer(Instance, TouchTexture, &OldLayout, &NewLayout, Params.GetTextureTransferSemaphore.take(), &Params.GetTextureTransferWaitValue);
				if (Params.GetTextureTransferResult != TEResultSuccess && Params.GetTextureTransferResult != TEResultNoMatchingEntity) //TEResultNoMatchingEntity would be raised if there is no texture transfer waiting
				{
					UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[ExportTexture] TEInstanceGetTextureTransfer returned `%s` for parameter `%s` for frame %lld"), *TEResultToString(Params.GetTextureTransferResult), *Params.ParameterName.ToString(), ParamsConst.FrameData.FrameID);
					//todo: would we have wanted to call TEInstanceLinkSetTextureValue in this case?
					return nullptr;
				}

				if (TESemaphoreGetType(Params.GetTextureTransferSemaphore) == TESemaphoreTypeVulkan)
				{
					FString* Name = new FString(FString::Printf(TEXT("texture `%s` for param `%s` for frame `%lld`"), *TextureData->DebugName, *Params.ParameterName.ToString(), Params.FrameData.FrameID));
					TEVulkanSemaphoreSetCallback((TEVulkanSemaphore*) Params.GetTextureTransferSemaphore.get(), [](HANDLE semaphore, TEObjectEvent Event, void * TE_NULLABLE info)
					{
						FString* Name = (FString*) info;
						UE_LOG(LogTouchEngineVulkanRHI, Warning, TEXT("[TEInstanceGetTextureTransfer::Semaphore[%s]] Event `%s` for `%s`"), *GetCurrentThreadStr(), *TEObjectEventToString(Event), **Name)
						if (Event == TEObjectEventRelease)
						{
							delete Name;
						}
					}, Name);
				}
			}
		}
		// const bool bHasTextureTransfer = TEInstanceHasTextureTransfer_Debug(Params.Instance, TextureData->GetTouchRepresentation());

		if (!TextureData->CanFitTexture(SourceRHI) || !TextureData->CanFitTexture(ParamsConst))
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[FTouchTextureExporterVulkan::ExportTexture_AnyThread] !TextureData->CanFitTexture(SourceRHI) for `%s`"), *ParamsConst.ParameterName.ToString())
		}
		// 3. Add a texture transfer 
		if (!TextureData->SignalSemaphoreData.IsSet())
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Export Texture - 3. CreateSemaphore"), STAT_TEExportTextureSemaphore, STATGROUP_TouchEngine);
			TextureData->SignalSemaphoreData = CreateAndExportSemaphore(SecurityAttributes->Get(), TextureData->CurrentSemaphoreValue,
				FString::Printf(TEXT("%s [%lld]"), *Params.ParameterName.ToString(), Params.FrameData.FrameID));
			TextureData->LogCompletedValue(FString("After `CreateAndExportSemaphore`:"));
		}
		
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Export Texture - 4. AddTextureTransfer"), STAT_TEExportTextureAddTransfer, STATGROUP_TouchEngine);
			const TEResult TransferResult = TEInstanceAddTextureTransfer(Params.Instance, TouchTexture, TextureData->SignalSemaphoreData->TouchSemaphore, TextureData->CurrentSemaphoreValue + 1);

			UE_LOG(LogTouchEngineVulkanRHI, Log, TEXT("[ExportTexture] TEInstanceAddTextureTransfer `%s` returned `%s` for parameter `%s` for frame %lld"), *TextureData->DebugName, *TEResultToString(TransferResult), *Params.ParameterName.ToString(), Params.FrameData.FrameID);

			if (TransferResult != TEResultSuccess)
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[ExportTexture] TEInstanceAddTextureTransfer `%s` returned `%s` for parameter `%s` for frame %lld"), *TextureData->DebugName, *TEResultToString(TransferResult), *Params.ParameterName.ToString(), Params.FrameData.FrameID);
				return nullptr;
			}
			// const TEResult Result = TEInstanceAddTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation(),
			//                                                                    *TextureData->SignalSemaphoreData->TouchSemaphore, TextureData->CurrentSemaphoreValue + 1);
			// //todo: we should be able to call  TEInstanceAddTextureTransfer as we do not care about keeping the data
			// const TEResult Result = TEInstanceAddVulkanTextureTransfer(
			// 	Params.Instance,
			// 	TextureData->GetTouchRepresentation(),
			// 	OldLayout,
			// 	NewLayout,
			// 	TextureData->SignalSemaphoreData->TouchSemaphore,
			// 	TextureData->CurrentSemaphoreValue + 1);
		}

		//4. Enqueue the copy of the texture
		ENQUEUE_RENDER_COMMAND(AccessTexture)([StrongThis = SharedThis(this), Params = MoveTemp(Params), SecurityAttributes = SecurityAttributes, SharedTextureData = TextureData.ToSharedRef(), SourceRHI](FRHICommandListImmediate& RHICmdList) mutable
		{
			const bool bBecameInvalidSinceRenderEnqueue = !IsValid(Params.Texture);
			if (bBecameInvalidSinceRenderEnqueue)
			{
				return;
			}
			if (!SharedTextureData->CanFitTexture(SourceRHI) || !SharedTextureData->CanFitTexture(Params))
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[FTouchTextureExporterVulkan::ExportTexture_AnyThread->AccessTexture] !TextureData->CanFitTexture(SourceRHI)"))
			}
			ALLOC_COMMAND_CL(RHICmdList, FRHICommandCopyUnrealToTouch)(StrongThis, Params, SharedTextureData, SecurityAttributes, SourceRHI);
		});
		
		return TouchTexture;
	}
}
