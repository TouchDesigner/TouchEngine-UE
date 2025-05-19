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
#include "Rendering/Exporting/ExportedTouchTexture.h"
#include "Util/TouchFenceCache.h"

namespace UE::TouchEngine::D3DX12
{
	class FTextureShareD3D12SharedResourceSecurityAttributes;
	
	class FExportedTextureD3D12 : public FExportedTouchTexture
	{
		friend class FTouchTextureExporterD3D12;
	public:
		
		static TSharedPtr<FExportedTextureD3D12> Create(const TSharedRef<FTouchTextureExporterD3D12>& InExporter, UTexture* InTexture);
		FExportedTextureD3D12(const TSharedRef<FTouchTextureExporterD3D12>& InExporter);

		virtual bool EnqueueTextureCopy(UTexture* SrcTexture) override;
		bool ShareTexture_RenderThread(const TSharedRef<FTextureShareD3D12SharedResourceSecurityAttributes>& SharedResourceSecurityAttributes);

	private:
		TWeakPtr<FTouchTextureExporterD3D12> WeakExporter;
		
		/** Handle to the shared resource */
		void* ResourceSharingHandle_RenderThread = nullptr;
		/** The Fence signalled when the copy is completed */
		TSharedRef<FTouchFenceCache::FFenceData> CopyCompletedFence;
		
		static void TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info);
	};

}

