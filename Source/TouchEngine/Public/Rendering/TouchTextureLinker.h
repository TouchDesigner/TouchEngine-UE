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

class UTexture2D;

namespace UE::TouchEngine
{
	struct FTouchLinkResult;

	struct FNativeTextureHandle
	{
		void* Handle = nullptr;
	};
	
	/** The object exclusively owns the object for the Unreal Engine application. When it is destroyed, the mutex is released back to Touch Engine. */
	template<typename T>
	class TMutexLifecyclePtr : public TSharedPtr<T>
	{
	public:

		TMutexLifecyclePtr() = default;
		TMutexLifecyclePtr(TSharedPtr<T> Ptr)
			: TSharedPtr<T>(MoveTemp(Ptr))
		{}

		TMutexLifecyclePtr(TSharedRef<T> Ptr)
			: TSharedPtr<T>(MoveTemp(Ptr))
		{}
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
		/** The parameter the output texture is bound to in the TE instance */
		FName ParameterName;
		
		/** E.g. TED3D11Texture */
		TMutexLifecyclePtr<FNativeTextureHandle> PlatformTexture;

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
		TOptional<TPromise<FTouchLinkResult>> ExecuteNext;
		/** The params for ExecuteNext */
		FTouchLinkParameters ExecuteNextParams;

		UTexture2D* UnrealTexture;
	};

	/** Util for importing a Touch Engine texture into a UTexture2D */
	class TOUCHENGINE_API FTouchTextureLinker : public TSharedFromThis<FTouchTextureLinker>
	{
	public:

		virtual ~FTouchTextureLinker();

		/** @return A future that executes once the UTexture2D has been updated (if successful) */
		TFuture<FTouchLinkResult> LinkTexture(const FTouchLinkParameters& LinkParams);

	protected:

		/** Acquires the shared texture (possibly waiting) and creates a platform texture from it. */
		virtual TFuture<TMutexLifecyclePtr<FNativeTextureHandle>> AcquireSharedAndCreatePlatformTexture(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture) = 0;
		virtual int32 GetPlatformTextureWidth(FNativeTextureHandle& Texture) const = 0;
		virtual int32 GetPlatformTextureHeight(FNativeTextureHandle& Texture) const = 0;
		virtual EPixelFormat GetPlatformTexturePixelFormat(FNativeTextureHandle& Texture) const = 0;
		/** Copies Source into Target using the graphics API. It is assumed that the rendering thread has mutex on Source, i.e. that TE isn't using it at the same time. */
		virtual bool CopyNativeToUnreal(FRHICommandListImmediate& RHICmdList, FNativeTextureHandle& Source, UTexture2D* Target) const = 0;

	private:
		
		FCriticalSection QueueTaskSection;
		TMap<FName, FTouchTextureLinkData> LinkData;

		TFuture<FTouchLinkResult> EnqueueLinkTextureRequest(FTouchTextureLinkData& TextureLinkData, const FTouchLinkParameters& LinkParams);
		void ExecuteLinkTextureRequest(TPromise<FTouchLinkResult>&& Promise, const FTouchLinkParameters& LinkParams);

		TFuture<FTouchTextureLinkJob> CreateJob(const FTouchLinkParameters& LinkParams);
		TFuture<FTouchTextureLinkJob> GetOrAllocateUnrealTexture(TFuture<FTouchTextureLinkJob>&& ContinueFrom);
		TFuture<FTouchTextureLinkJob> CopyTexture(TFuture<FTouchTextureLinkJob>&& ContinueFrom);
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


