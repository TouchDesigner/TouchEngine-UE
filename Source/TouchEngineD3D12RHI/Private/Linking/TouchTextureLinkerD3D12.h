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
#include "Rendering/TouchTextureLinker.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
THIRD_PARTY_INCLUDES_START
#include <wrl/client.h>
#include "d3d12.h"
THIRD_PARTY_INCLUDES_END
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

namespace UE::TouchEngine::D3DX12
{
	class FTouchPlatformTextureD3D12;

	struct FDX12PlatformTextureData
	{
		HANDLE SharedTextureHandle;
		HANDLE SharedFenceHandle;
	};

	class FTouchTextureLinkerD3D12 : public FTouchTextureLinker
	{
	public:

		FTouchTextureLinkerD3D12(ID3D12Device* Device);
		
	protected:

		//~ Begin FTouchTextureLinker Interface
		virtual TSharedPtr<ITouchPlatformTexture> CreatePlatformTexture(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture) override;
		//~ End FTouchTextureLinker Interface

	private:
		
		template<typename T>
		using TComPtr = Microsoft::WRL::ComPtr<T>;
		
		ID3D12Device* Device;
		TMap<HANDLE, TSharedRef<FTouchPlatformTextureD3D12>> CachedTextures;
		TMap<HANDLE, TComPtr<ID3D12Fence>> CachedFences;

		TSharedPtr<FTouchPlatformTextureD3D12> GetOrCreateSharedTexture(const TouchObject<TETexture>& Texture);
		TSharedPtr<FTouchPlatformTextureD3D12> GetSharedTexture(HANDLE Handle) const;

		TComPtr<ID3D12Fence> GetOrCreateSharedFence(const TouchObject<TESemaphore>& Semaphore);
		TComPtr<ID3D12Fence> GetSharedFence(HANDLE Handle) const;
		
		static void TextureCallback(HANDLE Handle, TEObjectEvent Event, void* TE_NULLABLE Info);
		static void	FenceCallback(HANDLE Handle, TEObjectEvent Event, void* TE_NULLABLE Info);
	};
}

