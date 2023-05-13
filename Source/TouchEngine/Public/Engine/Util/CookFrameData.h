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
#include "Rendering/Importing/TouchImportParams.h"
#include "TouchEngine/TEResult.h"

UENUM(BlueprintType)
enum class ECookFrameErrorCode : uint8
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
		
	/** This cook frame request has been cancelled because the cook queue limit has been reached  */
	InputBufferLimitReached,

	Count UMETA(Hidden)
};

namespace UE::TouchEngine
{
	struct TOUCHENGINE_API FCookFrameRequest
	{
		/** The frame time, with TimeScale already multiplied. */
		int64 FrameTime_Mill;
		int64 TimeScale;
		/** The FrameData information about the frame request. Contains a unique FrameID for this Cook */
		FTouchEngineInputFrameData FrameData;

		/** A copy of the variables and their values needed for that cook */
		FTouchEngineDynamicVariableContainer DynamicVariables;
	};


	static FString ECookFrameErrorCodeToString(ECookFrameErrorCode CookFrameErrorCode)
	{
		switch (CookFrameErrorCode)
		{
			case ECookFrameErrorCode::Success: return TEXT("Success");
			case ECookFrameErrorCode::BadRequest: return TEXT("BadRequest");
			case ECookFrameErrorCode::Replaced: return TEXT("Replaced");;
			case ECookFrameErrorCode::Cancelled: return TEXT("Cancelled");
			case ECookFrameErrorCode::FailedToStartCook: return TEXT("FailedToStartCook");
			case ECookFrameErrorCode::InternalTouchEngineError: return TEXT("InternalTouchEngineError");
			case ECookFrameErrorCode::TEFrameCancelled: return TEXT("TEFrameCancelled");
			case ECookFrameErrorCode::InputBufferLimitReached: return TEXT("InputBufferLimitReached");
			case ECookFrameErrorCode::Count: return TEXT("Count");
			default: return TEXT("[default]");
		}
	}
	
	struct TOUCHENGINE_API FCookFrameResult
	{
		ECookFrameErrorCode ErrorCode;
		TEResult TouchEngineInternalResult;

		FTouchEngineInputFrameData FrameData;

		TArray<FTextureCreationFormat> UTexturesToBeCreatedOnGameThread;

		/** Promise the caller need to set when they are done with the data and we could safely start the next. This does not start a next cook. Its is only set when the cook is done and could be null */
		TSharedPtr<TPromise<void>> CanStartNextCook; 


		static FCookFrameResult FromCookFrameRequest(const FCookFrameRequest& CookRequest, ECookFrameErrorCode ErrorCode, TEResult TouchEngineInternalResult)
		{
			return {ErrorCode, TouchEngineInternalResult, CookRequest.FrameData};
		}
		static FCookFrameResult FromCookFrameRequest(const FCookFrameRequest& CookRequest, ECookFrameErrorCode ErrorCode)
		{
			TEResult Result;
			switch (ErrorCode) {
			case ECookFrameErrorCode::Success: Result = TEResultSuccess; break;
			case ECookFrameErrorCode::BadRequest: Result = TEResultBadUsage; break;
			case ECookFrameErrorCode::Replaced: Result = TEResultCancelled; break;
			case ECookFrameErrorCode::Cancelled: Result = TEResultCancelled; break;
			case ECookFrameErrorCode::FailedToStartCook: Result = TEResultInternalError; break;
			case ECookFrameErrorCode::InternalTouchEngineError: Result = TEResultInternalError; break;
			case ECookFrameErrorCode::TEFrameCancelled: Result = TEResultCancelled; break;
			case ECookFrameErrorCode::InputBufferLimitReached: Result = TEResultCancelled; break;
			case ECookFrameErrorCode::Count: Result = TEResultBadUsage; break;
			default: Result = TEResultBadUsage; break;
			}
			return FromCookFrameRequest(CookRequest, ErrorCode, Result);
		}
	};
}