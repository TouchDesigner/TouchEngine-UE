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

#include "VulkanTouchUtils.h"

#if PLATFORM_WINDOWS
#include "VulkanPlatformDefines.h"
#endif
#include "VulkanViewport.h"
#include "IVulkanDynamicRHI.h"


namespace UE::TouchEngine::Vulkan
{
	EPixelFormat VulkanToUnrealTextureFormat(VkFormat Format)
	{
		for (int32 i = 0; i < PF_MAX; ++i)
		{
			if (GPixelFormats[i].PlatformFormat == Format)
			{
				return static_cast<EPixelFormat>(i);
			}
		}

		return PF_Unknown;
	}

	VkFormat UnrealToVulkanTextureFormat(EPixelFormat Format, const bool bSRGB)
	{
		// since 5.2, there is no easy way to call UEToVkTextureFormat as it creates a linker error (GVulkanSRGBFormat is only declared in VulkanDevice.cpp)
		// so the code below is inspired by UEToVkTextureFormat
		if (bSRGB)
		{
			// FVulkanDynamicRHI* DynamicRHI = static_cast<FVulkanDynamicRHI*>(GDynamicRHI);
			IVulkanDynamicRHI* DynamicRHI = static_cast<IVulkanDynamicRHI*>(GDynamicRHI);
			// return (VkFormat)GPixelFormats[Format].PlatformFormat; //todo:
			return DynamicRHI->RHIGetSwapChainVkFormat(Format); // this calls UEToVkTextureFormat with bSRGB = true
			// return UEToVkTextureFormat(Format, bSRGB); // this calls UEToVkTextureFormat with bSRGB = true
		}
		else
		{
			return (VkFormat)GPixelFormats[Format].PlatformFormat; // this is what UEToVkTextureFormat returns with bSRGB = false
		}
	}
	
	bool IsSRGB(VkFormat Format)
	{
		switch (Format)
		{
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
		case VK_FORMAT_R8_SRGB:
		case VK_FORMAT_R8G8_SRGB:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		case VK_FORMAT_BC2_SRGB_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
		case VK_FORMAT_BC7_SRGB_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
		case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
		case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
		case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
		case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
		case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
		case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
		case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
			return true;
		default:
			return false;
		}
	}
}