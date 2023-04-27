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

#include "Rendering/TouchResourceProvider.h"

#include "Logging.h"

#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PixelFormat.h"

namespace UE::TouchEngine
{
	TouchObject<TETexture> FTouchResourceProvider::ExportTextureToTouchEngine_GameThread(const FTouchExportParameters& Params)
	{
		const UTexture2D* Texture2D = Cast<UTexture2D>(Params.Texture);
		if (Texture2D && !CanExportPixelFormat(*Params.Instance.get(), Texture2D->GetPixelFormat()))
		{
			UE_LOG(LogTouchEngine, Error, TEXT("EPixelFormat %s is not supported for export to TouchEngine."), GPixelFormats[Texture2D->GetPixelFormat()].Name);
			return nullptr;
		}

		const UTextureRenderTarget2D* RenderTarget2D = Cast<UTextureRenderTarget2D>(Params.Texture);
		if (RenderTarget2D && !CanExportPixelFormat(*Params.Instance.get(), RenderTarget2D->GetFormat()))
		{
			UE_LOG(LogTouchEngine, Error, TEXT("EPixelFormat %s is not supported for export to TouchEngine."), GPixelFormats[RenderTarget2D->GetFormat()].Name);
			return nullptr;
		}

		if (Params.Texture != nullptr && !Texture2D && !RenderTarget2D)
		{
			UE_LOG(LogTouchEngine, Error, TEXT("Only UTexture2D and UTextureRenderTarget2D are supported for exporting."));
			return nullptr;
		}

		return ExportTextureToTouchEngineInternal_GameThread(Params);
	}
}
