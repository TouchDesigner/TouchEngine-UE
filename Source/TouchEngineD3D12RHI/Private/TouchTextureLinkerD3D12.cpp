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

#include "TouchTextureLinkerD3D12.h"

// Include order is important!
#include "D3D12RHIPrivate.h" 
#include "d3d12.h"

#include "TouchEngine/TED3D.h"

namespace UE::TouchEngine::D3DX12
{
	int32 FTouchTextureLinkerD3D12::GetPlatformTextureWidth(TETexture* Texture) const
	{
		// TODO DP
		return 512;
	}

	int32 FTouchTextureLinkerD3D12::GetPlatformTextureHeight(TETexture* Texture) const
	{
		// TODO DP
		return 512;
	}

	EPixelFormat FTouchTextureLinkerD3D12::GetPlatformTexturePixelFormat(TETexture* Texture) const
	{
		// TODO DP
		return EPixelFormat::PF_A16B16G16R16;
	}

	bool FTouchTextureLinkerD3D12::CopyNativeResources(TETexture* Source, UTexture2D* Target) const
	{
		// TODO DP
		return false;
	}

	TMutexLifecyclePtr<TouchObject<void>> FTouchTextureLinkerD3D12::CreatePlatformTextureWithMutex(
		const TouchObject<TEInstance>& Instance,
		const TouchObject<TESemaphore>& Semaphore,
		uint64 WaitValue,
		const TouchObject<TETexture>& SharedTexture) const
	{
		check(TESemaphoreGetType(Semaphore) == TESemaphoreTypeD3DFence);
		//HANDLE FenceHandle = TED3DSharedFenceGetHandle(Semaphore);
		
		FD3D12DynamicRHI*			RHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
		FD3D12Adapter&				RHIAdapter = RHI->GetAdapter(0);
		ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetD3DCommandQueue();

		//NativeCmdQ->Wait()
		return TSharedPtr<TouchObject<void>>();
	}
}

