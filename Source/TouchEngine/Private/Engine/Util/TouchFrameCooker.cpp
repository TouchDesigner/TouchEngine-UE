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
#include "Engine/Util/TouchErrorLog.h"
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
		TFuture<FCookFrameResult> Future = PendingCook.PendingCookPromise.GetFuture().Next([WeakThis = AsWeak()](FCookFrameResult Result)
		{
			// Here we mark each input texture as not being used by the current cook so they can be reused
			if (TSharedPtr<FTouchFrameCooker> This = WeakThis.Pin())
			{
				FScopeLock Lock(&This->PendingFrameMutex);
				if (This->InProgressFrameCook)
				{
					for (const TPair<FString, FTouchEngineDynamicVariableStruct>& Var : This->InProgressFrameCook->VariablesToSend)
					{
						const FTouchEngineDynamicVariableStruct& DynVar = Var.Value;
						if (DynVar.VarType != EVarType::Texture)
						{
							continue;
						}
						if (TSharedPtr<FExportedTouchTexture> Texture = DynVar.GetExportedTexture())
						{
							Texture->bIsUsedInCurrentCook = false;
						}
					}
					This->InProgressFrameCook->VariablesToSend.Reset();
				}
			}
			return Result;
		});

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
		switch (Result)
		{
		case TEResultSuccess: CookResult = ECookFrameResult::Success; break;
		case TEResultCancelled: CookResult = ECookFrameResult::Cancelled; break;
		default:
			const TESeverity Severity = TEResultGetSeverity(Result);
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
		CancelCurrentFrame_GameThread(-1, CookFrameResult);
		
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
			if (InProgressCookResult)
			{
				InProgressCookResult->Result = CookFrameResult; // We set the result we want which will not be overriden
			}

			if (InProgressFrameCook->bWasJobSentToTouchEngine)
			{
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceCancelFrame(TEInstance: '%p') [Thread: '%s', CookingFrame '%lld']"),
					TouchEngineInstance.get(),
					*GetCurrentThreadStr(),
					FrameID
				);
				const TEResult CancelResult = TEInstanceCancelFrame(TouchEngineInstance); // OnFrameFinishedCooking_AnyThread ends up being called before the following statements
				// After TEInstanceCancelFrame, InProgressFrameCook can be null at this point
				UE_CLOG(CancelResult != TEResultSuccess, LogTouchEngineTECalls, Error, TEXT("...Called TEInstanceCancelFrame for frame %lld returned '%s'"), FrameID, *TEResultToString(CancelResult));
			}
			else // if we have not started the cook, we cancel manually here
			{
				const int64_t EngineTime = ceil((InProgressFrameCook->FrameTimeInSeconds - FirstFrameStartTime) * InProgressFrameCook->TimeScale);
				const int64 TimeScale = InProgressFrameCook->TimeScale;

				OnFrameFinishedCooking_AnyThread(TEResultCancelled, true, static_cast<double>(EngineTime) / TimeScale, static_cast<double>(EngineTime) / TimeScale);
			}

			return true;
		}
		return false;
	}

	bool FTouchFrameCooker::CheckIfCookTimedOut_GameThread(double CookTimeoutInSeconds)
	{
		FScopeLock Lock(&PendingFrameMutex);
		if (InProgressFrameCook && (FDateTime::Now() - InProgressFrameCook->JobCreationTime).GetTotalSeconds() >= CookTimeoutInSeconds) // we check if the frame Timed-out
		{
			const int64 FrameID = InProgressFrameCook->FrameData.FrameID;
			CancelCurrentFrame_GameThread(FrameID, ECookFrameResult::TouchEngineCookTimeout);
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
				else if (TouchLinkResult.ResultType == EImportResultType::Failure && GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan
					&& VariableManager.GetErrorLog()) // todo: For UE 5.6, Vulkan Textures are not supported
				{
					VariableManager.GetErrorLog()->AddError(TEXT("Importing and Exporting Textures is not supported on Vulkan for UE 5.6"), ParamId.ToString());
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
		UE_LOG(LogTouchEngine, Log, TEXT("[EnqueueCookFrame[%s]] Enqueuing Cook for frame %lld (%d cooks currently in the queue, InputBufferLimit is %d )"),
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
		
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("  I.B [GT] Cook Frame"), STAT_TE_I_B, STATGROUP_TouchEngine);
		FPendingFrameCook CookRequest = PendingCookQueue.Pop();

		UE_LOG(LogTouchEngine, Log, TEXT("  --------- [FTouchFrameCooker::ExecuteCurrentCookFrame[%s]] Executing the cook for the frame %lld [Requested during frame %lld, Queue: %d cooks waiting] ---------"),
		       *GetCurrentThreadStr(), CookRequest.FrameData.FrameID, GetNextFrameID() - 1, PendingCookQueue.Num())

		// 1. First, we prepare the inputs to send
		TPromise<bool> InputsSentPromise;
		TFuture<bool> InputsSentFuture = InputsSentPromise.GetFuture();
		// just a shared object that calls a promise when destroyed
		TSharedPtr<void> InputsSentTracker = MakeShareable<void>(nullptr, [InputsSentPromise = MoveTemp(InputsSentPromise)](auto) mutable
			{
				InputsSentPromise.SetValue(true);
			});
		{
			ResourceProvider.PrepareForNewCook(CookRequest.FrameData);
			UE_LOG(LogTouchEngine, Verbose, TEXT("[ExecuteCurrentCookFrame[%s]] Calling `VariablesToSend.SendInputs` for frame %lld"),
			       *GetCurrentThreadStr(), CookRequest.FrameData.FrameID)
			for (TPair<FString, FTouchEngineDynamicVariableStruct>& Variable : CookRequest.VariablesToSend)
			{
				if (GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan && Variable.Value.VarType == EVarType::Texture &&
					Variable.Value.GetValueAsTexture() && !Variable.Value.GetExportedTexture() && VariableManager.GetErrorLog())
				{
					VariableManager.GetErrorLog()->AddError(TEXT("Importing and Exporting Textures is not supported on Vulkan for UE 5.6"), Variable.Value.VarName);
				}
				// Some inputs like textures cannot be sent right away as they need to be sent from a different thread.
				Variable.Value.SendInput(VariableManager, CookRequest.FrameData)
					.Next([InputsSentTracker](bool bResult) // The task token will be auto deleted after this promise is done
					{
						return bResult;
					});
			}
		}

		InProgressCookResult.Reset();
		InProgressCookResult = FCookFrameResult();
		InProgressCookResult->FrameData = CookRequest.FrameData;

		InProgressFrameCook = MoveTemp(CookRequest);

		InputsSentFuture.Next([WeakThis = AsWeak(), FrameData = InProgressCookResult->FrameData](auto) mutable // This can execute on AnyThread
		{
			UE_LOG(LogTouchEngine, Verbose, TEXT("[ExecuteCurrentCookFrame[%s]] InputsSentFuture.Next => Ready to Start Frame %lld"), *GetCurrentThreadStr(), FrameData.FrameID)
			TSharedPtr<FTouchFrameCooker> This = WeakThis.Pin();
			if (!This)
			{
				return;
			}
			This->ResourceProvider.FinalizeExportsToTouchEngine_AnyThread(FrameData);

			TEResult Result = static_cast<TEResult>(0);
			{
				FScopeLock Lock(&This->PendingFrameMutex);
				if (!This->InProgressFrameCook.IsSet() || This->InProgressFrameCook->FrameData.FrameID != FrameData.FrameID
					|| This->InProgressCookResult->Result != ECookFrameResult::Count) // if the cook was cancelled or we somehow started a different frame
				{
					return;
				}
				This->InProgressFrameCook->JobStartTime = FDateTime::Now();
				This->InProgressFrameCook->bWasJobSentToTouchEngine = true;

				switch (This->TimeMode)
				{
				case TETimeInternal:
					{
						Lock.Unlock(); // This is unlocked before calling TEInstanceStartFrameAtTime in case for whatever reason it finishes cooking the frame instantly. That would cause a deadlock.

						UE_LOG(LogTouchEngineTECalls, Log, TEXT("==== Calling TEInstanceStartFrameAtTime(TEInstance: '%p', time_value: '%d',  time_scale '%d', discontinuity 'false') [Thread: '%s', TimeMode: 'TETimeInternal', CookingFrame '%lld']"),
							This->TouchEngineInstance.get(),
							0,
							0,
							*GetCurrentThreadStr(),
							FrameData.FrameID
						)
						Result = TEInstanceStartFrameAtTime(This->TouchEngineInstance, 0, 0, false);
						UE_CLOG(Result != TEResultSuccess, LogTouchEngine, Error, TEXT("TEInstanceStartFrameAtTime[%s] (TETimeInternal) for frame `%lld`:  Time: %d  TimeScale: %d => %s (`%hs`)"), *GetCurrentThreadStr(), FrameData.FrameID, 0, 0, *TEResultToString(Result), TEResultGetDescription(Result));
						break;
					}
				case TETimeExternal:
					{
						if (This->FirstFrameStartTime == -1)
						{
							This->FirstFrameStartTime = This->InProgressFrameCook->FrameTimeInSeconds;
						}
						const int64_t EngineTime = ceil((This->InProgressFrameCook->FrameTimeInSeconds - This->FirstFrameStartTime) * This->InProgressFrameCook->TimeScale);
						int64 TimeScale = This->InProgressFrameCook->TimeScale;
						Lock.Unlock(); // This is unlocked before calling TEInstanceStartFrameAtTime in case for whatever reason it finishes cooking the frame instantly. That would cause a deadlock.

						UE_LOG(LogTouchEngineTECalls, Log, TEXT("==== Calling TEInstanceStartFrameAtTime(TEInstance: '%p', time_value: '%lld',  time_scale '%lld', discontinuity 'false') [Thread: '%s', TimeMode: 'TETimeExternal', CookingFrame '%lld']"),
							This->TouchEngineInstance.get(),
							EngineTime,
							TimeScale,
							*GetCurrentThreadStr(),
							FrameData.FrameID
						)
						Result = TEInstanceStartFrameAtTime(This->TouchEngineInstance, EngineTime, TimeScale, false);
						UE_CLOG(Result != TEResultSuccess, LogTouchEngine, Error, TEXT("TEInstanceStartFrameAtTime[%s] (TETimeExternal) for frame `%lld`:  Time: %lld  TimeScale: %lld => %s (`%hs`)"), *GetCurrentThreadStr(), FrameData.FrameID, This->AccumulatedTime, TimeScale, *TEResultToString(Result), TEResultGetDescription(Result));
						break;
					}
				default:
					This->InProgressFrameCook->bWasJobSentToTouchEngine = false;
				}
			}

			const bool bSuccess = Result == TEResultSuccess;
			if (!bSuccess) //if we are successful, FTouchEngine::TouchEventCallback_AnyThread will be called with the event TEEventFrameDidFinish, and OnFrameFinishedCooking_AnyThread will be called
			{
				// This will reacquire a lock - a bit meh but should not happen often
				FScopeLock Lock(&This->PendingFrameMutex);
				This->InProgressCookResult->Result = ECookFrameResult::FailedToStartCook;
				This->FinishCurrentCookFrame_AnyThread();
			}
		});
		
		// This is unlocked before calling TEInstanceStartFrameAtTime in case for whatever reason it finishes cooking the frame instantly. That would cause a deadlock.
		PendingFrameMutexLock.Unlock();
		InputsSentTracker = nullptr; // We can now call TEInstanceStartFrameAtTime if ready 

		return true;
	}

	void FTouchFrameCooker::FinishCurrentCookFrame_AnyThread()
	{
		FScopeLock Lock(&PendingFrameMutex);
		if (InProgressFrameCook.IsSet())
		{
			UE_LOG(LogTouchEngine, Log, TEXT(" === FinishCurrentCookFrame_AnyThread[%s] : =>  %s"), *GetCurrentThreadStr(), *UEnum::GetValueAsString(InProgressCookResult->Result))
			InProgressCookResult->OnReadyToStartNextCook = MakeShared<TPromise<void>>();
			InProgressCookResult->OnReadyToStartNextCook->GetFuture().Next([WeakThis = AsWeak(), FrameData = InProgressCookResult->FrameData]()
			{
				if (const TSharedPtr<FTouchFrameCooker> SharedThis = WeakThis.Pin())
				{
					FScopeLock Lock(&SharedThis->PendingFrameMutex);
					SharedThis->InProgressFrameCook.Reset();
					SharedThis->InProgressCookResult.Reset();
					SharedThis->ResourceProvider.GetTextureImporter().TexturePoolMaintenance(FrameData);
				}
			});
			FCookFrameResult CookResult = MoveTemp(InProgressCookResult.GetValue());
			InProgressCookResult.Reset(); // to be sure not to try to set it again if we cancel
			InProgressFrameCook->PendingCookPromise.SetValue(MoveTemp(CookResult));
		}
		else
		{
			InProgressFrameCook.Reset();
			InProgressCookResult.Reset();
		}
	}
}

