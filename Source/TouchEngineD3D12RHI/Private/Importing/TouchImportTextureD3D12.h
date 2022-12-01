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
#include "Rendering/Importing/TouchImportTexture_AcquireOnRenderThread.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
THIRD_PARTY_INCLUDES_START
#include <wrl/client.h>
#include <d3d12.h>
THIRD_PARTY_INCLUDES_END
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "TouchEngine/TouchObject.h"
#include "TouchEngine/TESemaphore.h"

namespace UE::TouchEngine::D3DX12
{
	class FTouchImportTextureD3D12 : public FTouchImportTexture_AcquireOnRenderThread
	{
		using Super = FTouchImportTexture_AcquireOnRenderThread;
	public:
		
		template<typename T>
		using TComPtr = Microsoft::WRL::ComPtr<T>;

		DECLARE_DELEGATE_RetVal_OneParam(TComPtr<ID3D12Fence>, FGetOrCreateSharedFence, const TouchObject<TESemaphore>& Semaphore); 

		static TSharedPtr<FTouchImportTextureD3D12> CreateTexture(ID3D12Device* Device, TED3DSharedTexture* Shared, FGetOrCreateSharedFence GetOrCreateSharedFenceDelegate);

		FTouchImportTextureD3D12(
			FTexture2DRHIRef TextureRHI,
			Microsoft::WRL::ComPtr<ID3D12Resource> SourceResource,
			Microsoft::WRL::ComPtr<ID3D12Fence> FenceNative,
			TouchObject<TED3DSharedFence> FenceTE,
			FGetOrCreateSharedFence GetOrCreateSharedFenceDelegate);
		
		//~ Begin ITouchPlatformTexture Interface
		virtual FTextureMetaData GetTextureMetaData() const override;
		//~ End ITouchPlatformTexture Interface
		
	protected:

		//~ Begin FTouchPlatformTexture_AcquireOnRenderThread Interface
		virtual bool AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) override;
		virtual FTexture2DRHIRef ReadTextureDuringMutex() override { return DestTextureRHI; }
		virtual void ReleaseMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) override;
		virtual void CopyTexture(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef SrcTexture, const FTexture2DRHIRef DstTexture) override;
		//~ End FTouchPlatformTexture_AcquireOnRenderThread Interface

	private:

		FTexture2DRHIRef DestTextureRHI;
		Microsoft::WRL::ComPtr<ID3D12Resource> SourceResource;
		Microsoft::WRL::ComPtr<ID3D12Fence> FenceNative;
		TouchObject<TED3DSharedFence> FenceTE;
		FGetOrCreateSharedFence GetOrCreateSharedFenceDelegate;
	};
}