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
#include "VulkanTouchUtils.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/MinimalWindowsApi.h"
#include "vulkan/vulkan_win32.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "VulkanRHIPrivate.h"
/*#include "VulkanViewport.h"
#include "VulkanDevice.h"*/
#include "VulkanUtil.h"
#include "VulkanDynamicRHI.h"

#include "TouchEngine/TEVulkan.h"

/*#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
THIRD_PARTY_INCLUDES_START
#include "Windows/MinimalWindowsApi.h"
THIRD_PARTY_INCLUDES_END
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"*/

namespace UE::TouchEngine::Vulkan
{
	namespace Private
	{
		static uint32_t GetMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties2& MemoryProperties, uint32 MemoryTypeBits, VkFlags RequirementsMask) 
		{
			for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
			{
				if ((MemoryTypeBits & 1) == 1)
				{
					if ((MemoryProperties.memoryProperties.memoryTypes[i].propertyFlags & RequirementsMask) == RequirementsMask)
					{
						return i;
					}
				}
				MemoryTypeBits >>= 1;
			}

			return VK_MAX_MEMORY_TYPES;
		}
		
		static FTexture2DRHIRef CreateTextureRHI(const TouchObject<TEVulkanTexture_>& SharedTexture)
		{
			const HANDLE Handle = TEVulkanTextureGetHandle(SharedTexture);
			const VkExternalMemoryHandleTypeFlagBits HandleTypeFlagBits = TEVulkanTextureGetHandleType(SharedTexture);
			const VkFormat FormatVk = TEVulkanTextureGetFormat(SharedTexture);

			const uint32 Width = TEVulkanTextureGetWidth(SharedTexture);
			const uint32 Height = TEVulkanTextureGetHeight(SharedTexture);
			const EPixelFormat FormatUnreal = VulkanToUnrealTextureFormat(FormatVk);
			if (FormatUnreal == PF_Unknown)
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to map VkFormat %d"), FormatVk);
				return nullptr;
			}
			
			FVulkanDynamicRHI* DynamicRHI = static_cast<FVulkanDynamicRHI*>(GDynamicRHI);
			FVulkanDevice* VulkanDevice = DynamicRHI->GetDevice();
			const VkPhysicalDevice VulkanPhysicalDeviceHandle = VulkanDevice->GetPhysicalHandle();
			const VkDevice VulkanDeviceHandle = VulkanDevice->GetInstanceHandle();

			VkPhysicalDeviceMemoryBudgetPropertiesEXT MemoryBudget;
			VkPhysicalDeviceMemoryProperties2 MemoryProperties;
			ZeroVulkanStruct(MemoryBudget, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT);
			ZeroVulkanStruct(MemoryProperties, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2);
			MemoryProperties.pNext = &MemoryBudget;
			VulkanRHI::vkGetPhysicalDeviceMemoryProperties2(VulkanPhysicalDeviceHandle, &MemoryProperties);

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
			VERIFYVULKANRESULT(VulkanDynamicAPI::vkCreateImage(VulkanDeviceHandle, &imageCreateInfo, nullptr, &VulkanImageHandle));
			
			VkMemoryRequirements ImageMemoryRequirements = { };
	        VulkanDynamicAPI::vkGetImageMemoryRequirements(VulkanDeviceHandle, VulkanImageHandle, &ImageMemoryRequirements);
	        const uint32 MemoryTypeIndex = GetMemoryTypeIndex(MemoryProperties, ImageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	        if (MemoryTypeIndex >= MemoryProperties.memoryProperties.memoryTypeCount)
	        {
		        UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Memory doesn't support sharing"));
	            return nullptr;
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
			VERIFYVULKANRESULT(VulkanDynamicAPI::vkAllocateMemory(VulkanDeviceHandle, &memInfo, nullptr, &VulkanTextureMemoryHandle));
	        VERIFYVULKANRESULT(VulkanDynamicAPI::vkBindImageMemory(VulkanDeviceHandle, VulkanImageHandle, VulkanTextureMemoryHandle, 0));
			
			constexpr uint32 NumMips = 0;
			constexpr uint32 NumSamples = 0;
			const ETextureCreateFlags TextureFlags = TexCreate_Shared | (IsSRGB(FormatVk) ? TexCreate_SRGB : TexCreate_None);
			return DynamicRHI->RHICreateTexture2DFromResource(FormatUnreal, Width, Height, NumMips, NumSamples, VulkanImageHandle, TextureFlags);
		}
	}
	
	TSharedPtr<FTouchImportTextureVulkan> FTouchImportTextureVulkan::CreateTexture(const TouchObject<TEVulkanTexture_>& Shared)
	{
		// Fail early if the pixel format is not known since we'll do some more construction on the render thread later
		const VkFormat FormatVk = TEVulkanTextureGetFormat(Shared);
		const EPixelFormat FormatUnreal = VulkanToUnrealTextureFormat(FormatVk);
		if (FormatUnreal == PF_Unknown)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to map VkFormat %d"), FormatVk);
			return nullptr;
		}

		FTexture2DRHIRef Texture = Private::CreateTextureRHI(Shared);
		return MakeShared<FTouchImportTextureVulkan>(Texture, Shared);
	}

	FTouchImportTextureVulkan::FTouchImportTextureVulkan(FTexture2DRHIRef TextureRHI, TouchObject<TEVulkanTexture_> SharedTexture)
		: TextureRHI(MoveTemp(TextureRHI))
		, SharedTexture(MoveTemp(SharedTexture))
	{}

	FTextureMetaData FTouchImportTextureVulkan::GetTextureMetaData() const
	{
		const uint32 Width = TEVulkanTextureGetWidth(SharedTexture);
		const uint32 Height = TEVulkanTextureGetHeight(SharedTexture);
		const VkFormat FormatVk = TEVulkanTextureGetFormat(SharedTexture);
		const EPixelFormat FormatUnreal = VulkanToUnrealTextureFormat(FormatVk);
		return { Width, Height, FormatUnreal };
	}

	bool FTouchImportTextureVulkan::AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
		// TODO Vulkan
		return true;
	}

	void FTouchImportTextureVulkan::ReleaseMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
		// TODO Vulkan
	}
}
