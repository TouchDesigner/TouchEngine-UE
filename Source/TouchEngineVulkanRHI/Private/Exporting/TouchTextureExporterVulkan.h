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
#include "Rendering/Exporting/ExportedTouchTextureCache.h"
#include "Rendering/Exporting/TouchTextureExporter.h"

class UTexture2D;

namespace UE::TouchEngine::Vulkan
{
	class FVulkanSharedResourceSecurityAttributes;
	
	class FTouchTextureExporterVulkan
		: public FTouchTextureExporter
		, public TExportedTouchTextureCache<FExportedTextureVulkan, FTouchTextureExporterVulkan>
	{
	public:

		FTouchTextureExporterVulkan(TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes);
		
		//~ Begin FTouchTextureExporter Interface
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() override;
		//~ End FTouchTextureExporter Interface
		
		//~ Begin TExportedTouchTextureCache Interface
		TSharedPtr<FExportedTextureVulkan> CreateTexture(const FTouchExportParameters& Params, const FRHITexture2D* ParamTextureRHI) const;
		void FinalizeExportsToTouchEngine_AnyThread(const FTouchEngineInputFrameData& FrameData);
		//~ End TExportedTouchTextureCache Interface
		
	protected:
		//~ Begin TExportedTouchTextureCache Interface
		virtual TEResult AddTETextureTransfer(FTouchExportParameters& Params, const TSharedPtr<FExportedTextureVulkan>& Texture) override;
		virtual void FinaliseExportAndEnqueueCopy_AnyThread(FTouchExportParameters& Params, TSharedPtr<FExportedTextureVulkan>& Texture) override;
		//~ End TExportedTouchTextureCache Interface
		
		//~ Begin FTouchTextureExporter Interface
		virtual TouchObject<TETexture> ExportTexture_AnyThread(const FTouchExportParameters& Params, TEGraphicsContext* GraphicsContext) override
		{
			return ExportTextureToTE_AnyThread(Params, GraphicsContext);
		}
		//~ End FTouchTextureExporter Interface

	private:

		TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes;
	};
}

