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

struct FTouchEngineDynamicVariableStruct;

namespace UE::TouchEngine
{
	struct TOUCHENGINE_API FTouchLoadSuccessResult
	{
		TArray<FTouchEngineDynamicVariableStruct> Inputs;
		TArray<FTouchEngineDynamicVariableStruct> Outputs;
	};
	
	struct TOUCHENGINE_API FTouchLoadErrorResult
	{
		FString ErrorMessage;
	};

	/** Implements an Either monad.  */
	struct TOUCHENGINE_API FTouchLoadResult
	{
		TOptional<FTouchLoadSuccessResult> SuccessResult;
		TOptional<FTouchLoadErrorResult> FailureResult;

		bool IsSuccess() const { return SuccessResult.IsSet(); }
		bool IsFailure() const { return FailureResult.IsSet(); }

		static FTouchLoadResult MakeSuccess(TArray<FTouchEngineDynamicVariableStruct> Inputs, TArray<FTouchEngineDynamicVariableStruct> Outputs)
		{
			return { FTouchLoadSuccessResult{ MoveTemp(Inputs), MoveTemp(Outputs) }, {} };
		}

		static FTouchLoadResult MakeFailure(FString ErrorMessage)
		{
			return {{}, FTouchLoadErrorResult{ MoveTemp(ErrorMessage) } };
		}
	};
}
