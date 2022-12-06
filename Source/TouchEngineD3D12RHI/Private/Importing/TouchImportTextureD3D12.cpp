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
	FRHICOMMAND_MACRO(FRHICopyFromTouchEngineToUnreal)
	{
		template<typename T>
		using TComPtr = Microsoft::WRL::ComPtr<T>;

		FTexture2DRHIRef DstTextureRHI;
		TouchObject<TEInstance> Instance;
		TouchObject<TETexture> SharedTexture;
		TSharedRef<FTouchImportTextureD3D12> Importer;
		TPromise<ECopyTouchToUnrealResult> Promise;

		FRHICopyFromTouchEngineToUnreal(
			FTexture2DRHIRef DstTextureRHI,
			TouchObject<TEInstance> Instance,
			TouchObject<TETexture> SharedTexture,
			TSharedRef<FTouchImportTextureD3D12> Importer,
			TPromise<ECopyTouchToUnrealResult>&& Promise)
			: DstTextureRHI(MoveTemp(DstTextureRHI))
			, Instance(MoveTemp(Instance))
			, SharedTexture(MoveTemp(SharedTexture))
			, Importer(MoveTemp(Importer))
			, Promise(MoveTemp(Promise))
		{}

		void Execute(FRHICommandListBase& CmdList)
		{
			if (!(SharedTexture && TEInstanceHasTextureTransfer(Instance, SharedTexture)))
			{
				Promise.SetValue(ECopyTouchToUnrealResult::Failure);
				return;
			}

			TouchObject<TESemaphore> Semaphore;
			uint64 WaitValue;
			const TEResult ResultCode = TEInstanceGetTextureTransfer(Instance, SharedTexture, Semaphore.take(), &WaitValue);
			if (ResultCode != TEResultSuccess)
			{
				Promise.SetValue(ECopyTouchToUnrealResult::Failure);
				return;
			}

			const TComPtr<ID3D12Fence> Fence = Importer->FenceCache->GetOrCreateSharedFence(Semaphore);
			if (!Fence)
			{
				Promise.SetValue(ECopyTouchToUnrealResult::Failure);
				return;
			}

			const FTexture2DRHIRef SrcTextureRHI = Importer->DestTextureRHI;
			if (!SrcTextureRHI)
			{
				Promise.SetValue(ECopyTouchToUnrealResult::Failure);
				return;
			}

			// 1. Acquire mutex
			FD3D12DynamicRHI* RHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
			ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetD3DCommandQueue();
			NativeCmdQ->Wait(Fence.Get(), WaitValue);

			// 2. Commit copy command
			CmdList.GetContext().RHICopyTexture(SrcTextureRHI, DstTextureRHI, FRHICopyTextureInfo{});

			// 3. Release mutex
			const uint64 ReleaseValue = WaitValue + 1;
			const TComPtr<ID3D12Fence> NativeFence = Importer->ReleaseMutexSemaphore->NativeFence;
			NativeCmdQ->Signal(NativeFence.Get(), ReleaseValue);
			const TEResult TransferResult = TEInstanceAddTextureTransfer(
				Instance, SharedTexture.get(), Importer->ReleaseMutexSemaphore->TouchFence, ReleaseValue);

			if (TransferResult != TEResultSuccess)
			{
				UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("TEInstanceAddTextureTransfer error code: %d"), static_cast<int32>(TransferResult));
				Promise.SetValue(ECopyTouchToUnrealResult::Failure);
				return;
			}

			// TODO: I tried to wait on the CPU side for Synchronized cook mode. But Sample 11 is still failing.
			// if (NativeFence->GetCompletedValue() < ReleaseValue)
			// {
			// 	const HANDLE Event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
			// 	NativeFence->SetEventOnCompletion(ReleaseValue, Event);
			// 	WaitForSingleObject(Event, INFINITE);
			// 	CloseHandle(Event);
			// }

			Promise.SetValue(ECopyTouchToUnrealResult::Success);
		}
	};

	TSharedPtr<FTouchImportTextureD3D12> FTouchImportTextureD3D12::CreateTexture_RenderThread(ID3D12Device* Device, TED3DSharedTexture* Shared, TSharedRef<FTouchFenceCache> FenceCache)
	{
		HANDLE Handle = TED3DSharedTextureGetHandle(Shared);
		check(TED3DSharedTextureGetHandleType(Shared) == TED3DHandleTypeD3D12ResourceNT);
		TComPtr<ID3D12Resource> Resource;
		HRESULT SharedHandle = Device->OpenSharedHandle(Handle, IID_PPV_ARGS(&Resource));
		if (FAILED(SharedHandle))
		{
			return nullptr;
		}

		const EPixelFormat Format = ConvertD3FormatToPixelFormat(Resource->GetDesc().Format);
		FD3D12DynamicRHI* DynamicRHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
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
		TComPtr<ID3D12Resource> SourceResource,
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

	TFuture<ECopyTouchToUnrealResult> FTouchImportTextureD3D12::CopyNativeToUnreal_RenderThread(
		const FTouchCopyTextureArgs& CopyArgs)
	{
		check(CopyArgs.Target);
		const TouchObject<TEInstance> Instance = CopyArgs.RequestParams.Instance;
		const TouchObject<TETexture> SharedTexture = CopyArgs.RequestParams.Texture;
		const FTexture2DRHIRef DstTextureRHI = CopyArgs.Target->GetResource()->TextureRHI->GetTexture2D();

		TPromise<ECopyTouchToUnrealResult> Promise;
		TFuture<ECopyTouchToUnrealResult> Future = Promise.GetFuture();

		ALLOC_COMMAND_CL(CopyArgs.RHICmdList, FRHICopyFromTouchEngineToUnreal)
		(
			DstTextureRHI,
			Instance,
			SharedTexture,
			SharedThis(this),
			MoveTemp(Promise)
		);

		return Future;
	}

	bool FTouchImportTextureD3D12::AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
		return false;
	}

	FTexture2DRHIRef FTouchImportTextureD3D12::ReadTextureDuringMutex()
	{
		return nullptr;
	}

	void FTouchImportTextureD3D12::ReleaseMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
	}

	void FTouchImportTextureD3D12::CopyTexture(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef SrcTexture, const FTexture2DRHIRef DstTexture)
	{
	}
}