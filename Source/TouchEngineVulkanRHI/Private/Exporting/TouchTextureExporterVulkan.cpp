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
		
		FRHICommandCopyUnrealToTouch(TPromise<FTouchExportResult> Promise, FTouchExportParameters ExportParameters)
			: Promise(MoveTemp(Promise))
			, ExportParameters(MoveTemp(ExportParameters))
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
			Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::UnsupportedOperation });
		}
	};
	
	TFuture<FTouchExportResult> FTouchTextureExporterVulkan::ExportTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTouchExportParameters& Params)
	{
		TPromise<FTouchExportResult> Promise; 
		TFuture<FTouchExportResult> Future = Promise.GetFuture();

		ALLOC_COMMAND_CL(RHICmdList, FRHICommandCopyUnrealToTouch)(MoveTemp(Promise), Params);
		return Future;
	}
}
