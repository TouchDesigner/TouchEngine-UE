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
#include "ITouchImportTexture.h"

namespace UE::TouchEngine
{
	/**
	 * Util implementation of ITouchImportTexture uses RHI's CopyTexture to copy the Touch texture into a UTexture.
	 * 1. AcquireMutex will make sure TE does not write to the texture (can be CPU mutex or GPU fence).
	 * 2. ReadTextureDuringMutex returns a (temporary or reused) RHI texture resource to pass to RHI's CopyTexture
	 * 3. ReleaseMutex tells TE it is ok to use the native texture again (can be CPU mutex or GPU fence).
	 */
	class TOUCHENGINE_API FTouchImportTexture_AcquireOnRenderThread : public ITouchImportTexture
	{
	public:

		//~ Begin ITouchPlatformTexture Interface
		virtual TFuture<ECopyTouchToUnrealResult> CopyNativeToUnreal_RenderThread(const FTouchCopyTextureArgs& CopyArgs) override;
		//~ End ITouchPlatformTexture Interface

	protected:
		
		/** Acquires the mutex. If this is a CPU mutex, this may block. If executed on the GPU, it is enqueued here. If this functions returns false, ReleaseMutex will not be called.  */
		virtual bool AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) = 0;
		/** Gets the texture while the mutex is acquired. */
		virtual FTexture2DRHIRef ReadTextureDuringMutex() = 0;
		/** Releases the mutex. If this is a CPU mutex, this may block. If executed on the GPU, it is enqueued here. */
		virtual void ReleaseMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) = 0;
		virtual void CopyTexture(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef SrcTexture, const FTexture2DRHIRef DstTexture) = 0;
	};
}