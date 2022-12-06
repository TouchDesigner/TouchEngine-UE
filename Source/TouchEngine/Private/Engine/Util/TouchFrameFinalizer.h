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
#include "Engine/Util/CookFrameFinalization.h"
#include "TouchEngine/TouchObject.h"

class FScopeLock;

namespace UE::TouchEngine
{
	class FTouchVariableManager;
	class FTouchResourceProvider;

	class FTouchFrameFinalizer : public TSharedFromThis<FTouchFrameFinalizer>
	{
	public:

		FTouchFrameFinalizer(TSharedRef<FTouchResourceProvider> ResourceProvider, TSharedRef<FTouchVariableManager> VariableManager, TouchObject<TEInstance> TouchEngineInstance);
		
		/** Adds a texture which is supposed to get imported as part of finalizing frame identified with CookFrameNumber. */
		void ImportTextureForCurrentFrame_AnyThread(const FName ParamId, uint64 CookFrameNumber, TouchObject<TETexture> TextureToImport);

		/** Called when a cook frame request has been made. */
		void NotifyFrameCookEnqueued_GameThread(uint64 CookFrameNumber);
		/**
		 * Promises that no more ImportTextureForCurrentFrame_AnyThread calls will be made for CookFrameNumber.
		 * All associated futures retrieved by OnFrameFinalized are executed either instantly or once any pending imports finish.
		 */
		void NotifyFrameFinishedCooking(const FCookFrameResult& CookFrameResult);

		/**
		 * Retrieves a future which is executed when the given frame has been finalized (i.e. all texture have been imported to Unreal).
		 * You may call this multiple times with the same CookFrameNumber.
		 * Important: The future can execute on any thread! However, you can stall threads by calling Wait on it.
		 */
		UE_NODISCARD TFuture<FCookFrameFinalizedResult> OnFrameFinalized_GameThread(uint64 CookFrameNumber);

	private:

		const TSharedRef<FTouchResourceProvider> ResourceProvider;
		const TSharedRef<FTouchVariableManager> VariableManager;
		const TouchObject<TEInstance> TouchEngineInstance;

		struct FFrameFinalizationData
		{
			/** Execute these promises when the frame has been finalized*/
			TArray<TPromise<FCookFrameFinalizedResult>> OnFrameFinalizedListeners;

			uint32 PendingImportCount = 0;
			TOptional<FCookFrameResult> CookFrameResult;
			
			bool HasFinishedCookingFrame() const { return CookFrameResult.IsSet(); }
		};
		TMap<uint64, FFrameFinalizationData> FramesPendingFinalization;

		// TODO: Investigate whether there is way to get rid of this mutex ... it may not be needed to synchronize this much.
		/** Needs to be acquired before accessing FramesPendingFinalization. */
		FCriticalSection FramesPendingFinalizationMutex;

		/**
		 * Must be called with FramesPendingFinalizationMutex acquired.
		 */
		void FinalizeFrameIfReady(uint64 CookFrameNumber, FScopeLock& FramesPendingFinalizationLock);
	};
}


