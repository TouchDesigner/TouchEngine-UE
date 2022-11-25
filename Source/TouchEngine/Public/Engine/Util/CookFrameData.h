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

namespace UE::TouchEngine
{
	struct TOUCHENGINE_API FCookFrameRequest
	{
		/** The frame time, with TimeScale already multiplied. */
		int64 FrameTime_Mill;
		int64 TimeScale;
	};
	
	enum class ECookFrameErrorCode
	{
		Success,

		/** Args were not correct */
		BadRequest,

		/** This cook frame request has not been started yet and has been replaced by a newer incoming request. */
		Replaced,
		
		/** The TE engine was requested to be shut down while a frame cook was in progress */
		Cancelled,

		/** TEInstanceStartFrameAtTime failed. */
		FailedToStartCook,
		
		/** TE failed to cook the frame */
		InternalTouchEngineError,

		/** TE told us the frame was cancelled */
		TEFrameCancelled,

		Count
	};
	
	struct TOUCHENGINE_API FCookFrameResult
	{
		ECookFrameErrorCode ErrorCode;
	};
}