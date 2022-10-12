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
#include "TouchPlatformTextureD3D12.h"
#include "TouchEngine/TED3D.h"

namespace UE::TouchEngine::D3DX12
{
	FTouchTextureLinkerD3D12::FTouchTextureLinkerD3D12(ID3D12Device* Device)
		: Device(Device)
	{}

	TFuture<TSharedPtr<ITouchPlatformTexture>> FTouchTextureLinkerD3D12::CreatePlatformTexture(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture)
	{
		const TSharedPtr<FTouchPlatformTextureD3D12> Texture = GetOrCreateSharedTexture(SharedTexture);
		const TSharedPtr<ITouchPlatformTexture> Result = Texture
			? StaticCastSharedPtr<ITouchPlatformTexture>(Texture)
			: nullptr;
		return MakeFulfilledPromise<TSharedPtr<ITouchPlatformTexture>>(Result).GetFuture();
	}

	TSharedPtr<FTouchPlatformTextureD3D12> FTouchTextureLinkerD3D12::GetOrCreateSharedTexture(const TouchObject<TETexture>& Texture)
	{
		check(TETextureGetType(Texture) == TETextureTypeD3DShared);
		TED3DSharedTexture* Shared = static_cast<TED3DSharedTexture*>(Texture.get());
		const HANDLE Handle = TED3DSharedTextureGetHandle(Shared);
		if (const TSharedPtr<FTouchPlatformTextureD3D12> Existing = GetSharedTexture(Handle))
		{
			return Existing;
		}
		
		const TSharedPtr<FTouchPlatformTextureD3D12> NewTexture = FTouchPlatformTextureD3D12::CreateTexture(
			Device,
			Shared,
			FTouchPlatformTextureD3D12::FGetOrCreateSharedFence::CreateRaw(this, &FTouchTextureLinkerD3D12::GetOrCreateSharedFence),
			FTouchPlatformTextureD3D12::FGetSharedFence::CreateRaw(this, &FTouchTextureLinkerD3D12::GetSharedFence)
			);
		if (!NewTexture)
		{
			return  nullptr;
		}
		
		TED3DSharedTextureSetCallback(Shared, TextureCallback, this);
		CachedTextures.Add(Handle, NewTexture.ToSharedRef());
		return NewTexture;
	}

	TSharedPtr<FTouchPlatformTextureD3D12> FTouchTextureLinkerD3D12::GetSharedTexture(HANDLE Handle) const
	{
		const TSharedRef<FTouchPlatformTextureD3D12>* Result = CachedTextures.Find(Handle);
		return Result
			? *Result
			: TSharedPtr<FTouchPlatformTextureD3D12>{ nullptr };
	}

	FTouchTextureLinkerD3D12::TComPtr<ID3D12Fence> FTouchTextureLinkerD3D12::GetOrCreateSharedFence(const TouchObject<TESemaphore>& Semaphore)
	{
		check(TESemaphoreGetType(Semaphore) == TESemaphoreTypeD3DFence);
		const HANDLE Handle = TED3DSharedFenceGetHandle(static_cast<TED3DSharedFence*>(Semaphore.get()));
		if (const TComPtr<ID3D12Fence> Existing = GetSharedFence(Handle))
		{
			return Existing;
		}
		
		TComPtr<ID3D12Fence> Fence;
		const HRESULT Result = Device->OpenSharedHandle(Handle, IID_PPV_ARGS(&Fence));
		if (FAILED(Result))
		{
			return nullptr;
		}
		
		TED3DSharedFenceSetCallback(static_cast<TED3DSharedFence*>(Semaphore.get()), FenceCallback, this);
		CachedFences.Add(Handle, Fence);
		return Fence;
	}

	FTouchTextureLinkerD3D12::TComPtr<ID3D12Fence> FTouchTextureLinkerD3D12::GetSharedFence(HANDLE Handle) const
	{
		const TComPtr<ID3D12Fence>* ComPtr = CachedFences.Find(Handle);
		return ComPtr
			? *ComPtr
			: TComPtr<ID3D12Fence>{ nullptr };
	}

	void FTouchTextureLinkerD3D12::TextureCallback(HANDLE Handle, TEObjectEvent Event, void* Info)
	{
		if (Event == TEObjectEventRelease)
		{
			FTouchTextureLinkerD3D12* This = static_cast<FTouchTextureLinkerD3D12*>(Info);
			This->CachedTextures.Remove(Handle);
		}
	}
	
	void FTouchTextureLinkerD3D12::FenceCallback(HANDLE Handle, TEObjectEvent Event, void* Info)
	{
		if (Event == TEObjectEventRelease)
		{
			FTouchTextureLinkerD3D12* This = static_cast<FTouchTextureLinkerD3D12*>(Info);
			This->CachedFences.Remove(Handle);
		}
	}
}

