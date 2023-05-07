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
#include "Rendering/TouchResourceProvider.h"
#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TEResult.h"
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

	TFuture<FCookFrameResult> FTouchFrameCooker::CookFrame_GameThread(FCookFrameRequest&& CookFrameRequest)
	{
		check(IsInGameThread());

		const bool bIsInDestructor = TouchEngineInstance.get() == nullptr;
		if (bIsInDestructor)
		{
			return MakeFulfilledPromise<FCookFrameResult>(FCookFrameResult{ECookFrameErrorCode::BadRequest, TEResultBadUsage, CookFrameRequest.FrameData}).GetFuture();
		}
		
		FPendingFrameCook PendingCook { MoveTemp(CookFrameRequest) };
		// PendingCook.PendingPromise = MakeShared<TPromise<FCookFrameResult>>(TPromise<FCookFrameResult>());
		TFuture<FCookFrameResult> Future = PendingCook.PendingPromise.GetFuture();

		{
			FScopeLock Lock(&PendingFrameMutex);
			if (InProgressFrameCook)
			{
				EnqueueCookFrame(MoveTemp(PendingCook));
			}
			else
			{
				ExecuteCurrentCookFrame_GameThread(MoveTemp(PendingCook), Lock);
			}
		}
		
		return  Future;
	}

	TFuture<FFinishTextureUpdateInfo> FTouchFrameCooker::GetTextureImportFuture_GameThread(const FInputTextureUpdateId& TextureUpdateId) const
	{
		return VariableManager.OnFinishAllTextureUpdatesUpTo(TextureUpdateId);
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
		InProgressCookResult->ErrorCode = ErrorCode;
		InProgressCookResult->TouchEngineInternalResult = Result;
		InProgressCookResult->UTexturesToBeCreatedOnGameThread = TexturesToImport->TexturesToCreateOnGameThread;

		FinishCurrentCookFrameAndExecuteNextCookFrame_AnyThread();
	}

	void FTouchFrameCooker::CancelCurrentAndNextCooks()
	{
		FScopeLock Lock(&PendingFrameMutex);
		if (InProgressFrameCook)
		{
			if (InProgressCookResult) //if we still have a cook result, that means that we haven't set the promise yet
			{
				InProgressFrameCook->PendingPromise.SetValue(FCookFrameResult{ ECookFrameErrorCode::Cancelled });
				InProgressCookResult.Reset();
			}
			InProgressFrameCook.Reset();
			TexturesToImport.Reset();
			
			TEInstanceCancelFrame(TouchEngineInstance);
		}
		
		while (!PendingCookQueue.IsEmpty())
		{
			FPendingFrameCook NextFrameCook = PendingCookQueue.Pop();
			NextFrameCook.PendingPromise.SetValue(FCookFrameResult{ ECookFrameErrorCode::Cancelled });
		}
	}

	void FTouchFrameCooker::ProcessLinkTextureValueChanged_AnyThread(const char* Identifier)
	{
		using namespace UE::TouchEngine;
		
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
		// todo: create a list of pending promises that could continue out of a dummy one or the previous one created, and set the value in ImportTextureToUnrealEngine_AnyThread.Next
		//todo: not fully sure how to chain them, or to wait all, or to when all
		// should: create a dummy promise and a future when sending the frame. Here, add something to that future and set the value of the future right here?
		// check https://github.com/splash-damage/future-extensions
		
		// TexturesToImport->TextureIdentifiersAwaitingImport.Add(ParamId);
		// InProgressCookResult->SlowImportedTextureIdentifiers.Add(ParamId);
		
		VariableManager.AllocateLinkedTop(ParamId); // Avoid system querying this param from generating an output error
		
		FTouchImportParameters LinkParams{ TouchEngineInstance, ParamId, Texture };
		// LinkParams.PendingTextureImportType = MakeShared<TPromise<FTextureCreationFormat>>();
		// LinkParams.PendingTextureImportType->GetFuture().Next( [ParamId, TexturesToImport = TexturesToImport, &TexturesToImportMutex = TexturesToImportMutex](FTextureCreationFormat TextureFormat) {
		// 	// This lambda technically executes on Render Thread
		// 	UE_LOG(LogTouchEngine, Warning, TEXT("Pending Texture Import Type [%s] = %s"), *ParamId.ToString(),	*EPendingTextureImportTypeToString(TextureFormat.PendingTextureImportType) );
		// 	if (TexturesToImport)
		// 	{
		// 		FScopeLock Lock (&TexturesToImportMutex);
		// 		TexturesToImport->TextureIdentifiersAwaitingImport.Remove(ParamId); // we remove it now to we know at which frame all the textures are allocated
		// 		if (TextureFormat.PendingTextureImportType == EPendingTextureImportType::NeedUTextureCreated)
		// 		{
		// 			TexturesToImport->TextureIdentifiersToCreateOnGameThread.Add(ParamId);
		// 			TexturesToImport->TexturesToCreateOnGameThread.Emplace(TextureFormat);
		// 		}
		// 		else //even though we might have cancelled or have an error, we want to deal with it this frame as the information is ready
		// 		{
		// 			TexturesToImport->TextureIdentifiersAwaitingImportThisTick.Add(ParamId);
		// 			TextureFormat.OnTextureCreated->SetValue(TextureFormat.OutUnrealTexture); // easy return
		// 		}
		// 		TexturesToImport->SetPromiseValueIfDone(); // ensure we set the promise if all the textures are for next frame
		// 	}
		// 	else
		// 	{
		// 		TextureFormat.OnTextureCreated->SetValue(nullptr);
		// 	}
		// });
		
		// below calls FTouchTextureImporter::ImportTexture_AnyThread for DX12
		ResourceProvider.ImportTextureToUnrealEngine_AnyThread(LinkParams, SharedThis(this))
			.Next([ParamId, TexturesToImport = TexturesToImport, &TexturesToImportMutex = TexturesToImportMutex, &VariableManager = VariableManager](const FTouchTextureImportResult& TouchLinkResult) //todo: make FTouchTextureImportResult also return a promise that loads the slow texture if existing
			{
				// InProgressCookResult is not available anymore at that point
				if (TouchLinkResult.ResultType == EImportResultType::Success)
				{
					UTexture2D* Texture = TouchLinkResult.ConvertedTextureObject.GetValue();
					UE_LOG(LogTouchEngine, Display, TEXT("[ImportTextureToUnrealEngine_AnyThread.Next[%s]] Calling `UpdateLinkedTOP` for Identifier `%s` for frame %lld"),
						*GetCurrentThreadStr(), *ParamId.ToString(), TexturesToImport->FrameData.FrameID)
					VariableManager.UpdateLinkedTOP(ParamId, Texture);
				}
				
				{
					// FScopeLock Lock (&TexturesToImportMutex);
					// // We are not setting the result, as it is successful by default
					// if (TexturesToImport->TextureIdentifiersToCreateOnGameThread.Contains(ParamId))
					// {
					// 	TexturesToImport->TextureIdentifiersToCreateOnGameThread.Remove(ParamId);
					// 	// TexturesToImport->TexturesImportedNextTick.ImportResults.Add(ParamId, TouchLinkResult);
					// 	UE_LOG(LogTouchEngine, Display, TEXT("[ImportTextureToUnrealEngine_AnyThread] Texture Identifier `%s` is imported for frame %lld [NeedUTextureCreated]"), *ParamId.ToString(), TexturesToImport->FrameData.FrameID)
					// }
					// else
					// {
					// 	UE_LOG(LogTouchEngine, Error, TEXT("[ImportTextureToUnrealEngine_AnyThread] Texture Identifier `%s` of frame %lld not allocated to a texture import tick. Will be processed this tick"), *ParamId.ToString(), TexturesToImport->FrameData.FrameID)
					// 	TexturesToImport->TexturesImportedThisTick.ImportResults.Add(ParamId, TouchLinkResult);
					// }
					// TexturesToImport->SetPromiseValueIfDone();
				}
			});
	}

	void FTouchFrameCooker::AddTextureToCreateOnGameThread(FTextureCreationFormat&& TextureFormat)
	{
		FScopeLock Lock (&TexturesToImportMutex);
		// TexturesToImport->TextureIdentifiersToCreateOnGameThread.Add(TextureFormat.Identifier);
		TexturesToImport->TexturesToCreateOnGameThread.Emplace(MoveTemp(TextureFormat));
		
		// TexturesToImport->SetPromiseValueIfDone(); // ensure we set the promise if all the textures are for next frame
	}

	void FTouchFrameCooker::EnqueueCookFrame(FPendingFrameCook&& CookRequest)
	{
		PendingCookQueue.Insert(MoveTemp(CookRequest), 0); // We enqueue at the start so we can easily use Pop
	}

	void FTouchFrameCooker::ExecuteCurrentCookFrame_GameThread(FPendingFrameCook&& CookRequest, FScopeLock& Lock)
	{
		check(!InProgressFrameCook.IsSet());

		UE_LOG(LogTouchEngine, Error, TEXT("  --------- [ExecuteCurrentCookFrame[%s]] Executing the cook for the frame %lld [Requested during frame %lld, Queue: %d cooks waiting] ---------"),
			*GetCurrentThreadStr(), CookRequest.FrameData.FrameID, GetLatestCookNumber(), PendingCookQueue.Num())

		ResourceProvider.PrepareForExportToTouchEngine_AnyThread();
		
		// 1. First, we send the inputs //todo: we should have started sending the textures beforehand
		UE_LOG(LogTouchEngine, Display, TEXT("[ExecuteCurrentCookFrame[%s]] Calling `DynamicVariables.SendInputs` for frame %lld"),
			*GetCurrentThreadStr(), CookRequest.FrameData.FrameID)
		CookRequest.DynamicVariables.SendInputs(VariableManager); // this needs to be on GameThread as this might end up calling UTexture functions that need to be called on the GameThread

		ResourceProvider.FinalizeExportToTouchEngine_AnyThread();
		
		InProgressCookResult.Reset();
		InProgressCookResult = FCookFrameResult();
		InProgressCookResult->FrameData = CookRequest.FrameData;
		TexturesToImport.Reset();
		TexturesToImport = MakeShared<FTexturesToImportForFrame>();

		TexturesToImport->FrameData = CookRequest.FrameData;
		
		// We may have waited for a short time so the start time should be the requested plus when we started
		CookRequest.FrameTime_Mill += CookRequest.TimeScale * (FDateTime::Now() - CookRequest.JobCreationTime).GetTotalMilliseconds();
		InProgressFrameCook.Emplace(MoveTemp(CookRequest));
		
		// This is unlocked before calling TEInstanceStartFrameAtTime in case for whatever reason it finishes cooking the frame instantly. That would cause a deadlock.
		Lock.Unlock();

		TEResult Result = (TEResult)0;
		switch (TimeMode)
		{
		case TETimeInternal:
			{
				UE_LOG(LogTouchEngine, Warning, TEXT("TEInstanceStartFrameAtTime[%s] (TETimeInternal):  Time: %d  TimeScale: %d"), *GetCurrentThreadStr(), 0, 0);
				Result = TEInstanceStartFrameAtTime(TouchEngineInstance, 0, 0, false);
				break;
			}
		case TETimeExternal:
			{
				AccumulatedTime += InProgressFrameCook->FrameTime_Mill;
				UE_LOG(LogTouchEngine, Warning, TEXT("TEInstanceStartFrameAtTime[%s] (TETimeExternal):  Time: %lld  TimeScale: %lld"), *GetCurrentThreadStr(), AccumulatedTime, InProgressFrameCook->TimeScale);
				Result = TEInstanceStartFrameAtTime(TouchEngineInstance, AccumulatedTime, InProgressFrameCook->TimeScale, false);
				break;
			}
		}
		FlushRenderingCommands(); //todo: this needs to be called on GameThread

		const bool bSuccess = Result == TEResultSuccess;
		if (!bSuccess) //if we are successful, FTouchEngine::TouchEventCallback_AnyThread will be called with the event TEEventFrameDidFinish, and OnFrameFinishedCooking will be called
		{
			// This will reacquire a lock - a bit meh but should not happen often
			InProgressCookResult->ErrorCode = ECookFrameErrorCode::FailedToStartCook;
			FinishCurrentCookFrameAndExecuteNextCookFrame_AnyThread();
		}
	}

	void FTouchFrameCooker::FinishCurrentCookFrameAndExecuteNextCookFrame_AnyThread()
	{
		{
			UE_LOG(LogTouchEngine, Error, TEXT("FinishCurrentCookFrameAndExecuteNextCookFrame_AnyThread[%s]"), *GetCurrentThreadStr())
			FScopeLock Lock(&PendingFrameMutex);
			if (InProgressFrameCook.IsSet())
			{
				InProgressFrameCook->PendingPromise.SetValue(*InProgressCookResult);
				InProgressCookResult.Reset();
			}
		}
		ExecuteOnGameThread<void>([WeakThis = AsWeak()]()
		{
			TSharedPtr<FTouchFrameCooker> SharedFrameCooker = WeakThis.Pin();
			if (SharedFrameCooker)
			{
				FScopeLock Lock(&SharedFrameCooker->PendingFrameMutex);

				// Might have gotten cancelled
				if (SharedFrameCooker->InProgressFrameCook.IsSet())
				{
					FScopeLock TexturesLock (&SharedFrameCooker->TexturesToImportMutex);
					// SharedFrameCooker->InProgressFrameCook->PendingPromise.SetValue(*SharedFrameCooker->InProgressCookResult);
					SharedFrameCooker->InProgressFrameCook.Reset();
					// SharedFrameCooker->InProgressCookResult.Reset();
					SharedFrameCooker->TexturesToImport.Reset();
				}

				if (!SharedFrameCooker->PendingCookQueue.IsEmpty())
				{
					FPendingFrameCook NextFrameCook = SharedFrameCooker->PendingCookQueue.Pop();
					// AsyncTask(ENamedThreads::GameThread, [StrongThis = SharedThis(this), NextFrameCook = MoveTemp(NextFrameCook), Lock = MoveTemp(Lock)]() mutable
					// {
						// StrongThis->
					SharedFrameCooker->ExecuteCurrentCookFrame_GameThread(MoveTemp(NextFrameCook), Lock);
					// });
				}
			}
		});
	}
}

