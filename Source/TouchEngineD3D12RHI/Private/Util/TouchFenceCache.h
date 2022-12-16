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
#include "Rendering/Importing/TouchTextureImporter.h"
#include "Containers/Queue.h"

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
	class FTouchFenceCache
	{
	public:
		
		template<typename T>
		using TComPtr = Microsoft::WRL::ComPtr<T>;
		
		struct FFenceData
		{
			TComPtr<ID3D12Fence> NativeFence;
			TouchObject<TED3DSharedFence> TouchFence;
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
		 * Gets or reuses a DX12 fence object that can be pass to TE. Once this pointer is reset and TE has seized using the semaphore,
		 * it is returned to the pool of available fences.
		 *
		 * The primary use case is for passing to TEInstanceAddTextureTransfer.
		 *
		 * This must be called on the rendering thread to ensure that OwnedFences is not modified concurrently. The rendering thread
		 * was chosen because it was the most convenient to the code at the time; there is not direct dependency on this particular thread
		 * per se: so in the future, you could change the synchronization thread to another if it becomes more convenient.
		 */
		TSharedPtr<FFenceData> GetOrCreateOwnedFence_RenderThread();

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

			bool IsReadyForReuse() const { return !bUsedByUnreal && (!Usage || Usage.GetValue() != TEObjectEventBeginUse); }
			TSharedRef<FFenceData> GetFenceData() const { return FenceData; } 
		};
		
		ID3D12Device* Device;
		
		FCriticalSection SharedFencesMutex;
		/** Created using GetOrCreateSharedFence */
		TMap<HANDLE, FSharedFenceData> SharedFences;

		/** Created using CreateUnrealOwnedFence */
		TMap<HANDLE, TSharedRef<FOwnedFenceData>> OwnedFences;
		/** When a fence is ready to be reused, it will be enqueued here. */
		TQueue<TSharedPtr<FOwnedFenceData>, EQueueMode::Mpsc> ReadyForUsage;

		TSharedPtr<FOwnedFenceData> CreateOwnedFence_RenderThread();
		
		static void	SharedFenceCallback(HANDLE Handle, TEObjectEvent Event, void* TE_NULLABLE Info);
		static void	OwnedFenceCallback(HANDLE Handle, TEObjectEvent Event, void* TE_NULLABLE Info);
	};

}

