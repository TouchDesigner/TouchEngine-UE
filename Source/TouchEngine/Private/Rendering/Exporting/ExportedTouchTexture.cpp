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

namespace UE::TouchEngine
{
	FExportedTouchTexture::FExportedTouchTexture(TouchObject<TETexture> InTouchRepresentation, TFunctionRef<void(const TouchObject<TETexture>&)> RegisterTouchCallback)
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
	}

	TFuture<FExportedTouchTexture::FOnTouchReleaseTexture> FExportedTouchTexture::Release()
	{
		TouchRepresentation.reset();
		
		if (!bIsInUseByTouchEngine)
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
		switch (Event)
		{
		case TEObjectEventBeginUse:
			bWasEverUsedByTouchEngine = true;
			bIsInUseByTouchEngine = true;
			break;
			
		case TEObjectEventRelease:
		{
			bIsInUseByTouchEngine = false;
			FScopeLock Lock(&TouchEngineMutex);
			if (ReleasePromise.IsSet())
			{
				ReleasePromise->SetValue({});
				ReleasePromise.Reset();
			}
			break;
		}
		case TEObjectEventEndUse:
			bIsInUseByTouchEngine = false;
			break;
		default: checkNoEntry(); break;
		}
	}
}
