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

#include "Rendering/TouchTextureLinker_AcquireOnRenderThread.h"

#include "Async/Future.h"
#include "RenderingThread.h"

#include "TouchEngine/TEInstance.h"

namespace UE::TouchEngine
{
	TFuture<TMutexLifecyclePtr<TouchObject<TETexture>>> FTouchTextureLinker_AcquireOnRenderThread::AcquireSharedAndCreatePlatformTexture(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture) const
	{
		if (SharedTexture && TEInstanceHasTextureTransfer(Instance, SharedTexture))
		{
			TouchObject<TESemaphore> Semaphore;
			uint64 WaitValue;
			const TEResult ResultCode = TEInstanceGetTextureTransfer(Instance, SharedTexture, Semaphore.take(), &WaitValue);
			if (ResultCode == TEResultSuccess)
			{
				// Post-bone the acquire as long as possible: it may already be done by the time we do the acquire
				// In the most common case the latest time is the next render frame.
				// The uncommon case is where the UnrealTexture still needs to be created on the GameThread first. In that case we have a one frame delay.
				TPromise<TMutexLifecyclePtr<TouchObject<TETexture>>> Promise;
				TFuture<TMutexLifecyclePtr<TouchObject<TETexture>>> Result = Promise.GetFuture();
				ENQUEUE_RENDER_COMMAND(WaitForMutexAndCopy)([this, Instance, Semaphore, WaitValue, SharedTexture, Promise = MoveTemp(Promise)](FRHICommandListImmediate& RHICmdList) mutable
				{
					// Can block rendering thread but usually enough time should already have elapsed so the mutex is acquired instantly
					TMutexLifecyclePtr<TouchObject<TETexture>> Result = CreatePlatformTextureWithMutex(Instance, Semaphore, WaitValue, SharedTexture);
					// MoveTemp is essential here: SetValue will execute callbacks. We want the callbacks to have fine grained control of when the mutex is released.
					// If we didn't MoveTemp, then Result would only get destroyed at the end of this scope, i.e. after SetValue is done.
					Promise.SetValue(MoveTemp(Result));
				});
				return Result;
			}

			// Weird failure ... we expect TEInstanceGetTextureTransfer to succeed 
			return MakeFulfilledPromise<TMutexLifecyclePtr<TouchObject<TETexture>>>(nullptr).GetFuture();
		}

		// We're lucky: the transfer is already done!
		const TouchObject<TETexture> PlatformTexture = CreatePlatformTexture(SharedTexture);
		// There is no mutex to release, just return a ptr with no custom deleter.
		TMutexLifecyclePtr<TouchObject<TETexture>> Result = MakeShared<TouchObject<TETexture>>(PlatformTexture);
		// MoveTemp not essential here but if somebody modifies the above code to include some kind of custom deleter function, MoveTemp becomes essential (see above).
		// We'll just future proof this case here to avoid accidental negligence.
		return MakeFulfilledPromise<TMutexLifecyclePtr<TouchObject<TETexture>>>(MoveTemp(Result)).GetFuture();
	}
}