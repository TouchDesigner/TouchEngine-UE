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

#include "TouchImportTextureVulkan.h"

#include "Logging.h"
#include "Util/TextureShareVulkanPlatformWindows.h"
#include "Util/VulkanGetterUtils.h"
#include "Util/VulkanRHIWrappers.h"
#include "Util/VulkanWindowsFunctions.h"
#include "VulkanTouchUtils.h"

#include "TouchEngine/TEVulkan.h"

namespace UE::TouchEngine::Vulkan
{
	namespace Private
	{
		struct FVulkanCopyOperationData
		{
			const FTouchCopyTextureArgs& CopyArgs;
			FVulkanPointers VulkanPointers;
			const FVulkanContext ContextInfo;

			/** Semaphores to signal when the copy queue is submitted */
			TArray<VkSemaphore> SignalSemaphores;

			FVulkanCopyOperationData(const FTouchCopyTextureArgs& CopyArgs)
				: CopyArgs(CopyArgs)
				, ContextInfo(CopyArgs.RHICmdList)
			{}
		};

		struct FTextureCreationResult
		{
			FTexture2DRHIRef Texture;
			/** Calls vkDestroyImage when reset. */
			const TSharedPtr<VkImage> ImageHandleOwnership;
			/** Calls vkFreeMemory when reset. */
			TSharedPtr<VkDeviceMemory> ImportedTextureMemoryOwnership;
		};
		
		static FTextureCreationResult CreateTextureRHI(const TouchObject<TEVulkanTexture_>& SharedTexture)
		{
			const VkFormat FormatVk = TEVulkanTextureGetFormat(SharedTexture);
			const EPixelFormat FormatUnreal = VulkanToUnrealTextureFormat(FormatVk);
			if (FormatUnreal == PF_Unknown)
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to map VkFormat %d"), FormatVk);
				return {};
			}
			
			const uint32 Width = TEVulkanTextureGetWidth(SharedTexture);
			const uint32 Height = TEVulkanTextureGetHeight(SharedTexture);
			const HANDLE Handle = TEVulkanTextureGetHandle(SharedTexture);
			const VkExternalMemoryHandleTypeFlagBits HandleTypeFlagBits = TEVulkanTextureGetHandleType(SharedTexture);
			const FVulkanPointers VulkanPointers;

			VkPhysicalDeviceMemoryBudgetPropertiesEXT MemoryBudget;
			VkPhysicalDeviceMemoryProperties2 MemoryProperties;
			ZeroVulkanStruct(MemoryBudget, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT);
			ZeroVulkanStruct(MemoryProperties, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2);
			MemoryProperties.pNext = &MemoryBudget;
			VulkanRHI::vkGetPhysicalDeviceMemoryProperties2(VulkanPointers.VulkanPhysicalDeviceHandle, &MemoryProperties);

			VkExternalMemoryImageCreateInfo externalMemoryImageInfo = { VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO };
			externalMemoryImageInfo.handleTypes = HandleTypeFlagBits;

			VkImage VulkanImageHandle; 
			VkImageCreateInfo imageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, &externalMemoryImageInfo };
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = FormatVk;
			imageCreateInfo.extent.width = Width;
			imageCreateInfo.extent.height = Height;
			imageCreateInfo.extent.depth = 1;
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			imageCreateInfo.flags = 0;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VERIFYVULKANRESULT(VulkanDynamicAPI::vkCreateImage(VulkanPointers.VulkanDeviceHandle, &imageCreateInfo, nullptr, &VulkanImageHandle));
			const TSharedPtr<VkImage> ImageOwnership = MakeShareable<VkImage>(new VkImage(VulkanImageHandle), [Device = VulkanPointers.VulkanDeviceHandle](VkImage* Memory)
			{
				VulkanRHI::vkDestroyImage(Device, *Memory, nullptr);
				delete Memory;
			});
			
