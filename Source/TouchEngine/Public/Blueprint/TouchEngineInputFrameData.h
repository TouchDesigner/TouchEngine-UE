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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	int64 FrameID = -1; // cannot be a uint because it is exposed to BP

	/** The time at which the frame started. Only used to compute the tick latency of the matching FTouchEngineOutputFrameData */
	double StartTime = 0.0;
};

USTRUCT(BlueprintType)
struct FTouchEngineOutputFrameData
{
	GENERATED_BODY()

	/** The frame identifier which is unique for this component until it is restarted */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	int64 FrameID = -1; // cannot be a uint because it is exposed to BP

	/** The number of ticks it took since On Start Frame was called  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	int TickLatency = 0;
	/** The number of seconds it took since On Start Frame was called */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	double Latency = 0.0;

	/**
	 * As TouchEngine can run at a different framerate than UE, it is possible that TouchEngine is not ready to process a new cook when we try to,
	 * in which case the inputs would not be processed and the frame would be dropped, but the cook would still be successful even though the output values would have not changed.
	 * */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	bool bWasFrameDropped = false;
	/**
	 * The frame identifier of the last frame we received updated data from TouchEngine.
	 * As TouchEngine can run at a different framerate than UE, it is possible that TouchEngine is not ready to process a new cook when we try to,
	 * in which case the inputs would not be processed and the frame would be dropped, but the cook would still be successful even though the output values would have not changed.
	 * */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	int64 FrameLastUpdated = -1; // cannot be a uint because it is exposed to BP

	/** The internal start_time of this Cook returned by TouchEngine. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	double CookStartTime = 0.0;
	/** The internal end_time of this Cook returned by TouchEngine. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	double CookEndTime = 0.0;
};