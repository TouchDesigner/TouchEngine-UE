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
#include "Rendering/TouchResourceProvider.h"

// TODO: Later this can be moved into TouchEngine and be used as generic helper
namespace UE::TouchEngine
{
	struct FTouchLinkResult;

	struct FTouchTextureLinkData
	{
		/** Whether a task is currently in progress */
		bool bIsInProgress;

		/** The task to execute after the currently running task */
		TOptional<TPromise<FTouchLinkResult>> ExecuteNext;
		/** The params for ExecuteNext */
		FTouchLinkParameters ExecuteNextParams;

		UTexture2D* Texture;
	};

	template<typename T>
	using TLinkJob = TPair<T, FTouchLinkParameters>;
	
	class FTouchTextureLinker
	{
	public:

		TFuture<FTouchLinkResult> LinkTexture(const FTouchLinkParameters& LinkParams);

	private:

		FCriticalSection QueueTaskSection;
		TMap<FName, FTouchTextureLinkData> LinkData;

		TFuture<FTouchLinkResult> EnqueueLinkTextureRequest(FTouchTextureLinkData& TextureLinkData, const FTouchLinkParameters& LinkParams);
		void ExecuteLinkTextureRequest(TPromise<FTouchLinkResult>&& Promise, const FTouchLinkParameters& LinkParams);

		TFuture<TLinkJob<UTexture2D*>> GetOrAllocateTexture(const FTouchLinkParameters& LinkParams);
	};

	template<typename T>
	TFuture<T> ExecuteOnGameThread(TUniqueFunction<T()> Func)
	{
		if (IsInGameThread())
		{
			return MakeFulfilledPromise<T>(Func()).GetFuture();
		}

		TPromise<T> Promise;
		TFuture<T> Result = Promise.GetFuture();
		AsyncTask(ENamedThreads::GameThread, [Func = MoveTemp(Func), Promise = MoveTemp(Promise)]() mutable
		{
			Promise.SetValue(Func());
		});
		return Result;
	}
}


