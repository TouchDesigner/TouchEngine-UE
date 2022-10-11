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
#include "TextureD3D12.h"
#include "TouchEngine/TED3D.h"

namespace UE::TouchEngine::D3DX12
{
	namespace Private
	{
		static FDX12PlatformTextureData* GetHandleValue(FNativeTextureHandle& Handle)
		{
			return static_cast<FDX12PlatformTextureData*>(Handle.Handle);
		}
			
	}
	FTouchTextureLinkerD3D12::FTouchTextureLinkerD3D12(ID3D12Device* Device)
		: Device(Device)
	{}

	int32 FTouchTextureLinkerD3D12::GetPlatformTextureWidth(FNativeTextureHandle& Texture) const
	{
		const TSharedPtr<FTextureD3D12> SharedTexture = GetSharedTexture(Private::GetHandleValue(Texture)->SharedTextureHandle);
		return SharedTexture
			? SharedTexture->GetWidth()
			: -1;
	}

	int32 FTouchTextureLinkerD3D12::GetPlatformTextureHeight(FNativeTextureHandle& Texture) const
	{
		const TSharedPtr<FTextureD3D12> SharedTexture = GetSharedTexture(Private::GetHandleValue(Texture)->SharedTextureHandle);
		return SharedTexture
			? SharedTexture->GetHeight()
			: -1;
	}

	EPixelFormat FTouchTextureLinkerD3D12::GetPlatformTexturePixelFormat(FNativeTextureHandle& Texture) const
	{
		const TSharedPtr<FTextureD3D12> SharedTexture = GetSharedTexture(Private::GetHandleValue(Texture)->SharedTextureHandle);
		return SharedTexture
			? SharedTexture->GetPixelFormat()
			: PF_Unknown;
	}

	bool FTouchTextureLinkerD3D12::CopyNativeToUnreal(FRHICommandListImmediate& RHICmdList, FNativeTextureHandle& Source, UTexture2D* Target) const
	{
		const TSharedPtr<FTextureD3D12> SharedTexture = GetSharedTexture(Private::GetHandleValue(Source)->SharedTextureHandle);
		if (!SharedTexture)
		{
			return false;
		}
		
		FRHITexture2D* DestRHI = Target->GetResource()->TextureRHI->GetTexture2D();
		RHICmdList.CopyTexture(SharedTexture->GetTextureRHI(), DestRHI, FRHICopyTextureInfo());
		return true;
	}

	TMutexLifecyclePtr<FNativeTextureHandle> FTouchTextureLinkerD3D12::CreatePlatformTextureWithMutex(
		const TouchObject<TEInstance>& Instance,
		const TouchObject<TESemaphore>& Semaphore,
		uint64 WaitValue,
		const TouchObject<TETexture>& SharedTexture)
	{
		// The texture and semaphore are shared: we open them once and release them once TE disposes of them
		const TPair<HANDLE, TSharedPtr<FTextureD3D12>> Texture = GetOrCreateSharedTexture(SharedTexture);
		const TPair<HANDLE, TComPtr<ID3D12Fence>> Fence = GetOrCreateSharedFence(Semaphore);

		const bool bHasTexture = Texture.Key != nullptr;
		const bool bHasFence = Fence.Key != nullptr;
		if (!bHasTexture || !bHasFence)
		{
			return TSharedPtr<FNativeTextureHandle>();
		}

		// In the common case the Wait and Signal calls below are called within in the same frame of Unreal
		// The only case this does not happen is if the UTexture2D* object must first be allocated (see FTouchTextureLinker::GetOrAllocateUnrealTexture).
		FD3D12DynamicRHI* RHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
		ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetD3DCommandQueue();
		NativeCmdQ->Wait(Fence.Value.Get(), WaitValue); // TODO DP: This is probably the wrong command queue... 

		FDX12PlatformTextureData* TextureData = new FDX12PlatformTextureData{ Texture.Key, Fence.Key };
		TSharedRef<FNativeTextureHandle> Result = MakeShareable<FNativeTextureHandle>(new FNativeTextureHandle{ TextureData }, [this, Instance, WaitValue, SharedTexture, Semaphore](FNativeTextureHandle* Handle)
		{
			const FDX12PlatformTextureData* Data = Private::GetHandleValue(*Handle);
			
			if (const TComPtr<ID3D12Fence> Fence = GetSharedFence(Private::GetHandleValue(*Handle)->SharedFenceHandle))
			{
				FD3D12DynamicRHI* RHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
				ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetD3DCommandQueue();
				
				const uint64 ReleaseValue = WaitValue + 1;
				NativeCmdQ->Signal(Fence.Get(), ReleaseValue);
				TEInstanceAddTextureTransfer(Instance, SharedTexture.get(), Semaphore, ReleaseValue);
			}
			
			delete Data;
			delete Handle;
		});
		return Result;
	}

	TPair<HANDLE, TSharedPtr<FTextureD3D12>> FTouchTextureLinkerD3D12::GetOrCreateSharedTexture(const TouchObject<TETexture>& Texture)
	{
		check(TETextureGetType(Texture) == TETextureTypeD3DShared);
		TED3DSharedTexture* Shared = static_cast<TED3DSharedTexture*>(Texture.get());
		const HANDLE Handle = TED3DSharedTextureGetHandle(Shared);
		if (const TSharedPtr<FTextureD3D12> Existing = GetSharedTexture(Handle))
		{
			return { Handle, Existing };
		}
		
		const TSharedPtr<FTextureD3D12> NewTexture = FTextureD3D12::CreateTexture(Device, Shared);
		if (!NewTexture)
		{
			return { nullptr, nullptr };
		}
		
		TED3DSharedTextureSetCallback(Shared, TextureCallback, this);
		CachedTextures.Add(Handle, NewTexture.ToSharedRef());
		return { Handle, NewTexture };
	}

	TSharedPtr<FTextureD3D12> FTouchTextureLinkerD3D12::GetSharedTexture(HANDLE Handle) const
	{
		const TSharedRef<FTextureD3D12>* Result = CachedTextures.Find(Handle);
		return Result
			? *Result
			: TSharedPtr<FTextureD3D12>{ nullptr };
	}

	TPair<HANDLE, FTouchTextureLinkerD3D12::TComPtr<ID3D12Fence>> FTouchTextureLinkerD3D12::GetOrCreateSharedFence(const TouchObject<TESemaphore>& Semaphore)
	{
		check(TESemaphoreGetType(Semaphore) == TESemaphoreTypeD3DFence);
		const HANDLE Handle = TED3DSharedFenceGetHandle(static_cast<TED3DSharedFence*>(Semaphore.get()));
		if (const TComPtr<ID3D12Fence> Existing = GetSharedFence(Handle))
		{
			return { Handle, Existing };
		}
		
		TComPtr<ID3D12Fence> Fence;
		const HRESULT Result = Device->OpenSharedHandle(Handle, IID_PPV_ARGS(&Fence));
		if (FAILED(Result))
		{
			return { nullptr, nullptr };
		}
		
		TED3DSharedFenceSetCallback(static_cast<TED3DSharedFence*>(Semaphore.get()), FenceCallback, this);
		CachedFences.Add(Handle, Fence);
		return { Handle, Fence };
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

