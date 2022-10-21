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

	FTouchTextureExporterD3D12::~FTouchTextureExporterD3D12()
	{
		checkf(
			ParamNameToTexture.IsEmpty() && CachedTextureData.IsEmpty(),
			TEXT("SuspendAsyncTasks was either not called or did not clean up the exported textures correctly. ~FExportedTextureD3D12 will now proceed to fatally crash.")
			);
	}

	TFuture<FTouchSuspendResult> FTouchTextureExporterD3D12::SuspendAsyncTasks()
	{
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
		
		TFuture<FTouchSuspendResult> FinishRenderingTasks = FTouchTextureExporter::SuspendAsyncTasks();
		// Once all the rendering tasks have finished using the copying textures, they can be released.
		FinishRenderingTasks.Next([this, Promise = MoveTemp(Promise)](auto) mutable
		{
			// Important: Do not copy iterate ParamNameToTexture and do not copy ParamNameToTexture... otherwise we'll keep
			// the contained shared pointers alive and RemoveTextureParameterDependency won't end up scheduling any kill commands.
			TArray<FName> UsedParameterNames;
			ParamNameToTexture.GenerateKeyArray(UsedParameterNames);
			for (FName ParameterName : UsedParameterNames)
			{
				RemoveTextureParameterDependency(ParameterName);
			}

			check(ParamNameToTexture.IsEmpty());
			check(CachedTextureData.IsEmpty());

			// Once all the texture clean-ups are done, we can tell whomever is waiting that the rendering resources have been cleared up.
			// From this point forward we're ready to be destroyed.
			PendingTextureReleases.Suspend().Next([Promise = MoveTemp(Promise)](auto) mutable
			{
				Promise.SetValue({});
			});
		});

		return Future;
	}

	FTouchExportResult FTouchTextureExporterD3D12::ExportTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTouchExportParameters& Params)
	{
		const TSharedPtr<FExportedTextureD3D12> TextureData = TryGetTexture(Params);
		if (!TextureData)
		{
			return FTouchExportResult{ ETouchExportErrorCode::InternalD3D12Error };
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
			return FTouchExportResult{ ETouchExportErrorCode::FailedTextureTransfer }; 
		}
		
		return FTouchExportResult{ ETouchExportErrorCode::Success, TextureData->GetTouchRepresentation() };
	}

	TSharedPtr<FExportedTextureD3D12> FTouchTextureExporterD3D12::TryGetTexture(const FTouchExportParameters& Params)
	{
		return CachedTextureData.Contains(Params.Texture)
			? ReallocateTextureIfNeeded(Params)
			: ShareTexture(Params);
	}

	TSharedPtr<FExportedTextureD3D12> FTouchTextureExporterD3D12::ShareTexture(const FTouchExportParameters& Params)
	{
		const FRHITexture2D* TextureRHI = GetRHIFromTexture(Params.Texture);
		TSharedPtr<FExportedTextureD3D12> ExportedTexture = FExportedTextureD3D12::Create(*TextureRHI, SharedResourceSecurityAttributes);
		if (ensure(ExportedTexture))
		{
			ParamNameToTexture.Add(Params.ParameterName, FTextureDependency{ Params.Texture, ExportedTexture });
			return CachedTextureData.Add(Params.Texture, ExportedTexture);
		}
		return nullptr;
	}

	TSharedPtr<FExportedTextureD3D12> FTouchTextureExporterD3D12::ReallocateTextureIfNeeded(const FTouchExportParameters& Params)
	{
		const FRHITexture2D* InputRHI = GetRHIFromTexture(Params.Texture);
		if (!ensure(LIKELY(InputRHI)))
		{
			RemoveTextureParameterDependency(Params.ParameterName);
			return nullptr;
		}
		
		if (const TSharedPtr<FExportedTextureD3D12>* Existing = CachedTextureData.Find(Params.Texture);
			// This will hit, e.g. when somebody sets the same UTexture as parameter twice.
			// If the texture has not changed structurally (i.e. resolution or format), we can reuse the shared handle and copy into the shared texture.
			Existing && Existing->Get()->CanFitTexture(*InputRHI))
		{
			return *Existing;
		}

		// The texture has changed structurally - release the old shared texture and create a new one
		RemoveTextureParameterDependency(Params.ParameterName);
		return ShareTexture(Params);
	}

	void FTouchTextureExporterD3D12::RemoveTextureParameterDependency(FName TextureParam)
	{
		FTextureDependency OldExportedTexture;
		ParamNameToTexture.RemoveAndCopyValue(TextureParam, OldExportedTexture);
		CachedTextureData.Remove(OldExportedTexture.UnrealTexture);

		// If this was the last parameter that used this UTexture, then OldExportedTexture should be the last one referencing the exported texture.
		if (OldExportedTexture.ExportedTexture.IsUnique())
		{
			// This will keep the data valid for as long as TE is using the texture
			const TSharedPtr<FExportedTextureD3D12> KeepAlive = OldExportedTexture.ExportedTexture;
			KeepAlive->Release()
				.Next([this, KeepAlive, TaskToken = PendingTextureReleases.StartTask()](auto){});
		}
	}

	void FTouchTextureExporterD3D12::ScheduleWaitFence(const TouchObject<TESemaphore>& AcquireSemaphore, uint64 AcquireValue)
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