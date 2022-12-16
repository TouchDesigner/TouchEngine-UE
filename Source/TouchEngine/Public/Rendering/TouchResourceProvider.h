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

#include "Exporting/TouchExportParams.h"
#include "Importing/TouchImportParams.h"
#include "PixelFormat.h"
#include "TouchSuspendResult.h"

#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TouchObject.h"

#include "Async/Future.h"

#include "RhiIncludeHelper.h"

class FTexture2DResource;
struct FTouchTOP;
typedef void FTouchEngineDevice;

namespace UE::TouchEngine
{
	struct TOUCHENGINE_API FTouchLoadInstanceResult
	{
		TOptional<FString> Error;

		static FTouchLoadInstanceResult MakeSuccess() { return {}; }
		static FTouchLoadInstanceResult MakeFailure(FString ErrorMessage) { return { { MoveTemp(ErrorMessage) } }; }
		
		bool IsSuccess() const { return !Error.IsSet(); }
		bool IsFailure() const { return !IsSuccess(); }

		FORCEINLINE operator bool() const
		{
			return IsSuccess();
		}
	};
	
	/** Common interface for rendering API implementations */
	class TOUCHENGINE_API FTouchResourceProvider : public TSharedFromThis<FTouchResourceProvider>
	{
	public:
		
		/** Configures the instance to work together with this resource provider. TEInstanceAssociateGraphicsContext has already been called with GetContext() as parameter. */
		virtual void ConfigureInstance(const TouchObject<TEInstance>& Instance) = 0;
		
		virtual TEGraphicsContext* GetContext() const = 0;

		/**
		 * Called when TEEventInstanceDidLoad event is received and the result is successful.
		 * @return Whether this instance is compatible with UE or not.
		 */
		virtual FTouchLoadInstanceResult ValidateLoadedTouchEngine(TEInstance& Instance) = 0;
		
		/** Whether the given pixel format can be used for textures passed to ExportTextureToTouchEngine. Must be called after the TE instance has sent TEEventInstanceDidLoad event. */
		bool CanExportPixelFormat(TEInstance& Instance, EPixelFormat Format) { return GetExportablePixelTypes(Instance).Contains(Format); }
		virtual TSet<EPixelFormat> GetExportablePixelTypes(TEInstance& Instance) = 0;
		
		/** Converts an Unreal texture to a TE texture so it can be used as input to TE. */
		TFuture<FTouchExportResult> ExportTextureToTouchEngine(const FTouchExportParameters& Params);

		/** Converts a TE texture received from TE to an Unreal texture. */
		virtual TFuture<FTouchImportResult> ImportTextureToUnrealEngine(const FTouchImportParameters& LinkParams) = 0;

		/**
		 * Prevents further async tasks from being enqueued, cancels running tasks where possible, and executes the future once all tasks are done.
		 * This is not a replacement of ~FTouchResourceProvider. Actively running tasks may reference this resource provider thus preventing its destruction.
		 * This function allows the tasks to complete on the CPU and GPU. After this is is possible to destroy FTouchResourceProvider in a defined state. 
		 */
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() = 0;
		
		virtual ~FTouchResourceProvider() = default;
		
	private:
		
		/** Converts an Unreal texture to a TE texture so it can be used as input to TE. */
		virtual TFuture<FTouchExportResult> ExportTextureToTouchEngineInternal(const FTouchExportParameters& Params) = 0;
	};
}
