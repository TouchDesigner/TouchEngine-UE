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

#include "TouchTextureExporterVulkan.h"

#include "vulkan_core.h"
#include "VulkanRHIPrivate.h"

#include "Rendering/Exporting/TouchExportParams.h"
#include "TouchEngine/TEVulkan.h"
#include "Util/VulkanCommandBuilder.h"


namespace UE::TouchEngine::Vulkan
{
	FRHICOMMAND_MACRO(FRHICommandCopyUnrealToTouch)
	{
		TPromise<FTouchExportResult> Promise;
		bool bFulfilledPromise = false;

		const FTouchExportParameters ExportParameters;
		const TSharedPtr<FExportedTextureVulkan> SharedTextureResources;
		FVulkanCommandBuilder CommandBuilder;
		
		FRHICommandCopyUnrealToTouch(TPromise<FTouchExportResult> Promise, FTouchExportParameters ExportParameters, TSharedPtr<FExportedTextureVulkan> InSharedTextureResources)
			: Promise(MoveTemp(Promise))
			, ExportParameters(MoveTemp(ExportParameters))
			, SharedTextureResources(MoveTemp(InSharedTextureResources))
			, CommandBuilder(SharedTextureResources->GetCommandBuffer().Get())
		{}

		~FRHICommandCopyUnrealToTouch()
		{
			if (!ensureMsgf(bFulfilledPromise, TEXT("Investigate broken promise")))
			{
				Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::UnknownFailure });
			}
		}

		VkCommandBuffer GetCommandBuffer() const { return SharedTextureResources->GetCommandBuffer().Get(); }

		void Execute(FRHICommandListBase& CmdList)
		{
			CommandBuilder.BeginCommands();
			
			// 1. If TE is using it, schedule a wait operation
			/*VkImageLayout AcquireOldLayout;
			VkImageLayout AcquireNewLayout;
			TouchObject<TESemaphore> Semaphore;
			uint64 WaitValue;
			const TEResult ResultCode = TEInstanceGetVulkanTextureTransfer(ExportParameters.Instance, SharedTextureResources->GetTouchRepresentation(), &AcquireOldLayout, &AcquireNewLayout, Semaphore.take(), &WaitValue);
			// Will be false the very first time the texture is created because TE has never received the texture
			if (ResultCode == TEResultSuccess)
			{
				
			}*/
			
			CommandBuilder.Submit(CmdList);
			bFulfilledPromise = true;
			Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::UnsupportedOperation });
		}
	};
	
	FTouchTextureExporterVulkan::FTouchTextureExporterVulkan(TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes)
		: SecurityAttributes(MoveTemp(SecurityAttributes))
	{}

	TFuture<FTouchSuspendResult> FTouchTextureExporterVulkan::SuspendAsyncTasks()
	{
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
		
		TFuture<FTouchSuspendResult> FinishRenderingTasks = FTouchTextureExporter::SuspendAsyncTasks();
		// Once all the rendering tasks have finished using the copying textures, they can be released.
		FinishRenderingTasks.Next([this, Promise = MoveTemp(Promise)](auto) mutable
		{
			ReleaseTextures().Next([Promise = MoveTemp(Promise)](auto) mutable
			{
				Promise.SetValue({});
			});
		});

		return Future;
	}

	TSharedPtr<FExportedTextureVulkan> FTouchTextureExporterVulkan::CreateTexture(const FTextureCreationArgs& Params)
	{
		const FRHITexture2D* SourceRHI = GetRHIFromTexture(Params.Texture);
		return FExportedTextureVulkan::Create(*SourceRHI, Params.CmdList, SecurityAttributes);
	}

	TFuture<FTouchExportResult> FTouchTextureExporterVulkan::ExportTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTouchExportParameters& Params)
	{
		const TSharedPtr<FExportedTextureVulkan> SharedTextureResources = GetOrTryCreateTexture(MakeTextureCreationArgs(Params, { RHICmdList }));
		if (!SharedTextureResources)
		{
			return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::InternalGraphicsDriverError }).GetFuture();
		}
		
		TPromise<FTouchExportResult> Promise; 
		TFuture<FTouchExportResult> Future = Promise.GetFuture();

		ALLOC_COMMAND_CL(RHICmdList, FRHICommandCopyUnrealToTouch)(MoveTemp(Promise), Params, SharedTextureResources);
		return Future;
	}
}
