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

#pragma once

#include "CoreMinimal.h"
#include "Containers/CircularQueue.h"
#include "Rendering/Importing/TouchTextureImporter.h"
#include "Containers/Queue.h"
#include "Util/TouchEngineStatsGroup.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
THIRD_PARTY_INCLUDES_START
#include <wrl/client.h>
#include "d3d12.h"
THIRD_PARTY_INCLUDES_END
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

namespace UE::TouchEngine::D3DX12
{
	class FTouchFenceCache : public TSharedFromThis<FTouchFenceCache>
	{
	public:
		
		template<typename T>
		using TComPtr = Microsoft::WRL::ComPtr<T>;
		
		struct FFenceData
		{
			TComPtr<ID3D12Fence> NativeFence;
			TouchObject<TED3DSharedFence> TouchFence;
			uint64 LastValue;
			FString DebugName;
		};

		FTouchFenceCache(ID3D12Device* Device);
		~FTouchFenceCache();

		/**
		 * Gets or associates a (new) ID3D12Fence with the semaphore's handle.
		 * 
		 * The primary use case is for reusing fences for semaphores retrieved by TEInstanceGetTextureTransfer.
		 */
		TComPtr<ID3D12Fence> GetOrCreateSharedFence(const TouchObject<TESemaphore>& Semaphore);
		TComPtr<ID3D12Fence> GetSharedFence(HANDLE Handle) const;

		/**
		 * Gets or reuses a DX12 fence object that can be passed to TE. Once this pointer is reset and TE has seized using the semaphore,
		 * it is returned to the pool of available fences.
		 *
		 * The primary use case is for passing to TEInstanceAddTextureTransfer.
		 */
		TSharedPtr<FFenceData> GetOrCreateOwnedFence_AnyThread(bool bForceNewFence = false);
		
		/** To be called before destruction, to ensure that all the fence have fired their callbacks before we destroy this class */
		TFuture<FTouchSuspendResult> ReleaseFences();

	private:

		struct FSharedFenceData : FFenceData
		{};

		class FOwnedFenceData
		{
			TSharedRef<FFenceData> FenceData;
			
			TOptional<TEObjectEvent> Usage;
			bool bUsedByUnreal = true;

		public:

			FOwnedFenceData(TSharedRef<FFenceData> FenceData)
				: FenceData(MoveTemp(FenceData))
			{}
			~FOwnedFenceData();

			void ReleasedByUnreal();
			void UpdateTouchUsage(TEObjectEvent NewUsage);
			const TOptional<TEObjectEvent>& GetTouchUsage() const { return Usage; }

			bool IsReadyForReuse() const { return !bUsedByUnreal && (!Usage || Usage.GetValue() != TEObjectEventBeginUse); }
			TSharedRef<FFenceData> GetFenceData() const { return FenceData; } 
			TSharedRef<FFenceData>& GetFenceData() { return FenceData; } 
		};
		
		ID3D12Device* Device;
		
		mutable FCriticalSection SharedFencesMutex; // mutable for const functions
		/** Created using GetOrCreateSharedFence */
		TMap<HANDLE, FSharedFenceData> SharedFences;

		uint64 LastCreatedID = 0;

		/** Created using CreateUnrealOwnedFence */
		TMap<HANDLE, TSharedRef<FOwnedFenceData>> OwnedFences;
		FCriticalSection OwnedFencesMutex;
		/** When a fence is ready to be reused, it will be enqueued here. */
		TCircularQueue<TSharedPtr<FOwnedFenceData>> ReadyForUsage {10}; // Circular Queue to have access to the queue size and limit the amount of items
		FCriticalSection ReadyForUsageMutex;
		
		TSharedPtr<FOwnedFenceData> CreateOwnedFence_AnyThread();
		
		static void	SharedFenceCallback(HANDLE Handle, TEObjectEvent Event, void* TE_NULLABLE Info);
		static void	OwnedFenceCallback(HANDLE Handle, TEObjectEvent Event, void* TE_NULLABLE Info);
		
		/** Function called when the given SharedFence receives the event TEObjectEventRelease from TouchEngine. Note that the SharedFence is about to be deleted. */
		void OnSharedFenceReleasedByTouchEngine(const FSharedFenceData& SharedFence);
		/** Function called when the given OwnedFence receives the event TEObjectEventRelease from TouchEngine. */
		void OnOwnedFenceReleasedByTouchEngine(TSharedRef<FOwnedFenceData> OwnedFence);
		bool AreAllFencesReleased();

		FCriticalSection ReleaseFencesPromiseMutex;
		TOptional<TPromise<FTouchSuspendResult>> ReleaseFencesPromise;

		bool bIsGettingDestroyed = false;
	};

}

