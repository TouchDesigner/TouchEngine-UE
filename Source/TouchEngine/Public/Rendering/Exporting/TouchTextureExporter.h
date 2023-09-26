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
#include "Util/TaskSuspender.h"
#include "TouchEngine/TouchObject.h"

class FRHICommandListImmediate;
class FRHICommandList;
class FRHICommandListBase;
class UTexture;

namespace UE::TouchEngine
{
	class FTouchVariableManager;
	struct FTouchExportResult;
	struct FTouchExportParameters;
	struct FTouchSuspendResult;

	/** Util for exporting textures from Unreal to TouchEngine */
	class TOUCHENGINE_API FTouchTextureExporter : public TSharedFromThis<FTouchTextureExporter>
	{
	public:

		virtual ~FTouchTextureExporter() = default;

		TouchObject<TETexture> ExportTextureToTouchEngine_AnyThread(const FTouchExportParameters& Params, TEGraphicsContext* GraphicsContext);
		
		/** Prevents further async tasks from being enqueued, cancels running tasks where possible, and executes the future once all tasks are done. */
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() { return TaskSuspender.Suspend(); }
		virtual FTaskSuspender::FTaskTracker StartAsyncTask() { return TaskSuspender.StartTask(); }
		bool IsSuspended() const { return TaskSuspender.IsSuspended(); }
		
	protected:
		
		virtual TouchObject<TETexture> ExportTexture_AnyThread(const FTouchExportParameters& Params, TEGraphicsContext* GraphicContext) = 0;

	private:

		/** Tracks running tasks and helps us execute an event when all tasks are done (once they've been suspended). */
		FTaskSuspender TaskSuspender;
	};
}
