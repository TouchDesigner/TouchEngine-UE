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
			return MakeFulfilledPromise<FCookFrameResult>(FCookFrameResult::FromCookFrameRequest(CookFrameRequest, ECookFrameResult::BadRequest, TEResultBadUsage)).GetFuture();
		}
		
		FPendingFrameCook PendingCook { MoveTemp(CookFrameRequest) };
		TFuture<FCookFrameResult> Future = PendingCook.PendingCookPromise.GetFuture();

		{
			FScopeLock Lock(&PendingFrameMutex);
			EnqueueCookFrame(MoveTemp(PendingCook), InputBufferLimit);
			++NextFrameID; // We increase the next cook number as soon as we have enqueued the previous set of inputs.
			ExecuteNextPendingCookFrame_GameThread(Lock);
		}
		
		return  Future;
	}

	void FTouchFrameCooker::OnFrameFinishedCooking_AnyThread(TEResult Result, bool bInWasFrameDropped, double CookStartTime, double CookEndTime)
	{
		ECookFrameResult CookResult;
		TESeverity Severity = TESeverityNone;
		switch (Result)
		{
		case TEResultSuccess: CookResult = ECookFrameResult::Success; break;
		case TEResultCancelled: CookResult = ECookFrameResult::Cancelled; break;
		default:
			Severity = TEResultGetSeverity(Result);
			CookResult = Severity == TESeverityError ? ECookFrameResult::InternalTouchEngineError : ECookFrameResult::Success; //todo: check with TD team
		}
		
		FScopeLock Lock(&PendingFrameMutex);
		if (ensure(InProgressCookResult))
		{
			InProgressCookResult->bWasFrameDropped = bInWasFrameDropped && FrameLastUpdated > -1; // if it is the first frame, we cannot consider it dropped
			if (CookResult == ECookFrameResult::Success && !InProgressCookResult->bWasFrameDropped) // if the cook was successful and the frame not dropped, we update the FrameLastUpdated
			{
				FrameLastUpdated = InProgressCookResult->FrameData.FrameID;
			}
			if (InProgressCookResult->Result == ECookFrameResult::Count) // The default Value is Count, so if it has been changed, we keep the overriden value 
			{
				InProgressCookResult->Result = CookResult;
			}
			InProgressCookResult->TouchEngineInternalResult = Result;
			InProgressCookResult->TECookStartTime = CookStartTime;
			InProgressCookResult->TECookEndTime = CookEndTime;
		}
		
		if ((CookResult == ECookFrameResult::Success || CookResult == ECookFrameResult::Cancelled) && ensure(InProgressCookResult))
		{
			FinishCurrentCookFrame_AnyThread();
		}
		else
		{
			CancelCurrentAndNextCooks(CookResult);
		}
	}

	void FTouchFrameCooker::CancelCurrentAndNextCooks(ECookFrameResult CookFrameResult)
	{
		FScopeLock Lock(&PendingFrameMutex);
		if (InProgressFrameCook)
		{
			if (InProgressCookResult)
			{
				InProgressCookResult->Result = CookFrameResult; // We set the result we want which will not be overriden
			}
			const int64 FrameID = InProgressFrameCook->FrameData.FrameID;
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("Calling TEInstanceCancelFrame for frame %lld..."), FrameID);
			const TEResult CancelResult = TEInstanceCancelFrame(TouchEngineInstance); // OnFrameFinishedCooking_AnyThread ends up being called before the following statements
			// After TEInstanceCancelFrame, InProgressFrameCook can be null at this point
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("...Called TEInstanceCancelFrame for frame %lld returned '%s'"), FrameID, *TEResultToString(CancelResult));
			// todo: is there any case where the above code would not end up raising TouchEventCallback_AnyThread? this could be an issue
			
			// if (!ensure(CancelResult == TEResultSuccess)) // TEInstanceCancelFrame would end up calling OnFrameFinishedCooking_AnyThread which would take care of the below, but if this did not happen we need to clean manually
			// {
			// 		const FCookFrameResult CookResult = FCookFrameResult::FromCookFrameRequest(InProgressFrameCook.GetValue(), CookFrameResult, FrameLastUpdated);
			// 		InProgressFrameCook->PendingCookPromise.SetValue(CookResult);
			// 		InProgressCookResult.Reset();
			// }
			// InProgressFrameCook.Reset();
		}
		
		while (!PendingCookQueue.IsEmpty())
		{
			FPendingFrameCook NextFrameCook = PendingCookQueue.Pop();
			NextFrameCook.PendingCookPromise.SetValue(FCookFrameResult::FromCookFrameRequest(NextFrameCook, ECookFrameResult::Cancelled, FrameLastUpdated));
		}
	}

	bool FTouchFrameCooker::CancelCurrentFrame_GameThread(int64 FrameID, ECookFrameResult CookFrameResult)
	{
		FScopeLock Lock(&PendingFrameMutex);
		if (InProgressFrameCook && (InProgressFrameCook->FrameData.FrameID == FrameID || FrameID < 0))
		{
			//if we still have a cook result, that means that we haven't set the promise yet, so we check if this is the frame we are supposed to cancel
			TEResult CancelResult = TEResultCancelled; // just a default non successful result
			if (InProgressCookResult) 
			{
				InProgressCookResult->Result = CookFrameResult; // We set the result we want which will not be overriden
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("Calling TEInstanceCancelFrame for frame %lld..."), FrameID);
				CancelResult = TEInstanceCancelFrame(TouchEngineInstance); // OnFrameFinishedCooking_AnyThread ends up being called before the following statements
				// After TEInstanceCancelFrame, InProgressFrameCook can be null at this point
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("...Called TEInstanceCancelFrame for frame %lld returned '%s'"), FrameID, *TEResultToString(CancelResult));
				// todo: is there any case where the above code would not end up raising TouchEventCallback_AnyThread? this could be an issue
			}
			
			// if (!ensure(CancelResult == TEResultSuccess)) // TEInstanceCancelFrame would end up calling OnFrameFinishedCooking_AnyThread which would take care of the below, but if this did not happen we need to clean manually
			// {
			// 		const FCookFrameResult CookResult = FCookFrameResult::FromCookFrameRequest(InProgressFrameCook.GetValue(), CookFrameResult, FrameLastUpdated);
			// 		InProgressFrameCook->PendingCookPromise.SetValue(CookResult);
			// 	InProgressCookResult.Reset();
			// 	InProgressFrameCook.Reset();
			// }

			return true;
		}
		return false;
	}

	bool FTouchFrameCooker::CheckIfCookTimedOut_GameThread(double CookTimeoutInSeconds)
	{
		FScopeLock Lock(&PendingFrameMutex);
		if (InProgressFrameCook && (FDateTime::Now() - InProgressFrameCook->JobStartTime).GetTotalSeconds() >= CookTimeoutInSeconds) // we check if the frame Timed-out
		{
			CancelCurrentFrame_GameThread(InProgressFrameCook->FrameData.FrameID, ECookFrameResult::TouchEngineCookTimeout);
			return true;
		}
		return false;
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
		const TSharedRef<FTouchFrameCooker> This = SharedThis(this);
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

	void FTouchFrameCooker::ResetTouchEngineInstance()
	{
		TouchEngineInstance.reset();
	}

	void FTouchFrameCooker::EnqueueCookFrame(FPendingFrameCook&& CookRequest, int32 InputBufferLimit)
	{
		InputBufferLimit = FMath::Max(1, InputBufferLimit);
		UE_LOG(LogTouchEngine, Log, TEXT("[EnqueueCookFrame[%s]] Enqueing Cook for frame %lld (%d cooks currently in the queue, InputBufferLimit is %d )"),
			*GetCurrentThreadStr(), CookRequest.FrameData.FrameID, PendingCookQueue.Num(), InputBufferLimit)
		
		// here we remove one more item than the buffer limit as we are going to add the given CookRequest
		while (!PendingCookQueue.IsEmpty() && PendingCookQueue.Num() >= InputBufferLimit)
		{
			FPendingFrameCook CookToCancel = PendingCookQueue.Pop();
			UE_LOG(LogTouchEngine, Log, TEXT("[EnqueueCookFrame[%s]]   Cancelling Cook for frame %lld (%d cooks currently in the queue, InputBufferLimit is %d )"),
				*GetCurrentThreadStr(), CookToCancel.FrameData.FrameID, PendingCookQueue.Num(), InputBufferLimit)

			// Before dropping the inputs, we are trying to merge them with the next set of inputs,
			// which will end up sending them to TE unless they are being set by the next set of inputs
			FPendingFrameCook& NextFutureCook = PendingCookQueue.IsEmpty() ? CookRequest : PendingCookQueue.Last();
			for (TPair<FString, FTouchEngineDynamicVariableStruct>& Variable : CookToCancel.VariablesToSend)
			{
				NextFutureCook.VariablesToSend.FindOrAdd(Variable.Key, MoveTemp(Variable.Value));
			}
			
			CookToCancel.PendingCookPromise.SetValue(FCookFrameResult::FromCookFrameRequest(CookToCancel, ECookFrameResult::InputsDiscarded, FrameLastUpdated));
		}
		
		PendingCookQueue.Insert(MoveTemp(CookRequest), 0); // We enqueue at the start so we can easily use Pop to get the last element
	}

	bool FTouchFrameCooker::ExecuteNextPendingCookFrame_GameThread()
	{
		FScopeLock Lock(&PendingFrameMutex);
		return ExecuteNextPendingCookFrame_GameThread(Lock);
	}

	bool FTouchFrameCooker::ExecuteNextPendingCookFrame_GameThread(FScopeLock& PendingFrameMutexLock)
	{
		if (InProgressFrameCook || PendingCookQueue.IsEmpty())
		{
			return false;
		}
		
		TEResult Result = static_cast<TEResult>(0);
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("  I.B [GT] Cook Frame"), STAT_TE_I_B, STATGROUP_TouchEngine);
			FPendingFrameCook CookRequest = PendingCookQueue.Pop();

			UE_LOG(LogTouchEngine, Log, TEXT("  --------- [FTouchFrameCooker::ExecuteCurrentCookFrame[%s]] Executing the cook for the frame %lld [Requested during frame %lld, Queue: %d cooks waiting] ---------"),
			       *GetCurrentThreadStr(), CookRequest.FrameData.FrameID, GetNextFrameID() - 1, PendingCookQueue.Num())

			// 1. First, we prepare the inputs to send
			{
				ResourceProvider.PrepareForNewCook(CookRequest.FrameData);
				UE_LOG(LogTouchEngine, Verbose, TEXT("[ExecuteCurrentCookFrame[%s]] Calling `VariablesToSend.SendInputs` for frame %lld"),
				       *GetCurrentThreadStr(), CookRequest.FrameData.FrameID)
				for (TPair<FString, FTouchEngineDynamicVariableStruct>& Variable : CookRequest.VariablesToSend)
				{
					Variable.Value.SendInput(VariableManager, CookRequest.FrameData);
				}
				CookRequest.VariablesToSend.Reset();
				ResourceProvider.FinalizeExportsToTouchEngine_GameThread(CookRequest.FrameData);
			}

			InProgressCookResult.Reset();
			InProgressCookResult = FCookFrameResult();
			InProgressCookResult->FrameData = CookRequest.FrameData;

			// We may have waited for a short time so the start time should be the requested plus when we started
			// CookRequest.FrameTimeInSeconds += (FDateTime::Now() - CookRequest.JobCreationTime).GetTotalSeconds(); //todo: check with TE team if this should be added back
			InProgressFrameCook.Emplace(MoveTemp(CookRequest));
			InProgressFrameCook->JobStartTime = FDateTime::Now();

			// This is unlocked before calling TEInstanceStartFrameAtTime in case for whatever reason it finishes cooking the frame instantly. That would cause a deadlock.
			
			PendingFrameMutexLock.Unlock();
			
			switch (TimeMode)
			{
			case TETimeInternal:
				{
					Result = TEInstanceStartFrameAtTime(TouchEngineInstance, 0, 0, false);
					UE_LOG(LogTouchEngineTECalls, Log, TEXT("====TEInstanceStartFrameAtTime (TETimeInternal) with time_value '%d', time_scale '%d', and discontinuity 'false' for CookingFrame '%lld' returned '%s'"),
										0, 0, InProgressFrameCook->FrameData.FrameID, *TEResultToString(Result))
					UE_CLOG(Result != TEResultSuccess, LogTouchEngine, Error, TEXT("TEInstanceStartFrameAtTime[%s] (TETimeInternal) for frame `%lld`:  Time: %d  TimeScale: %d => %s (`%hs`)"), *GetCurrentThreadStr(), InProgressFrameCook->FrameData.FrameID, 0, 0, *TEResultToString(Result), TEResultGetDescription(Result));
					break;
				}
			case TETimeExternal:
				{
					AccumulatedTime += InProgressFrameCook->FrameTimeInSeconds * InProgressFrameCook->TimeScale ;
					Result = TEInstanceStartFrameAtTime(TouchEngineInstance, AccumulatedTime, InProgressFrameCook->TimeScale, false);
					UE_LOG(LogTouchEngineTECalls, Log, TEXT("====TEInstanceStartFrameAtTime with time_value '%lld', time_scale '%lld', and discontinuity 'false' for CookingFrame '%lld' returned '%s'"),
										AccumulatedTime, InProgressFrameCook->TimeScale, InProgressFrameCook->FrameData.FrameID, *TEResultToString(Result))
					UE_CLOG(Result != TEResultSuccess, LogTouchEngine, Error, TEXT("TEInstanceStartFrameAtTime[%s] (TETimeExternal) for frame `%lld`:  Time: %lld  TimeScale: %lld => %s (`%hs`)"), *GetCurrentThreadStr(), InProgressFrameCook->FrameData.FrameID, AccumulatedTime, InProgressFrameCook->TimeScale, *TEResultToString(Result), TEResultGetDescription(Result));
					break;
				}
			}
		}
		
		const bool bSuccess = Result == TEResultSuccess;
		if (!bSuccess) //if we are successful, FTouchEngine::TouchEventCallback_AnyThread will be called with the event TEEventFrameDidFinish, and OnFrameFinishedCooking_AnyThread will be called
		{
			// This will reacquire a lock - a bit meh but should not happen often
			InProgressCookResult->Result = ECookFrameResult::FailedToStartCook;
			FinishCurrentCookFrame_AnyThread();
		}
		return true;
	}

	void FTouchFrameCooker::FinishCurrentCookFrame_AnyThread()
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
			InProgressFrameCook->PendingCookPromise.SetValue(*InProgressCookResult);
			InProgressCookResult.Reset(); // to be sure not to try to set it again if we cancel
		}
		else
		{
			InProgressFrameCook.Reset();
			InProgressCookResult.Reset();
		}
	}
}

