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

#include "RhiIncludeHelper.h"
#include "vulkan_core.h"
#include "VulkanRHIPrivate.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/MinimalWindowsApi.h"
#include "vulkan/vulkan_win32.h"
#include "Windows/HideWindowsPlatformTypes.h"

namespace UE::TouchEngine::Vulkan
{
	extern PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR;
	extern PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR;
	extern PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR;

	bool IsVulkanSelected();
	void ConditionallySetupVulkanExtensions();

	void ConditionallyLoadVulkanFunctionsForWindows();
	bool AreVulkanFunctionsForWindowsLoaded();
}

#endif

