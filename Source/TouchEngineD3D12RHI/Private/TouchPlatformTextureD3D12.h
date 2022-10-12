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
#include "Rendering/TouchPlatformTexture_AcquireOnRenderThread.h"

#include "Windows/PreWindowsApi.h"
#include <wrl/client.h>
#include <d3d12.h>
#include "Windows/PostWindowsApi.h"

#include "TouchEngine/TouchObject.h"
#include "TouchEngine/TESemaphore.h"

namespace UE::TouchEngine::D3DX12
{
	class FTouchPlatformTextureD3D12 : public FTouchPlatformTexture_AcquireOnRenderThread
	{
		using Super = FTouchPlatformTexture_AcquireOnRenderThread;
	public:
		
		template<typename T>
		using TComPtr = Microsoft::WRL::ComPtr<T>;

		DECLARE_DELEGATE_RetVal_OneParam(TComPtr<ID3D12Fence>, FGetOrCreateSharedFence, const TouchObject<TESemaphore>& Semaphore); 
		DECLARE_DELEGATE_RetVal_OneParam(TComPtr<ID3D12Fence>, FGetSharedFence, TESemaphore* Semaphore); 

		static TSharedPtr<FTouchPlatformTextureD3D12> CreateTexture(ID3D12Device* Device, TED3DSharedTexture* Shared, FGetOrCreateSharedFence GetOrCreateSharedFenceDelegate, FGetSharedFence GetSharedFenceDelegate);

		FTouchPlatformTextureD3D12(FTexture2DRHIRef TextureRHI, FGetOrCreateSharedFence GetOrCreateSharedFenceDelegate, FGetSharedFence GetSharedFenceDelegate);

	protected:

		//~ Begin FTouchPlatformTexture_AcquireOnRenderThread Interface
		virtual bool AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) override;
		virtual void ReleaseMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) override;
		//~ End FTouchPlatformTexture_AcquireOnRenderThread Interface

	private:

		FGetOrCreateSharedFence GetOrCreateSharedFenceDelegate;
		FGetSharedFence GetSharedFenceDelegate;

		FTexture2DRHIRef TextureRHI;
	};
}
