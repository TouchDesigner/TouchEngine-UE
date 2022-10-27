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

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/MinimalWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "TouchImportTextureVulkan.h"
#include "TouchEngine/TEVulkan.h"

namespace UE::TouchEngine::Vulkan
{
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
		TEInstanceSetVulkanAcquireImageLayout(Instance, TEScopeOutput, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	}

	TFuture<TSharedPtr<ITouchImportTexture>> FTouchTextureImporterVulkan::CreatePlatformTexture(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture)
	{
		const TSharedPtr<FTouchImportTextureVulkan> Texture = GetOrCreateSharedTexture(SharedTexture);
		const TSharedPtr<ITouchImportTexture> Result = Texture
			? StaticCastSharedPtr<ITouchImportTexture>(Texture)
			: nullptr;
		return MakeFulfilledPromise<TSharedPtr<ITouchImportTexture>>(Result).GetFuture();
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
		
			const TSharedPtr<FTouchImportTextureVulkan> CreationResult = FTouchImportTextureVulkan::CreateTexture(
				Shared
				);
			if (!CreationResult)
			{
				return  nullptr;
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
			FTouchTextureImporterVulkan* This = static_cast<FTouchTextureImporterVulkan*>(Info);
			FScopeLock Lock(&This->CachedTexturesMutex);
			This->CachedTextures.Remove(Handle);
		}
	}
}
