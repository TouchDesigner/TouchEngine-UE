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
#include "Util/TouchFenceCache.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "d3d12.h"
#include "Windows/HideWindowsPlatformTypes.h"

class UTexture2D;

namespace UE::TouchEngine::D3DX12
{
	class FTouchFenceCache;

	class FTouchTextureExporterD3D12
		: public FTouchTextureExporter
	{
		friend struct FRHICopyFromUnrealToVulkanAndSignalFence;
	public:
		
		FTouchTextureExporterD3D12(TSharedRef<FTouchFenceCache> FenceCache);
		virtual ~FTouchTextureExporterD3D12() override;
				
		//~ Begin FTouchTextureExporter Interface
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() override;
		virtual void InitializeExportsToTouchEngine_GameThread(const FTouchEngineInputFrameData& FrameData) override;
		virtual bool ShareTexture_RenderThread(const FTouchExportParameters& ParamsConst) override
		{
			const TSharedRef<FExportedTextureD3D12> Texture = StaticCastSharedRef<FExportedTextureD3D12>(ParamsConst.TextureToBeExported);
			return Texture->ShareTexture_RenderThread(SharedResourceSecurityAttributes);
		}

		FTouchFenceCache::TComPtr<ID3D12Fence> GetOrCreateSharedFence(const TouchObject<TESemaphore>& Semaphore)
		{
			return FenceCache->GetOrCreateSharedFence(Semaphore);
		}
		TSharedPtr<FTouchFenceCache::FFenceData> GetOrCreateOwnedFence_AnyThread(bool bForceNewFence = false)
		{
			return FenceCache->GetOrCreateOwnedFence_AnyThread(bForceNewFence);
		}
	protected:
		virtual TSharedPtr<FExportedTouchTexture> CreateTexture(UTexture* InTexture) override
		{
			return StaticCastSharedPtr<FExportedTouchTexture>(FExportedTextureD3D12::Create(SharedThis(this), InTexture));
		}
		virtual TEResult AddTETextureTransfer_RenderThread(const FTouchExportParameters& Params, const TSharedRef<FExportedTouchTexture>& Texture) override;
		//~ End FTouchTextureExporter Interface

	private:
		
		/** Used to wait on input texture being ready before modifying them */
		TSharedRef<FTouchFenceCache> FenceCache;
		
		/** Settings to use for opening shared textures */
		TSharedRef<FTextureShareD3D12SharedResourceSecurityAttributes> SharedResourceSecurityAttributes;
	};
}


