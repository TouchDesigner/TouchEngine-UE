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

#include "TouchTextureExporterD3D11.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "d3d11.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "D3D11TouchUtils.h"
#include "Logging.h"
#include "TouchTextureImporterD3D11.h"
#include "Rendering/Exporting/TouchExportParams.h"
#include "TouchEngine/TED3D11.h"

namespace UE::TouchEngine
{
	TEResult FTouchTextureExporterD3D11::AddTETextureTransfer_RenderThread(const FTouchExportParameters& Params, const TSharedRef<FExportedTouchTexture>& Texture)
	{
		return TEResultBadUsage;
	}
}

