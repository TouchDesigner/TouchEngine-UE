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
#include "RHI.h"
#include "TextureResource.h"

#include "Logging.h"
#include "RHICommmandCopyUnrealToTouch.h"
#include "VulkanTouchUtils.h"
THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END

#include "Engine/Texture.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "vulkan_win32.h"
// Very hacky way of getting access to DXGI_SHARED_RESOURCE_READ and DXGI_SHARED_RESOURCE_WRITE without curios dependency in build.cs file
#include "ThirdParty/Windows/DirectX/include/dxgi1_2.h"
#include "Windows/HideWindowsPlatformTypes.h"
#include "Util/VulkanWindowsFunctions.h"
#endif

#include "Engine/TEDebug.h"
#include "Importing/VulkanImportUtils.h"
#include "TEVulkanInclude.h"
#include "Util/TextureShareVulkanPlatformWindows.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/TouchHelpers.h"
#include "Util/VulkanGetterUtils.h"


DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Export - No Vulkan Textures"), STAT_TENoVulkanTextures, STATGROUP_TouchEngine);

namespace UE::TouchEngine::Vulkan
{
	namespace Private
	{
		static TOptional<FExportedTextureVulkan::FOutputVulkanTextureData> CreateVulkanTextureToBeShared_RenderThread(const FIntPoint& Resolution, const VkFormat VulkanFormat, const TSharedRef<FVulkanSharedResourceSecurityAttributes>& SecurityAttributes, FRHITextureCreateDesc& InOutTextureCreateDesc)
		{
#if !PLATFORM_WINDOWS
			static_assert(false, "You must implement a different sharing method. On windows we used NT handles.");
#endif

			const FVulkanPointers Vulkan;
			FExportedTextureVulkan::FOutputVulkanTextureData Result;

			VkExternalMemoryImageCreateInfo ExternalMemoryImageCreateInfo = {VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
			constexpr bool bUseWindowNT = true;
			Result.MemoryHandleFlags = bUseWindowNT ? VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT : VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
			ExternalMemoryImageCreateInfo.handleTypes = Result.MemoryHandleFlags;
			VkImageCreateInfo TexCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, &ExternalMemoryImageCreateInfo};
			{
				TexCreateInfo.imageType = VK_IMAGE_TYPE_2D;
				TexCreateInfo.format = VulkanFormat;
				TexCreateInfo.extent.width = Resolution.X;
				TexCreateInfo.extent.height = Resolution.Y;
				TexCreateInfo.extent.depth = 1;
				TexCreateInfo.mipLevels = 1;
				TexCreateInfo.arrayLayers = 1;
				TexCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				TexCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				TexCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				TexCreateInfo.flags = 0;
				TexCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				TexCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

				VkImage NakedImageHandle;
				VERIFYVULKANRESULT(VulkanRHI::vkCreateImage(Vulkan.VulkanDeviceHandle, &TexCreateInfo, NULL, &NakedImageHandle));
				INC_DWORD_STAT(STAT_TENoVulkanTextures)
				Result.ImageOwnership = MakeShareable<VkImage>(new VkImage(NakedImageHandle), [Device = Vulkan.VulkanDeviceHandle](const VkImage* Memory)
				{
					VulkanRHI::vkDestroyImage(Device, *Memory, nullptr);
					DEC_DWORD_STAT(STAT_TENoVulkanTextures)
					delete Memory;
				});
			}

			VkMemoryRequirements ImageMemoryRequirements = {};
			VulkanRHI::vkGetImageMemoryRequirements(Vulkan.VulkanDeviceHandle, *Result.ImageOwnership.Get(), &ImageMemoryRequirements);

			VkPhysicalDeviceMemoryBudgetPropertiesEXT MemoryBudget{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};
			VkPhysicalDeviceMemoryProperties2 MemoryProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2, &MemoryBudget};
			VulkanRHI::vkGetPhysicalDeviceMemoryProperties2(Vulkan.VulkanPhysicalDeviceHandle, &MemoryProperties);
			const uint32 MemoryTypeIndex = GetMemoryTypeIndex(MemoryProperties, ImageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (MemoryTypeIndex >= MemoryProperties.memoryProperties.memoryTypeCount)
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Memory doesn't support sharing"));
				return {};
			}

			VkExportMemoryAllocateInfo ExportMemInfo = {VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO};
			ExportMemInfo.handleTypes = ExternalMemoryImageCreateInfo.handleTypes;
			VkExportMemoryWin32HandleInfoKHR ExportMemWin32Info = {VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR};
			if (ExternalMemoryImageCreateInfo.handleTypes == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT)
			{
				ExportMemWin32Info.dwAccess = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
				ExportMemWin32Info.pAttributes = SecurityAttributes->Get();
				ExportMemInfo.pNext = &ExportMemWin32Info;
			}

