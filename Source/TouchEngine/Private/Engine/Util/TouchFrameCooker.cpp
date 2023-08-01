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
#include "Engine/TEDebug.h"
#include "Engine/Util/CookFrameData.h"
#include "Engine/Util/TouchVariableManager.h"
#include "Rendering/TouchResourceProvider.h"
#include "Rendering/Importing/TouchTextureImporter.h"
#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TEResult.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/TouchHelpers.h"

namespace UE::TouchEngine
{
	FTouchFrameCooker::FTouchFrameCooker(TouchObject<TEInstance> InTouchEngineInstance, FTouchVariableManager& InVariableManager, FTouchResourceProvider& InResourceProvider)
	: TouchEngineInstance(MoveTemp(InTouchEngineInstance))
	, VariableManager(InVariableManager)
	, ResourceProvider(InResourceProvider)
	{
		check(this->TouchEngineInstance);
	}

	FTouchFrameCooker::~FTouchFrameCooker()
	{
		UE_LOG(LogTouchEngine, Verbose, TEXT("Shutting down ~FTouchFrameCooker"));
		
		// Set TouchEngineInstance to nullptr in case any of the callbacks triggers below cause a CookFrame_GameThread call
		TouchEngineInstance.set(nullptr);

		CancelCurrentAndNextCooks();
	}

	TFuture<FCookFrameResult> FTouchFrameCooker::CookFrame_GameThread(FCookFrameRequest&& CookFrameRequest, int32 InputBufferLimit)
	{
		check(IsInGameThread());

		const bool bIsInDestructor = TouchEngineInstance.get() == nullptr;
		if (bIsInDestructor)
		{
			return MakeFulfilledPromise<FCookFrameResult>(FCookFrameResult::FromCookFrameRequest(CookFrameRequest, ECookFrameErrorCode::BadRequest, TEResultBadUsage)).GetFuture();
		}
		
		FPendingFrameCook PendingCook { MoveTemp(CookFrameRequest) };
		TFuture<FCookFrameResult> Future = PendingCook.PendingPromise.GetFuture();

		{
			FScopeLock Lock(&PendingFrameMutex);
			EnqueueCookFrame(MoveTemp(PendingCook), InputBufferLimit);
			ExecuteNextPendingCookFrame_GameThread(Lock);
		}
		
		return  Future;
	}

	void FTouchFrameCooker::OnFrameFinishedCooking(TEResult Result, bool bInWasFrameDropped)
	{
		ECookFrameErrorCode ErrorCode;
		switch (Result)
		{
		case TEResultSuccess: ErrorCode = ECookFrameErrorCode::Success; break;
		case TEResultCancelled: ErrorCode = ECookFrameErrorCode::Cancelled; break;
		default:
			ErrorCode = ECookFrameErrorCode::InternalTouchEngineError;
		}
		
		if (ErrorCode == ECookFrameErrorCode::Success && ensure(InProgressCookResult))
		{
			if (!bInWasFrameDropped || FrameLastUpdated == -1)
			{
				FrameLastUpdated = InProgressCookResult->FrameData.FrameID;
				bInWasFrameDropped = false;
			}
			InProgressCookResult->ErrorCode = ErrorCode;
			InProgressCookResult->TouchEngineInternalResult = Result;
			InProgressCookResult->bWasFrameDropped = bInWasFrameDropped;
			InProgressCookResult->FrameLastUpdated = FrameLastUpdated;

			FinishCurrentCookFrame_AnyThread();
		}
		else
		{
			CancelCurrentAndNextCooks();
		}
	}

	void FTouchFrameCooker::CancelCurrentAndNextCooks()
	{
		FScopeLock Lock(&PendingFrameMutex);
		if (InProgressFrameCook)
		{
			if (InProgressCookResult) //if we still have a cook result, that means that we haven't set the promise yet
			{
				InProgressFrameCook->PendingPromise.SetValue(FCookFrameResult{ECookFrameErrorCode::Cancelled, TEResultCancelled, InProgressCookResult->FrameData});
				InProgressCookResult.Reset();
			}
			InProgressFrameCook.Reset();
			
			TEInstanceCancelFrame(TouchEngineInstance);
		}
		
		while (!PendingCookQueue.IsEmpty())
		{
			FPendingFrameCook NextFrameCook = PendingCookQueue.Pop();
			NextFrameCook.PendingPromise.SetValue(FCookFrameResult::FromCookFrameRequest(NextFrameCook, ECookFrameErrorCode::Cancelled));
		}
	}

