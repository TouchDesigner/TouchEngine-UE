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
#include "TouchEngineDynamicVariableStruct.h"
#include "Blueprint/TouchEngineInputFrameData.h"
#include "TouchEngine/TEResult.h"
#include "Async/Future.h"

UENUM(BlueprintType)
enum class ECookFrameResult : uint8
{
	/** The cook was successful. It does not mean that the frame was not dropped by TouchEngine. */
	Success,

	/** The inputs were discarded because the input queue became bigger than the Input Buffer Limit.
	 * When this happens, the value of the inputs will be given to the next set of inputs in the queue, unless they already define a value for those inputs.
	 * This should only happen in Delayed Synchronized and Independent modes. */
	InputsDiscarded,

	/** TouchEngine failed to cook the frame */
	InternalTouchEngineError UMETA(DisplayName = "Internal TouchEngine Error"),

	/** TouchEngine was requested to be shut down while a Cook was under process */
	Cancelled,

	/** Arguments were not correct or the TouchEngine instance was not valid when we wanted to start a cook */
	BadRequest,
	
	/** TouchEngine returned an error when the frame was started. */
	FailedToStartCook,
	
	/** TouchEngine did not send a response to the cook. You can look at increasing the Timout in the TouchEngine Component */
	TouchEngineCookTimeout,
	
	Count UMETA(Hidden)
};

namespace UE::TouchEngine
{
	struct TOUCHENGINE_API FCookFrameRequest
	{
		/** The frame time in Seconds, with TimeScale not yet multiplied. */
		double FrameTimeInSeconds;
		/** The Timescale passed to TEInstanceStartFrameAtTime. Should be a multiplier of the target FPS*/
		int64 TimeScale;
		/** The FrameData information about the frame request. Contains a unique FrameID for this Cook */
		FTouchEngineInputFrameData FrameData;

		/** A copy of the variables and their values needed for that cook */
		TMap<FString, FTouchEngineDynamicVariableStruct> VariablesToSend;
	};

	
	struct TOUCHENGINE_API FCookFrameResult
	{
		ECookFrameResult Result = ECookFrameResult::Count;
		TEResult TouchEngineInternalResult;

		FTouchEngineInputFrameData FrameData;
		
		/** Promise the caller need to set when they are done with the data and we could safely start the next cook. This does not start a next cook. Its is only set when the cook is done and could be null */
		TSharedPtr<TPromise<void>> OnReadyToStartNextCook;

		/** If true, TouchEngine did not process the new inputs and only the previous outputs are available. */
		bool bWasFrameDropped = false;
		/** The FrameID of when the last cook was processed. If bWasFrameDropped is false, it will be equal to FrameData.FrameID, otherwise they will differ. */
		int64 FrameLastUpdated = -1;

		/** The start_time returned by the TEInstanceEventCallback for this TE Cook. */
		double TECookStartTime = 0.0;
		/** The end_time returned by the TEInstanceEventCallback for this TE Cook. */
		double TECookEndTime = 0.0;


		static FCookFrameResult FromCookFrameRequest(const FCookFrameRequest& CookRequest, ECookFrameResult ErrorCode, int64 FrameLastUpdated, TEResult TouchEngineInternalResult)
		{
			FCookFrameResult CookFrameResult {ErrorCode, TouchEngineInternalResult, CookRequest.FrameData};
			CookFrameResult.FrameLastUpdated = FrameLastUpdated;
			return CookFrameResult;
		}
		static FCookFrameResult FromCookFrameRequest(const FCookFrameRequest& CookRequest, ECookFrameResult ErrorCode, int64 FrameLastUpdated)
		{
			TEResult Result;
			switch (ErrorCode) {
			case ECookFrameResult::Success: Result = TEResultSuccess; break;
			case ECookFrameResult::BadRequest: Result = TEResultBadUsage; break;
			case ECookFrameResult::Cancelled: Result = TEResultCancelled; break;
			case ECookFrameResult::FailedToStartCook: Result = TEResultInternalError; break;
			case ECookFrameResult::InternalTouchEngineError: Result = TEResultInternalError; break;
			case ECookFrameResult::InputsDiscarded: Result = TEResultCancelled; break;
			case ECookFrameResult::Count: Result = TEResultBadUsage; break;
			case ECookFrameResult::TouchEngineCookTimeout: Result = TEResultCancelled; break;
			default:
				static_assert(static_cast<int32>(ECookFrameResult::Count) == 7, "Update this switch");
				Result = TEResultBadUsage;
				break;
			}
			return FromCookFrameRequest(CookRequest, ErrorCode, FrameLastUpdated, Result);
		}
	};
}