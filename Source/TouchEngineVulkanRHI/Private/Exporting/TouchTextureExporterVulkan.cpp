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

#include "Rendering/Exporting/TouchExportParams.h"

namespace UE::TouchEngine::Vulkan
{
	FRHICOMMAND_MACRO(FRHICommandCopyUnrealToTouch)
	{
		TPromise<FTouchExportResult> Promise;
		bool bFulfilledPromise = false;

		const FTouchExportParameters ExportParameters;
		const TSharedPtr<FExportedTextureVulkan> SharedTextureResources;
		
		FRHICommandCopyUnrealToTouch(TPromise<FTouchExportResult> Promise, FTouchExportParameters ExportParameters, TSharedPtr<FExportedTextureVulkan> SharedTextureResources)
			: Promise(MoveTemp(Promise))
			, ExportParameters(MoveTemp(ExportParameters))
			, SharedTextureResources(MoveTemp(SharedTextureResources))
		{}

		~FRHICommandCopyUnrealToTouch()
		{
			if (!ensureMsgf(bFulfilledPromise, TEXT("Investigate broken promise")))
			{
				Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::UnknownFailure });
			}
		}

		void Execute(FRHICommandListBase& CmdList)
		{
			// TODO DP: Synchronize and copy textures
			Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::UnsupportedOperation });
		}
	};

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
		return FExportedTextureVulkan::Create(*SourceRHI);
	}

	TFuture<FTouchExportResult> FTouchTextureExporterVulkan::ExportTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTouchExportParameters& Params)
	{
		const TSharedPtr<FExportedTextureVulkan> SharedTextureResources = GetOrTryCreateTexture(MakeTextureCreationArgs(Params));
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
