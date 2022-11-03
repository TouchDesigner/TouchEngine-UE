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

#include "SemaphoreVulkanUtils.h"

#include "Logging.h"
#include "Util/VulkanGetterUtils.h"
#include "Util/VulkanWindowsFunctions.h"

#include "TouchEngine/TouchObject.h"

namespace UE::TouchEngine::Vulkan
{
	TOptional<FTouchVulkanSemaphoreImport> ImportTouchSemaphore(TouchObject<TEVulkanSemaphore> SemaphoreTE, TEVulkanSemaphoreCallback Callback, void* Info)
	{
		const VkSemaphoreType SemaphoreType = TEVulkanSemaphoreGetType(SemaphoreTE);
		const bool bIsTimeline = SemaphoreType == VK_SEMAPHORE_TYPE_TIMELINE_KHR || SemaphoreType == VK_SEMAPHORE_TYPE_TIMELINE; 
		if (!bIsTimeline)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Received a binary semaphore but only timeline semaphores are implemented."))
			return {};
		}
		
		const HANDLE SharedHandle = TEVulkanSemaphoreGetHandle(SemaphoreTE);
		if (SharedHandle == nullptr)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Invalid semaphore handle received from Touch Engine"));
			return {};
		}
		
		const FVulkanPointers VulkanPointers;
		const VkExternalSemaphoreHandleTypeFlagBits SemaphoreHandleType = TEVulkanSemaphoreGetHandleType(SemaphoreTE);
		// Other types not allowed for vkImportSemaphoreWin32HandleKHR
		if (SemaphoreHandleType != VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT && SemaphoreHandleType != VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Unexpected VkExternalSemaphoreHandleTypeFlagBits for Vulkan semaphore exported from Touch Designer (VkExternalSemaphoreHandleTypeFlagBits = %d)"), static_cast<int32>(SemaphoreHandleType))
			return {};
		}

		VkSemaphoreTypeCreateInfo SemTypeCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
		SemTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		VkSemaphoreCreateInfo SemCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &SemTypeCreateInfo };
		VkSemaphore VulkanSemaphore;
		VERIFYVULKANRESULT(VulkanRHI::vkCreateSemaphore(VulkanPointers.VulkanDeviceHandle, &SemCreateInfo, NULL, &VulkanSemaphore));

		TSharedRef<VkSemaphore> SharedVulkanSemaphore = MakeShareable<VkSemaphore>(new VkSemaphore(VulkanSemaphore), [](VkSemaphore* VulkanSemaphore)
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

		VERIFYVULKANRESULT(vkImportSemaphoreWin32HandleKHR(VulkanPointers.VulkanDeviceHandle, &ImportSemWin32Info));
		TEVulkanSemaphoreSetCallback(SemaphoreTE, Callback, Info);
		
		return FTouchVulkanSemaphoreImport{ SharedHandle, SemaphoreTE, SharedVulkanSemaphore };
	}

	FTouchVulkanSemaphoreExport CreateAndExportSemaphore(const SECURITY_ATTRIBUTES* SecurityAttributes, uint64 InitialSemaphoreValue)
	{
		const FVulkanPointers VulkanPointers;
		FTouchVulkanSemaphoreExport Result;
		
		constexpr bool bUseNTHandles = true;
		VkExportSemaphoreCreateInfo ExportSemInfo = { VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO };
		ExportSemInfo.handleTypes = bUseNTHandles ? VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT : VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
		VkExportSemaphoreWin32HandleInfoKHR ExportSemWin32Info = { VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR };
		if (bUseNTHandles)
		{
			ExportSemWin32Info.dwAccess = STANDARD_RIGHTS_ALL | SEMAPHORE_MODIFY_STATE;
			ExportSemWin32Info.pAttributes = SecurityAttributes;
			ExportSemInfo.pNext = &ExportSemWin32Info;
		}

		VkSemaphoreTypeCreateInfo SemaphoreTypeCreateInfo { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, &ExportSemInfo };
		SemaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		SemaphoreTypeCreateInfo.initialValue = InitialSemaphoreValue;
		
		VkSemaphoreCreateInfo SemCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &SemaphoreTypeCreateInfo };
		VkSemaphore VulkanSemaphore;
		VERIFYVULKANRESULT(VulkanRHI::vkCreateSemaphore(VulkanPointers.VulkanDeviceHandle, &SemCreateInfo, NULL, &VulkanSemaphore));
		Result.VulkanSemaphore = MakeShareable<VkSemaphore>(new VkSemaphore(VulkanSemaphore), [](VkSemaphore* VulkanSemaphore)
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
		VERIFYVULKANRESULT(vkGetSemaphoreWin32HandleKHR(VulkanPointers.VulkanDeviceHandle, &semaphoreGetWin32HandleInfo, &Result.ExportedHandle));

		TEVulkanSemaphore* TouchSemaphore = TEVulkanSemaphoreCreate(SemaphoreTypeCreateInfo.semaphoreType, Result.ExportedHandle, (VkExternalSemaphoreHandleTypeFlagBits)ExportSemInfo.handleTypes, nullptr, nullptr);
		Result.TouchSemaphore.set(TouchSemaphore);

		return Result;
	}
}
