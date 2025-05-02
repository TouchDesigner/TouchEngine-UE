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

#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END
#if PLATFORM_WINDOWS
#include "WindowsVulkanPlatformDefines.h"
#endif
#include "VulkanViewport.h"
#include "VulkanDynamicRHI.h"
#include "VulkanRHIPrivate.h"
#include "VulkanDevice.h"


//this was removed from Engine\Source\Runtime\VulkanRHI\Public\VulkanConfiguration.h in UE 5.2
#ifndef VULKAN_SUPPORTS_SEPARATE_DEPTH_STENCIL_LAYOUTS
	#ifdef VK_KHR_separate_depth_stencil_layouts
		#define VULKAN_SUPPORTS_SEPARATE_DEPTH_STENCIL_LAYOUTS	1
	#else
		#define VULKAN_SUPPORTS_SEPARATE_DEPTH_STENCIL_LAYOUTS	0
	#endif
#endif
//this was removed from Engine\Source\Runtime\VulkanRHI\Public\VulkanConfiguration.h in UE 5.3
#ifndef VULKAN_SUPPORTS_FRAGMENT_DENSITY_MAP
	#ifdef VK_EXT_fragment_density_map
		#define VULKAN_SUPPORTS_FRAGMENT_DENSITY_MAP			1
	#else
		#define VULKAN_SUPPORTS_FRAGMENT_DENSITY_MAP			0
	#endif
#endif
#ifndef VULKAN_SUPPORTS_FRAGMENT_SHADING_RATE
	#ifdef VK_KHR_fragment_shading_rate
		#define VULKAN_SUPPORTS_FRAGMENT_SHADING_RATE 1
	#else
		#define VULKAN_SUPPORTS_FRAGMENT_SHADING_RATE 0
	#endif
#endif

namespace UE::TouchEngine::Vulkan
{
	struct FVulkanPointers
	{
		FVulkanDynamicRHI* DynamicRHI = static_cast<FVulkanDynamicRHI*>(GDynamicRHI);
		FVulkanDevice* VulkanDevice = DynamicRHI->GetDevice();
		const VkPhysicalDevice VulkanPhysicalDeviceHandle = VulkanDevice->GetPhysicalHandle();
		const VkDevice VulkanDeviceHandle = VulkanDevice->GetInstanceHandle();
	};

	// struct FVulkanContext
	// {
	// 	FVulkanCommandListContext& VulkanContext;
	// 	FVulkanCommandBufferManager* BufferManager;
	// 	FVulkanCmdBuffer* UploadBuffer;
	// 		
	// 	FVulkanContext(FRHICommandListBase& List)
	// 		: VulkanContext(static_cast<FVulkanCommandListContext&>(List.GetContext()))
	// 		, BufferManager(VulkanContext.GetCommandBufferManager())
	// 		, UploadBuffer(BufferManager->GetUploadCmdBuffer())
	// 	{
	// 		check(UploadBuffer);
	// 	}
	// };

	inline uint32 GetMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties2& MemoryProperties, uint32 MemoryTypeBits, VkFlags RequirementsMask) 
	{
		for (uint32 i = 0; i < VK_MAX_MEMORY_TYPES; i++)
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

	// Copied from Unreal Vulkan RHI
	static VkAccessFlags GetVkAccessMaskForLayout(const VkImageLayout Layout)
	{
		VkAccessFlags Flags = 0;

		switch (Layout)
		{
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				Flags = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				Flags = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				Flags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
				Flags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
				Flags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				Flags = VK_ACCESS_SHADER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
			case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
				Flags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
				Flags = 0;
				break;

			case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
				Flags = VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
				break;

			case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
				Flags = VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
				break;

			case VK_IMAGE_LAYOUT_GENERAL:
				// todo-jn: could be used for R64 in read layout
			case VK_IMAGE_LAYOUT_UNDEFINED:
				Flags = 0;
				break;

			case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL:
				// todo-jn: sync2 currently only used by depth/stencil targets
				Flags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:
				// todo-jn: sync2 currently only used by depth/stencil targets
				Flags = VK_ACCESS_SHADER_READ_BIT;
				break;

			default:
				checkNoEntry();
				break;
		}

		return Flags;
	}
	
	static VkPipelineStageFlags GetVkStageFlagsForLayout(VkImageLayout Layout)
	{
		VkPipelineStageFlags Flags = 0;

		switch (Layout)
		{
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				Flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				Flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				Flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
	#if VULKAN_SUPPORTS_SEPARATE_DEPTH_STENCIL_LAYOUTS
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR:
			case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR:
	#endif
				Flags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				Flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
	#if VULKAN_SUPPORTS_MAINTENANCE_LAYER2
			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR:
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR:
	#endif
	#if VULKAN_SUPPORTS_SEPARATE_DEPTH_STENCIL_LAYOUTS
			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR:
			case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR:
	#endif
				Flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				break;

			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
				Flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				break;

	#if VULKAN_SUPPORTS_FRAGMENT_DENSITY_MAP
			case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
				Flags = VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT;
				break;
	#endif

	#if VULKAN_SUPPORTS_FRAGMENT_SHADING_RATE
			case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
				Flags = VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
				break;
	#endif
				
			case VK_IMAGE_LAYOUT_GENERAL:
			case VK_IMAGE_LAYOUT_UNDEFINED:
				Flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				break;

			default:
				checkNoEntry();
				break;
		}

		return Flags;
	}
}