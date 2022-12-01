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
	FRHICOMMAND_MACRO(FRHICopyFromTouchEngineToUnreal)
	{
		template<typename T>
		using TComPtr = Microsoft::WRL::ComPtr<T>;

		FTexture2DRHIRef DstTextureRHI;
		TouchObject<TEInstance> Instance;
		TouchObject<TETexture> SharedTexture;
		TSharedPtr<FTouchImportTextureD3D12> Importer;
		TPromise<ECopyTouchToUnrealResult> Promise;

		FRHICopyFromTouchEngineToUnreal(
			FTexture2DRHIRef DstTextureRHI,
			TouchObject<TEInstance> Instance,
			TouchObject<TETexture> SharedTexture,
			TSharedPtr<FTouchImportTextureD3D12> Importer,
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

			const TComPtr<ID3D12Fence> Fence = Importer->GetOrCreateSharedFence(Semaphore);
			if (!Fence)
			{
				Promise.SetValue(ECopyTouchToUnrealResult::Failure);
				return;
			}

			const FTexture2DRHIRef SrcTextureRHI = Importer->GetDestTextureRHI();
			if (!SrcTextureRHI)
			{
				Promise.SetValue(ECopyTouchToUnrealResult::Failure);
				return;
			}

			// 1. Acquire mutex
			const ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
			ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetCommandQueue();
			NativeCmdQ->Wait(Fence.Get(), WaitValue);

			if (Fence->GetCompletedValue() < WaitValue)
			{
				const HANDLE Event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
				Fence->SetEventOnCompletion(WaitValue, Event);
				WaitForSingleObject(Event, INFINITE);
				CloseHandle(Event);
			}

			// 2. Commit copy command
			CmdList.GetContext().RHICopyTexture(SrcTextureRHI, DstTextureRHI, FRHICopyTextureInfo{});

			// 3. Release mutex
			const uint64 ReleaseValue = WaitValue + 1;
			const TComPtr<ID3D12Fence> FenceNative = Importer->GetFenceNative();
			NativeCmdQ->Signal(FenceNative.Get(), ReleaseValue);
			const TEResult TransferResult = TEInstanceAddTextureTransfer(Instance, SharedTexture.get(), Importer->GetTESharedFence(), ReleaseValue);
			if (TransferResult != TEResultSuccess)
			{
				UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("TEInstanceAddTextureTransfer error code: %d"), static_cast<int32>(TransferResult));
				Promise.SetValue(ECopyTouchToUnrealResult::Failure);
				return;
			}

			if (FenceNative->GetCompletedValue() < ReleaseValue)
			{
				const HANDLE Event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
				FenceNative->SetEventOnCompletion(ReleaseValue, Event);
				WaitForSingleObject(Event, INFINITE);
				CloseHandle(Event);
			}

			Promise.SetValue(ECopyTouchToUnrealResult::Success);
		}
	};

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
		ID3D12DynamicRHI* DynamicRHI = static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
		const FTexture2DRHIRef SrcRHI = DynamicRHI->RHICreateTexture2DFromResource(Format, TexCreate_Shared, FClearValueBinding::None, Resource.Get()).GetReference();

		HRESULT Result;
		Microsoft::WRL::ComPtr<ID3D12Fence> FenceNative;
		Result = Device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&FenceNative));
		if (FAILED(Result))
		{
			return nullptr;
		}

		HANDLE SharedFenceHandle;
		Result = Device->CreateSharedHandle(FenceNative.Get(), nullptr, GENERIC_ALL, nullptr, &SharedFenceHandle);
		if (FAILED(Result))
		{
			return nullptr;
		}

		TouchObject<TED3DSharedFence> FenceTE;
		FenceTE.take(TED3DSharedFenceCreate(SharedFenceHandle, nullptr, nullptr));
		// TouchEngine duplicates the handle, so close it now
		CloseHandle(SharedFenceHandle);
		if (!FenceTE)
		{
			return nullptr;
		}

		return MakeShared<FTouchImportTextureD3D12>(
			SrcRHI,
			MoveTemp(Resource),
			MoveTemp(FenceNative),
			MoveTemp(FenceTE),
			MoveTemp(GetOrCreateSharedFenceDelegate));
	}

	FTouchImportTextureD3D12::FTouchImportTextureD3D12(
		FTexture2DRHIRef TextureRHI,
		TComPtr<ID3D12Resource> SourceResource,
		TComPtr<ID3D12Fence> FenceNative,
		TouchObject<TED3DSharedFence> FenceTE,
		FGetOrCreateSharedFence GetOrCreateSharedFenceDelegate)
		: DestTextureRHI(TextureRHI)
		, SourceResource(MoveTemp(SourceResource))
		, FenceNative(MoveTemp(FenceNative))
		, FenceTE(MoveTemp(FenceTE))
		, GetOrCreateSharedFenceDelegate(MoveTemp(GetOrCreateSharedFenceDelegate))
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

	FTouchImportTextureD3D12::TComPtr<ID3D12Fence> FTouchImportTextureD3D12::GetOrCreateSharedFence(
		const TouchObject<TESemaphore>& Semaphore) const
	{
		return GetOrCreateSharedFenceDelegate.Execute(Semaphore);
	}
}