			VkMemoryRequirements ImageMemoryRequirements = { };
	        VulkanDynamicAPI::vkGetImageMemoryRequirements(VulkanPointers.VulkanDeviceHandle, VulkanImageHandle, &ImageMemoryRequirements);
	        const uint32 MemoryTypeIndex = GetMemoryTypeIndex(MemoryProperties, ImageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	        if (MemoryTypeIndex >= MemoryProperties.memoryProperties.memoryTypeCount)
	        {
		        UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Memory doesn't support sharing"));
	            return {};
	        }

	        VkImportMemoryWin32HandleInfoKHR importMemInfo = { VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR };
	        importMemInfo.handleType = (VkExternalMemoryHandleTypeFlagBits)externalMemoryImageInfo.handleTypes;
	        importMemInfo.handle = Handle;

			VkPhysicalDeviceExternalImageFormatInfo externalImageFormatInfo = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO };
			externalImageFormatInfo.handleType = HandleTypeFlagBits;

			// Copied from https://developer.nvidia.com/vrworks > VkRender.cpp ~ line 745 > TODO: Is this needed?
			/*VkPhysicalDeviceImageFormatInfo2 ImageFormatInfo = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2, &externalImageFormatInfo };
			ImageFormatInfo.format  = FormatVk;
			ImageFormatInfo.type    = VK_IMAGE_TYPE_2D;
			ImageFormatInfo.tiling  = VK_IMAGE_TILING_OPTIMAL;
			ImageFormatInfo.usage   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			ImageFormatInfo.flags   = 0;
	        VkMemoryDedicatedAllocateInfo DedicatedAllocationMemoryAllocationInfo = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
			VkExternalImageFormatProperties ExternalImageFormatProperties = { VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES };
			VkImageFormatProperties2 ImageFormatProperties = { VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, &ExternalImageFormatProperties };
			VERIFYVULKANRESULT(vkGetPhysicalDeviceImageFormatProperties2(VulkanPhysicalDeviceHandle, &ImageFormatInfo, &ImageFormatProperties));
			bool bUseDedicatedMemory = ExternalImageFormatProperties.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT;
	        if (bUseDedicatedMemory)
	        {
	            DedicatedAllocationMemoryAllocationInfo.image = VulkanImageHandle;
	            importMemInfo.pNext = &DedicatedAllocationMemoryAllocationInfo;
	        }*/
			