	void FTouchFrameCooker::ProcessLinkTextureValueChanged_AnyThread(const char* Identifier)
	{
		using namespace UE::TouchEngine;
		
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("  III.A [AT] ProcessLink"), STAT_TE_III_A, STATGROUP_TouchEngine);
		// Stash the state, we don't do any actual renderer work from this thread
		TouchObject<TETexture> Texture = nullptr;
		const TEResult Result = TEInstanceLinkGetTextureValue(TouchEngineInstance, Identifier, TELinkValueCurrent, Texture.take());
		if (Result != TEResultSuccess)
		{
			return;
		}

		// Do not create any more values until we've processed this one (better performance)
		TEInstanceLinkSetInterest(TouchEngineInstance, Identifier, TELinkInterestSubsequentValues);

		const FName ParamId(Identifier);
		VariableManager.AllocateLinkedTop(ParamId); // Avoid system querying this param from generating an output error

		const FTouchImportParameters LinkParams{ TouchEngineInstance, ParamId, Texture, InProgressFrameCook.IsSet() ? InProgressFrameCook->FrameData : FTouchEngineInputFrameData() };
		
		// below calls FTouchTextureImporter::ImportTexture_AnyThread for DX12
		TSharedRef<FTouchFrameCooker> This = SharedThis(this);
		ResourceProvider.ImportTextureToUnrealEngine_AnyThread(LinkParams, This)
			.Next([ParamId, &VariableManager = VariableManager, FrameID = LinkParams.FrameData.FrameID](const FTouchTextureImportResult& TouchLinkResult)
			{
				// InProgressCookResult is not available anymore at that point
				UTexture2D* ExistingTextureToBePooled = nullptr;
				if (TouchLinkResult.ResultType == EImportResultType::Success)
				{
					UTexture2D* Texture = TouchLinkResult.ConvertedTextureObject.GetValue();
					UE_LOG(LogTouchEngine, Verbose, TEXT("[ImportTextureToUnrealEngine_AnyThread.Next[%s]] Calling `UpdateLinkedTOP` for Identifier `%s` for frame %lld"),
						*GetCurrentThreadStr(), *ParamId.ToString(), FrameID)
					ExistingTextureToBePooled = VariableManager.UpdateLinkedTOP(ParamId, Texture);
				}
				if (TouchLinkResult.PreviousTextureToBePooledPromise)
				{
					TouchLinkResult.PreviousTextureToBePooledPromise->SetValue(ExistingTextureToBePooled);
				}
			});
	}
	
	void FTouchFrameCooker::EnqueueCookFrame(FPendingFrameCook&& CookRequest, int32 InputBufferLimit)
	{
		UE_LOG(LogTouchEngine, Log, TEXT("[EnqueueCookFrame[%s]] Enqueing Cook for frame %lld (%d cooks currently in the queue, InputBufferLimit is %d )"),
			*GetCurrentThreadStr(), CookRequest.FrameData.FrameID, PendingCookQueue.Num(), InputBufferLimit)
		
		if (InputBufferLimit >= 0)
		{
			// here we remove one more item than the buffer limit as we are going to add the given CookRequest
			while (!PendingCookQueue.IsEmpty() && PendingCookQueue.Num() >= InputBufferLimit)
			{
				FPendingFrameCook CookToCancel = PendingCookQueue.Pop();
				UE_LOG(LogTouchEngine, Log, TEXT("[EnqueueCookFrame[%s]]   Cancelling Cook for frame %lld (%d cooks currently in the queue, InputBufferLimit is %d )"),
					*GetCurrentThreadStr(), CookToCancel.FrameData.FrameID, PendingCookQueue.Num(), InputBufferLimit)
				
				CookToCancel.PendingPromise.SetValue(FCookFrameResult::FromCookFrameRequest(CookToCancel, ECookFrameErrorCode::InputDropped));
			}
			
			if (InputBufferLimit == 0 && InProgressFrameCook)
			{
				// Particular case if we don't want to queue anything, only add to the queue if there is no ongoing cook
				// which will end up processing this CookRequest right after this function is called
				UE_LOG(LogTouchEngine, Log, TEXT("[EnqueueCookFrame[%s]]   Cancelling Cook for frame %lld (%d cooks currently in the queue, InputBufferLimit is %d )"),
					*GetCurrentThreadStr(), CookRequest.FrameData.FrameID, PendingCookQueue.Num(), InputBufferLimit)
				CookRequest.PendingPromise.SetValue(FCookFrameResult::FromCookFrameRequest(CookRequest, ECookFrameErrorCode::InputDropped));
				return;
			}
		}
		// if InputBufferLimit is less than 0, we consider that there is no limit to the queue
		
		PendingCookQueue.Insert(MoveTemp(CookRequest), 0); // We enqueue at the start so we can easily use Pop to get the last element
	}

	bool FTouchFrameCooker::ExecuteNextPendingCookFrame_GameThread()
	{
		FScopeLock Lock(&PendingFrameMutex);
		return ExecuteNextPendingCookFrame_GameThread(Lock);
	}

	bool FTouchFrameCooker::ExecuteNextPendingCookFrame_GameThread(FScopeLock& Lock)
	{
		if (InProgressFrameCook || PendingCookQueue.IsEmpty())
		{
			return false;
		}
		
		TEResult Result = (TEResult)0;
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("  I.B [GT] Cook Frame"), STAT_TE_I_B, STATGROUP_TouchEngine);
			FPendingFrameCook CookRequest = PendingCookQueue.Pop();

			UE_LOG(LogTouchEngine, Log, TEXT("  --------- [FTouchFrameCooker::ExecuteCurrentCookFrame[%s]] Executing the cook for the frame %lld [Requested during frame %lld, Queue: %d cooks waiting] ---------"),
			       *GetCurrentThreadStr(), CookRequest.FrameData.FrameID, GetLatestCookNumber(), PendingCookQueue.Num())

			// 1. First, we prepare the inputs to send
			{
				UE_LOG(LogTouchEngine, Verbose, TEXT("[ExecuteCurrentCookFrame[%s]] Calling `DynamicVariables.SendInputs` for frame %lld"),
				       *GetCurrentThreadStr(), CookRequest.FrameData.FrameID)
				CookRequest.DynamicVariables.SendInputs(VariableManager, CookRequest.FrameData); // this needs to be on GameThread as this might end up calling UTexture functions that need to be called on the GameThread
				CookRequest.DynamicVariables.Reset();
				ResourceProvider.FinalizeExportsToTouchEngine_AnyThread(CookRequest.FrameData);
			}

			InProgressCookResult.Reset();
			InProgressCookResult = FCookFrameResult();
			InProgressCookResult->FrameData = CookRequest.FrameData;

			// We may have waited for a short time so the start time should be the requested plus when we started
			CookRequest.FrameTimeInSeconds += (FDateTime::Now() - CookRequest.JobCreationTime).GetTotalSeconds(); // * CookRequest.TimeScale ;
			InProgressFrameCook.Emplace(MoveTemp(CookRequest));

			// This is unlocked before calling TEInstanceStartFrameAtTime in case for whatever reason it finishes cooking the frame instantly. That would cause a deadlock.
			
			Lock.Unlock();

			switch (TimeMode)
			{
			case TETimeInternal:
				{
					Result = TEInstanceStartFrameAtTime(TouchEngineInstance, 0, 0, false);
					if (Result == TEResultSuccess)
					{
						UE_LOG(LogTouchEngine, Log, TEXT("TEInstanceStartFrameAtTime[%s] (TETimeInternal) for frame `%lld`:  Time: %d  TimeScale: %d => %s"), *GetCurrentThreadStr(), InProgressCookResult->FrameData.FrameID, 0, 0, *TEResultToString(Result));
					}
					else
					{
						UE_LOG(LogTouchEngine, Error, TEXT("TEInstanceStartFrameAtTime[%s] (TETimeInternal) for frame `%lld`:  Time: %d  TimeScale: %d => %s (`%hs`)"), *GetCurrentThreadStr(), InProgressCookResult->FrameData.FrameID, 0, 0, *TEResultToString(Result), TEResultGetDescription(Result));
					}
					break;
				}
			case TETimeExternal:
				{
					AccumulatedTime += InProgressFrameCook->FrameTimeInSeconds * InProgressFrameCook->TimeScale ;
					Result = TEInstanceStartFrameAtTime(TouchEngineInstance, AccumulatedTime, InProgressFrameCook->TimeScale, false);
					UE_LOG(LogTouchEngineTECalls, Warning, TEXT("====TEInstanceStartFrameAtTime with time_value `%lld` and time_scale `%lld` for CookingFrame `%lld`"),
										AccumulatedTime, InProgressFrameCook->TimeScale, InProgressCookResult->FrameData.FrameID)
					if (Result == TEResultSuccess)
					{
						UE_LOG(LogTouchEngine, Log, TEXT("TEInstanceStartFrameAtTime[%s] (TETimeExternal) for frame `%lld`:  Time: %lld  TimeScale: %lld => %s"), *GetCurrentThreadStr(), InProgressCookResult->FrameData.FrameID, AccumulatedTime, InProgressFrameCook->TimeScale, *TEResultToString(Result));
					}
					else
					{
						UE_LOG(LogTouchEngine, Error, TEXT("TEInstanceStartFrameAtTime[%s] (TETimeExternal) for frame `%lld`:  Time: %lld  TimeScale: %lld => %s (`%hs`)"), *GetCurrentThreadStr(), InProgressCookResult->FrameData.FrameID, AccumulatedTime, InProgressFrameCook->TimeScale, *TEResultToString(Result), TEResultGetDescription(Result));
					}
					break;
				}
			}
		}

		const bool bSuccess = Result == TEResultSuccess;
		if (!bSuccess) //if we are successful, FTouchEngine::TouchEventCallback_AnyThread will be called with the event TEEventFrameDidFinish, and OnFrameFinishedCooking will be called
		{
			// This will reacquire a lock - a bit meh but should not happen often
			InProgressCookResult->ErrorCode = ECookFrameErrorCode::FailedToStartCook;
			FinishCurrentCookFrame_AnyThread();
		}
		return true;
	}

	void FTouchFrameCooker::FinishCurrentCookFrame_AnyThread() //todo: rename
	{
		UE_LOG(LogTouchEngine, Log, TEXT("FinishCurrentCookFrame_AnyThread[%s]"), *GetCurrentThreadStr())
		FScopeLock Lock(&PendingFrameMutex);
		if (InProgressFrameCook.IsSet())
		{
			InProgressCookResult->OnReadyToStartNextCook = MakeShared<TPromise<void>>();
			InProgressCookResult->OnReadyToStartNextCook->GetFuture().Next([WeakThis = AsWeak(), FrameData = InProgressCookResult->FrameData](int)
			{
				if (const TSharedPtr<FTouchFrameCooker> SharedThis = WeakThis.Pin())
				{
					FScopeLock Lock(&SharedThis->PendingFrameMutex);
					SharedThis->InProgressFrameCook.Reset();
					SharedThis->InProgressCookResult.Reset();
					SharedThis->ResourceProvider.GetImporter().TexturePoolMaintenance(FrameData);
				}
			});
			InProgressFrameCook->PendingPromise.SetValue(*InProgressCookResult);
			InProgressCookResult.Reset(); // to be sure not to try to set it again if we cancel
		}
		else
		{
			InProgressFrameCook.Reset();
			InProgressCookResult.Reset();
		}
	}
}

