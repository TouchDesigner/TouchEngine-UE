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

#include "TouchFenceCache.h"

#include "Logging.h"
#include "TouchEngine/TED3D.h"

namespace UE::TouchEngine::D3DX12
{
	FTouchFenceCache::FTouchFenceCache(ID3D12Device* Device)
		: Device(Device)
	{}

	FTouchFenceCache::~FTouchFenceCache()
	{
		FScopeLock Lock(&SharedFencesMutex);
		for(auto Pair : SharedFences)
		{
			TED3DSharedFenceSetCallback(Pair.Value.TouchFence.get(), nullptr, nullptr);
		}
	}

	FTouchFenceCache::TComPtr<ID3D12Fence> FTouchFenceCache::GetOrCreateSharedFence(const TouchObject<TESemaphore>& Semaphore)
	{
		check(TESemaphoreGetType(Semaphore) == TESemaphoreTypeD3DFence);
		FScopeLock Lock(&SharedFencesMutex);
		
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
		
		TED3DSharedFenceSetCallback(static_cast<TED3DSharedFence*>(Semaphore.get()), SharedFenceCallback, this);
		TouchObject<TED3DSharedFence> FenceObject;
		FenceObject.set(static_cast<TED3DSharedFence*>(Semaphore.get()));
		SharedFences.Add(Handle, { Fence, FenceObject });
		return Fence;
	}

	FTouchFenceCache::TComPtr<ID3D12Fence> FTouchFenceCache::GetSharedFence(HANDLE Handle) const
	{
		const FSharedFenceData* FenceData = SharedFences.Find(Handle);
		return FenceData && ensure(FenceData->NativeFence)
			? FenceData->NativeFence
			: TComPtr<ID3D12Fence>{ nullptr };
	}

	TSharedPtr<FTouchFenceCache::FFenceData> FTouchFenceCache::GetOrCreateOwnedFence_AnyThread(bool bForceNewFence)
	{
		TSharedPtr<FOwnedFenceData> OwnedData;
		{
			FScopeLock Lock(&ReadyForUsageMutex);
			if (!bForceNewFence && ReadyForUsage.Dequeue(OwnedData)) //todo: maybe make sure the queue is not too long?
			{
				// UINT64 CompletedValue = OwnedData->GetFenceData()->NativeFence->GetCompletedValue();
				// OwnedData->GetFenceData()->NativeFence->Signal(0);
				// UINT64 NewValue = OwnedData->GetFenceData()->NativeFence->GetCompletedValue();
				OwnedData->GetFenceData()->LastValue = OwnedData->GetFenceData()->NativeFence->GetCompletedValue();
				UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("Reusing owned fence `%s` of inital value: `%llu` (GetCompletedValue returned: `%llu`)"),
					*OwnedData->GetFenceData()->DebugName, OwnedData->GetFenceData()->LastValue, OwnedData->GetFenceData()->NativeFence->GetCompletedValue());
			}
			else
			{
				OwnedData = CreateOwnedFence_AnyThread();
				// UINT64 NewValue = OwnedData->GetFenceData()->NativeFence->GetCompletedValue();
				UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("Creating new owned fence `%s` of inital value: `%llu` (GetCompletedValue returned: `%llu`)"),
					*OwnedData->GetFenceData()->DebugName, OwnedData->GetFenceData()->LastValue, OwnedData->GetFenceData()->NativeFence->GetCompletedValue());
			}
		}

		return MakeShareable<FFenceData>(&OwnedData->GetFenceData().Get(), [this, OwnedData](FFenceData*)
		{
			OwnedData->ReleasedByUnreal();
			if (OwnedData->IsReadyForReuse())
			{
				FScopeLock Lock(&ReadyForUsageMutex);
				ReadyForUsage.Enqueue(OwnedData);
			}
		});
	}

	TSharedPtr<FTouchFenceCache::FOwnedFenceData> FTouchFenceCache::CreateOwnedFence_AnyThread()
	{
		HRESULT ErrorResultCode;
		Microsoft::WRL::ComPtr<ID3D12Fence> FenceNative;
		constexpr uint64 InitialValue = 0;
		ErrorResultCode = Device->CreateFence(InitialValue, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&FenceNative));
		if (FAILED(ErrorResultCode))
		{
			return nullptr;
		}

		HANDLE SharedFenceHandle;
		ErrorResultCode = Device->CreateSharedHandle(FenceNative.Get(), nullptr, GENERIC_ALL, nullptr, &SharedFenceHandle);
		if (FAILED(ErrorResultCode))
		{
			return nullptr;
		}

		TouchObject<TED3DSharedFence> FenceTE;
		FenceTE.take(TED3DSharedFenceCreate(SharedFenceHandle, OwnedFenceCallback, this));
		// TouchEngine duplicates the handle, so close it now
		CloseHandle(SharedFenceHandle);
		if (!FenceTE)
		{
			return nullptr;
		}

		const TSharedPtr<FFenceData> FenceData = MakeShared<FFenceData>(FSharedFenceData{ FenceNative , FenceTE });
		const TSharedRef<FOwnedFenceData> OwnedData = MakeShared<FOwnedFenceData>(FenceData.ToSharedRef());
		
		{
			FScopeLock Lock(&OwnedFencesMutex);
			FenceData->DebugName = FString::Printf(TEXT("__fence_%lld"), ++LastCreatedID);
			OwnedFences.Add(SharedFenceHandle, OwnedData);
		}
		
		return OwnedData;
	}

	FTouchFenceCache::FOwnedFenceData::~FOwnedFenceData()
	{
		TED3DSharedFenceSetCallback(FenceData->TouchFence.get(), nullptr, nullptr);
		if (FenceData->NativeFence)
		{
			FenceData->NativeFence->Release();
		}
	}

	void FTouchFenceCache::FOwnedFenceData::ReleasedByUnreal()
	{
		bUsedByUnreal = false;
	}

	void FTouchFenceCache::FOwnedFenceData::UpdateTouchUsage(TEObjectEvent NewUsage)
	{
		Usage = NewUsage;
	}

	void FTouchFenceCache::SharedFenceCallback(HANDLE Handle, TEObjectEvent Event, void* Info)
	{
		if (Event == TEObjectEventRelease)
		{
			FTouchFenceCache* This = static_cast<FTouchFenceCache*>(Info);
			FScopeLock Lock(&This->SharedFencesMutex);
			This->SharedFences.Remove(Handle);
		}
	}

	void FTouchFenceCache::OwnedFenceCallback(HANDLE Handle, TEObjectEvent Event, void* Info)
	{
		FTouchFenceCache* This = static_cast<FTouchFenceCache*>(Info);
		check(This);
		FScopeLock OwnedFencesLock(&This->OwnedFencesMutex);
		if (TSharedRef<FOwnedFenceData>* Owned = This->OwnedFences.Find(Handle))
		{
			Owned->Get().UpdateTouchUsage(Event);
			if (Owned->Get().IsReadyForReuse())
			{
				FScopeLock Lock(&This->ReadyForUsageMutex);
				This->ReadyForUsage.Enqueue(*Owned);
			}
		}
	}
}
