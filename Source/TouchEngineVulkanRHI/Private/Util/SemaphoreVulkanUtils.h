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

#if PLATFORM_WINDOWS

#include "CoreMinimal.h"
#include "vulkan_core.h"
#include "VulkanRHIPrivate.h"
#include "TouchEngine/TEVulkan.h"
#include "TouchEngine/TouchObject.h"

namespace UE::TouchEngine::Vulkan
{
	struct FTouchVulkanSemaphoreImport
	{
		HANDLE Handle;
		TouchObject<TEVulkanSemaphore> TouchSemaphore;
		TSharedPtr<VkSemaphore> VulkanSemaphore;
	};
	
	TOptional<FTouchVulkanSemaphoreImport> ImportTouchSemaphore(TouchObject<TEVulkanSemaphore> SemaphoreTE, TEVulkanSemaphoreCallback Callback, void* Info);
	
	struct FTouchVulkanSemaphoreExport
	{
		HANDLE ExportedHandle = nullptr;
		TouchObject<TEVulkanSemaphore> TouchSemaphore;
		TSharedPtr<VkSemaphore> VulkanSemaphore;
	};
	FTouchVulkanSemaphoreExport CreateAndExportSemaphore(const SECURITY_ATTRIBUTES* SecurityAttributes, uint64 InitialSemaphoreValue);
};

#endif