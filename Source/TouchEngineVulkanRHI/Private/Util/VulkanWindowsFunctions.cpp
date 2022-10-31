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

#include "VulkanWindowsFunctions.h"

#include "Logging.h"
#include "VulkanGetterUtils.h"

#if PLATFORM_WINDOWS

namespace UE::TouchEngine::Vulkan
{
	PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR;
	PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR;

	bool CanLoadVulkan()
	{
		return FString(GDynamicRHI->GetName()).Equals(TEXT("Vulkan"));
	}
	
	void ConditionallyLoadVulkanFunctionsForWindows()
	{
		if (CanLoadVulkan())
		{
			const FVulkanPointers Pointers;

			// TODO: Fix compiler warnings
			vkImportSemaphoreWin32HandleKHR = reinterpret_cast<PFN_vkImportSemaphoreWin32HandleKHR>(VulkanRHI::vkGetDeviceProcAddr(Pointers.VulkanDeviceHandle, "vkImportSemaphoreWin32HandleKHR"));
			UE_CLOG(vkImportSemaphoreWin32HandleKHR == nullptr, LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Proc address for \"vkImportSemaphoreWin32HandleKHR\" not found."));

			vkGetSemaphoreWin32HandleKHR = reinterpret_cast<PFN_vkGetSemaphoreWin32HandleKHR>(VulkanRHI::vkGetDeviceProcAddr(Pointers.VulkanDeviceHandle, "vkGetSemaphoreWin32HandleKHR"));
			UE_CLOG(vkGetSemaphoreWin32HandleKHR == nullptr, LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Proc address for \"vkGetSemaphoreWin32HandleKHR\" not found."));
		}
	}
	
	bool AreVulkanFunctionsForWindowsLoaded()
	{
		return vkImportSemaphoreWin32HandleKHR && vkGetSemaphoreWin32HandleKHR;
    }
}
#endif