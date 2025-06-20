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

#include "VulkanCommandBuilder.h"

THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END
#if PLATFORM_WINDOWS
#include "WindowsVulkanPlatformDefines.h"
#endif
#include "VulkanRHIPrivate.h"
#include "VulkanContext.h"

namespace UE::TouchEngine::Vulkan
{
	void FVulkanCommandBuilder::BeginCommands()
	{
		VERIFYVULKANRESULT(VulkanRHI::vkResetCommandBuffer(GetCommandBuffer(), VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
		
		VkCommandBufferBeginInfo CmdBufBeginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		CmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VERIFYVULKANRESULT(VulkanRHI::vkBeginCommandBuffer(GetCommandBuffer(), &CmdBufBeginInfo));
	}

	void FVulkanCommandBuilder::EndCommands()
	{
		VERIFYVULKANRESULT(VulkanRHI::vkEndCommandBuffer(GetCommandBuffer()));
	}

	void FVulkanCommandBuilder::Submit(FRHICommandListBase& CmdList)
	{
		EndCommands();
		
		FVulkanCommandListContext& CmdListContext = static_cast<FVulkanCommandListContext&>(CmdList.GetContext());
		const VkQueue Queue = CmdListContext.Queue.GetHandle();

		VkTimelineSemaphoreSubmitInfo SemaphoreSubmitInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
		VkSubmitInfo SubmitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO, &SemaphoreSubmitInfo };
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &CommandBuffer;

		constexpr uint64 ExpectedNumSignalSemaphores = 1;
		constexpr uint64 ExpectedNumWaitSemaphores = 1;
		
		TArray<VkSemaphore, TInlineAllocator<ExpectedNumSignalSemaphores>> SignalSemaphores;
		TArray<uint64, TInlineAllocator<ExpectedNumSignalSemaphores>> SignalValues;
		for (const FSignalSemaphoreData& SignalData : SemaphoresToSignal)
		{
			SignalSemaphores.Add(SignalData.Signal);
			SignalValues.Add(SignalData.ValueToSignal);
		}
		SubmitInfo.signalSemaphoreCount = SemaphoresToSignal.Num();
		SubmitInfo.pSignalSemaphores = SignalSemaphores.GetData();
		SemaphoreSubmitInfo.signalSemaphoreValueCount = SignalValues.Num(); 
		SemaphoreSubmitInfo.pSignalSemaphoreValues = SignalValues.GetData();

		TArray<VkSemaphore, TInlineAllocator<ExpectedNumWaitSemaphores>> WaitSemaphores;
		TArray<uint64, TInlineAllocator<ExpectedNumWaitSemaphores>> WaitValues;
		VkPipelineStageFlags WaitStageFlags = 0;
		for (const FWaitSemaphoreData& WaitData : SemaphoresToAwait)
		{
			WaitSemaphores.Add(WaitData.Wait);
			WaitValues.Add(WaitData.ValueToAwait);
			WaitStageFlags |= WaitData.WaitStageFlags;
		}
		SubmitInfo.waitSemaphoreCount = SemaphoresToAwait.Num();
		SubmitInfo.pWaitSemaphores = WaitSemaphores.GetData();
		SubmitInfo.pWaitDstStageMask = &WaitStageFlags;
		SemaphoreSubmitInfo.waitSemaphoreValueCount = WaitValues.Num(); 
		SemaphoreSubmitInfo.pWaitSemaphoreValues = WaitValues.GetData();
		
		VERIFYVULKANRESULT(VulkanRHI::vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE));
	}
}
