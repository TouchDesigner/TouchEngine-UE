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
#include "vulkan_core.h"

class FRHICommandListBase;

namespace UE::TouchEngine::Vulkan
{
	/** Semaphores to wait on before starting the command list */
	struct FWaitSemaphoreData
	{
		VkSemaphore Wait;
		uint64 ValueToAwait;
		VkPipelineStageFlags WaitStageFlags;
	};
	
	/** Semaphores to signal when the command list is done executing */
	struct FSignalSemaphoreData
	{
		VkSemaphore Signal;
		uint64 ValueToSignal;
	};
	
	class FVulkanCommandBuilder
	{
	public:
		
		FVulkanCommandBuilder(VkCommandBuffer CommandBuffer)
			: CommandBuffer(CommandBuffer)
		{}

		VkCommandBuffer GetCommandBuffer() const { return CommandBuffer; }
		void AddWaitSemaphore(const FWaitSemaphoreData& Data) { SemaphoresToAwait.Add(Data); }
		void AddSignalSemaphore(const FSignalSemaphoreData& Data) { SemaphoresToSignal.Add(Data); }

		void BeginCommands();
		void Submit(FRHICommandListBase& CmdList);

	private:
		
		VkCommandBuffer CommandBuffer;
		TArray<FWaitSemaphoreData> SemaphoresToAwait;
		TArray<FSignalSemaphoreData> SemaphoresToSignal;
		
		void EndCommands();
	};
}
