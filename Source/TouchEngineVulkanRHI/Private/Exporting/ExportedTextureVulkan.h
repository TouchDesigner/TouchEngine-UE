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
#include "vulkan_core.h"

namespace UE::TouchEngine::Vulkan
{
	class FVulkanSharedResourceSecurityAttributes;

	class FExportedTextureVulkan : public FExportedTouchTexture
	{
		template <typename ObjectType, ESPMode Mode>
		friend class SharedPointerInternals::TIntrusiveReferenceController;
	public:

		static TSharedPtr<FExportedTextureVulkan> Create(const FRHITexture2D& SourceRHI, const TSharedRef<FVulkanSharedResourceSecurityAttributes>& SecurityAttributes);

		//~ Begin FExportedTouchTexture Interface
		virtual bool CanFitTexture(const FTouchExportParameters& Params) const override;
		//~ End FExportedTouchTexture Interface

	private:

		const EPixelFormat PixelFormat;
		const FIntPoint Resolution;

		const TSharedRef<VkImage> ImageOwnership;
		const TSharedRef<VkDeviceMemory> TextureMemoryOwnership;
		
		FExportedTextureVulkan(TouchObject<TEVulkanTexture> SharedTexture, EPixelFormat PixelFormat, FIntPoint Resolution, TSharedRef<VkImage> ImageOwnership, TSharedRef<VkDeviceMemory> TextureMemoryOwnership);
		
		static void TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info);
	};
}

