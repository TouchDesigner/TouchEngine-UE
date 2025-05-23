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
#include "PixelFormat.h"
#include "Engine/Texture2D.h"
#include "Rendering/Importing/TouchTextureImporter.h"

namespace UE::TouchEngine
{
	void FTouchResourceProvider::ConfigureInstance(const TouchObject<TEInstance>& InInstance)
	{
		Instance = InInstance;
	}

	TFuture<TouchObject<TETexture>> FTouchResourceProvider::ExportTextureToTouchEngine_AnyThread(const FTouchExportParameters& Params)
	{
		return GetTextureExporter().ExportTextureToTouchEngine_AnyThread(Params);
	}

	void FTouchResourceProvider::PrepareForNewCook(const FTouchEngineInputFrameData& FrameData)
	{
		InitializeExportsToTouchEngine_GameThread(FrameData);
		GetTextureImporter().PrepareForNewCook(FrameData);
	}

	void FTouchResourceProvider::SetExportedTexturePoolSize(int ExportedTexturePoolSize)
	{
		GetTextureExporter().PoolSize = FMath::Max(ExportedTexturePoolSize, 0);
	}

	void FTouchResourceProvider::SetImportedTexturePoolSize(int ImportedTexturePoolSize)
	{
		GetTextureImporter().PoolSize = FMath::Max(ImportedTexturePoolSize, 0);
	}

	void FTouchResourceProvider::ClearSavedInstance()
	{
		Instance.reset();
	}
}
