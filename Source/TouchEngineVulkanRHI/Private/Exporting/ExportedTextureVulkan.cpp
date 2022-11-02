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

#include "Exporting/ExportedTextureVulkan.h"

#include "Rendering/Exporting/TouchExportParams.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "TouchEngine/TEVulkan.h"
#include "Windows/HideWindowsPlatformTypes.h"
#else
#include "TouchEngine/TEVulkan.h"
#endif

namespace UE::TouchEngine::Vulkan
{
	TSharedPtr<FExportedTextureVulkan> FExportedTextureVulkan::Create(const FRHITexture2D& SourceRHI)
	{
		const EPixelFormat PixelFormat = SourceRHI.GetFormat();
		const FIntPoint Resolution = SourceRHI.GetSizeXY();
		TouchObject<TEVulkanTexture> SharedTouchTexture;
		// TODO:
		
		return MakeShared<FExportedTextureVulkan>(SharedTouchTexture, PixelFormat, Resolution);
	}
	
	FExportedTextureVulkan::FExportedTextureVulkan(TouchObject<TEVulkanTexture> SharedTexture, EPixelFormat PixelFormat, FIntPoint TextureBounds)
		: FExportedTouchTexture(MoveTemp(SharedTexture), [this](TouchObject<TETexture> Texture)
		{
			TEVulkanTexture* VulkanTexture = static_cast<TEVulkanTexture*>(Texture.get());
			TEVulkanTextureSetCallback(VulkanTexture, TouchTextureCallback, this);
		})
		, PixelFormat(PixelFormat)
		, Resolution(Resolution)
	{}

	bool FExportedTextureVulkan::CanFitTexture(const FTouchExportParameters& Params) const
	{
		const FRHITexture2D& SourceRHI = *Params.Texture->GetResource()->TextureRHI->GetTexture2D();
		return SourceRHI.GetSizeXY() == Resolution
			&& SourceRHI.GetFormat() == PixelFormat;
	}

	void FExportedTextureVulkan::TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info)
	{
		FExportedTextureVulkan* This = static_cast<FExportedTextureVulkan*>(Info);
		This->OnTouchTextureUseUpdate(Event);
	}
}
