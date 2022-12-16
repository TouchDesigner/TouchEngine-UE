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

#include "Rendering/Exporting/TouchTextureExporter.h"

#include "Logging.h"
#include "Rendering/Exporting/TouchExportParams.h"

#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"

namespace UE::TouchEngine
{
	TFuture<FTouchExportResult> FTouchTextureExporter::ExportTextureToTouchEngine(const FTouchExportParameters& Params)
	{
		if (TaskSuspender.IsSuspended())
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("FTouchTextureExporter is suspended. Your task will be ignored."));
			return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::Cancelled }).GetFuture();
		}
		
		if (!IsValid(Params.Texture))
		{
			return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::Success }).GetFuture();
		}
		
		const bool bIsSupportedTexture = Params.Texture->IsA<UTexture2D>() || Params.Texture->IsA<UTextureRenderTarget2D>();
		if (!bIsSupportedTexture)
		{
			return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::UnsupportedTextureObject }).GetFuture();
		}

		TPromise<FTouchExportResult> Promise;
		TFuture<FTouchExportResult> Future = Promise.GetFuture();
		// This needs to be on the render thread to make sure nobody writes to the texture in the mean time
		ENQUEUE_RENDER_COMMAND(AccessTexture)([StrongThis = SharedThis(this), Params, Promise = MoveTemp(Promise)](FRHICommandListImmediate& RHICmdList) mutable
		{
			if (StrongThis->TaskSuspender.IsSuspended())
			{
				Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::Cancelled });
			}
			else
			{
				StrongThis->ExecuteExportTextureTask(RHICmdList, MoveTemp(Promise), Params);
			}
		});

		return Future.Next([TaskToken = TaskSuspender.StartTask()](auto Result)
		{
			return Result;
		});
	}

	void FTouchTextureExporter::ExecuteExportTextureTask(FRHICommandListImmediate& RHICmdList, TPromise<FTouchExportResult>&& Promise, const FTouchExportParameters& Params)
	{
		const bool bBecameInvalidSinceRenderEnqueue = !IsValid(Params.Texture);
		if (bBecameInvalidSinceRenderEnqueue)
		{
			Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::UnsupportedTextureObject });
			return;
		}

		ExportTexture_RenderThread(RHICmdList, Params)
			.Next([Promise = MoveTemp(Promise)](FTouchExportResult Result) mutable
			{
				Promise.SetValue(Result);
			});
	}

	FRHITexture2D* FTouchTextureExporter::GetRHIFromTexture(UTexture* Texture)
	{
		if (UTexture2D* Tex2D = Cast<UTexture2D>(Texture))
		{
			return Tex2D->GetResource()->TextureRHI->GetTexture2D();
			
		}
		if (UTextureRenderTarget2D* RT = Cast<UTextureRenderTarget2D>(Texture))
		{
			return RT->GetResource()->TextureRHI->GetTexture2D();;
		}
		return nullptr;
	}
}
