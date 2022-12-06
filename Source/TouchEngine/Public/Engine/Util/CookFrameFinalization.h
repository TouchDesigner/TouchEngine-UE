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
#include "CookFrameData.h"

namespace UE::TouchEngine
{
	enum class ECookFrameFinalizationErrorCode
	{
		Success,

		/** Request is invalid */
		RequestInvalid,
		
		Count
	};

	/** The result of  */
	struct TOUCHENGINE_API FCookFrameFinalizedResult
	{
		ECookFrameErrorCode CookFrameErrorCode;
		ECookFrameFinalizationErrorCode FinalizationErrorCode;
		uint64 FrameNumber;

		FCookFrameFinalizedResult(ECookFrameErrorCode CookFrameErrorCode, ECookFrameFinalizationErrorCode ErrorCode, uint64 FrameNumber)
			: CookFrameErrorCode(CookFrameErrorCode)
			, FinalizationErrorCode(ErrorCode)
			, FrameNumber(FrameNumber)
		{}
	};
}
