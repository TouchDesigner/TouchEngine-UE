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
		static constexpr int64 FIRST_FRAME_ID = 1;
		
		FTouchFrameCooker(TouchObject<TEInstance> InTouchEngineInstance, FTouchVariableManager& InVariableManager, FTouchResourceProvider& InResourceProvider);
		~FTouchFrameCooker();

		void SetTimeMode(TETimeMode InTimeMode) { TimeMode = InTimeMode; }

		TFuture<FCookFrameResult> CookFrame_GameThread(FCookFrameRequest&& CookFrameRequest, int32 InputBufferLimit);
		bool ExecuteNextPendingCookFrame_GameThread();
		/**
		 * @brief 
		 * @param Result The Result Returned by TouchEngine
		 * @param bInWasFrameDropped Will be true if TouchEngine did not process the cook and therefore did not update the variables.
		 */
		void OnFrameFinishedCooking_AnyThread(TEResult Result, bool bInWasFrameDropped, double CookStartTime, double CookEndTime);
		void CancelCurrentAndNextCooks(ECookFrameResult CookFrameResult = ECookFrameResult::Cancelled);
		/**
		 * Cancel the current Frame if it matches the given FrameID
		 * @param FrameID The FrameID of the Frame to cancel. As parts of the code is asynchronous, this is to ensure we are cancelling the right frame. Pass -1 to cancel the current frame
		 * @param CookFrameResult The Result to give back to the user
		 * @return Returns true if the frame with the GivenID was cancelled
		 */
		bool CancelCurrentFrame_GameThread(int64 FrameID, ECookFrameResult CookFrameResult = ECookFrameResult::Cancelled);
		bool CheckIfCookTimedOut_GameThread(double CookTimeoutInSeconds);

		/** Returns the FrameID to be used for the next cook. */
		int64 GetNextFrameID() const { return NextFrameID; }

		/** Gets the last FrameID at which we received some outputs from TouchEngine. Returns -1 if we have not received outputs yet */
		int64 GetFrameLastUpdated() const { return FrameLastUpdated; }

		bool IsCookingFrame() const { return InProgressFrameCook.IsSet(); }
		/** returns the FrameID of the current cooking frame, or -1 if no frame is cooking */
		int64 GetCookingFrameID() const { return InProgressFrameCook.IsSet() ? InProgressFrameCook->FrameData.FrameID : -1; }

		void ProcessLinkTextureValueChanged_AnyThread(const char* Identifier);
		void ResetTouchEngineInstance();

	private:
		/** The FrameID that will be used for the next cook. Is increased after a cook is started */
		int64 NextFrameID = FIRST_FRAME_ID;
		double FirstFrameStartTime = -1.0;
		
		struct FPendingFrameCook : FCookFrameRequest
		{
			/* The time at which the job was created */
			FDateTime JobCreationTime = FDateTime::Now();
			/* The time at which the job was started by calling TEInstanceStartFrameAtTime. Used to check the Timeout */
			FDateTime JobStartTime = FDateTime::MinValue();
			/* As we are waiting for textures to be available in RenderThread before sending the cook to TE, the cook might not have actually started yet */
			bool bWasJobSentToTouchEngine = false;
			TPromise<FCookFrameResult> PendingCookPromise;
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
		TArray<FPendingFrameCook> PendingCookQueue;
		FCriticalSection PendingCookQueueMutex;

		/**
		 * Enqueue the given Cook Request to be processed. There should be a lock to PendingCookQueueMutex before calling this function.
		 * @param CookRequest The Request to enqueue
		 * @param InputBufferLimit The maximum number of cooks to hold in the queue. The older ones will be cancelled if the queue reach this limit
		 */
		void EnqueueCookFrame(FPendingFrameCook&& CookRequest, int32 InputBufferLimit);
		bool ExecuteNextPendingCookFrame_GameThread(FScopeLock& PendingFrameMutexLock);
		void FinishCurrentCookFrame_AnyThread();
	};
}


