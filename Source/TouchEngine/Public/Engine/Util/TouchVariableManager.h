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
#include "TouchEngineDynamicVariableStruct.h"
#include "Engine/TouchVariables.h"
#include "TouchEngine/TouchObject.h"

namespace UE::TouchEngine
{
	class FTouchErrorLog;
	class FTouchResourceProvider;

	using FInputTextureUpdateId = int64;
	struct FTextureInputUpdateInfo
	{
		FName Texture;
		FInputTextureUpdateId TextureUpdateId;
	};

	enum class ETextureUpdateErrorCode
	{
		Success,
		Cancelled
	};
	
	struct FFinishTextureUpdateInfo
	{
		ETextureUpdateErrorCode ErrorCode;
	};
	
	class FTouchVariableManager : public TSharedFromThis<FTouchVariableManager>
	{
	public:

		FTouchVariableManager(TouchObject<TEInstance> TouchEngineInstance, TSharedPtr<FTouchResourceProvider> ResourceProvider, TSharedPtr<FTouchErrorLog> ErrorLog);
		~FTouchVariableManager();
		
		void AllocateLinkedTop(FName ParamName);
		void UpdateLinkedTOP(FName ParamName, UTexture2D* Texture);

		FInputTextureUpdateId GetNextTextureUpdateId() const { return NextTextureUpdateId; }
		/** @return A future that is executed (possibly immediately) when all texture updates up until (excluding) the passed in one are done. */
		TFuture<FFinishTextureUpdateInfo> OnFinishAllTextureUpdatesUpTo(const FInputTextureUpdateId TextureUpdateId);
		
		FTouchCHOPFull GetCHOPOutputSingleSample(const FString& Identifier);
		FTouchCHOPFull GetCHOPOutputs(const FString& Identifier);
		UTexture2D* GetTOPOutput(const FString& Identifier);
		TTouchVar<bool> GetBooleanOutput(const FString& Identifier);
		TTouchVar<double> GetDoubleOutput(const FString& Identifier);
		TTouchVar<int32_t> GetIntegerOutput(const FString& Identifier);
		TTouchVar<TEString*> GetStringOutput(const FString& Identifier);
		FTouchDATFull GetTableOutput(const FString& Identifier);
		TArray<FString> GetCHOPChannelNames(const FString& Identifier) const;

		void SetCHOPInputSingleSample(const FString &Identifier, const FTouchCHOPSingleSample& CHOP);
		void SetCHOPInput(const FString& Identifier, const FTouchCHOPFull& CHOP);
		/**
		 * @param bReuseExistingTexture Set this to true if you never change the content of Texture (e.g. the pixels).
		 * If Texture was used as parameter in the past, this parameter determines whether it is safe to reuse that data.
		 * In that case, we will skip allocating a new texture resource and copying Texture into it:
		 * we'll just return the existing resource.
		 */
		void SetTOPInput(const FString& Identifier, UTexture* Texture, bool bReuseExistingTexture = true);
		void SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op);
		void SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op);
		void SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32_t>>& Op);
		void SetStringInput(const FString& Identifier, TTouchVar<const char*>& Op);
		void SetTableInput(const FString& Identifier, FTouchDATFull& Op);
		
	private:

		struct FInputTextureUpdateTask
		{
			FInputTextureUpdateId TaskId;
			/** It's done but we're waiting to notify the listeners. */
			bool bIsAwaitingFinalisation = false;
		};
		
		TouchObject<TEInstance>	TouchEngineInstance;
		TSharedPtr<FTouchResourceProvider> ResourceProvider;
		TSharedPtr<FTouchErrorLog> ErrorLog;
		
		TMap<FString, FTouchCHOPSingleSample> CHOPSingleOutputs;
		TMap<FString, FTouchCHOPFull> CHOPFullOutputs;
		TMap<FName, UTexture2D*> TOPOutputs;
		FCriticalSection TOPLock;

		/** Incremented whenever SetTOPInput is called. */
		FInputTextureUpdateId NextTextureUpdateId = 0;
		/**
		 * Optimization: the highest task ID in SortedActiveTextureUpdates that has bIsAwaitingFinalisation = true.
		 * Effectively reduces how many elements must be traversed when a task is completed.
		 */
		FInputTextureUpdateId HighestTaskIdAwaitingFinalisation = 0;

		/** This mutex must be acquired to read or write TextureUpdateListeners. */
		FCriticalSection TextureUpdateListenersLock;
		/** Binds a texture update ID to all the listeners waiting for it (and its predecessors!) to be completed. */
		TMap<FInputTextureUpdateId, TArray<TPromise<FFinishTextureUpdateInfo>>> TextureUpdateListeners;

		/** This mutex must be acquired to read or write SortedActiveTextureUpdates. */
		FCriticalSection ActiveTextureUpdatesLock;
		/**
		 * Texture updates that have not been completed, yet. They are initiated using SetTOPInput.
		 * Sorted in ascending order.
		 */
		TArray<FInputTextureUpdateTask> SortedActiveTextureUpdates;

		/** Fires delegates and notifies anybody waiting for the finalisation of a texture. */
		void OnFinishInputTextureUpdate(const FTextureInputUpdateInfo& UpdateInfo);
		/** Checks whether all tasks before UpdateId are done. Optionally you can exclude all tasks before StartIndex. */
		bool CanFinalizeTextureUpdateTask(const FInputTextureUpdateId UpdateId, bool bJustFinishedTask = false) const;
		/** Collects update tasks that were scheduled after TextureUpdateId and may be waiting for TextureUpdateId's completion. */
		void CollectAllDoneTexturesPendingFinalization(TArray<FInputTextureUpdateId>& Result) const;
		/** Gets all listeners for the given UpdateIds */
		TArray<TPromise<FFinishTextureUpdateInfo>> RemoveAndGetListenersFor(const TArray<FInputTextureUpdateId>& UpdateIds);
	};
}


