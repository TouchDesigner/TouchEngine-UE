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
#include "TouchImportTextureVulkan.h"
#include "Rendering/Importing/TouchTextureImporter.h"

namespace UE::TouchEngine::Vulkan
{
	class FVulkanSharedResourceSecurityAttributes;
	class FTouchImportTextureVulkan;

	class FTouchTextureImporterVulkan : public FTouchTextureImporter
	{
		friend struct FRHIGetOrCreateSharedTexture;
	public:

		using FHandle = void*;

		FTouchTextureImporterVulkan(TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes);
		virtual ~FTouchTextureImporterVulkan() override;
		
		void ConfigureInstance(const TouchObject<TEInstance>& Instance);

	protected:

		//~ Begin FTouchTextureImporter Interface
		virtual TSharedPtr<ITouchImportTexture> CreatePlatformTexture_RenderThread(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture) override;
		virtual FTextureMetaData GetTextureMetaData(const TouchObject<TETexture>& Texture) const override;
		virtual FTouchTextureTransfer GetTextureTransfer(const FTouchImportParameters& ImportParams) override;
		//~ End FTouchTextureImporter Interface

	private:

		/** Must be acquired to access CachedTextures */
		FCriticalSection CachedTexturesMutex;
		TMap<FHandle, TSharedRef<FTouchImportTextureVulkan>> CachedTextures;

		TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes;

		TSharedPtr<FTouchImportTextureVulkan> GetOrCreateSharedTexture(const TouchObject<TETexture>& Texture);
		TSharedPtr<FTouchImportTextureVulkan> GetSharedTexture_Unsynchronized(FHandle Handle) const;
		
		static void TextureCallback(FHandle Handle, TEObjectEvent Event, void* TE_NULLABLE Info);
	};
}


