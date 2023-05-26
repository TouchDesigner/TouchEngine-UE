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
#include "TouchEngineInputFrameData.generated.h"

USTRUCT(BlueprintType)
struct FTouchEngineInputFrameData
{
	GENERATED_BODY()

	/** The frame identifier which is unique for this component until it is restarted */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine", meta=(DisplayPriority=0))
	int64 FrameID; // cannot be a uint because it is exposed to BP

	/** The time at which the frame started. Only used to compute the tick latency of the matching FTouchEngineOutputFrameData */
	double StartTime;
};

USTRUCT(BlueprintType)
struct FTouchEngineOutputFrameData : public FTouchEngineInputFrameData
{
	GENERATED_BODY()

	/** The number of ticks it took to from SetInputs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine", meta=(DisplayAfter="FrameID"))
	int TickLatency;
	/** The number of seconds it took to from SetInputs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine", meta=(DisplayAfter="TickLatency"))
	double Latency;
};