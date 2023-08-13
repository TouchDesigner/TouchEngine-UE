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
#include "Util/TouchHelpers.h"

namespace UE::TouchEngine
{
	TouchObject<TETexture> FTouchTextureExporter::ExportTextureToTouchEngine_AnyThread(const FTouchExportParameters& Params, TEGraphicsContext* GraphicsContext)
	{
		if (TaskSuspender.IsSuspended())
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("[ExportTextureToTouchEngine_AnyThread[%s]] FTouchTextureExporter is suspended. Your task will be ignored."), *GetCurrentThreadStr());
			return nullptr;
		}
		
		check(Params.Texture)
		
		return ExportTexture_AnyThread(Params, GraphicsContext);
	}
}
