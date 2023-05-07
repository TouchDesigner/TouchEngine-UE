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
#include "Rendering/Exporting/TouchTextureExporter.h"
#include "TouchEngine/TouchObject.h"
#include "TextureShareD3D12PlatformWindows.h"

// #include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include "d3d12.h"
#include "Rendering/Exporting/ExportedTouchTextureCache.h"
#include "Util/TouchFenceCache.h"
#include "wrl/client.h"
#include "Windows/PostWindowsApi.h"
// #include "Windows/HideWindowsPlatformTypes.h"

class UTexture2D;

namespace UE::TouchEngine::D3DX12
{
	class FTouchFenceCache;
	class FExportedTextureD3D12;

	struct FDummy {};

	class FTouchTextureExporterD3D12
		: public FTouchTextureExporter
		, public TExportedTouchTextureCache<FExportedTextureD3D12, FDummy, FTouchTextureExporterD3D12>
	{
		friend struct FRHICopyFromUnrealToVulkanCommand;
	public:

		static TSharedPtr<FTouchTextureExporterD3D12> Create(TSharedRef<FTouchFenceCache> FenceCache);

		FTouchTextureExporterD3D12(TSharedRef<FTouchFenceCache> FenceCache);

		//~ Begin FTouchTextureExporter Interface
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() override;
		//~ End FTouchTextureExporter Interface

		//~ Begin TExportedTouchTextureCache Interface
		TSharedPtr<FExportedTextureD3D12> CreateTexture(const FTextureCreationArgs& Params) const;
		//~ End TExportedTouchTextureCache Interface

		virtual void PrepareForExportToTouchEngine_AnyThread() override;
		virtual void FinalizeExportToTouchEngine_AnyThread() override;
		
	protected:

		//~ Begin FTouchTextureExporter Interface
		virtual bool GetNextOrAllocPooledTETexture_Internal(const FTouchExportParameters& TouchExportParameters, bool& bIsNewTexture, TouchObject<TETexture>& OutTexture) override;
		virtual TouchObject<TETexture> ExportTexture_AnyThread(const FTouchExportParameters& Params, TEGraphicsContext* GraphicsContext) override;
		//~ End FTouchTextureExporter Interface

	private:

		uint32 NumberTextureNeedingCopy = 0;
		// TSharedPtr<FTouchFenceCache::FFenceData> GraphicCopyFence;
		// uint64 GraphicCopyFenceWaitValue;
		// TRefCountPtr<ID3D12GraphicsCommandList> GraphicCopyCommandList;
		// TRefCountPtr<ID3D12CommandAllocator> GraphicCopyCommandAllocator;
		// TRefCountPtr<ID3D12CommandQueue> GraphicCopyCommandQueue;
		
		/** Used to wait on input texture being ready before modifying them */
		TSharedRef<FTouchFenceCache> FenceCache;
		
		/** The Native D3D Fence used by this exporter */
		// Microsoft::WRL::ComPtr<ID3D12Fence> FenceNative;
		/** The TouchEngine fence that is linked to FenceNative */
		// TouchObject<TED3DSharedFence> FenceTE;
		// uint64 NextFenceValue = 0;
		
		/** Settings to use for opening shared textures */
		FTextureShareD3D12SharedResourceSecurityAttributes SharedResourceSecurityAttributes;
		
		void ScheduleWaitFence(const TouchObject<TESemaphore>& AcquireSemaphore, uint64 AcquireValue) const;
	public:
		// uint64 GetNextFenceValue() const { return NextFenceValue + 1; };
	};
}


