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
		
		/**
		 * Promises that no more ImportTextureForCurrentFrame_AnyThread calls will be made for CookFrameNumber.
		 * All associated futures retrieved by OnFrameFinalized are executed either instantly or once any pending imports finish.
		 */
		void NotifyFrameFinishedCooking(uint64 CookFrameNumber);

		/**
		 * Retrieves a future which is executed when the given frame has been finalized (i.e. all texture have been imported to Unreal).
		 * You may call this multiple times with the same CookFrameNumber.
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

			std::atomic_int PendingImportCount = 0;
			bool bHasFinishedCookingFrame = false;
		};
		TMap<uint64, FFrameFinalizationData> FramesPendingFinalization;
	};
}


