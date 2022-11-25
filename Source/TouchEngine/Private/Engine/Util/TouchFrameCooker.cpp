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

#include "TouchFrameCooker.h"

#include "Logging.h"
#include "Engine/Util/CookFrameData.h"
#include "Engine/Util/TouchVariableManager.h"
#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TEResult.h"

namespace UE::TouchEngine
{
	FTouchFrameCooker::FTouchFrameCooker(TouchObject<TEInstance> TouchEngineInstance, FTouchVariableManager& VariableManager)
	: TouchEngineInstance(MoveTemp(TouchEngineInstance))
	, VariableManager(VariableManager)
	{
		check(this->TouchEngineInstance);
	}

	FTouchFrameCooker::~FTouchFrameCooker()
	{
		UE_LOG(LogTouchEngine, Verbose, TEXT("Shutting down ~FTouchFrameCooker"));
		
		// Set TouchEngineInstance to nullptr in case any of the callbacks triggers below cause a CookFrame_GameThread call
		TouchEngineInstance.set(nullptr);

		CancelCurrentAndNextCook();
	}

	TFuture<FCookFrameResult> FTouchFrameCooker::CookFrame_GameThread(const FCookFrameRequest& CookFrameRequest)
	{
		check(IsInGameThread());

		const bool bIsInDestructor = TouchEngineInstance.get() == nullptr; 
		if (bIsInDestructor)
		{
			return MakeFulfilledPromise<FCookFrameResult>(FCookFrameResult{ ECookFrameErrorCode::BadRequest }).GetFuture();
		}
		
		FPendingFrameCook PendingCook { CookFrameRequest };
		TFuture<FCookFrameResult> Future = PendingCook.PendingPromise.GetFuture();
		// We expect all input textures to have been submitted and be in progress already... even if a cook is in progress already
		// start waiting for input textures now so we do not wind up waiting on newer input textures when the in progress request is eventually done. 
		VariableManager.OnFinishAllTextureUpdatesUpTo(VariableManager.GetNextTextureUpdateId())
			.Next([this, PendingCook = MoveTemp(PendingCook)](FFinishTextureUpdateInfo Info) mutable
			{
				// VariableManager will be or has been destroyed. Do not kick off any more tasks and do not dereference "this"!
				if (Info.ErrorCode == ETextureUpdateErrorCode::Cancelled)
				{
					PendingCook.PendingPromise.SetValue(FCookFrameResult{ ECookFrameErrorCode::Cancelled });
					return;
				}
				
				FScopeLock Lock(&PendingFrameMutex);
				if (InProgressFrameCook)
				{
					EnqueueCookFrame(MoveTemp(PendingCook));
				}
				else
				{
					ExecuteCurrentCookFrame(MoveTemp(PendingCook), Lock);
				}
			});

		return Future;
	}

	void FTouchFrameCooker::OnFrameFinishedCooking(TEResult Result)
	{
		ECookFrameErrorCode ErrorCode;
		switch (Result)
		{
		case TEResultSuccess: ErrorCode = ECookFrameErrorCode::Success; break;
		case TEResultCancelled: ErrorCode = ECookFrameErrorCode::TEFrameCancelled; break;
		default:
			ErrorCode = ECookFrameErrorCode::InternalTouchEngineError;
		}
		FinishCurrentCookFrameAndExecuteNextCookFrame(FCookFrameResult{ ErrorCode });
	}

	void FTouchFrameCooker::CancelCurrentAndNextCook()
	{
		FScopeLock Lock(&PendingFrameMutex);
		if (InProgressFrameCook)
		{
			InProgressFrameCook->PendingPromise.SetValue(FCookFrameResult{ ECookFrameErrorCode::Cancelled });
			InProgressFrameCook.Reset();

			TEInstanceCancelFrame(TouchEngineInstance);
		}
		if (NextFrameCook)
		{
			NextFrameCook->PendingPromise.SetValue(FCookFrameResult{ ECookFrameErrorCode::Cancelled });
			NextFrameCook.Reset();
		}
	}

	void FTouchFrameCooker::EnqueueCookFrame(FPendingFrameCook&& CookRequest)
	{
		if (NextFrameCook.IsSet())
		{
			NextFrameCook->Combine(MoveTemp(CookRequest));
		}
		else
		{
			NextFrameCook.Emplace(MoveTemp(CookRequest));
		}
	}

	void FTouchFrameCooker::ExecuteCurrentCookFrame(FPendingFrameCook&& CookRequest, FScopeLock& Lock)
	{
		check(!InProgressFrameCook.IsSet());
		
		// We may have waited for a short time so the start time should be the requested plus when we started
		CookRequest.FrameTime_Mill += CookRequest.TimeScale * (FDateTime::Now() - CookRequest.JobCreationTime).GetTotalMilliseconds();
		InProgressFrameCook.Emplace(MoveTemp(CookRequest)); // Note that MoveTemp is needed again because CookRequest is a l-value (to a temporary object!)
		
		// This is unlocked before calling TEInstanceStartFrameAtTime in case for whatever reason it finishes cooking the frame instantly. That would cause a deadlock.
		Lock.Unlock();
		
		TEResult Result = (TEResult)0;
		//FlushRenderingCommands();
		switch (TimeMode)
		{
		case TETimeInternal:
			{
				Result = TEInstanceStartFrameAtTime(TouchEngineInstance, 0, 0, false);
				break;
			}
		case TETimeExternal:
			{
				AccumulatedTime += InProgressFrameCook->FrameTime_Mill;
				Result = TEInstanceStartFrameAtTime(TouchEngineInstance, AccumulatedTime, InProgressFrameCook->TimeScale, false);
				break;
			}
		}

		const bool bSuccess = Result == TEResultSuccess;
		if (!bSuccess)
		{
			// This will reacquire a lock - a bit meh but should not happen often
			FinishCurrentCookFrameAndExecuteNextCookFrame(FCookFrameResult{ ECookFrameErrorCode::FailedToStartCook });
		}
	}

	void FTouchFrameCooker::FinishCurrentCookFrameAndExecuteNextCookFrame(FCookFrameResult Result)
	{
		FScopeLock Lock(&PendingFrameMutex);

		// Might have gotten cancelled
		if (InProgressFrameCook.IsSet())
		{
			InProgressFrameCook->PendingPromise.SetValue(Result);
			InProgressFrameCook.Reset();
		}

		if (NextFrameCook)
		{
			FPendingFrameCook CookRequest = MoveTemp(*NextFrameCook);
			NextFrameCook.Reset();
			ExecuteCurrentCookFrame(MoveTemp(CookRequest), Lock);
		}
	}
}

