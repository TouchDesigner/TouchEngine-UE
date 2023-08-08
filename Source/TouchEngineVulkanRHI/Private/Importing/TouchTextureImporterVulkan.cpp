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

#include "TouchTextureImporterVulkan.h"

#include "Logging.h"
#include "Util/TextureShareVulkanPlatformWindows.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include "Windows/MinimalWindowsApi.h"
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "TouchImportTextureVulkan.h"
#include "VulkanTouchUtils.h"
#include "TouchEngine/TEVulkan.h"
#include "Util/TouchEngineStatsGroup.h"

namespace UE::TouchEngine::Vulkan
{
	FTouchTextureImporterVulkan::FTouchTextureImporterVulkan(TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes)
		: SecurityAttributes(MoveTemp(SecurityAttributes))
	{}

	FTouchTextureImporterVulkan::~FTouchTextureImporterVulkan()
	{
		FScopeLock Lock(&CachedTexturesMutex);
		// Make sure TE is not left with a dangling this pointer
		for (auto Pair : CachedTextures)
		{
			TEVulkanTextureSetCallback(Pair.Value->GetSharedTexture(), nullptr, nullptr);
		}
		CachedTextures.Empty();
	}

	void FTouchTextureImporterVulkan::ConfigureInstance(const TouchObject<TEInstance>& Instance)
	{
		TEInstanceSetVulkanOutputAcquireImageLayout(Instance, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	}
	
	TSharedPtr<ITouchImportTexture> FTouchTextureImporterVulkan::CreatePlatformTexture_RenderThread(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture)
	{
		const TSharedPtr<FTouchImportTextureVulkan> Texture = GetOrCreateSharedTexture(SharedTexture);
		return StaticCastSharedPtr<ITouchImportTexture>(Texture); //Future;
	}

	FTextureMetaData FTouchTextureImporterVulkan::GetTextureMetaData(const TouchObject<TETexture>& Texture) const
	{
		const TEVulkanTexture_* Source = static_cast<TEVulkanTexture_*>(Texture.get());
		const VkFormat FormatVk = TEVulkanTextureGetFormat(Source);
		FTextureMetaData Result;
		Result.SizeX = TEVulkanTextureGetWidth(Source);
		Result.SizeY = TEVulkanTextureGetHeight(Source);
		Result.PixelFormat = VulkanToUnrealTextureFormat(FormatVk, Result.IsSRGB);
		return Result;
	}

	FTouchTextureTransfer FTouchTextureImporterVulkan::GetTextureTransfer(const FTouchImportParameters& ImportParams)
	{
		FTouchTextureTransfer Transfer;
		VkImageLayout AcquireOldLayout;
		VkImageLayout AcquireNewLayout;
		Transfer.Result = TEInstanceGetVulkanTextureTransfer(ImportParams.Instance, ImportParams.TETexture, &AcquireOldLayout,
			&AcquireNewLayout, Transfer.Semaphore.take(), &Transfer.WaitValue);
		Transfer.VulkanOldLayout = AcquireOldLayout;
		Transfer.VulkanNewLayout = AcquireNewLayout;
		return Transfer;
	}

	TSharedPtr<FTouchImportTextureVulkan> FTouchTextureImporterVulkan::GetOrCreateSharedTexture(const TouchObject<TETexture>& Texture)
	{
		check(TETextureGetType(Texture) == TETextureTypeVulkan);
		TouchObject<TEVulkanTexture_> Shared;
		Shared.set(static_cast<TEVulkanTexture_*>(Texture.get()));
		const HANDLE Handle = TEVulkanTextureGetHandle(Shared);
		
		FScopeLock Lock(&CachedTexturesMutex);
		{
			if (const TSharedPtr<FTouchImportTextureVulkan> Existing = GetSharedTexture_Unsynchronized(Handle))
			{
				return Existing;
			}
		
			const TSharedPtr<FTouchImportTextureVulkan> CreationResult = FTouchImportTextureVulkan::CreateTexture(Shared, SecurityAttributes);
			if (!CreationResult)
			{
				return nullptr;
			}

			TEVulkanTextureSetCallback(Shared, TextureCallback, this);
			CachedTextures.Add(Handle, CreationResult.ToSharedRef());
			return CreationResult;
		}
	}

	TSharedPtr<FTouchImportTextureVulkan> FTouchTextureImporterVulkan::GetSharedTexture_Unsynchronized(FHandle Handle) const
	{
		const TSharedRef<FTouchImportTextureVulkan>* Result = CachedTextures.Find(Handle);
		return Result
			? *Result
			: TSharedPtr<FTouchImportTextureVulkan>{ nullptr };
	}

	void FTouchTextureImporterVulkan::TextureCallback(FHandle Handle, TEObjectEvent Event, void* Info)
	{
		if (Info && Event == TEObjectEventRelease)
		{
			// TODO Maybe synchronize this with ENQUEUE_RENDER_COMMAND instead?
			FTouchTextureImporterVulkan* This = static_cast<FTouchTextureImporterVulkan*>(Info);
			FScopeLock Lock(&This->CachedTexturesMutex);
			This->CachedTextures.Remove(Handle);
		}
	}
}
