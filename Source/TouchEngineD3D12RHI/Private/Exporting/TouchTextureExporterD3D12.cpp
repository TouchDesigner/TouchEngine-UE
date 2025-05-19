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

#include "Exporting/TouchTextureExporterD3D12.h"

#include "ExportedTextureD3D12.h"
#include "Logging.h"
#include "Rendering/Exporting/TouchExportParams.h"

#include "ID3D12DynamicRHI.h"
#include "RenderingThread.h"
#include "Tasks/Task.h"
#include "TouchEngine/TED3D.h"
#include "TouchEngine/Public/Logging.h"
#include "Util/TouchHelpers.h"


namespace UE::TouchEngine::D3DX12
{
	FTouchTextureExporterD3D12::FTouchTextureExporterD3D12(TSharedRef<FTouchFenceCache> FenceCache)
		: FenceCache(MoveTemp(FenceCache)), SharedResourceSecurityAttributes(MakeShared<FTextureShareD3D12SharedResourceSecurityAttributes>())
	{
	}

	FTouchTextureExporterD3D12::~FTouchTextureExporterD3D12()
	{
	}

	TFuture<FTouchSuspendResult> FTouchTextureExporterD3D12::SuspendAsyncTasks()
	{
		UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[FTouchTextureExporterD3D12::SuspendAsyncTasks] About to suspends the async tasks..."))
		TFuture<FTouchSuspendResult> FinishRenderingTasks = FTouchTextureExporter::SuspendAsyncTasks();

		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();

		// Once all the rendering tasks have finished using the copying textures, they can be released.
		FinishRenderingTasks.Next([this, Promise = MoveTemp(Promise)](auto) mutable
		{
			UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[FTouchTextureExporterD3D12::SuspendAsyncTasks] ... Done suspending the Async Tasks. About to release textures..."))
			ReleaseTextures().Next([this, Promise = MoveTemp(Promise)](auto) mutable
			{
				// After all the textures are released, we are ready to wait for the fences to be released, as they will still create callbacks.
				UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[FTouchTextureExporterD3D12::SuspendAsyncTasks] ... Done releasing the textures. About to release the fences..."))
				FenceCache->ReleaseFences().Next([this, Promise = MoveTemp(Promise)](auto) mutable
				{
					// When the fences are released, we are ready to be destroyed
					UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[FTouchTextureExporterD3D12::SuspendAsyncTasks] ... Done releasing the fences. Ready to be destroyed."))
					Promise.SetValue({});
				});
			});
		});

		return Future;
	}

	void FTouchTextureExporterD3D12::InitializeExportsToTouchEngine_GameThread(const FTouchEngineInputFrameData& FrameData)
	{
		TexturePoolMaintenance();
	}

	TEResult FTouchTextureExporterD3D12::AddTETextureTransfer_RenderThread(const FTouchExportParameters& Params, const TSharedRef<FExportedTouchTexture>& Texture)
	{
		TSharedRef<FExportedTextureD3D12> D3DTexture = StaticCastSharedRef<FExportedTextureD3D12>(Texture);
		
		D3DTexture->CopyCompletedFence->DebugName = FString::Printf(TEXT("Fence_%lld"), Params.FrameData.FrameID);
		const uint64 WaitValue = D3DTexture->CopyCompletedFence->LastValue; //  we need to wait until that fence value is reached
		
		UE_LOG(LogTouchEngineTECalls, Verbose, TEXT("  TEInstanceAddTextureTransfer(TEInstance: '%p', texture: [TE: '%p', UE: '%s'], semaphore: '%p' ('%s'), value: '%lld') [Thread: '%s', Parameter: '%s', CookingFrame '%lld', CurrentSemaphoreValue: '%lld]"),
			Params.Instance.get(),
			Texture->GetTouchRepresentation_RenderThread().get(),
			*Texture->DebugName,
			D3DTexture->CopyCompletedFence->TouchFence.get(),
			*D3DTexture->CopyCompletedFence->DebugName,
			WaitValue,
			*GetCurrentThreadStr(),
			*Params.ParameterName.ToString(),
			Params.FrameData.FrameID,
			D3DTexture->CopyCompletedFence->NativeFence->GetCompletedValue()
		);
		return TEInstanceAddTextureTransfer(Params.Instance, Texture->GetTouchRepresentation_RenderThread(), D3DTexture->CopyCompletedFence->TouchFence, WaitValue);
	}
}