			VkMemoryAllocateInfo MemInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, &ExportMemInfo};
			{
				MemInfo.allocationSize = ImageMemoryRequirements.size;
				MemInfo.memoryTypeIndex = MemoryTypeIndex;

				// Is this needed?
				/*VkPhysicalDeviceExternalImageFormatInfo ExternalImageFormatInfo = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO };
				ExternalImageFormatInfo.handleType = Result.MemoryHandleFlags;
		        VkMemoryDedicatedAllocateInfo memoryDedicatedAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
				VkPhysicalDeviceImageFormatInfo2 ImageFormatInfo = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2, &ExternalImageFormatInfo };
				ImageFormatInfo.format  = VulkanFormat;
				ImageFormatInfo.type    = TexCreateInfo.imageType;
				ImageFormatInfo.tiling  = TexCreateInfo.tiling;
				ImageFormatInfo.usage   = TexCreateInfo.usage;
				ImageFormatInfo.flags   = TexCreateInfo.flags;
				VkMemoryDedicatedAllocateInfo DedicatedAllocationMemoryAllocationInfo = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
				VkExternalImageFormatProperties ExternalImageFormatProperties = { VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES };
				VkImageFormatProperties2 ImageFormatProperties = { VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, &ExternalImageFormatProperties };
				VERIFYVULKANRESULT(vkGetPhysicalDeviceImageFormatProperties2(Vulkan.VulkanPhysicalDeviceHandle, &ImageFormatInfo, &ImageFormatProperties));
				bool bUseDedicatedMemory = ExternalImageFormatProperties.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT;
		        if (bUseDedicatedMemory)
	        	{
		            memoryDedicatedAllocateInfo.image = *Result.ImageOwnership.Get();
		            memoryDedicatedAllocateInfo.pNext = MemInfo.pNext;
		            MemInfo.pNext = &memoryDedicatedAllocateInfo;
		        }*/

				VkDeviceMemory NakedMemoryHandle;
				VERIFYVULKANRESULT(VulkanRHI::vkAllocateMemory(Vulkan.VulkanDeviceHandle, &MemInfo, nullptr, &NakedMemoryHandle));
				Result.TextureMemoryOwnership = MakeShareable<VkDeviceMemory>(new VkDeviceMemory(NakedMemoryHandle), [Device = Vulkan.VulkanDeviceHandle](const VkDeviceMemory* Memory)
				{
					VulkanRHI::vkFreeMemory(Device, *Memory, nullptr);
					UE_LOG(LogTouchEngineVulkanRHI, VeryVerbose, TEXT("[Result.TextureMemoryOwnership DELETER]"))
					delete Memory;
				});
			}

			VERIFYVULKANRESULT(VulkanRHI::vkBindImageMemory(Vulkan.VulkanDeviceHandle, *Result.ImageOwnership.Get(), *Result.TextureMemoryOwnership.Get(), 0));

			InOutTextureCreateDesc.SetNumMips(TexCreateInfo.mipLevels)
			    .SetNumSamples(TexCreateInfo.samples);

