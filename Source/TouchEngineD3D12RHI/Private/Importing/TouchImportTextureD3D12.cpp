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

#include "D3D12TouchUtils.h"
#include "ID3D12DynamicRHI.h"
#include "Logging.h"
#include "TouchEngine/TED3D.h"

namespace UE::TouchEngine::D3DX12
{
	TSharedPtr<FTouchImportTextureD3D12> FTouchImportTextureD3D12::CreateTexture_RenderThread(ID3D12Device* Device, TED3DSharedTexture* Shared, TSharedRef<FTouchFenceCache> FenceCache)
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
		ID3D12DynamicRHI* DynamicRHI = static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
		const FTexture2DRHIRef SrcRHI = DynamicRHI->RHICreateTexture2DFromResource(Format, TexCreate_Shared, FClearValueBinding::None, Resource.Get()).GetReference();

		TSharedPtr<FTouchFenceCache::FFenceData> ReleaseMutexSemaphore = FenceCache->GetOrCreateOwnedFence_RenderThread();
		if (!ReleaseMutexSemaphore)
		{
			return nullptr;
		}

		return MakeShared<FTouchImportTextureD3D12>(
			SrcRHI,
			MoveTemp(Resource),
			MoveTemp(FenceCache),
			ReleaseMutexSemaphore.ToSharedRef()
			);
	}

	FTouchImportTextureD3D12::FTouchImportTextureD3D12(
		FTexture2DRHIRef TextureRHI,
		Microsoft::WRL::ComPtr<ID3D12Resource> SourceResource,
		TSharedRef<FTouchFenceCache> FenceCache,
		TSharedRef<FTouchFenceCache::FFenceData> ReleaseMutexSemaphore
		)
		: DestTextureRHI(TextureRHI)
		, SourceResource(MoveTemp(SourceResource))
		, FenceCache(MoveTemp(FenceCache))
		, ReleaseMutexSemaphore(MoveTemp(ReleaseMutexSemaphore))
	{}

	FTextureMetaData FTouchImportTextureD3D12::GetTextureMetaData() const
	{
		D3D12_RESOURCE_DESC TextureDesc = SourceResource->GetDesc();
		FTextureMetaData Result;
		Result.SizeX = TextureDesc.Width;
		Result.SizeY = TextureDesc.Height;
		Result.PixelFormat = ConvertD3FormatToPixelFormat(TextureDesc.Format);
		return Result;
	}

	bool FTouchImportTextureD3D12::AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
		if (const TComPtr<ID3D12Fence> Fence = FenceCache->GetOrCreateSharedFence(Semaphore))
		{
			const ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
			ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetCommandQueue();
			NativeCmdQ->Wait(Fence.Get(), WaitValue);
			return true;
		}

		UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("FTouchPlatformTextureD3D12: Failed to wait on ID3D12Fence"));
		return false;
	}

	void FTouchImportTextureD3D12::ReleaseMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
		const ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
		ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetCommandQueue();

		const uint64 ReleaseValue = WaitValue + 1;
		NativeCmdQ->Signal(ReleaseMutexSemaphore->NativeFence.Get(), ReleaseValue);
		TEInstanceAddTextureTransfer(CopyArgs.RequestParams.Instance, CopyArgs.RequestParams.Texture.get(), ReleaseMutexSemaphore->TouchFence, ReleaseValue);
	}

	void FTouchImportTextureD3D12::CopyTexture(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef SrcTexture, const FTexture2DRHIRef DstTexture)
	{
		// Need to immediately flush commands such that RHI commands can be enqueued in native command queue
		check(SrcTexture.IsValid() && DstTexture.IsValid());
		check(SrcTexture->GetFormat() == DstTexture->GetFormat());
		RHICmdList.CopyTexture(SrcTexture, DstTexture, FRHICopyTextureInfo());
		RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
	}
}