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
#include "Logging.h"

#if PLATFORM_WINDOWS

#include "CoreMinimal.h"
THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END

#include "TEVulkanInclude.h"
#include "TouchEngine/TouchObject.h"
#include "VulkanWindowsFunctions.h"
#include "VulkanGetterUtils.h"

namespace UE::TouchEngine::Vulkan
{
	inline uint64 GetCompletedSemaphoreValue(const VkSemaphore* VulkanSemaphore, const FString& DebugName)
	{
		if (!VulkanSemaphore)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[GetCompletedSemaphoreValue] Semaphore `%s` is null"), *DebugName);
			return 0;
		}
		const FVulkanPointers VulkanPointers;
		uint64 Value;
		UE::TouchEngine::Vulkan::vkGetSemaphoreCounterValueKHR(VulkanPointers.VulkanDeviceHandle, *VulkanSemaphore, &Value);
		// UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("[GetCompletedSemaphoreValue] Value%s `vkGetSemaphoreCounterValue`: `%lld`"),
		// 	*(DebugName.IsEmpty() ? "" : FString::Printf(TEXT("of semaphore `%s` "), *DebugName)), Value)
		return Value;
	}

	struct FTouchVulkanSemaphoreImport
	{
		HANDLE Handle;
		TouchObject<TEVulkanSemaphore> TouchSemaphore;
		TSharedPtr<VkSemaphore> VulkanSemaphore;
		FString DebugName;
		uint64 GetCompletedSemaphoreValue() const
		{
			return UE::TouchEngine::Vulkan::GetCompletedSemaphoreValue(VulkanSemaphore.Get(), DebugName);
		}
	};
	
	TOptional<FTouchVulkanSemaphoreImport> ImportTouchSemaphore(const TouchObject<TEVulkanSemaphore>& SemaphoreTE);
	
	struct FTouchVulkanSemaphoreExport
	{
		HANDLE ExportedHandle = nullptr;
		TouchObject<TEVulkanSemaphore> TouchSemaphore;
		TSharedPtr<VkSemaphore> VulkanSemaphore;
		FString DebugName;
		uint64 GetCompletedSemaphoreValue() const
		{
			return UE::TouchEngine::Vulkan::GetCompletedSemaphoreValue(VulkanSemaphore.Get(), DebugName);
		}
	};
	FTouchVulkanSemaphoreExport CreateAndExportSignalSemaphore(const SECURITY_ATTRIBUTES* SecurityAttributes, uint64 InitialSemaphoreValue, FString DebugName);
};

#endif