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
#include "Engine/TEDebug.h"
#include "TouchEngine/TED3D.h"

namespace UE::TouchEngine::D3DX12
{
	DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("FenceCache - No SharedFence"), STAT_TE_FenceCache_SharedFence, STATGROUP_TouchEngine)
	DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("FenceCache - No OwnedFence"), STAT_TE_FenceCache_OwnedFence, STATGROUP_TouchEngine)
	
	FTouchFenceCache::FTouchFenceCache(ID3D12Device* Device)
		: Device(Device)
	{}

	FTouchFenceCache::~FTouchFenceCache()
	{
		{
			FScopeLock Lock(&SharedFencesMutex);
			for(TTuple<void*, FSharedFenceData>& FenceData : SharedFences)
			{
				TED3DSharedFenceSetCallback(FenceData.Value.TouchFence.get(), nullptr, nullptr);
				FenceData.Value.TouchFence.reset();
			}
			SharedFences.Empty();
		}
		{
			FScopeLock Lock(&OwnedFencesMutex);
			for(const TTuple<void*, TSharedRef<FOwnedFenceData>>& OwnedFenceData : OwnedFences)
			{
				TED3DSharedFenceSetCallback(OwnedFenceData.Value->GetFenceData()->TouchFence.get(), nullptr, nullptr);
				OwnedFenceData.Value->GetFenceData()->TouchFence.reset();
			}
			OwnedFences.Empty();
		}
		{
			FScopeLock Lock(&ReadyForUsageMutex);
			TSharedPtr<FOwnedFenceData> OwnedFenceData;
			while (ReadyForUsage.Dequeue(OwnedFenceData))
			{
				if (OwnedFenceData)
				{
					TED3DSharedFenceSetCallback(OwnedFenceData->GetFenceData()->TouchFence.get(), nullptr, nullptr);
					OwnedFenceData->GetFenceData()->TouchFence.reset();
				}
			}
		}
		SET_DWORD_STAT(STAT_TE_FenceCache_SharedFence, 0)
		SET_DWORD_STAT(STAT_TE_FenceCache_OwnedFence, 0)
	}

	FTouchFenceCache::TComPtr<ID3D12Fence> FTouchFenceCache::GetOrCreateSharedFence(const TouchObject<TESemaphore>& Semaphore)
	{
		if (!ensureMsgf(!bIsGettingDestroyed, TEXT("FTouchFenceCache::GetOrCreateSharedFence was called while the FTouchFenceCache was getting destroyed")))
		{
			return nullptr;
		}
		
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
		INC_DWORD_STAT(STAT_TE_FenceCache_SharedFence);
		
		TED3DSharedFenceSetCallback(static_cast<TED3DSharedFence*>(Semaphore.get()), SharedFenceCallback, this);
		TouchObject<TED3DSharedFence> FenceObject;
		FenceObject.set(static_cast<TED3DSharedFence*>(Semaphore.get()));
		SharedFences.Add(Handle, { Fence, FenceObject });
		return Fence;
	}

	FTouchFenceCache::TComPtr<ID3D12Fence> FTouchFenceCache::GetSharedFence(HANDLE Handle) const
	{
		if (!ensureMsgf(!bIsGettingDestroyed, TEXT("FTouchFenceCache::GetSharedFence was called while the FTouchFenceCache was getting destroyed")))
		{
			return nullptr;
		}

		FScopeLock Lock(&SharedFencesMutex);
		const FSharedFenceData* FenceData = SharedFences.Find(Handle);
		return FenceData && ensure(FenceData->NativeFence)
			? FenceData->NativeFence
			: TComPtr<ID3D12Fence>{ nullptr };
	}

	TSharedPtr<FTouchFenceCache::FFenceData> FTouchFenceCache::GetOrCreateOwnedFence_AnyThread(bool bForceNewFence)
	{
		if (!ensureMsgf(!bIsGettingDestroyed, TEXT("FTouchFenceCache::GetOrCreateOwnedFence_AnyThread was called while the FTouchFenceCache was getting destroyed")))
		{
			return nullptr;
		}

		TSharedPtr<FOwnedFenceData> OwnedData;
		{
			if (!bForceNewFence)
			{
				FScopeLock Lock(&ReadyForUsageMutex);
				ReadyForUsage.Dequeue(OwnedData);
			}
			if (OwnedData)
			{
				OwnedData->GetFenceData()->LastValue = OwnedData->GetFenceData()->NativeFence->GetCompletedValue();
				UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("Reusing owned fence `%s` of inital value: `%llu` (GetCompletedValue returned: `%llu`)"),
					*OwnedData->GetFenceData()->DebugName, OwnedData->GetFenceData()->LastValue, OwnedData->GetFenceData()->NativeFence->GetCompletedValue());
			}
			else
			{
				OwnedData = CreateOwnedFence_AnyThread();
				UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("Creating new owned fence `%s` of inital value: `%llu` (GetCompletedValue returned: `%llu`)"),
					*OwnedData->GetFenceData()->DebugName, OwnedData->GetFenceData()->LastValue, OwnedData->GetFenceData()->NativeFence->GetCompletedValue());
			}
		}

		return MakeShareable<FFenceData>(&OwnedData->GetFenceData().Get(), [WeakThis = AsWeak(), OwnedData](FFenceData*)
		{
			OwnedData->ReleasedByUnreal();
			if (OwnedData->IsReadyForReuse())
			{
				const TSharedPtr<FTouchFenceCache> PinThis = WeakThis.Pin();
				if (PinThis)
				{
					FScopeLock Lock(&PinThis->ReadyForUsageMutex);
					PinThis->ReadyForUsage.Enqueue(OwnedData); // If the queue is full, this item will not be enqueued and will end up being destroyed
				}
			}
		});
	}

	TFuture<FTouchSuspendResult> FTouchFenceCache::ReleaseFences()
	{
		check(!bIsGettingDestroyed);
		bIsGettingDestroyed = true;
		
		{
			FScopeLock Lock(&ReleaseFencesPromiseMutex);
			check(!ReleaseFencesPromise.IsSet());

			if (AreAllFencesReleased())
			{
				ReleaseFencesPromise = MakeFulfilledPromise<FTouchSuspendResult>();
				return ReleaseFencesPromise->GetFuture();
			}
			ReleaseFencesPromise = TPromise<FTouchSuspendResult>();
		}

		// Now force the release of the fences from our side
		{
			FScopeLock Lock(&SharedFencesMutex);
			TArray<HANDLE> Handles;
			SharedFences.GetKeys(Handles);
			for(const HANDLE& Handle : Handles)
			{
				if (FSharedFenceData* FenceData = SharedFences.Find(Handle))
				{
					FenceData->TouchFence.reset(); // this will end up calling SharedFenceCallback which will modify the TMap SharedFences
				}
			}
		}
		{
			FScopeLock Lock(&OwnedFencesMutex);
			for(const TTuple<void*, TSharedRef<FOwnedFenceData>>& OwnedFenceData : OwnedFences)
			{
				OwnedFenceData.Value->GetFenceData()->TouchFence.reset();
			}
		}
		{
			FScopeLock Lock(&ReadyForUsageMutex);
			TSharedPtr<FOwnedFenceData> OwnedFenceData;
			while (ReadyForUsage.Dequeue(OwnedFenceData))
			{
				if (OwnedFenceData)
				{
					OwnedFenceData->GetFenceData()->TouchFence.reset();
				}
			}
		}
		
		return ReleaseFencesPromise->GetFuture();
	}

	TSharedPtr<FTouchFenceCache::FOwnedFenceData> FTouchFenceCache::CreateOwnedFence_AnyThread()
	{
		if (!ensureMsgf(!bIsGettingDestroyed, TEXT("FTouchFenceCache::CreateOwnedFence_AnyThread was called while the FTouchFenceCache was getting destroyed")))
		{
			return nullptr;
		}

		Microsoft::WRL::ComPtr<ID3D12Fence> FenceNative;
		constexpr uint64 InitialValue = 0;
		HRESULT ErrorResultCode = Device->CreateFence(InitialValue, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&FenceNative));
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
		UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("About to create Fence %s  %p..."), *FString::Printf(TEXT("__fence_%lld"), LastCreatedID+1), SharedFenceHandle)
		FenceTE.take(TED3DSharedFenceCreate(SharedFenceHandle, OwnedFenceCallback, this));
		// TouchEngine duplicates the handle, so close it now
		CloseHandle(SharedFenceHandle);
		if (!FenceTE)
		{
			return nullptr;
		}
		
		INC_DWORD_STAT(STAT_TE_FenceCache_OwnedFence);
		const TSharedPtr<FFenceData> FenceData = MakeShared<FFenceData>(FSharedFenceData{ FenceNative , FenceTE });
		const TSharedRef<FOwnedFenceData> OwnedData = MakeShared<FOwnedFenceData>(FenceData.ToSharedRef());
		
		{
			HANDLE NewFenceHandle = TED3DSharedFenceGetHandle(FenceTE);
			FenceData->DebugName = FString::Printf(TEXT("__fence_%lld"), ++LastCreatedID);
			UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("Creating Fence %s   Old Handle: %p  New Handle: %p"), *FenceData->DebugName, SharedFenceHandle, NewFenceHandle)
			FScopeLock Lock(&OwnedFencesMutex);
			if (!ensure(!OwnedFences.Find(NewFenceHandle)))
			{
				UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("Fence '%s' Already exist for this handle: %p"), *OwnedFences.Find(NewFenceHandle)->Get().GetFenceData()->DebugName, SharedFenceHandle)
			}
			OwnedFences.Add(NewFenceHandle, OwnedData);
		}
		
		return OwnedData;
	}

	FTouchFenceCache::FOwnedFenceData::~FOwnedFenceData()
	{
		DEC_DWORD_STAT(STAT_TE_FenceCache_OwnedFence);
		if (FenceData->NativeFence)
		{
			UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("Releasing NativeFence '%s' with value completedvalue %lld, and LastValue %lld.  Usage: %s"),
				*FenceData->DebugName, FenceData->NativeFence->GetCompletedValue(), FenceData->LastValue, Usage ? *TEObjectEventToString(Usage.GetValue()) : TEXT("<not set>"))
			FenceData->NativeFence->Release();
		}
	}

	void FTouchFenceCache::FOwnedFenceData::ReleasedByUnreal()
	{
		bUsedByUnreal = false;
	}

	void FTouchFenceCache::FOwnedFenceData::UpdateTouchUsage(TEObjectEvent NewUsage)
	{
		UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("[UpdateTouchUsage] '%s' usage => %s"), *FenceData->DebugName, *TEObjectEventToString(NewUsage));
		Usage = NewUsage;
	}

	void FTouchFenceCache::OnSharedFenceReleasedByTouchEngine(const FSharedFenceData& SharedFence)
	{
		FScopeLock Lock(&ReleaseFencesPromiseMutex);
		if (ReleaseFencesPromise && AreAllFencesReleased())
		{
			ReleaseFencesPromise->SetValue({});
		}
	}

	void FTouchFenceCache::OnOwnedFenceReleasedByTouchEngine(TSharedRef<FOwnedFenceData> OwnedFence)
	{
		FScopeLock Lock(&ReleaseFencesPromiseMutex);
		if (ReleaseFencesPromise && AreAllFencesReleased())
		{
			ReleaseFencesPromise->SetValue({});
		}
	}

	bool FTouchFenceCache::AreAllFencesReleased()
	{
		bool bAllFencesReleased;
		{
			FScopeLock SharedFenceLock(&SharedFencesMutex);
			bAllFencesReleased = SharedFences.IsEmpty();
			if (!bAllFencesReleased)
			{
				UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("[AreAllFencesReleased] %d SharedFences remaining..."), SharedFences.Num());
				for(TTuple<HANDLE, FSharedFenceData>& Fence : SharedFences)
				{
					UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("    - '%s'   %p"), *Fence.Value.DebugName, Fence.Key);
				}
			}
		}
		if (bAllFencesReleased)
		{
			FScopeLock OwnedFenceLock(&OwnedFencesMutex);
			UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("[AreAllFencesReleased] %d OwnedFences remaining..."), OwnedFences.Num());
			for(const TTuple<void*, TSharedRef<FOwnedFenceData>> OwnedFenceData : OwnedFences)
			{
				// OwnedFenceData.Value->GetFenceData()->TouchFence.reset();
				const TOptional<TEObjectEvent>& Usage = OwnedFenceData.Value->GetTouchUsage();
				UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("    - '%s'   %p   usage => %s"), *OwnedFenceData.Value->GetFenceData()->DebugName, OwnedFenceData.Key,
					Usage ? *TEObjectEventToString(Usage.GetValue()) : TEXT("<not set>"));
				if (Usage && Usage.GetValue() != TEObjectEventRelease)
				{
					bAllFencesReleased = false;
					break;
				}
			}
		}
		return bAllFencesReleased;
	}

	void FTouchFenceCache::SharedFenceCallback(HANDLE Handle, TEObjectEvent Event, void* Info)
	{
		if (Event == TEObjectEventRelease)
		{
			DEC_DWORD_STAT(STAT_TE_FenceCache_SharedFence);
			FTouchFenceCache* This = static_cast<FTouchFenceCache*>(Info);
			FSharedFenceData SharedFence;
			{
				FScopeLock Lock(&This->SharedFencesMutex);
				This->SharedFences.RemoveAndCopyValue(Handle, SharedFence);
				UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("Releasing SharedFence Handle %p '%s'"), Handle, *SharedFence.DebugName);
			}
			This->OnSharedFenceReleasedByTouchEngine(SharedFence);
		}
	}

	void FTouchFenceCache::OwnedFenceCallback(HANDLE Handle, TEObjectEvent Event, void* Info)
	{
		FTouchFenceCache* This = static_cast<FTouchFenceCache*>(Info);
		check(This);
		FScopeLock OwnedFencesLock(&This->OwnedFencesMutex);
		if (const TSharedRef<FOwnedFenceData>* Owned = This->OwnedFences.Find(Handle))
		{
			Owned->Get().UpdateTouchUsage(Event);
			if (Owned->Get().IsReadyForReuse())
			{
				FScopeLock Lock(&This->ReadyForUsageMutex);
				This->ReadyForUsage.Enqueue(*Owned); // If the queue is full, this item will not be enqueued and will end up being destroyed
			}
			if (Event == TEObjectEventRelease)
			{
				UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("Releasing OwnedFence Handle %p '%s'"), Handle, *Owned->Get().GetFenceData()->DebugName);
				This->OnOwnedFenceReleasedByTouchEngine(*Owned);
			}
		}
	}
}
