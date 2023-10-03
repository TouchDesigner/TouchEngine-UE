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
#include "Misc/Optional.h"
#include "Rendering/TouchSuspendResult.h"

#include <atomic>

namespace UE::TouchEngine
{
	/** Helps you track latent tasks. Provides a Suspend function which executes the returned future when all running tasks are done. */
	class FTaskSuspender
	{
	public:

		using FTaskTracker = TSharedPtr<void>;

		/** Increments task count. Decrements it when the task is destroyed. */
		FTaskTracker StartTask()
		{
			check(!IsSuspended());
			
			++NumberTasks;
			TSharedPtr<void> Tracker = MakeShareable<void>(nullptr, [this](auto)
			{
				--NumberTasks;

				if (NumberTasks == 0 && IsSuspended() && SuspensionPromise)
				{
					SuspensionPromise->SetValue(FTouchSuspendResult{});
					SuspensionPromise.Reset();
				}
			});
			return Tracker;
		}
		
		TFuture<FTouchSuspendResult> Suspend()
		{
			FScopeLock Lock(&PromiseMutex);
			check(!bWasSuspended);
			bWasSuspended = true;
			
			if (NumberTasks == 0)
			{
				return MakeFulfilledPromise<FTouchSuspendResult>(FTouchSuspendResult{}).GetFuture();
			}
			
			TPromise<FTouchSuspendResult> Promise;
			TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
			SuspensionPromise = MoveTemp(Promise);
			return Future;
		}

		bool IsSuspended() const
		{
			if (!bWasSuspended)
			{
				FScopeLock Lock(&PromiseMutex);
				return bWasSuspended;
			}
			return true;
		}
		
	private:

		std::atomic<uint32> NumberTasks;
		mutable FCriticalSection PromiseMutex;
		
		bool bWasSuspended = false;
		TOptional<TPromise<FTouchSuspendResult>> SuspensionPromise;
	};
}
