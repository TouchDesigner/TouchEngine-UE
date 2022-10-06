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

#include "Rendering/TouchTextureExporter.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Rendering/TouchExportParams.h"

namespace UE::TouchEngine
{
	TFuture<FTouchExportResult> FTouchTextureExporter::ExportTextureToTouchEngine(const FTouchExportParameters& Params)
	{
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
		ENQUEUE_RENDER_COMMAND(AccessTexture)([this, Texture = Params.Texture, Promise = MoveTemp(Promise)](FRHICommandListImmediate& RHICmdList) mutable
		{
			const bool bBecameInvalidSinceRenderEnqueue = !IsValid(Texture);
			if (bBecameInvalidSinceRenderEnqueue)
			{
				Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::UnsupportedTextureObject });
				return;
			}
			
			if (UTexture2D* Tex2D = Cast<UTexture2D>(Texture))
			{
				FRHITexture2D* const RHI_Texture = Tex2D->GetResource()->TextureRHI->GetTexture2D();
				const EPixelFormat Format = Tex2D->GetPixelFormat();
				Promise.SetValue(ExportTexture(RHI_Texture, Format));
			}
			else if (UTextureRenderTarget2D* RT = Cast<UTextureRenderTarget2D>(Texture))
			{
				FRHITexture2D* const  RHI_Texture = RT->GetResource()->TextureRHI->GetTexture2D();
				const EPixelFormat Format = GetPixelFormatFromRenderTargetFormat(RT->RenderTargetFormat);
				Promise.SetValue(ExportTexture(RHI_Texture, Format));
			}
		});

		return Future;
		
	}
}
