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
#include "Rendering/TouchTextureLinker_AcquireOnRenderThread.h"

namespace UE::TouchEngine::D3DX12
{
	class FTouchTextureLinkerD3D12 : public FTouchTextureLinker_AcquireOnRenderThread
	{
	protected:

		//~ Begin FTouchTextureLinker Interface
		virtual int32 GetPlatformTextureWidth(TETexture* Texture) const override;
		virtual int32 GetPlatformTextureHeight(TETexture* Texture) const override;
		virtual EPixelFormat GetPlatformTexturePixelFormat(TETexture* Texture) const override;
		virtual bool CopyNativeResources(TETexture* Source, UTexture2D* Target) const override;
		//~ End FTouchTextureLinker Interface

		//~ Begin FTouchTextureLinker_AcquireOnRenderThread Interface
		virtual TMutexLifecyclePtr<TouchObject<TETexture>> CreatePlatformTextureWithMutex(const TouchObject<TEInstance>& Instance, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue, const TouchObject<TETexture>& SharedTexture) const;
		//~ End FTouchTextureLinker_AcquireOnRenderThread Interface
	};
}

