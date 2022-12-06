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
#include "Engine/Util/CookFrameData.h"
#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TouchObject.h"

class FScopeLock;

namespace UE::TouchEngine
{
	class FTouchVariableManager;

	class FTouchFrameCooker
	{
	public:

		FTouchFrameCooker(TouchObject<TEInstance> TouchEngineInstance, FTouchVariableManager& VariableManager);
		~FTouchFrameCooker();

		void SetTimeMode(TETimeMode InTimeMode) { TimeMode = InTimeMode; }
		
		TFuture<FCookFrameResult> CookFrame_GameThread(const FCookFrameRequest& CookFrameRequest);
		void OnFrameFinishedCooking(TEResult Result);
		void CancelCurrentAndNextCook();

		bool IsCookingFrame() const { return InProgressFrameCook.IsSet(); }
		
	private:

		class FPendingFrameCook : public FCookFrameRequest
		{
			uint64 CookFrameNumber;
			TPromise<FCookFrameResult> PendingPromise;
		public:
			
			const FDateTime JobCreationTime = FDateTime::Now();

			FPendingFrameCook(const FCookFrameRequest& Request, uint64 CookFrameNumber)
				: FCookFrameRequest(Request)
				, CookFrameNumber(CookFrameNumber)
			{}

			void Combine(FPendingFrameCook&& NewRequest)
			{
				ensureAlwaysMsgf(TimeScale == NewRequest.TimeScale, TEXT("Changing time scale is not supported. You'll get weird results."));
				// 1 Keep the old JobCreationTime because our job has not yet started - we're just updating it
				// 2 Keep the old FrameTime_Mill because it will implicitly be included when we compute the time elapsed since JobCreationTime
				CookFrameNumber = FMath::Max(CookFrameNumber, NewRequest.CookFrameNumber);
				EmplacePromise(ECookFrameErrorCode::Replaced);
				PendingPromise = MoveTemp(NewRequest.PendingPromise);
			}

			TFuture<FCookFrameResult> GetFuture() { return PendingPromise.GetFuture(); }
			
			void EmplacePromise(ECookFrameErrorCode ErrorCode)
			{
				PendingPromise.SetValue(FCookFrameResult{ ErrorCode, CookFrameNumber });
			}
		};
		
		TouchObject<TEInstance>	TouchEngineInstance;
		FTouchVariableManager& VariableManager;
		
		TETimeMode TimeMode = TETimeInternal;
		
		int64 AccumulatedTime = 0;
		/** Increased every time CookFrame_GameThread is called and acts as unique ID for cook commands. */
		uint64 CookFrameNumber = 0;

		/** Must be obtained to read or write InProgressFrameCook and NextFrameCook. */
		FCriticalSection PendingFrameMutex;
		/** The cook frame request that is currently in progress if any. */
		TOptional<FPendingFrameCook> InProgressFrameCook;
		/** The next frame cook to execute after InProgressFrameCook is done. If multiple cooks are requested, the current value is combined with the latest request. */
		TOptional<FPendingFrameCook> NextFrameCook;

		void EnqueueCookFrame(FPendingFrameCook&& CookRequest);
		void ExecuteCurrentCookFrame(FPendingFrameCook&& CookRequest, FScopeLock& Lock);
		void FinishCurrentCookFrameAndExecuteNextCookFrame(ECookFrameErrorCode ErrorCode);
	};
}


