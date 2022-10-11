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
#include "TouchTextureLinker.h"

namespace UE::TouchEngine
{
	/**
	 * Implements a default protocol for acquiring a mutex for a platform texture.
	 * Serves as base implementation for graphics APIs that must wait shared textures, such as D3D11.
	 * 
	 * The default implementation is to wait as long for the acquire as possible: on the rendering thread.
	 * See AcquireSharedAndCreatePlatformTexture for more comments.
	 */
	class TOUCHENGINE_API FTouchTextureLinker_AcquireOnRenderThread : public FTouchTextureLinker
	{
	protected:

		//~ Begin FTouchTextureLinker_AcquireOnRenderThread Interface
		virtual TFuture<TMutexLifecyclePtr<FNativeTextureHandle>> AcquireSharedAndCreatePlatformTexture(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture) override;
		//~ Begin FTouchTextureLinker_AcquireOnRenderThread Interface
		
		/** Acquires the mutex for the shared texture and creates the PlatformTexture. When the TMutexLifecyclePtr is destroyed, the mutex is released so TE can clean-up the texture. */
		virtual TMutexLifecyclePtr<FNativeTextureHandle> CreatePlatformTextureWithMutex(const TouchObject<TEInstance>& Instance, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue, const TouchObject<TETexture>& SharedTexture) = 0;
	};
}


