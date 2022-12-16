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
#include "Util/TaskSuspender.h"

#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"

class FRHICommandListImmediate;
class FRHICommandList;
class FRHICommandListBase;
class UTexture2D;

namespace UE::TouchEngine
{
	class ITouchImportTexture;
	struct FTouchImportResult;
	struct FTouchSuspendResult;
	
	/**  */
	struct FTouchLinkJobId
	{
		const FName ParameterName;
		TETexture* Texture;
	};

	enum class ETouchLinkErrorCode
	{
		Success,
		Cancelled,

		/** An error handling the passed in TE texture */
		FailedToCreatePlatformTexture,
		/** An error creating the UTexture2D */
		FailedToCreateUnrealTexture,
		/** Failed to copy the TE texture data into the UTexture2D*/
		FailedToCopyResources
	};
	
	struct FTouchTextureLinkJob
	{
		/** The parameters originally passed in for this request */
		FTouchImportParameters RequestParams;
		
		TSharedPtr<ITouchImportTexture> PlatformTexture;
		/** The texture that is returned by this process. It will contain the contents of PlatformTexture. */
		UTexture2D* UnrealTexture = nullptr;
		
		/** The intermediate result of processing this job */
		ETouchLinkErrorCode ErrorCode = ETouchLinkErrorCode::Success;
	};
	
	struct FTouchTextureLinkData
	{
		/** Whether a task is currently in progress */
		bool bIsInProgress;

		/** The task to execute after the currently running task */
		TOptional<TPromise<FTouchImportResult>> ExecuteNext;
		/** The params for ExecuteNext */
		FTouchImportParameters ExecuteNextParams;

		UTexture2D* UnrealTexture;
	};

	/** Util for importing a Touch Engine texture into a UTexture2D */
	class TOUCHENGINE_API FTouchTextureImporter : public TSharedFromThis<FTouchTextureImporter>
	{
	public:

		virtual ~FTouchTextureImporter();

		/** @return A future that executes once the UTexture2D has been updated (if successful) */
		TFuture<FTouchImportResult> ImportTexture(const FTouchImportParameters& LinkParams);

		/** Prevents further async tasks from being enqueued, cancels running tasks where possible, and executes the future once all tasks are done. */
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks();

	protected:

		/** Acquires the shared texture (possibly waiting) and creates a platform texture from it. */
		virtual TFuture<TSharedPtr<ITouchImportTexture>> CreatePlatformTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture) = 0;

		/** Subclasses can use this when the enqueue more rendering tasks on which must be waited when SuspendAsyncTasks is called. */
		FTaskSuspender::FTaskTracker StartRenderThreadTask() { return TaskSuspender.StartTask(); }
		
	private:
		
		/** Tracks running tasks and helps us execute an event when all tasks are done (once they've been suspended). */
		FTaskSuspender TaskSuspender;
		TMap<FName, FTouchTextureLinkData> LinkData;

		void EnqueueLinkTextureRequest(FTouchTextureLinkData& TextureLinkData, TPromise<FTouchImportResult>&& NewPromise, const FTouchImportParameters& LinkParams);
		void ExecuteLinkTextureRequest_RenderThread(FRHICommandListImmediate& RHICmdList, TPromise<FTouchImportResult>&& Promise, const FTouchImportParameters& LinkParams);

		TFuture<FTouchTextureLinkJob> CreateJob_RenderThread(FRHICommandListImmediate& RHICmdList, const FTouchImportParameters& LinkParams);
		TFuture<FTouchTextureLinkJob> GetOrAllocateUnrealTexture_RenderThread(TFuture<FTouchTextureLinkJob>&& ContinueFrom);
		TFuture<FTouchTextureLinkJob> CopyTexture_RenderThread(TFuture<FTouchTextureLinkJob>&& ContinueFrom);
	};

	template<typename T>
	inline TFuture<T> ExecuteOnGameThread(TUniqueFunction<T()> Func)
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

	template<>
	inline TFuture<void> ExecuteOnGameThread<void>(TUniqueFunction<void()> Func)
	{
		if (IsInGameThread())
		{
			Func();
			TPromise<void> Promise;
			Promise.EmplaceValue();
			return Promise.GetFuture();
		}

		TPromise<void> Promise;
		TFuture<void> Result = Promise.GetFuture();
		AsyncTask(ENamedThreads::GameThread, [Func = MoveTemp(Func), Promise = MoveTemp(Promise)]() mutable
		{
			Func();
			Promise.SetValue();
		});
		return Result;
	}
}


