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

#include "TouchTextureExporterVulkan.h"
#include "RHI.h"
#include "RHICommmandCopyUnrealToTouch.h"
#include "TextureResource.h"
#include "Rendering/Exporting/TouchExportParams.h"
#include "Util/SemaphoreVulkanUtils.h"
#include "Util/TextureShareVulkanPlatformWindows.h"
#include "Engine/Texture.h"
#include "Engine/Util/TouchVariableManager.h"


namespace UE::TouchEngine::Vulkan
{
	FTouchTextureExporterVulkan::FTouchTextureExporterVulkan(TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes)
		: SecurityAttributes(MoveTemp(SecurityAttributes))
	{}

	TFuture<FTouchSuspendResult> FTouchTextureExporterVulkan::SuspendAsyncTasks()
	{
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
		
		TFuture<FTouchSuspendResult> FinishRenderingTasks = FTouchTextureExporter::SuspendAsyncTasks();
		// Once all the rendering tasks have finished using the copying textures, they can be released.
		FinishRenderingTasks.Next([this, Promise = MoveTemp(Promise)](auto) mutable
		{
			ReleaseTextures().Next([Promise = MoveTemp(Promise)](auto) mutable
			{
				Promise.SetValue({});
			});
		});

		return Future;
	}

	void FTouchTextureExporterVulkan::InitializeExportsToTouchEngine_GameThread(const FTouchEngineInputFrameData& FrameData)
	{
		TexturePoolMaintenance(); //todo: is this the right place for this?
	}

	void FTouchTextureExporterVulkan::FinalizeExportsToTouchEngine_GameThread(const FTouchEngineInputFrameData& FrameData)
	{
		// TexturePoolMaintenance(); //todo: is this the right place for this?
	}

	TEResult FTouchTextureExporterVulkan::AddTETextureTransfer(const FTouchExportParameters& Params, const TSharedPtr<FExportedTouchTexture>& Texture)
	{
		TSharedPtr<FExportedTextureVulkan> VulkanTexture = StaticCastSharedPtr<FExportedTextureVulkan>(Texture);
		if (!VulkanTexture->SignalSemaphoreData.IsSet()) // todo: review, the semaphore should have now been created before this
		{
			VulkanTexture->SignalSemaphoreData = CreateAndExportSemaphore(SecurityAttributes->Get(), VulkanTexture->CurrentSemaphoreValue,
				FString::Printf(TEXT("%s [%lld]"), *Params.ParameterName.ToString(), Params.FrameData.FrameID));
			VulkanTexture->LogCompletedValue(FString("After `CreateAndExportSemaphore`:"));
		}
		
		return TEInstanceAddTextureTransfer(Params.Instance, Texture->GetTouchRepresentation_RenderThread(), VulkanTexture->SignalSemaphoreData->TouchSemaphore, VulkanTexture->CurrentSemaphoreValue + 1);
	}

	void FTouchTextureExporterVulkan::FinaliseExportAndEnqueueCopy_AnyThread(const FTouchExportParameters& Params, TSharedPtr<FExportedTouchTexture>& Texture)
	{
		// For Vulkan, we enqueue the signalling on the render thread.
		ENQUEUE_RENDER_COMMAND(SignalCopy)([WeakThis = SharedThis(this).ToWeakPtr(), Params = Params,
			ExportedTexture = Texture.ToSharedRef()](FRHICommandListImmediate& RHICmdList) mutable
		{
			const TSharedPtr<FTouchTextureExporterVulkan> ThisPin = WeakThis.Pin();
			if (!ThisPin || ThisPin->IsSuspended())
			{
				return;
			}
			TSharedRef<FExportedTextureVulkan> ExportedTextureVulkan = StaticCastSharedRef<FExportedTextureVulkan>(ExportedTexture);
			SignalCopyFromUnrealToTouchRHICommand(RHICmdList, Params.Instance, ExportedTextureVulkan);
		});
	}
}
