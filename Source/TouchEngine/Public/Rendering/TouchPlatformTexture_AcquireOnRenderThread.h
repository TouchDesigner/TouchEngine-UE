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
#include "ITouchPlatformTexture.h"

namespace UE::TouchEngine
{
	class TOUCHENGINE_API FTouchPlatformTexture_AcquireOnRenderThread : public ITouchPlatformTexture
	{
	public:

		FTouchPlatformTexture_AcquireOnRenderThread(FTexture2DRHIRef TextureRHI);

		//~ Begin ITouchPlatformTexture Interface
		virtual FTexture2DRHIRef GetTextureRHI() const override { return TextureRHI; }
		virtual bool CopyNativeToUnreal(const FTouchCopyTextureArgs& CopyArgs) override;
		//~ End ITouchPlatformTexture Interface

	protected:
		
		/** Acquires the mutex. If this is a CPU mutex, this may block. If executed on the GPU, it is enqueued here. If this functions returns false, ReleaseMutex will not be called.  */
		virtual bool AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) = 0;
		/** Releases the mutex. If this is a CPU mutex, this may block. If executed on the GPU, it is enqueued here. */
		virtual void ReleaseMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) = 0;

	private:

		FTexture2DRHIRef TextureRHI;
	};
}
