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
#include "ExportedTextureD3D12.h" // cannot forward declare due to TExportedTouchTextureCache

#include "Rendering/Exporting/ExportedTouchTextureCache.h"
#include "Util/TouchFenceCache.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include "d3d12.h"
#include "wrl/client.h"
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

class UTexture2D;

namespace UE::TouchEngine::D3DX12
{
	class FTouchFenceCache;

	class FTouchTextureExporterD3D12
		: public FTouchTextureExporter
		, public TExportedTouchTextureCache<FExportedTextureD3D12, FTouchTextureExporterD3D12>
	{
		friend struct FRHICopyFromUnrealToVulkanAndSignalFence;
	public:
		
		FTouchTextureExporterD3D12(TSharedRef<FTouchFenceCache> FenceCache);
		virtual ~FTouchTextureExporterD3D12() override;
				
		//~ Begin FTouchTextureExporter Interface
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() override;
		//~ End FTouchTextureExporter Interface

		//~ Begin TExportedTouchTextureCache Interface
		TSharedPtr<FExportedTextureD3D12> CreateTexture(const FTouchExportParameters& Params, const FRHITexture2D* ParamTextureRHI) const;
		void FinalizeExportsToTouchEngine_AnyThread(const FTouchEngineInputFrameData& FrameData);
		//~ End TExportedTouchTextureCache Interface

	protected:

		//~ Begin TExportedTouchTextureCache Interface
		virtual TEResult AddTETextureTransfer(FTouchExportParameters& Params, const TSharedPtr<FExportedTextureD3D12>& Texture) override;
		virtual void FinaliseExportAndEnqueueCopy_AnyThread(FTouchExportParameters& Params, TSharedPtr<FExportedTextureD3D12>& Texture) override;
		//~ End TExportedTouchTextureCache Interface

		//~ Begin FTouchTextureExporter Interface
		virtual TouchObject<TETexture> ExportTexture_AnyThread(const FTouchExportParameters& Params, TEGraphicsContext* GraphicsContext) override
		{
			return ExportTextureToTE_AnyThread(Params, GraphicsContext);
		}
		//~ End FTouchTextureExporter Interface

	private:
		
		/** Used to wait on input texture being ready before modifying them */
		TSharedRef<FTouchFenceCache> FenceCache;

		/** Custom CommandQueue separate from UE's one, used only for exporting. It makes it easier to manage, especially as Dx12 uses a lot of Async functions */
		TRefCountPtr<ID3D12CommandQueue> D3DCommandQueue;
		TSharedPtr<FTouchFenceCache::FFenceData> CommandQueueFence;

		struct FExportCopyParams
		{
			FTouchExportParameters ExportParams;
			TSharedPtr<FExportedTextureD3D12> DestinationTETexture;
		};
		TArray<FExportCopyParams> TextureExports;
		
		/** Settings to use for opening shared textures */
		FTextureShareD3D12SharedResourceSecurityAttributes SharedResourceSecurityAttributes;
		
		// void ScheduleWaitFence(const TouchObject<TESemaphore>& AcquireSemaphore, uint64 AcquireValue) const;
	};
}


