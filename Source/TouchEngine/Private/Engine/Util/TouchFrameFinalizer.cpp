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

#include "TouchFrameFinalizer.h"

#include "Logging.h"
#include "Engine/Util/TouchVariableManager.h"
#include "Rendering/TouchResourceProvider.h"

namespace UE::TouchEngine
{
	FTouchFrameFinalizer::FTouchFrameFinalizer(TSharedRef<FTouchResourceProvider> ResourceProvider, TSharedRef<FTouchVariableManager> VariableManager, TouchObject<TEInstance> TouchEngineInstance)
		: ResourceProvider(MoveTemp(ResourceProvider))
		, VariableManager(MoveTemp(VariableManager))
		, TouchEngineInstance(MoveTemp(TouchEngineInstance))
	{}
	
	void FTouchFrameFinalizer::ImportTextureForCurrentFrame_AnyThread(const FName ParamId, uint64 CookFrameNumber, TouchObject<TETexture> TextureToImport)
	{
		{
			// The lock should not be held while ImportTextureToUnrealEngine is called in case its returned future instantly executes
			FScopeLock Lock(&FramesPendingFinalizationMutex);
			++FramesPendingFinalization[CookFrameNumber].PendingImportCount;
		}
		
		VariableManager->AllocateLinkedTop(ParamId); // Avoid system querying this param from generating an output error
		ResourceProvider->ImportTextureToUnrealEngine({ TouchEngineInstance, ParamId, TextureToImport })
			.Next([this, ParamId, CookFrameNumber](const FTouchImportResult& TouchLinkResult)
			{
				ON_SCOPE_EXIT
				{
					FScopeLock Lock(&FramesPendingFinalizationMutex);
					--FramesPendingFinalization[CookFrameNumber].PendingImportCount;
					FinalizeFrameIfReady(CookFrameNumber, Lock);
				};
				
				if (TouchLinkResult.ResultType != EImportResultType::Success)
				{
					return;
				}

				UTexture2D* Texture = TouchLinkResult.ConvertedTextureObject.GetValue();
				VariableManager->UpdateLinkedTOP(ParamId, Texture);
			});
	}

	void FTouchFrameFinalizer::NotifyFrameCookEnqueued_GameThread(uint64 CookFrameNumber)
	{
		FScopeLock Lock(&FramesPendingFinalizationMutex);
		FramesPendingFinalization.Add(CookFrameNumber);
	}

	void FTouchFrameFinalizer::NotifyFrameFinishedCooking(const FCookFrameResult& CookFrameResult)
	{
		FScopeLock Lock(&FramesPendingFinalizationMutex);
		
		FramesPendingFinalization[CookFrameResult.FrameNumber].CookFrameResult = CookFrameResult;
		FinalizeFrameIfReady(CookFrameResult.FrameNumber, Lock);
	}

	TFuture<FCookFrameFinalizedResult> FTouchFrameFinalizer::OnFrameFinalized_GameThread(uint64 CookFrameNumber)
	{
		FScopeLock Lock(&FramesPendingFinalizationMutex);
		FFrameFinalizationData* FinalizationData = FramesPendingFinalization.Find(CookFrameNumber);
		if (!FinalizationData)
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("Frame %lu is not in progress"), CookFrameNumber);
			return MakeFulfilledPromise<FCookFrameFinalizedResult>(FCookFrameFinalizedResult{ ECookFrameErrorCode::BadRequest, ECookFrameFinalizationErrorCode::RequestInvalid, CookFrameNumber }).GetFuture();
		}

		TPromise<FCookFrameFinalizedResult> Promise;
		TFuture<FCookFrameFinalizedResult> Future = Promise.GetFuture();
		FinalizationData->OnFrameFinalizedListeners.Emplace(MoveTemp(Promise));
		return Future;
	}

	void FTouchFrameFinalizer::FinalizeFrameIfReady(uint64 CookFrameNumber, FScopeLock& FramesPendingFinalizationLock)
	{
		FCookFrameResult CookFrameResult;
		TArray<TPromise<FCookFrameFinalizedResult>> PromisesToFulfill;
		{
			FFrameFinalizationData& Data = FramesPendingFinalization[CookFrameNumber];
			if (!Data.HasFinishedCookingFrame() || Data.PendingImportCount > 0)
			{
				return;
			}

			CookFrameResult = *Data.CookFrameResult;
			PromisesToFulfill = MoveTemp(Data.OnFrameFinalizedListeners);
			// Remove it now in case any of the executed promises try to subscribe to the same frame again ...
			FramesPendingFinalization.Remove(CookFrameNumber);
		}

		// Must unlock now in case any of the executed promises call a function that would cause a lock
		FramesPendingFinalizationLock.Unlock();
		for (TPromise<FCookFrameFinalizedResult>& Promise : PromisesToFulfill)
		{
			Promise.EmplaceValue(CookFrameResult.ErrorCode, ECookFrameFinalizationErrorCode::Success, CookFrameNumber);
		}
	}
}
