// Copyright © Derivative Inc. 2022

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
	class FTouchResourceProvider
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
