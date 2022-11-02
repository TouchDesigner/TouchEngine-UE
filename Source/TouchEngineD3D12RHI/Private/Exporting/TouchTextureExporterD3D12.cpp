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

#include "Exporting/TouchTextureExporterD3D12.h"

#include "ExportedTextureD3D12.h"
#include "Logging.h"
#include "Rendering/Exporting/TouchExportParams.h"
#include "Util/TouchFenceCache.h"

#include "D3D12RHIPrivate.h"

#include "TouchEngine/TED3D.h"

// macro to deal with COM calls inside a function that returns {} on failure
#define CHECK_HR_DEFAULT(COM_call)\
	{\
	HRESULT Res = COM_call;\
	if (FAILED(Res))\
	{\
	UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("`" #COM_call "` failed: 0x%X - %s"), Res, *GetComErrorDescription(Res)); \
	return {};\
	}\
	}

namespace UE::TouchEngine::D3DX12
{
	TSharedPtr<FTouchTextureExporterD3D12> FTouchTextureExporterD3D12::Create(ID3D12Device* Device, TSharedRef<FTouchFenceCache> FenceCache)
	{
		Microsoft::WRL::ComPtr<ID3D12Fence> FenceNative;
		CHECK_HR_DEFAULT(Device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&FenceNative)))

		HANDLE SharedFenceHandle;
		CHECK_HR_DEFAULT(Device->CreateSharedHandle(FenceNative.Get(), nullptr, GENERIC_ALL, nullptr, &SharedFenceHandle));

		TouchObject<TED3DSharedFence> FenceTE;
		FenceTE.take(TED3DSharedFenceCreate(SharedFenceHandle, nullptr, nullptr));
        // TouchEngine duplicates the handle, so close it now
		CloseHandle(SharedFenceHandle);
		if (!FenceTE)
		{
			return nullptr;
		}

		return MakeShared<FTouchTextureExporterD3D12>(MoveTemp(FenceCache), MoveTemp(FenceNative), MoveTemp(FenceTE));
	}

	FTouchTextureExporterD3D12::FTouchTextureExporterD3D12(TSharedRef<FTouchFenceCache> FenceCache, Microsoft::WRL::ComPtr<ID3D12Fence> FenceNative, TouchObject<TED3DSharedFence> FenceTE)
		: FenceCache(MoveTemp(FenceCache))
		, FenceNative(MoveTemp(FenceNative))
		, FenceTE(MoveTemp(FenceTE))
	{}

	TFuture<FTouchSuspendResult> FTouchTextureExporterD3D12::SuspendAsyncTasks()
	{
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
		
		TFuture<FTouchSuspendResult> FinishRenderingTasks = FTouchTextureExporter::SuspendAsyncTasks();
		// Once all the rendering tasks have finished using the copying textures, they can be released.
		FinishRenderingTasks.Next([this, Promise = MoveTemp(Promise)](auto) mutable
		{
			ReleaseTextures().Next([Promise = MoveTemp(Promise)](auto) mutable
			{
				Promise.SetValue({});
			});
		});

		return Future;
	}

	TSharedPtr<FExportedTextureD3D12> FTouchTextureExporterD3D12::CreateTexture(const FTextureCreationArgs& Params)
	{
		const FRHITexture2D* SourceRHI = GetRHIFromTexture(Params.Texture);
		return FExportedTextureD3D12::Create(*SourceRHI, SharedResourceSecurityAttributes);
	}

	TFuture<FTouchExportResult> FTouchTextureExporterD3D12::ExportTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTouchExportParameters& Params)
	{
		const TSharedPtr<FExportedTextureD3D12> TextureData = GetOrTryCreateTexture(MakeTextureCreationArgs(Params));
		if (!TextureData)
		{
			return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::InternalGraphicsDriverError }).GetFuture();
		}

		// 1. If TE is using it, schedule a wait operation
		TouchObject<TESemaphore> AcquireSemaphore;
		uint64 AcquireValue;
		if (TEInstanceGetTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation(), AcquireSemaphore.take(), &AcquireValue) == TEResultSuccess)
		{
			ScheduleWaitFence(AcquireSemaphore, AcquireValue);
		}

		// 2. 
		FRHITexture2D* SourceRHI = GetRHIFromTexture(Params.Texture);
		RHICmdList.CopyTexture(SourceRHI, TextureData->GetSharedTextureRHI(), FRHICopyTextureInfo());

		// 3. Tell TE it's save to use the texture
		const uint64 WaitValue = IncrementAndSignalFence();
		const TEResult TransferResult = TEInstanceAddTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation(), FenceTE, WaitValue);
		if (TransferResult != TEResultSuccess)
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("TEInstanceAddTextureTransfer error code: %d"), static_cast<int32>(TransferResult));
			return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::FailedTextureTransfer }).GetFuture(); 
		}
		
		return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::Success, TextureData->GetTouchRepresentation() }).GetFuture();
	}

	void FTouchTextureExporterD3D12::ScheduleWaitFence(const TouchObject<TESemaphore>& AcquireSemaphore, uint64 AcquireValue) const
	{
		// Is nullptr if there is nothing to wait on
		if (!AcquireSemaphore)
		{
			return;
		}
		
		if (const Microsoft::WRL::ComPtr<ID3D12Fence> NativeFence = FenceCache->GetOrCreateSharedFence(AcquireSemaphore))
		{
			FD3D12DynamicRHI* RHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
			ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetD3DCommandQueue();
			NativeCmdQ->Wait(NativeFence.Get(), AcquireValue);
		}
		else
		{
			UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("Failed to get shared ID3D12Fence for input texture that is still in use. Texture will be overwriten without access synchronization!"));
		}
	}

	uint64 FTouchTextureExporterD3D12::IncrementAndSignalFence()
	{
		FD3D12DynamicRHI* RHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
		ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetD3DCommandQueue();
		++NextFenceValue;
		NativeCmdQ->Signal(FenceNative.Get(), NextFenceValue);
		return NextFenceValue;
	}
}

#undef CHECK_HR_DEFAULT