			return Result;
		}

		static bool ShareVulkanTexture_RenderThread(FExportedTextureVulkan::FOutputVulkanTextureData& InOutVulkanTextureData, const TSharedRef<FVulkanSharedResourceSecurityAttributes>& SecurityAttributes)
		{
#if !PLATFORM_WINDOWS
			static_assert(false, "You must implement a different sharing method. On windows we used NT handles.");
#endif

			const FVulkanPointers Vulkan;
			
			VkMemoryGetWin32HandleInfoKHR memoryGetWin32HandleInfo = { VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR };
			memoryGetWin32HandleInfo.memory = *InOutVulkanTextureData.TextureMemoryOwnership.Get();
			memoryGetWin32HandleInfo.handleType = static_cast<VkExternalMemoryHandleTypeFlagBits>(InOutVulkanTextureData.MemoryHandleFlags);
			VERIFYVULKANRESULT(Vulkan::vkGetMemoryWin32HandleKHR(Vulkan.VulkanDeviceHandle, &memoryGetWin32HandleInfo, &InOutVulkanTextureData.VulkanSharedHandle));

			return true;
		}
	}
	
	TSharedPtr<FExportedTextureVulkan> FExportedTextureVulkan::Create(UTexture* InTexture, const TSharedRef<FVulkanSharedResourceSecurityAttributes>& SecurityAttributes)
	{
		if (!IsValid(InTexture))
		{
			return nullptr;
		}

		TSharedRef<FExportedTextureVulkan> ExportedTexture = MakeShared<FExportedTextureVulkan>(SecurityAttributes);
		
		FTextureResource* SourceTextureResource = InTexture->GetResource();
		
		ENQUEUE_RENDER_COMMAND(CreateVulkanTexture)([SourceTextureResource, WeakThis = ExportedTexture->AsWeak()](FRHICommandListImmediate& RHICmdList) mutable
		{
			TSharedPtr<FExportedTextureVulkan> This = StaticCastSharedPtr<FExportedTextureVulkan>(WeakThis.Pin());
			if (!This)
			{
				return;
			}
			
			const FTextureRHIRef& SourceTextureRHI = SourceTextureResource->GetTextureRHI();
			if (!SourceTextureRHI.IsValid())
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to retrieve RHI of texture '%s'"), *This->DebugName);
				return;
			}
			
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.a [GT] Cook Frame - Vulkan::CreateTexture"), STAT_TE_I_B_1_a_Vulkan, STATGROUP_TouchEngine);

			const FRHITextureDesc& ExistingTextureDesc = SourceTextureResource->GetTextureRHI()->GetDesc();
			
			const EPixelFormat PixelFormat = ExistingTextureDesc.Format;
			const FIntPoint Resolution = ExistingTextureDesc.Extent;
			const bool bIsSRGB = EnumHasAnyFlags(ExistingTextureDesc.Flags, ETextureCreateFlags::SRGB);
					
			VkComponentMapping Mapping;
			const VkFormat VulkanFormat = UnrealToVulkanTextureFormat(PixelFormat, bIsSRGB, Mapping);
			if (VulkanFormat == VK_FORMAT_UNDEFINED)
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to import because PixelFormat %s could not be mapped"), GPixelFormats[PixelFormat].Name);
				return;
			}

			TOptional<FExportedTextureVulkan::FOutputVulkanTextureData> VulkanTextureData;
			FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2D(TEXT("CreateVulkanTextureToBeShared_RenderThread"), Resolution.X, Resolution.Y, PixelFormat);
			if (bIsSRGB)
			{
				TextureCreateDesc.AddFlags(ETextureCreateFlags::SRGB);
			}
			
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.b [GT] Cook Frame - Vulkan::CreateTexture - CreateVulkanTexture"), STAT_TE_I_B_1_b_Vulkan, STATGROUP_TouchEngine);
				VulkanTextureData = Private::CreateVulkanTextureToBeShared_RenderThread(Resolution, VulkanFormat, This->SecurityAttributes, TextureCreateDesc);
			}
			if (!VulkanTextureData)
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to import because the shared Vulkan texture could not be created"));
				return;
			}

			IVulkanDynamicRHI* VulkanRHI = GetIVulkanDynamicRHI();
			FTextureRHIRef VulkanTextureRHI = VulkanRHI->RHICreateTexture2DFromResource(
				TextureCreateDesc.Format,
				TextureCreateDesc.Extent.X,
				TextureCreateDesc.Extent.Y,
				TextureCreateDesc.NumMips,
				TextureCreateDesc.NumSamples,
				*VulkanTextureData->ImageOwnership.Get(),
				TextureCreateDesc.Flags
			);

			This->SetVulkanTexture_RenderThread(VulkanTextureRHI, VulkanTextureData.GetValue());
		});
		
		return ExportedTexture;
	}

	const TSharedPtr<VkCommandBuffer>& FExportedTextureVulkan::EnsureCommandBufferInitialized_RenderThread(FRHICommandListBase& RHICmdList)
	{
		// if (!CommandBuffer) //todo: look at removing this buffer variable
		{
			CommandBuffer = CreateCommandBuffer(RHICmdList);
		}
		return CommandBuffer;
	}

	void FExportedTextureVulkan::SetVulkanTexture_RenderThread(const FTextureRHIRef& VulkanRHI, const FOutputVulkanTextureData& InOutputVulkanTextureData)
	{
		SetTextureRHI_RenderThread(VulkanRHI);
		VulkanTextureData = InOutputVulkanTextureData;

		if (!SignalSemaphoreData.IsSet())
		{
			SignalSemaphoreData = CreateAndExportSemaphore(SecurityAttributes->Get(), CurrentSemaphoreValue, FString::Printf(TEXT("%s"), *DebugName));
			LogCompletedValue(FString("After `CreateAndExportSemaphore`:"));
		}
	}

	bool FExportedTextureVulkan::ShareTexture_RenderThread()
	{
		if (!CanBeShared_RenderThread())
		{
			if (IsShared_RenderThread())
			{
				return true;
			}
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Vulkan Texture '%s' cannot be shared"), *DebugName)
			return false;
		}

		const bool bIsSRGB = GetIsSRGB();
		VkComponentMapping Mapping;
		const VkFormat VulkanFormat = UnrealToVulkanTextureFormat(GetPixelFormat_RenderThread(), bIsSRGB, Mapping);
		if (!ensure(VulkanFormat != VK_FORMAT_UNDEFINED)) // This should have been checked previously when creating the texture
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to import because PixelFormat %s could not be mapped"), GPixelFormats[GetPixelFormat_RenderThread()].Name);
			return false;
		}
		
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.b [GT] Cook Frame - Vulkan::CreateTexture - ShareVulkanTexture_RenderThread"), STAT_TE_I_B_1_b_Vulkan, STATGROUP_TouchEngine);
			Private::ShareVulkanTexture_RenderThread(VulkanTextureData.GetValue(), SecurityAttributes);
			if (!VulkanTextureData)
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to export because the shared Vulkan texture could not be shared"));
				return false;
			}
		}

		FIntPoint Resolution = GetResolution_RenderThread();
		TouchObject<TEVulkanTexture> TouchRepresentation = TouchObject<TEVulkanTexture>::make_take(TEVulkanTextureCreate(VulkanTextureData->VulkanSharedHandle, VulkanTextureData->MemoryHandleFlags, VulkanFormat, Resolution.X, Resolution.Y, TETextureOriginTopLeft, Mapping, nullptr, nullptr));
		if (!TouchRepresentation)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("TEVulkanTextureCreate failed"));
			return false;
		}

		if (ensure(VulkanTextureData->MemoryHandleFlags == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT)) // todo: What should happen in other cases?
		{
			CloseHandle(VulkanTextureData->VulkanSharedHandle); // we need to release the vulkan handle
		}
		
		SetTouchRepresentation_RenderThread(MoveTemp(TouchRepresentation), [this](const TouchObject<TETexture>& Texture)
			{
				TEVulkanTexture* VulkanTexture = static_cast<TEVulkanTexture*>(Texture.get());
				TEVulkanTextureSetCallback(VulkanTexture, TouchTextureCallback, this);
			});
			
		return true;
	}

	bool FExportedTextureVulkan::CanFitTexture(UTexture* TextureToFit) const
	{
		return FExportedTouchTexture::CanFitTexture(TextureToFit);
	}

	bool FExportedTextureVulkan::EnqueueTextureCopy(UTexture* SrcTexture)
	{
		ENQUEUE_RENDER_COMMAND(AccessTexture)([SourceTextureResource = SrcTexture->GetResource(), WeakThis = SharedThis(this).ToWeakPtr()](FRHICommandListImmediate& RHICmdList) mutable
		{
			const TSharedPtr<FExportedTextureVulkan> This = WeakThis.Pin();
			if (!This || !SourceTextureResource)
			{
				return;
			}
			RHICmdList.Transition(FRHITransitionInfo(SourceTextureResource->GetTextureRHI(), ERHIAccess::Unknown, ERHIAccess::CopySrc));
			CopyUnrealToTouchRHICommand(RHICmdList, SourceTextureResource, This.ToSharedRef());
		});
		return true;
	}

	void FExportedTextureVulkan::TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info)
	{
		FExportedTextureVulkan* This = static_cast<FExportedTextureVulkan*>(Info);
		UE_LOG(LogTouchEngineVulkanRHI, Warning, TEXT("[FExportedTextureVulkan::TouchTextureCallback[%s]] Received FExportedTextureVulkan Event `%s` for `%s`"), *GetCurrentThreadStr(), *TEObjectEventToString(Event), *This->DebugName)
		if (Event == TEObjectEventRelease)
		{
			This->VulkanTextureData->VulkanSharedHandle = nullptr; // So we can reshare
		}
		This->OnTouchTextureUseUpdate(Event);
	}

	void FExportedTextureVulkan::OnWaitVulkanSemaphoreUsageChanged(void* Semaphore, TEObjectEvent Event, void* Info)
	{
		//todo: this sometimes gets called after the semaphore has been destroyed
		// FExportedTextureVulkan* This = static_cast<FExportedTextureVulkan*>(Info);
		// UE_LOG(LogTouchEngineVulkanRHI, Warning, TEXT("[FExportedTextureVulkan::OnWaitVulkanSemaphoreUsageChanged[%s]] Event `%s` for `%s`"), *GetCurrentThreadStr(), *TEObjectEventToString(Event), *(This ? This->DebugName : TEXT("")))
		// I think if it stops being used it is ok to just keep the semaphore alive and reuse in the future ... not need to destroy it, right?
	}
}

// Do not pollute other cpp files in unity builds
#if PLATFORM_WINDOWS
#endif