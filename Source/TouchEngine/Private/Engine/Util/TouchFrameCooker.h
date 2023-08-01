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
#include "Engine/Util/TouchVariableManager.h"
#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TouchObject.h"

class FScopeLock;

namespace UE::TouchEngine
{
	class FTouchVariableManager;

	/**
	 * The Frame Cooker is responsible for cooking the frame and handling the different frame cooking callbacks 
	 */
	class FTouchFrameCooker : public TSharedFromThis<FTouchFrameCooker>
	{
	public:

		FTouchFrameCooker(TouchObject<TEInstance> InTouchEngineInstance, FTouchVariableManager& InVariableManager, FTouchResourceProvider& InResourceProvider);
		~FTouchFrameCooker();

		void SetTimeMode(TETimeMode InTimeMode) { TimeMode = InTimeMode; }

		TFuture<FCookFrameResult> CookFrame_GameThread(FCookFrameRequest&& CookFrameRequest, int32 InputBufferLimit); //todo: probably doesn't need to be a future
		bool ExecuteNextPendingCookFrame_GameThread();
		/**
		 * @brief 
		 * @param Result The Result Returned by TouchEngine
		 * @param bInWasFrameDropped Will be true if TouchEngine did not process the cook and therefore did not update the variables.
		 */
		void OnFrameFinishedCooking(TEResult Result, bool bInWasFrameDropped);
		void CancelCurrentAndNextCooks();

		/** Gets the latest CookNumber that was requested. Should be 0 if not started */
		int64 GetLatestCookNumber() const { return FrameCookNumber; }
		/** Increments the CookNumber and returns the new CookNumber */
		int64 IncrementCookNumber() { return ++FrameCookNumber; }
		
		bool IsCookingFrame() const { return InProgressFrameCook.IsSet(); }
		/** returns the FrameID of the current cooking frame, or -1 if no frame is cooking */
		int64 GetCookingFrameID() const { return InProgressFrameCook.IsSet() ? InProgressFrameCook->FrameData.FrameID : -1; }

		void ProcessLinkTextureValueChanged_AnyThread(const char* Identifier);

	private:
		int64 FrameCookNumber = 0;
		
		struct FPendingFrameCook : FCookFrameRequest
		{
			FDateTime JobCreationTime = FDateTime::Now();
			TPromise<FCookFrameResult> PendingPromise;
		};
		
		TouchObject<TEInstance>	TouchEngineInstance;
		FTouchVariableManager& VariableManager;
		/** The Resource provider is used to handle Texture callbacks */
		FTouchResourceProvider& ResourceProvider;
		
		TETimeMode TimeMode = TETimeInternal;
		int64 AccumulatedTime = 0;

		/** The last frame we receive a successful cook that was not skipped */
		int64 FrameLastUpdated = -1;

		/** Must be obtained to read or write InProgressFrameCook. */
		FCriticalSection PendingFrameMutex;
		/** The cook frame request that is currently in progress if any. */
		TOptional<FPendingFrameCook> InProgressFrameCook;
		/** The cook frame result for the frame in progress, if any. */
		TOptional<FCookFrameResult> InProgressCookResult;
		
		/** The next frame cooks to execute after InProgressFrameCook is done. Implemented as Array to have access to size and keep FPendingFrameCook.Promise not shared*/
		TArray<FPendingFrameCook> PendingCookQueue; //todo: probably an issue with pulse type variables as we are copying before setting the value
		FCriticalSection PendingCookQueueMutex;

		/**
		 * Enqueue the given Cook Request to be processed. There should be a lock to PendingCookQueueMutex before calling this function.
		 * @param CookRequest The Request to enqueue
		 * @param InputBufferLimit The maximum number of cooks to hold in the queue. The older ones will be cancelled if the queue reach this limit
		 */
		void EnqueueCookFrame(FPendingFrameCook&& CookRequest, int32 InputBufferLimit);
		bool ExecuteNextPendingCookFrame_GameThread(FScopeLock& Lock);
		void FinishCurrentCookFrame_AnyThread();
	};
}


