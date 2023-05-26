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
#include "Rendering/Importing/TouchTextureImporter.h"
#include "Util/TouchEngineStatsGroup.h"

namespace UE::TouchEngine
{
	TouchObject<TETexture> FTouchResourceProvider::ExportTextureToTouchEngine_AnyThread(const FTouchExportParameters& Params)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Cook Frame - FTouchResourceProvider::ExportTextureToTouchEngine_AnyThread"), STAT_FTouchResourceProviderExportTextureToTouchEngine_AnyThread, STATGROUP_TouchEngine);

		const UTexture2D* Texture2D = Cast<UTexture2D>(Params.Texture);
		if (Texture2D)
		{
			// We are using PlatformData directly instead of TargetTexture->GetSizeX()/GetSizeY()/GetPixelFormat() as
			// they do some unnecessary ensures (for our case) that fails as we are not always on the GameThread
			const FTexturePlatformData* PlatformData = Texture2D->GetPlatformData();
			if(!CanExportPixelFormat(*Params.Instance.get(), PlatformData->GetLayerPixelFormat(0)))
			{
				UE_LOG(LogTouchEngine, Error, TEXT("EPixelFormat %s is not supported for export to TouchEngine."), GPixelFormats[Texture2D->GetPixelFormat()].Name);
				return nullptr;
			}
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

		return ExportTextureToTouchEngineInternal_AnyThread(Params);
	}

	TFuture<FTouchTextureImportResult> FTouchResourceProvider::ImportTextureToUnrealEngine_AnyThread(const FTouchImportParameters& LinkParams, const TSharedPtr<FTouchFrameCooker>& FrameCooker)
	{
		return GetImporter().ImportTexture_AnyThread(LinkParams, FrameCooker);
	}
}
