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

#include "VulkanRHIWrappers.h"

#include "vulkan_core.h"
#include "VulkanRHIPrivate.h"
#include "VulkanGetterUtils.h"

namespace UE::TouchEngine::Vulkan
{
	void EnqueueCopyTextureOnUploadQueue(FVulkanCommandListContext& VulkanContext, FRHITexture* SourceTexture, FRHITexture* DestTexture)
	{
		// Minimised version of FVulkanCommandListContext::RHICopyTexture
		FVulkanTextureBase* Source = static_cast<FVulkanTextureBase*>(SourceTexture->GetTextureBaseRHI());
		FVulkanTextureBase* Dest = static_cast<FVulkanTextureBase*>(DestTexture->GetTextureBaseRHI());

		FVulkanSurface& SrcSurface = Source->Surface;
		FVulkanSurface& DstSurface = Dest->Surface;

		FVulkanLayoutManager& LayoutManager = VulkanContext.GetLayoutManager();
		VkImageLayout SrcLayout = LayoutManager.FindLayoutChecked(SrcSurface.Image);
		ensureMsgf(SrcLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXT("Expected source texture to be in VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, actual layout is %d"), SrcLayout);

		VkImageLayout DstLayout = LayoutManager.FindLayoutChecked(DstSurface.Image);
		ensureMsgf(DstLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXT("Expected destination texture to be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, actual layout is %d"), DstLayout);

		VkImageCopy Region;
		FMemory::Memzero(Region);
		const FPixelFormatInfo& PixelFormatInfo = GPixelFormats[DestTexture->GetFormat()];
		ensure(SrcSurface.Width <= DstSurface.Width && SrcSurface.Height <= DstSurface.Height);
		Region.extent.width = FMath::Max<uint32>(PixelFormatInfo.BlockSizeX, SrcSurface.Width);
		Region.extent.height = FMath::Max<uint32>(PixelFormatInfo.BlockSizeY, SrcSurface.Height);
		Region.extent.depth = FMath::Max<uint32>(PixelFormatInfo.BlockSizeZ, SrcSurface.Depth);
		Region.srcSubresource.aspectMask = SrcSurface.GetFullAspectMask();
		Region.srcSubresource.layerCount = 1;
		Region.dstSubresource.aspectMask = DstSurface.GetFullAspectMask();
		Region.dstSubresource.layerCount = 1;

		FVulkanCmdBuffer* InCmdBuffer = VulkanContext.GetCommandBufferManager()->GetUploadCmdBuffer();
		check(InCmdBuffer->IsOutsideRenderPass());
		VkCommandBuffer CmdBuffer = InCmdBuffer->GetHandle();
		VulkanRHI::vkCmdCopyImage(CmdBuffer, SrcSurface.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstSurface.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
	}
}