			VkMemoryAllocateInfo memInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, &importMemInfo };
			memInfo.allocationSize = ImageMemoryRequirements.size;
			memInfo.memoryTypeIndex = MemoryTypeIndex;
			
			VkDeviceMemory VulkanTextureMemoryHandle;
			VERIFYVULKANRESULT(VulkanDynamicAPI::vkAllocateMemory(VulkanPointers.VulkanDeviceHandle, &memInfo, nullptr, &VulkanTextureMemoryHandle));
			const TSharedPtr<VkDeviceMemory> TextureMemoryOwnership = MakeShareable<VkDeviceMemory>(new VkDeviceMemory(VulkanTextureMemoryHandle), [Device = VulkanPointers.VulkanDeviceHandle](VkDeviceMemory* Memory)
			{
				VulkanRHI::vkFreeMemory(Device, *Memory, nullptr);
				delete Memory;
			});
	        VERIFYVULKANRESULT(VulkanDynamicAPI::vkBindImageMemory(VulkanPointers.VulkanDeviceHandle, VulkanImageHandle, VulkanTextureMemoryHandle, 0));
			
			constexpr uint32 NumMips = 0;
			constexpr uint32 NumSamples = 0;
			const ETextureCreateFlags TextureFlags = TexCreate_Shared | (IsSRGB(FormatVk) ? TexCreate_SRGB : TexCreate_None);
			return { VulkanPointers.DynamicRHI->RHICreateTexture2DFromResource(FormatUnreal, Width, Height, NumMips, NumSamples, VulkanImageHandle, TextureFlags), ImageOwnership, TextureMemoryOwnership };
		}
	}
	
	TSharedPtr<FTouchImportTextureVulkan> FTouchImportTextureVulkan::CreateTexture(const TouchObject<TEVulkanTexture_>& Shared, TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes)
	{
		if (!AreVulkanFunctionsForWindowsLoaded())
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to import because Vulkan Windows functions are not loaded"));
			return nullptr;
		}
		
		// Fail early if the pixel format is not known since we'll do some more construction on the render thread later
		const VkFormat FormatVk = TEVulkanTextureGetFormat(Shared);
		const EPixelFormat FormatUnreal = VulkanToUnrealTextureFormat(FormatVk);
		if (FormatUnreal == PF_Unknown)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to import because VkFormat %d could not be mapped"), FormatVk);
			return nullptr;
		}

		Private::FTextureCreationResult TextureCreationResult = Private::CreateTextureRHI(Shared);
		return MakeShared<FTouchImportTextureVulkan>(TextureCreationResult.Texture, TextureCreationResult.ImageHandleOwnership, TextureCreationResult.ImportedTextureMemoryOwnership, Shared, MoveTemp(SecurityAttributes));
	}

	FTouchImportTextureVulkan::FTouchImportTextureVulkan(FTexture2DRHIRef TextureRHI, TSharedPtr<VkImage> ImageHandle, TSharedPtr<VkDeviceMemory> ImportedTextureMemoryOwnership, TouchObject<TEVulkanTexture_> SharedTexture, TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes)
		: SourceTextureRHI(MoveTemp(TextureRHI))
		, ImageHandle(MoveTemp(ImageHandle))
		, ImportedTextureMemoryOwnership(MoveTemp(ImportedTextureMemoryOwnership))
		, SharedTexture(MoveTemp(SharedTexture))
		, SecurityAttributes(MoveTemp(SecurityAttributes))
	{}

	FTouchImportTextureVulkan::~FTouchImportTextureVulkan()
	{
		if (WaitSemaphoreData.IsSet())
		{
			TEVulkanSemaphoreSetCallback(WaitSemaphoreData->TouchSemaphore, nullptr, nullptr);
		}
	}

	FTextureMetaData FTouchImportTextureVulkan::GetTextureMetaData() const
	{
		const uint32 Width = TEVulkanTextureGetWidth(SharedTexture);
		const uint32 Height = TEVulkanTextureGetHeight(SharedTexture);
		const VkFormat FormatVk = TEVulkanTextureGetFormat(SharedTexture);
		const EPixelFormat FormatUnreal = VulkanToUnrealTextureFormat(FormatVk);
		return { Width, Height, FormatUnreal };
	}

	bool FTouchImportTextureVulkan::CopyNativeToUnreal_RenderThread(const FTouchCopyTextureArgs& CopyArgs)
	{
		const TouchObject<TEInstance> Instance = CopyArgs.RequestParams.Instance;
		const TouchObject<TETexture> TextureToCopy = CopyArgs.RequestParams.Texture;
		check(SharedTexture == CopyArgs.RequestParams.Texture);
		
		if (TextureToCopy && TEInstanceHasVulkanTextureTransfer(Instance, TextureToCopy))
		{
			VkImageLayout AcquireOldLayout;
			VkImageLayout AcquireNewLayout; // Will be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL because we called TEInstanceSetVulkanAcquireImageLayout with it
			
			TouchObject<TESemaphore> Semaphore;
			uint64 WaitValue;
			const TEResult ResultCode = TEInstanceGetVulkanTextureTransfer(Instance, TextureToCopy, &AcquireOldLayout, &AcquireNewLayout, Semaphore.take(), &WaitValue);
			if (ResultCode != TEResultSuccess)
			{
				return false;
			}
			ensureMsgf(AcquireOldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXT("Vulkan RHI's CopyTexture requires VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL format. Check that we called TEInstanceSetVulkanAcquireImageLayout correctly."));

			Private::FVulkanCopyOperationData CopyOperationData(CopyArgs);
			const bool bSuccess = AcquireMutex(CopyOperationData, Semaphore, WaitValue, AcquireOldLayout, AcquireNewLayout);
			if (bSuccess)
			{
				CopyTexture(CopyOperationData);
				ReleaseMutex(CopyOperationData);
				// TODO: What happens if we receive a OnWaitVulkanSemaphoreUsageChanged or OnSignalVulkanSemaphoreUsageChanged event here? Maybe yield the thread and then send it?
				CopyOperationData.ContextInfo.BufferManager.SubmitUploadCmdBuffer(CopyOperationData.SignalSemaphores.Num(), CopyOperationData.SignalSemaphores.GetData());
				return true;
			}
		}

		return false;
	}

	bool FTouchImportTextureVulkan::AcquireMutex(Private::FVulkanCopyOperationData& CopyOperationData, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue, VkImageLayout AcquireOldLayout, VkImageLayout AcquireNewLayout)
	{
		using namespace Private;
		
		TEVulkanSemaphore* SemaphoreTE = static_cast<TEVulkanSemaphore*>(Semaphore.get());
		const VkSemaphoreType SemaphoreType = TEVulkanSemaphoreGetType(SemaphoreTE);
		const bool bIsTimeline = SemaphoreType == VK_SEMAPHORE_TYPE_TIMELINE_KHR || SemaphoreType == VK_SEMAPHORE_TYPE_TIMELINE; 
		if (!bIsTimeline)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Received a binary semaphore but only timeline semaphores are implemented."))
			return false;
		}

		AllocateWaitSemaphore(CopyOperationData, SemaphoreTE);
		if (!WaitSemaphoreData.IsSet())
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Failed to copy Vulkan semaphore."))
			return false;
		}
		
		// Not sure about VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		CopyOperationData.ContextInfo.UploadBuffer.AddWaitSemaphore(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, &WaitSemaphoreData->UnrealSemaphore);

		VkImageMemoryBarrier ImageBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		ImageBarrier.pNext = nullptr;
		ImageBarrier.srcAccessMask = GetVkStageFlagsForLayout(AcquireOldLayout);
		ImageBarrier.dstAccessMask = GetVkStageFlagsForLayout(AcquireNewLayout);
		ImageBarrier.oldLayout = AcquireOldLayout;
		ImageBarrier.newLayout = AcquireNewLayout;
		ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.image = *ImageHandle.Get();
		
		VulkanRHI::vkCmdPipelineBarrier(
			CopyOperationData.ContextInfo.UploadBuffer.GetHandle(),
			GetVkStageFlagsForLayout(AcquireOldLayout),
			GetVkStageFlagsForLayout(AcquireNewLayout),
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&ImageBarrier
		);
		
		return true;
	}

	bool FTouchImportTextureVulkan::AllocateWaitSemaphore(Private::FVulkanCopyOperationData& CopyOperationData, TEVulkanSemaphore* SemaphoreTE)
	{
		const HANDLE SharedHandle = TEVulkanSemaphoreGetHandle(SemaphoreTE);
		if (!WaitSemaphoreData.IsSet() || WaitSemaphoreData->Handle != SharedHandle)
		{
			const VkExternalSemaphoreHandleTypeFlagBits SemaphoreHandleType = TEVulkanSemaphoreGetHandleType(SemaphoreTE);
			// Other types not allowed for vkImportSemaphoreWin32HandleKHR
			if (SemaphoreHandleType == VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT || SemaphoreHandleType == VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT)
			{
				UE_LOG(LogVulkanRHI, Error, TEXT("Unexpected VkExternalSemaphoreHandleTypeFlagBits for Vulkan semaphore exported from Touch Designer (VkExternalSemaphoreHandleTypeFlagBits = %s)"), static_cast<int32>(SemaphoreHandleType))
				return false;
			}
			
			VkSemaphoreCreateInfo SemCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
			VkSemaphore VulkanSemaphore;
			VERIFYVULKANRESULT(VulkanRHI::vkCreateSemaphore(CopyOperationData.VulkanPointers.VulkanDeviceHandle, &SemCreateInfo, NULL, &VulkanSemaphore));

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

			VERIFYVULKANRESULT(vkImportSemaphoreWin32HandleKHR(CopyOperationData.VulkanPointers.VulkanDeviceHandle, &ImportSemWin32Info));
			TEVulkanSemaphoreSetCallback(SemaphoreTE, &OnWaitVulkanSemaphoreUsageChanged, this);

			VulkanRHI::FSemaphore UnrealSemaphore(*CopyOperationData.VulkanPointers.VulkanDevice, *SharedVulkanSemaphore.Get());
			WaitSemaphoreData.Emplace(SharedHandle, TouchSemaphore, SharedVulkanSemaphore, UnrealSemaphore);
		}

		return true;
	}

	void FTouchImportTextureVulkan::CopyTexture(const Private::FVulkanCopyOperationData& ContextInfo)
	{
		const FTexture2DRHIRef TargetTexture = ContextInfo.CopyArgs.Target->GetResource()->TextureRHI->GetTexture2D();
		EnqueueCopyTextureOnUploadQueue(ContextInfo.ContextInfo.VulkanContext, SourceTextureRHI, TargetTexture);
	}

	void FTouchImportTextureVulkan::ReleaseMutex(Private::FVulkanCopyOperationData& CopyOperationData)
	{
		if (!SignalSemaphoreData.IsSet())
		{
			FSignalSemaphoreData NewSemaphoreData;

			VkExportSemaphoreCreateInfo ExportSemInfo = { VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO };
			ExportSemInfo.handleTypes = bUseNTHandles ? VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT : VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
			VkExportSemaphoreWin32HandleInfoKHR ExportSemWin32Info = { VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR };
			if (bUseNTHandles)
			{
				ExportSemWin32Info.dwAccess = STANDARD_RIGHTS_ALL | SEMAPHORE_MODIFY_STATE;
				//ExportSemWin32Info.pAttributes = SecurityAttributes->Get();
				ExportSemInfo.pNext = &ExportSemWin32Info;
			}

			VkSemaphoreTypeCreateInfo SemaphoreTypeCreateInfo { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, &ExportSemInfo };
			SemaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
			SemaphoreTypeCreateInfo.initialValue = CurrentSemaphoreValue;
			
			VkSemaphoreCreateInfo SemCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &SemaphoreTypeCreateInfo };
			VkSemaphore VulkanSemaphore;
			VERIFYVULKANRESULT(VulkanRHI::vkCreateSemaphore(CopyOperationData.VulkanPointers.VulkanDeviceHandle, &SemCreateInfo, NULL, &VulkanSemaphore));
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
			VERIFYVULKANRESULT(vkGetSemaphoreWin32HandleKHR(CopyOperationData.VulkanPointers.VulkanDeviceHandle, &semaphoreGetWin32HandleInfo, &NewSemaphoreData.ExportedHandle));

			TEVulkanSemaphore* TouchSemaphore = TEVulkanSemaphoreCreate(SemaphoreTypeCreateInfo.semaphoreType, NewSemaphoreData.ExportedHandle, (VkExternalSemaphoreHandleTypeFlagBits)ExportSemInfo.handleTypes, nullptr, nullptr);
			NewSemaphoreData.TouchSemaphore.set(TouchSemaphore);

			SignalSemaphoreData = NewSemaphoreData;
		}
		
		// SignalSemaphoreData will be passed to SubmitUploadCmdBuffer
		CopyOperationData.SignalSemaphores.Add(*SignalSemaphoreData->VulkanSemaphore.Get());
		++CurrentSemaphoreValue;
		TEInstanceAddTextureTransfer(CopyOperationData.CopyArgs.RequestParams.Instance, CopyOperationData.CopyArgs.RequestParams.Texture, SignalSemaphoreData->TouchSemaphore, CurrentSemaphoreValue);
	}

	void FTouchImportTextureVulkan::OnWaitVulkanSemaphoreUsageChanged(void* semaphore, TEObjectEvent event, void* info)
	{
		// TODO: Time to release the vulkan shared ptr
	}
}
