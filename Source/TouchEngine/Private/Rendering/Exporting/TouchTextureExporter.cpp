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
#include "RenderingThread.h"
#include "TextureResource.h"

#include "Logging.h"
#include "Rendering/Exporting/TouchExportParams.h"

#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Rendering/TouchResourceProvider.h"
#include "Util/TouchHelpers.h"

namespace UE::TouchEngine
{
	TFuture<TouchObject<TETexture>> FTouchTextureExporter::ExportTextureToTouchEngine_AnyThread(const FTouchExportParameters& Params)
	{
		if (TaskSuspender.IsSuspended())
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("[ExportTextureToTouchEngine_AnyThread[%s]] FTouchTextureExporter is suspended. Your task will be ignored."), *GetCurrentThreadStr());
			return MakeFulfilledPromise<TouchObject<TETexture>>(nullptr).GetFuture();
		}
		
		check(Params.TextureToBeExported)
		
		return ExportTextureToTE_AnyThread(Params);
	}

	TFuture<TSharedPtr<FExportedTouchTexture>> FTouchTextureExporter::EnqueueShareTexture(const FTouchExportParameters& ParamsConst)
	{
		TPromise<TSharedPtr<FExportedTouchTexture>> Promise;
		TFuture<TSharedPtr<FExportedTouchTexture>> Future = Promise.GetFuture();
		
		ENQUEUE_RENDER_COMMAND(ExportedTextureShared)([Promise = MoveTemp(Promise), WeakThis = AsWeak(), ParamsConst](FRHICommandListImmediate& RHICmdList) mutable
		{
			TSharedPtr<FTouchTextureExporter> This = WeakThis.Pin();
			if (!This || !ParamsConst.TextureToBeExported)
			{
				Promise.SetValue(nullptr);
				return;
			}

			TSharedPtr<FTouchResourceProvider> Provider = This->GetWeakProvider().Pin();
			if (!Provider)
			{
				Promise.SetValue(nullptr);
				return;
			}
			
			if (!ParamsConst.TextureToBeExported->IsCreatedOnRenderThread())
			{
				UE_LOG(LogTouchEngine, Error, TEXT("RHI has not yet been created for '%s'"), *ParamsConst.TextureToBeExported->DebugName);
				Promise.SetValue(nullptr);
				return;
			}
			
			if (! Provider->CanExportPixelFormat(*ParamsConst.Instance.get(), ParamsConst.TextureToBeExported->GetSharedTextureRHI_RenderThread()->GetFormat()))
			{
				UE_LOG(LogTouchEngine, Error, TEXT("EPixelFormat `%s` is not supported for export to TouchEngine. %s"), GetPixelFormatString(ParamsConst.TextureToBeExported->GetSharedTextureRHI_RenderThread()->GetFormat()), *ParamsConst.TextureToBeExported->DebugName);
				Promise.SetValue(nullptr);
				return;
			}

			if (This->ShareTexture_RenderThread(ParamsConst))
			{
				Promise.SetValue(ParamsConst.TextureToBeExported);
			}
			else
			{
				Promise.SetValue(nullptr);
			}
		});

		return Future;
	}
}
