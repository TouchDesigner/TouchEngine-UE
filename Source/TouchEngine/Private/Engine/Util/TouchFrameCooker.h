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
#include "Logging.h"
#include "Async/Future.h"
#include "Chaos/AABBTree.h"
#include "Engine/Util/CookFrameData.h"
#include "Engine/Util/TouchVariableManager.h"
#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TouchObject.h"
#include "Util/TouchHelpers.h"

class FScopeLock;

namespace UE::TouchEngine
{
	class FTouchVariableManager;

	/**
	 * The Frame Cooker is responsible for cooking the frame and handling the different frame cooking callbacks 
	 */
	class FTouchFrameCooker
	{
	public:

		FTouchFrameCooker(TouchObject<TEInstance> InTouchEngineInstance, FTouchVariableManager& InVariableManager, FTouchResourceProvider& InResourceProvider);
		~FTouchFrameCooker();

		void SetTimeMode(TETimeMode InTimeMode) { TimeMode = InTimeMode; }

		TFuture<FCookFrameResult> CookFrame_GameThread(const FCookFrameRequest& CookFrameRequest); //todo: probably doesn't need to be a future
		TFuture<FFinishTextureUpdateInfo> GetTextureImportFuture_GameThread(const FInputTextureUpdateId& TextureUpdateId) const;
		void OnFrameFinishedCooking(TEResult Result);
		void CancelCurrentAndNextCook();

		bool IsCookingFrame() const { return InProgressFrameCook.IsSet(); }
		/** returns the FrameID of the current cooking frame, or -1 if no frame is cooking */
		int64 GetCookingFrameID() const { return InProgressFrameCook.IsSet() ? InProgressFrameCook->FrameData.FrameID : -1; }

		// void AddPendingImportTextureIdentifier(const FName& Identifier);
		void ProcessLinkTextureValueChanged_AnyThread(const char* Identifier);
		
	private:

		struct FPendingFrameCook : FCookFrameRequest
		{
			FDateTime JobCreationTime = FDateTime::Now();
			TPromise<FCookFrameResult> PendingPromise;

			void Combine(FPendingFrameCook&& NewRequest)
			{
				// 1 Keep the old JobCreationTime because our job has not yet started - we're just updating it
				// 2 Keep the old FrameTime_Mill because it will implicitly be included when we compute the time elapsed since JobCreationTime
				ensureAlwaysMsgf(TimeScale == NewRequest.TimeScale, TEXT("Changing time scale is not supported. You'll get weird results."));
				PendingPromise.SetValue(FCookFrameResult{ ECookFrameErrorCode::Replaced });
				PendingPromise = MoveTemp(NewRequest.PendingPromise);
			}
		};
		
		TouchObject<TEInstance>	TouchEngineInstance;
		FTouchVariableManager& VariableManager;
		/** The Resource provider is used to handle Texture callbacks */
		FTouchResourceProvider& ResourceProvider;
		
		TETimeMode TimeMode = TETimeInternal;
		
		int64 AccumulatedTime = 0;

		/** Must be obtained to read or write InProgressFrameCook and NextFrameCook. */
		FCriticalSection PendingFrameMutex;
		/** The cook frame request that is currently in progress if any. */
		TOptional<FPendingFrameCook> InProgressFrameCook;
		
		TOptional<FCookFrameResult> InProgressCookResult;

		struct FTexturesToImportForFrame
		{
			FTouchEngineInputFrameData FrameData;
			/** number of textures to import for that frame */
			TSet<FName> TextureIdentifiersAwaitingImport;
			TSet<FName> TextureIdentifiersAwaitingImportThisTick;
			TSet<FName> TextureIdentifiersAwaitingImportNextTick;
			/** the data of the imported textures as well as the frame data */
			FTouchTexturesReady TexturesImportedThisTick;
			FTouchTexturesReady TexturesImportedNextTick;

			TArray<FTextureFormat> TexturesToCreateOnGameThread;
			TSharedPtr<TPromise<TArray<FTextureFormat>>> UTexturesToBeCreatedOnGameThread;
			/** The Promise to call when done. Is made Optional otherwise does not compile */
			TSharedPtr<TPromise<FTouchTexturesReady>> PendingTexturesImportThisTick;
			TSharedPtr<TPromise<FTouchTexturesReady>> PendingTexturesImportNextTick;
			// TUniquePtr<TPromise<FTouchTexturesReady>> Promise;
		private:
			bool bAlreadySetUTexturesReadyToBeCreated = false;
			bool bAlreadySetImportPromiseThisTick = false;
			bool bAlreadySetImportPromiseNextTick = false;
		public:
			bool SetPromiseValueIfDone()
			{
				bool bResult = false;
				if (!TextureIdentifiersAwaitingImport.IsEmpty()) // when it is empty, all the textures have been allocated to be processed either this tick or the next tick
				{
					return bResult;
				}
				if (!bAlreadySetUTexturesReadyToBeCreated)
				{
					bAlreadySetUTexturesReadyToBeCreated = true;
					UTexturesToBeCreatedOnGameThread->SetValue(TexturesToCreateOnGameThread);
				}
				if (TextureIdentifiersAwaitingImportThisTick.IsEmpty() && !bAlreadySetImportPromiseThisTick)
				{
					bAlreadySetImportPromiseThisTick = true;
					UE_LOG(LogTouchEngine, Display, TEXT("[FTexturesToImportForFrame[%s]] Setting `Textures Imported THIS Tick` for frame %lld: %s  NbTextures: %d"),
						*GetCurrentThreadStr(),
						FrameData.FrameID,
						*EImportResultTypeToString(TexturesImportedThisTick.Result),
						TexturesImportedThisTick.ImportResults.Num());

					if (PendingTexturesImportThisTick)
					{
						PendingTexturesImportThisTick->SetValue(TexturesImportedThisTick); // we could technically find a way to pass the identifiers of textures still awaiting import
					}
					bResult = true;
				}
				if (TextureIdentifiersAwaitingImportNextTick.IsEmpty() && !bAlreadySetImportPromiseNextTick)
				{
					bAlreadySetImportPromiseNextTick = true;
					UE_LOG(LogTouchEngine, Display, TEXT("[FTexturesToImportForFrame[%s]] Setting `Textures Imported NEXT Tick` for frame %lld: %s  NbTextures: %d"),
						*GetCurrentThreadStr(),
						FrameData.FrameID,
						*EImportResultTypeToString(TexturesImportedNextTick.Result),
						TexturesImportedNextTick.ImportResults.Num());

					if (PendingTexturesImportNextTick)
					{
						PendingTexturesImportNextTick->SetValue(TexturesImportedNextTick);
					}
					bResult = true;
				}
				return bResult;
			}
		};
		TSharedPtr<FTexturesToImportForFrame> TexturesToImport;

		FCriticalSection TexturesToImportMutex;
		
		/** The next frame cook to execute after InProgressFrameCook is done. If multiple cooks are requested, the current value is combined with the latest request. */
		TOptional<FPendingFrameCook> NextFrameCook;

		void EnqueueCookFrame(FPendingFrameCook&& CookRequest);
		void ExecuteCurrentCookFrame(FPendingFrameCook&& CookRequest, FScopeLock& Lock);
		void FinishCurrentCookFrameAndExecuteNextCookFrame();
	};
}


