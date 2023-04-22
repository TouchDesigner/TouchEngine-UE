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
#include "Async/Async.h"
#include "Engine/Util/CookFrameData.h"
#include "Engine/Util/TouchVariableManager.h"
#include "Rendering/TouchResourceProvider.h"
#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TEResult.h"

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

		CancelCurrentAndNextCook();
	}

	TFuture<FCookFrameResult> FTouchFrameCooker::CookFrame_GameThread(const FCookFrameRequest& CookFrameRequest)
	{
		check(IsInGameThread());

		const bool bIsInDestructor = TouchEngineInstance.get() == nullptr;
		if (bIsInDestructor)
		{
			return MakeFulfilledPromise<FCookFrameResult>(FCookFrameResult{ECookFrameErrorCode::BadRequest, TEResultBadUsage, CookFrameRequest.FrameData}).GetFuture();
		}
		
		FPendingFrameCook PendingCook { CookFrameRequest };
		TFuture<FCookFrameResult> Future = PendingCook.PendingPromise.GetFuture();

		{
			FScopeLock Lock(&PendingFrameMutex);
			if (InProgressFrameCook)
			{
				EnqueueCookFrame(MoveTemp(PendingCook)); // todo: is that really a reachable point? might destroy a lot of the logic
			}
			else
			{
				ExecuteCurrentCookFrame(MoveTemp(PendingCook), Lock);
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
		FinishCurrentCookFrameAndExecuteNextCookFrame();
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
		// todo: create a list of pending promises that could continue out of a dummy one or the previous one created, and set the value in ImportTextureToUnrealEngine.Next
		//todo: not fully sure how to chain them, or to wait all, or to when all
		// should: create a dummy promise and a future when sending the frame. Here, add something to that future and set the value of the future right here?
		// check https://github.com/splash-damage/future-extensions
		// const int64 FrameID = GetCookingFrameID();
		// if (FrameID >= 0)
		// {
		// 	// FScopeLock Lock (&ImportedTexturesMutex);
		// 	// if (FTexturesToImportForFrame* Data = ImportedTexturesJob.Find(FrameID))
		// 	// {
		// 	// 	++Data->NumberTexturesToImport;
		// 	// }
		// }
		
		TexturesToImport->TextureIdentifiersAwaitingImport.Add(ParamId);
		// InProgressCookResult->SlowImportedTextureIdentifiers.Add(ParamId);
		
		VariableManager.AllocateLinkedTop(ParamId); // Avoid system querying this param from generating an output error

		// TPromise<FTextureFormat> TextureCreationPromise;
		// TFuture<FTextureFormat> TextureCreationFuture = TextureCreationPromise.GetFuture();
		
		FTouchImportParameters LinkParams{ TouchEngineInstance, ParamId, Texture };
		LinkParams.PendingTextureImportType = MakeShared<TPromise<FTextureFormat>>();
		LinkParams.PendingTextureImportType->GetFuture().Next( [ParamId, TexturesToImport = TexturesToImport, &TexturesToImportMutex = TexturesToImportMutex](FTextureFormat TextureFormat) {
			// This lambda technically executes on Render Thread
			UE_LOG(LogTouchEngine, Warning, TEXT("Pending Texture Import Type [%s] = %s"), *ParamId.ToString(),	*EPendingTextureImportTypeToString(TextureFormat.PendingTextureImportType) );
			if (TexturesToImport)
			{
				FScopeLock Lock (&TexturesToImportMutex);
				TexturesToImport->TextureIdentifiersAwaitingImport.Remove(ParamId); // we remove it now to we know at which frame all the textures are allocated
				if (TextureFormat.PendingTextureImportType == EPendingTextureImportType::NextTick)
				{
					TexturesToImport->TextureIdentifiersAwaitingImportNextTick.Add(ParamId);
				}
				else //even though we might have cancelled or have an error, we want to deal with it this frame as the information is ready
				{
					TexturesToImport->TextureIdentifiersAwaitingImportThisTick.Add(ParamId); 
				}
				TexturesToImport->SetPromiseValueIfDone(); // ensure we set the promise if all the textures are for next frame
			}
		});
		
		// below calls FTouchTextureImporter::ImportTexture for DX12
		ResourceProvider.ImportTextureToUnrealEngine(LinkParams)
			.Next([ParamId, TexturesToImport = TexturesToImport, &TexturesToImportMutex = TexturesToImportMutex, &VariableManager = VariableManager](const FTouchTextureImportResult& TouchLinkResult) //todo: make FTouchTextureImportResult also return a promise that loads the slow texture if existing
			{
				// InProgressCookResult->FastImportedTextureIdentifiers.Add(ParamId);
				// InProgressCookResult is not available anymore at that point
				if (TouchLinkResult.ResultType == EImportResultType::Success)
				{
					UTexture2D* Texture = TouchLinkResult.ConvertedTextureObject.GetValue();
					VariableManager.UpdateLinkedTOP(ParamId, Texture);
				}
				
				{
					FScopeLock Lock (&TexturesToImportMutex);
					// We are not setting the result, as it is successful by default
					// TexturesToImport->TextureIdentifiersAwaitingImport.Remove(ParamId);
					if (TexturesToImport->TextureIdentifiersAwaitingImportThisTick.Contains(ParamId))
					{
						TexturesToImport->TextureIdentifiersAwaitingImportThisTick.Remove(ParamId);
						TexturesToImport->TexturesImportedThisTick.ImportResults.Add(ParamId, TouchLinkResult);
						UE_LOG(LogTouchEngine, Display, TEXT("[ImportTextureToUnrealEngine] Texture Identifier `%s` is imported for frame %lld [ThisTick]"), *ParamId.ToString(), TexturesToImport->FrameData.FrameID)
					}
					else if (TexturesToImport->TextureIdentifiersAwaitingImportNextTick.Contains(ParamId))
					{
						TexturesToImport->TextureIdentifiersAwaitingImportNextTick.Remove(ParamId);
						TexturesToImport->TexturesImportedNextTick.ImportResults.Add(ParamId, TouchLinkResult);
						UE_LOG(LogTouchEngine, Display, TEXT("[ImportTextureToUnrealEngine] Texture Identifier `%s` is imported for frame %lld [NextTick]"), *ParamId.ToString(), TexturesToImport->FrameData.FrameID)
					}
					else
					{
						UE_LOG(LogTouchEngine, Error, TEXT("[ImportTextureToUnrealEngine] Texture Identifier `%s` of frame %lld not allocated to a texture import tick. Will be processed this tick"), *ParamId.ToString(), TexturesToImport->FrameData.FrameID)
						TexturesToImport->TexturesImportedThisTick.ImportResults.Add(ParamId, TouchLinkResult);
					}
					TexturesToImport->SetPromiseValueIfDone();
				}

				// UTexture2D* Texture = TouchLinkResult.ConvertedTextureObject.GetValue();
				// // if (IsInGameThread()) // todo: is this needed? going back to gamethread would create a deadlock
				// {
				// 	VariableManager.UpdateLinkedTOP(ParamId, Texture);
				// 	{
				// 		FScopeLock Lock (&TexturesToImportMutex);
				// 		// We are not setting the result, as it is successful by default
				// 		// TexturesToImport->TextureIdentifiersAwaitingImport.Remove(ParamId);
				// 		if (TexturesToImport->TextureIdentifiersAwaitingImportThisTick.Contains(ParamId))
				// 		{
				// 			TexturesToImport->TextureIdentifiersAwaitingImportThisTick.Remove(ParamId);
				// 			TexturesToImport->TexturesImportedThisTick.ImportResults.Add(ParamId, TouchLinkResult);
				// 		}
				// 		else if (TexturesToImport->TextureIdentifiersAwaitingImportNextTick.Contains(ParamId))
				// 		{
				// 			TexturesToImport->TextureIdentifiersAwaitingImportNextTick.Remove(ParamId);
				// 			TexturesToImport->TexturesImportedNextTick.ImportResults.Add(ParamId, TouchLinkResult);
				// 		}
				// 		TexturesToImport->SetPromiseValueIfDone();
				// 	}
				// }
			});
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

		InProgressCookResult.Reset();
		InProgressCookResult = FCookFrameResult();
		InProgressCookResult->FrameData = CookRequest.FrameData;
		TexturesToImport.Reset();
		TexturesToImport = MakeShared<FTexturesToImportForFrame>();

		TexturesToImport->FrameData = CookRequest.FrameData;
		TexturesToImport->PendingTexturesImportThisTick = MakeShared<TPromise<UE::TouchEngine::FTouchTexturesReady>>(); //todo: check if there are even expected texture outputs
		TexturesToImport->PendingTexturesImportNextTick = MakeShared<TPromise<UE::TouchEngine::FTouchTexturesReady>>();
		TexturesToImport->TexturesImportedThisTick.Result = EImportResultType::Success; // successful by default, unless told otherwise
		TexturesToImport->TexturesImportedThisTick.FrameData = TexturesToImport->FrameData;
		TexturesToImport->TexturesImportedNextTick.Result = EImportResultType::Success; // successful by default, unless told otherwise
		TexturesToImport->TexturesImportedNextTick.FrameData = TexturesToImport->FrameData;
		
		
		InProgressCookResult->PendingTexturesImportThisTick = MakeShared<TFuture<UE::TouchEngine::FTouchTexturesReady>>(TexturesToImport->PendingTexturesImportThisTick->GetFuture()); //todo: should we share the future?
		InProgressCookResult->PendingTexturesImportNextTick = MakeShared<TFuture<UE::TouchEngine::FTouchTexturesReady>>(TexturesToImport->PendingTexturesImportNextTick->GetFuture());
		
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
				UE_LOG(LogTouchEngine, Warning, TEXT("TEInstanceStartFrameAtTime (TETimeInternal):  Time: %ld  TimeScale: %ld"), 0,0 );
				Result = TEInstanceStartFrameAtTime(TouchEngineInstance, 0, 0, false);
				break;
			}
		case TETimeExternal:
			{
				AccumulatedTime += InProgressFrameCook->FrameTime_Mill;
				UE_LOG(LogTouchEngine, Warning, TEXT("TEInstanceStartFrameAtTime (TETimeExternal):  Time: %lld  TimeScale: %lld"), AccumulatedTime, InProgressFrameCook->TimeScale );
				Result = TEInstanceStartFrameAtTime(TouchEngineInstance, AccumulatedTime, InProgressFrameCook->TimeScale, false);
				break;
			}
		}

		const bool bSuccess = Result == TEResultSuccess;
		if (!bSuccess) //if we are successful, FTouchEngine::TouchEventCallback_AnyThread will be called with the event TEEventFrameDidFinish, and OnFrameFinishedCooking will be called
		{
			// This will reacquire a lock - a bit meh but should not happen often
			InProgressCookResult->ErrorCode = ECookFrameErrorCode::FailedToStartCook;
			FinishCurrentCookFrameAndExecuteNextCookFrame();
		}
	}

	void FTouchFrameCooker::FinishCurrentCookFrameAndExecuteNextCookFrame()
	{
		FScopeLock Lock(&PendingFrameMutex);

		// Might have gotten cancelled
		if (InProgressFrameCook.IsSet())
		{
			check(InProgressCookResult);
			InProgressFrameCook->PendingPromise.SetValue(*InProgressCookResult);
			InProgressFrameCook.Reset();
			InProgressCookResult.Reset();
			if (TexturesToImport)
			{
				FScopeLock TexturesLock (&TexturesToImportMutex);
				TexturesToImport->SetPromiseValueIfDone(); // if there was no texture to import, we would fallback here
			}
			// if (TexturesToImport && TexturesToImport->PendingTexturesImportThisTick)
			// {
			// 	UE_LOG(LogTouchEngine, Error, TEXT("`PendingTexturesImportThisTick` Promise was not set before reaching FTouchFrameCooker::FinishCurrentCookFrameAndExecuteNextCookFrame"))
			// 	TexturesToImport->PendingTexturesImportThisTick->SetValue({EImportResultType::Failure});
			// }
			// if (TexturesToImport && TexturesToImport->PendingTexturesImportNextTick)
			// {
			// 	UE_LOG(LogTouchEngine, Error, TEXT("`PendingTexturesImportNextTick` Promise was not set before reaching FTouchFrameCooker::FinishCurrentCookFrameAndExecuteNextCookFrame"))
			// 	TexturesToImport->PendingTexturesImportNextTick->SetValue({EImportResultType::Failure});
			// }
			TexturesToImport.Reset();
		}

		if (NextFrameCook)
		{
			FPendingFrameCook CookRequest = MoveTemp(*NextFrameCook);
			NextFrameCook.Reset();
			ExecuteCurrentCookFrame(MoveTemp(CookRequest), Lock);
		}
	}
}

