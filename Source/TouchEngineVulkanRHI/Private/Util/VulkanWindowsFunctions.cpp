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

#include <vulkan_core.h>

#include "VulkanRHIBridge.h"

#if PLATFORM_WINDOWS

namespace UE::TouchEngine::Vulkan
{
	PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR;
	PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR;
	PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR;

	bool IsVulcanSelected()
	{
#if PLATFORM_WINDOWS
		const TCHAR* DynamicRHIModuleName = GetSelectedDynamicRHIModuleName(false);
#elif PLATFORM_LINUX
		const TCHAR* DynamicRHIModuleName = TEXT("VulkanRHI");
#endif
				
		return FString("VulkanRHI") == FString(DynamicRHIModuleName);
	}

	void ConditionallySetupVulkanExtensions()
	{
		if (IsVulcanSelected())
		{
			const TArray<const ANSICHAR*> ExtentionsToAdd{ VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME };
			VulkanRHIBridge::AddEnabledDeviceExtensionsAndLayers(ExtentionsToAdd, TArray<const ANSICHAR*>());
		}
	}
	
	void ConditionallyLoadVulkanFunctionsForWindows()
	{
		if (IsVulcanSelected())
		{
			const FVulkanPointers Pointers;
			check(Pointers.VulkanDeviceHandle);

#pragma warning(push)
#pragma warning(disable : 4191)
			vkImportSemaphoreWin32HandleKHR = reinterpret_cast<PFN_vkImportSemaphoreWin32HandleKHR>(VulkanRHI::vkGetDeviceProcAddr(Pointers.VulkanDeviceHandle, "vkImportSemaphoreWin32HandleKHR"));
			UE_CLOG(vkImportSemaphoreWin32HandleKHR == nullptr, LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Proc address for \"vkImportSemaphoreWin32HandleKHR\" not found (GetLastError(): %d)."), GetLastError());
			ensure(vkImportSemaphoreWin32HandleKHR);
			
			vkGetSemaphoreWin32HandleKHR = reinterpret_cast<PFN_vkGetSemaphoreWin32HandleKHR>(VulkanRHI::vkGetDeviceProcAddr(Pointers.VulkanDeviceHandle, "vkGetSemaphoreWin32HandleKHR"));
			UE_CLOG(vkGetSemaphoreWin32HandleKHR == nullptr, LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Proc address for \"vkGetSemaphoreWin32HandleKHR\" not found (GetLastError(): %d)."), GetLastError());
			ensure(vkGetSemaphoreWin32HandleKHR);

			vkGetMemoryWin32HandleKHR = reinterpret_cast<PFN_vkGetMemoryWin32HandleKHR>(VulkanRHI::vkGetDeviceProcAddr(Pointers.VulkanDeviceHandle, "vkGetMemoryWin32HandleKHR"));
			UE_CLOG(vkGetMemoryWin32HandleKHR == nullptr, LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Proc address for \"vkGetMemoryWin32HandleKHR\" not found (GetLastError(): %d)."), GetLastError());
			ensure(vkGetMemoryWin32HandleKHR);
#pragma warning(pop) 
		}
	}
	
	bool AreVulkanFunctionsForWindowsLoaded()
	{
		return vkImportSemaphoreWin32HandleKHR && vkGetSemaphoreWin32HandleKHR;
    }
}
#endif