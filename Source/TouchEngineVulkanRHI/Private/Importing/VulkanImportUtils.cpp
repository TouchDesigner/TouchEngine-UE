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

#include "VulkanImportUtils.h"

// Include order matters
THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END

#include "Logging.h"
#include "Util/TextureShareVulkanPlatformWindows.h"
#include "Util/VulkanGetterUtils.h"
#include "Util/VulkanWindowsFunctions.h"
#include "VulkanTouchUtils.h"

#if PLATFORM_WINDOWS
#include "WindowsVulkanPlatformDefines.h"
#endif
#include "VulkanContext.h"

#include "Engine/Texture.h"

#include "TEVulkanInclude.h"

namespace UE::TouchEngine::Vulkan
{
	FTextureCreationResult CreateSharedTouchVulkanTexture(const TouchObject<TEVulkanTexture_>& SharedTexture)
	{
		const VkFormat FormatVk = TEVulkanTextureGetFormat(SharedTexture);
		bool bIsSRGB;
		const EPixelFormat FormatUnreal = VulkanToUnrealTextureFormat(FormatVk, bIsSRGB);
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

		VkPhysicalDeviceMemoryBudgetPropertiesEXT MemoryBudget { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT };
		VkPhysicalDeviceMemoryProperties2 MemoryProperties { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2, &MemoryBudget };
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
		const TSharedPtr<VkImage> ImageOwnership = MakeShareable<VkImage>(new VkImage(VulkanImageHandle), [Device = VulkanPointers.VulkanDeviceHandle](const VkImage* Memory)
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
        importMemInfo.handleType = static_cast<VkExternalMemoryHandleTypeFlagBits>(externalMemoryImageInfo.handleTypes);
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
		const TSharedPtr<VkDeviceMemory> TextureMemoryOwnership = MakeShareable<VkDeviceMemory>(new VkDeviceMemory(VulkanTextureMemoryHandle),
			[Device = VulkanPointers.VulkanDeviceHandle](const VkDeviceMemory* Memory)
		{
			VulkanRHI::vkFreeMemory(Device, *Memory, nullptr);
			delete Memory;
		});
        VERIFYVULKANRESULT(VulkanDynamicAPI::vkBindImageMemory(VulkanPointers.VulkanDeviceHandle, VulkanImageHandle, VulkanTextureMemoryHandle, 0));
		
		return { ImageOwnership, TextureMemoryOwnership };
	}

	TSharedPtr<VkCommandBuffer> CreateCommandBuffer(FRHICommandListBase& RHICmdList)
	{
		const FVulkanPointers VulkanPointers;
		VkDevice Device = VulkanPointers.VulkanDeviceHandle;
		FVulkanCommandListContext& CommandList = static_cast<FVulkanCommandListContext&>(RHICmdList.GetContext());
		VkCommandPool Pool = CommandList.GetHandle();
		
		VkCommandBufferAllocateInfo CreateCmdBufInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		CreateCmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		CreateCmdBufInfo.commandBufferCount = 1;
		CreateCmdBufInfo.commandPool = Pool;

		VkCommandBuffer CommandBuffer;
		VERIFYVULKANRESULT(VulkanRHI::vkAllocateCommandBuffers(Device, &CreateCmdBufInfo, &CommandBuffer));
		return MakeShareable(new VkCommandBuffer(CommandBuffer), [Device, Pool](const VkCommandBuffer* CommandBuffer)
		{
			VulkanRHI::vkFreeCommandBuffers(Device, Pool, 1, CommandBuffer);
			delete CommandBuffer;
		});
	}
}