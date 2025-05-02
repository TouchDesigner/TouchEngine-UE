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
#include "ExportedTextureVulkan.h"
#include "Rendering/Exporting/TouchTextureExporter.h"

class UTexture2D;

namespace UE::TouchEngine::Vulkan
{
	class FVulkanSharedResourceSecurityAttributes;
	
	class FTouchTextureExporterVulkan
		: public FTouchTextureExporter
	{
	public:

		FTouchTextureExporterVulkan(TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes);
		
		//~ Begin FTouchTextureExporter Interface
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() override;
		virtual void InitializeExportsToTouchEngine_GameThread(const FTouchEngineInputFrameData& FrameData) override;
		virtual void FinalizeExportsToTouchEngine_GameThread(const FTouchEngineInputFrameData& FrameData) override;
		virtual bool ShareTexture_RenderThread(const FTouchExportParameters& ParamsConst) override
		{
			TSharedPtr<FExportedTextureVulkan> Texture = StaticCastSharedPtr<FExportedTextureVulkan>(ParamsConst.TextureToBeExported);
			if (ensure(Texture))
			{
				return Texture->ShareTexture_RenderThread(); //todo Make this function virtual instead of the TextureExporter one
			}
			return false;
		}


	protected:
		virtual TSharedPtr<FExportedTouchTexture> CreateTexture(UTexture* InTexture) override
		{
			return StaticCastSharedPtr<FExportedTouchTexture>(FExportedTextureVulkan::Create(InTexture, SecurityAttributes));
		}
		virtual TEResult AddTETextureTransfer_RenderThread(const FTouchExportParameters& Params, const TSharedPtr<FExportedTouchTexture>& Texture) override;
		virtual void FinaliseExport_RenderThread(const FTouchExportParameters& Params, TSharedPtr<FExportedTouchTexture>& Texture) override;
		//~ End FTouchTextureExporter Interface

	private:

		TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes;
	};
}

