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
#include "PixelFormat.h"
#include "Rendering/Importing/TouchTextureImporter.h"

namespace UE::TouchEngine
{
	TouchObject<TETexture> FTouchResourceProvider::ExportTextureToTouchEngine_AnyThread(const FTouchExportParameters& Params)
	{
		if (!Params.Texture)
		{
			UE_LOG(LogTouchEngine, Error, TEXT("We can only export valid Textures. Make sure the texture passed as input is valid. %s"), *Params.GetDebugDescription());
			return nullptr;
		}

		const EPixelFormat PixelFormat = GetPixelFormat(Params.Texture);
		if (!CanExportPixelFormat(*Params.Instance.get(), PixelFormat))
		{
			UE_LOG(LogTouchEngine, Error, TEXT("EPixelFormat `%s` is not supported for export to TouchEngine. %s"), GetPixelFormatString(PixelFormat), *Params.GetDebugDescription());
			return nullptr;
		}

		return ExportTextureToTouchEngineInternal_AnyThread(Params);
	}

	TFuture<FTouchTextureImportResult> FTouchResourceProvider::ImportTextureToUnrealEngine_AnyThread(const FTouchImportParameters& LinkParams, const TSharedPtr<FTouchFrameCooker>& FrameCooker)
	{
		return GetImporter().ImportTexture_AnyThread(LinkParams, FrameCooker);
	}
}
