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

#include "Importing/TouchImportTextureD3D12.h"

#include "D3D12RHIPrivate.h"

#include "D3D12TouchUtils.h"
#include "Logging.h"
#include "TouchEngine/TED3D.h"

namespace UE::TouchEngine::D3DX12
{
	TSharedPtr<FTouchImportTextureD3D12> FTouchImportTextureD3D12::CreateTexture(ID3D12Device* Device, TED3DSharedTexture* Shared, FGetOrCreateSharedFence GetOrCreateSharedFenceDelegate)
	{
		HANDLE Handle = TED3DSharedTextureGetHandle(Shared);
		check(TED3DSharedTextureGetHandleType(Shared) == TED3DHandleTypeD3D12ResourceNT);
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
		HRESULT SharedHandle = Device->OpenSharedHandle(Handle, IID_PPV_ARGS(&Resource));
		if (FAILED(SharedHandle))
		{
			return nullptr;
		}

		const EPixelFormat Format = ConvertD3FormatToPixelFormat(Resource->GetDesc().Format);
		FD3D12DynamicRHI* DynamicRHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
		const FTexture2DRHIRef SrcRHI = DynamicRHI->RHICreateTexture2DFromResource(Format, TexCreate_Shared, FClearValueBinding::None, Resource.Get()).GetReference();
		return MakeShared<FTouchImportTextureD3D12>(SrcRHI, MoveTemp(GetOrCreateSharedFenceDelegate));
	}

	FTouchImportTextureD3D12::FTouchImportTextureD3D12(FTexture2DRHIRef TextureRHI, FGetOrCreateSharedFence GetOrCreateSharedFenceDelegate)
		: TextureRHI(TextureRHI)
		, GetOrCreateSharedFenceDelegate(MoveTemp(GetOrCreateSharedFenceDelegate))
	{}
	
	bool FTouchImportTextureD3D12::AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
		if (const TComPtr<ID3D12Fence> Fence = GetOrCreateSharedFenceDelegate.Execute(Semaphore))
		{
			// TODO DP: This is probably the wrong command queue... take a look at CopyTexture RHI command and use the same queue... probably the copy queue
			FD3D12DynamicRHI* RHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
			ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetD3DCommandQueue();
			NativeCmdQ->Wait(Fence.Get(), WaitValue);
			return true;
		}

		UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("FTouchPlatformTextureD3D12: Failed to wait on ID3D12Fence"));
		return false;
	}

	void FTouchImportTextureD3D12::ReleaseMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
		if (const TComPtr<ID3D12Fence> Fence = GetOrCreateSharedFenceDelegate.Execute(Semaphore))
		{
			FD3D12DynamicRHI* RHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
			ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetD3DCommandQueue();
				
			const uint64 ReleaseValue = WaitValue + 1;
			NativeCmdQ->Signal(Fence.Get(), ReleaseValue);
			TEInstanceAddTextureTransfer(CopyArgs.RequestParams.Instance, CopyArgs.RequestParams.Texture.get(), Semaphore, ReleaseValue);
		}
		else
		{
			UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("FTouchPlatformTextureD3D12: Failed to signal ID3D12Fence"));
		}
	}
}