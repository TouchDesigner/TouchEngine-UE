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

#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "PixelFormat.h"

#include "TouchExportParams.h"
#include "TouchLinkParams.h"

#include "TouchEngine/TETexture.h"
#include "TouchEngine/TouchObject.h"

class FTexture2DResource;
class FRHITexture2D;
struct FTouchTOP;
typedef void FTouchEngineDevice;

namespace UE::TouchEngine
{
	
	/** Common interface for rendering API implementations */
	class TOUCHENGINE_API FTouchResourceProvider
	{
	public:

		virtual TEGraphicsContext* GetContext() const = 0;

		/** Converts an Unreal texture to a TE texture so it can be used as input to TE. */
		virtual TFuture<FTouchExportResult> ExportTextureToTouchEngine(const FTouchExportParameters& Params) = 0;

		/** Converts a TE texture received from TE to an Unreal texture. */
		virtual TFuture<FTouchLinkResult> LinkTexture(const FTouchLinkParameters& LinkParams) = 0;
		
		virtual ~FTouchResourceProvider() = default;
	};
}
