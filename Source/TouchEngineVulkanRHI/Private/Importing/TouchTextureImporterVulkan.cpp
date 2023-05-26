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
#include "Windows/MinimalWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "TouchImportTextureVulkan.h"
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
	
	TSharedPtr<ITouchImportTexture> FTouchTextureImporterVulkan::CreatePlatformTexture_AnyThread(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Link Texture Import - Create Platform Texture"), STAT_LinkTextureImportPlatformTexture, STATGROUP_TouchEngine);
		const TSharedPtr<FTouchImportTextureVulkan> Texture = GetOrCreateSharedTexture(SharedTexture);
		return StaticCastSharedPtr<ITouchImportTexture>(Texture); //Future;
	}

	TEResult FTouchTextureImporterVulkan::GetTextureTransfer(const FTouchImportParameters& ImportParams)
	{
		VkImageLayout AcquireOldLayout;
		VkImageLayout AcquireNewLayout;
		ImportParams.GetTextureTransferResult = TEInstanceGetVulkanTextureTransfer(ImportParams.Instance, ImportParams.Texture, &AcquireOldLayout,
			&AcquireNewLayout, ImportParams.GetTextureTransferSemaphore.take(), &ImportParams.GetTextureTransferWaitValue);
		ImportParams.VulkanAcquireOldLayout = AcquireOldLayout; //todo: is this allowed?
		ImportParams.VulkanAcquireNewLayout = AcquireNewLayout;
		return ImportParams.GetTextureTransferResult;
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
