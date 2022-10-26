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
#include "Rendering/Importing/TouchImportTexture_AcquireOnRenderThread.h"

#include "TouchEngine/TouchObject.h"
#include "TouchEngine/TESemaphore.h"

namespace UE::TouchEngine::Vulkan
{
	enum class EVulkanImportLoadResult
	{
		Success,
		Failure
	};
	
	class FTouchImportTextureVulkan : public FTouchImportTexture_AcquireOnRenderThread
	{
		using Super = FTouchImportTexture_AcquireOnRenderThread;
	public:

		using FHandle = void*;
		
		static TSharedPtr<FTouchImportTextureVulkan> CreateTexture(const TouchObject<TEVulkanTexture_>& Shared);

		FTouchImportTextureVulkan(FTexture2DRHIRef TextureRHI, TouchObject<TEVulkanTexture_> SharedTexture);
		
		//~ Begin ITouchPlatformTexture Interface
		virtual FTextureMetaData GetTextureMetaData() const override;
		//~ End ITouchPlatformTexture Interface

		TouchObject<TEVulkanTexture_> GetSharedTexture() const { return SharedTexture; }
		
	protected:

		//~ Begin FTouchPlatformTexture_AcquireOnRenderThread Interface
		virtual bool AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) override;
		virtual FTexture2DRHIRef ReadTextureDuringMutex() override { return TextureRHI; }
		virtual void ReleaseMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) override;
		//~ End FTouchPlatformTexture_AcquireOnRenderThread Interface

	private:
		
		FTexture2DRHIRef TextureRHI;
		TouchObject<TEVulkanTexture_> SharedTexture;
	};
}
