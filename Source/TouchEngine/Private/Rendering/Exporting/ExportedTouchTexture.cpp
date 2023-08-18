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

#include "Rendering/Exporting/ExportedTouchTexture.h"

#include "Logging.h"
#include "Engine/TEDebug.h"

namespace UE::TouchEngine
{
	FExportedTouchTexture::FExportedTouchTexture(TouchObject<TETexture> InTouchRepresentation, const TFunctionRef<void(const TouchObject<TETexture>&)>& RegisterTouchCallback)
		: TouchRepresentation(MoveTemp(InTouchRepresentation))
	{
		// This RegisterTouchCallback is technically useless... the subclass constructor could just set up the callback however a developer may
		// forget about setting it up. But requiring this callback function here, we force them to not forget.

		RegisterTouchCallback.CheckCallable(); // See above: Do not forget to setup callback code to call TouchTextureCallback!
		RegisterTouchCallback(TouchRepresentation);
	}

	FExportedTouchTexture::~FExportedTouchTexture()
	{
		// We must wait for TouchEngine to stop using the texture - FExportedTouchTexture::Release implements that logic.
		const bool bDestroyedBeforeTouchRelease = bIsInUseByTouchEngine;
		ensure(!bDestroyedBeforeTouchRelease);
		UE_CLOG(bDestroyedBeforeTouchRelease, LogTouchEngine, Error, TEXT("You didn't let the destruction be handled by FExportedTouchTexture::Release. We are causing undefined behavior for TouchEngine..."));

		FScopeLock Lock(&TouchEngineMutex);
		if (!ensureMsgf(!ReleasePromise.IsSet(), TEXT("FExportedTouchTexture being destroyed before receiving a TEObjectEventRelease event")))
		{
			// we are not supposed to delete the texture manually but to wait for the TEObjectEventRelease event
			UE_LOG(LogTouchEngine, Error, TEXT("[~FExportedTouchTexture] The FExportedTouchTexture was destroyed before receiving a TEObjectEventRelease event."));
			ReleasePromise->SetValue({});
			ReleasePromise.Reset();
		}
		if (!ensureMsgf(bReceivedReleaseEvent, TEXT("FExportedTouchTexture is being destroyed but we haven't received the TEObjectEventRelease")))
		{
			UE_LOG(LogTouchEngine, Error, TEXT("[~FExportedTouchTexture] The FExportedTouchTexture was destroyed before receiving a TEObjectEventRelease event."));
		}
		bDestroyed = true;
	}

	TFuture<FExportedTouchTexture::FOnTouchReleaseTexture> FExportedTouchTexture::Release()
	{
		TouchRepresentation.reset(); //todo: there might be a problem if we clear this here and we have not received the event yet. Would we still get the event?
		// TEInstance.reset();
		RHIOfTextureToCopy.SafeRelease();
		
		if (!bIsInUseByTouchEngine && bReceivedReleaseEvent)
		{
			return MakeFulfilledPromise<FOnTouchReleaseTexture>(FOnTouchReleaseTexture{}).GetFuture();
		}
		
		FScopeLock Lock(&TouchEngineMutex);
		checkf(!ReleasePromise.IsSet(), TEXT("Release called twice."));
		TPromise<FOnTouchReleaseTexture> Promise;
		TFuture<FOnTouchReleaseTexture> Future = Promise.GetFuture();
		ReleasePromise = MoveTemp(Promise);
		return Future;
	}

	void FExportedTouchTexture::OnTouchTextureUseUpdate(TEObjectEvent Event)
	{
		if (!ensureMsgf(!bDestroyed, TEXT("FExportedTouchTexture is already destroyed but still receiving TEObjectEvent")))
		{
			return;
		}

		UE_LOG(LogTouchEngine, Warning, TEXT("[FExportedTouchTexture::OnTouchTextureUseUpdate] `%s` for texture `%s`"), *TEObjectEventToString(Event), *DebugName) //todo: change back log level when issue is fixed
		
		switch (Event)
		{
		case TEObjectEventBeginUse:
			bWasEverUsedByTouchEngine = true;
			bIsInUseByTouchEngine = true;
			break;

		case TEObjectEventRelease:
			{
				bIsInUseByTouchEngine = false;
				bReceivedReleaseEvent = true;
				TOptional<TPromise<FOnTouchReleaseTexture>> Promise;
				{
					FScopeLock Lock(&TouchEngineMutex);
					// we want to reset first the ReleasePromise as SetValue might end up calling the destructor which will want to acquire a lock, so we are moving it to a temporary object.
					Promise = MoveTemp(ReleasePromise);
					ReleasePromise.Reset();
				}
				if (Promise.IsSet())
				{
					Promise->SetValue({});
				}
				break;
			}
		case TEObjectEventEndUse:
			bIsInUseByTouchEngine = false;
			break;
		default: checkNoEntry();
			break;
		}
	}
}
