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
#include "TouchEngine/TED3D.h"
#include "Util/TouchHelpers.h"


namespace UE::TouchEngine::D3DX12
{
	FTouchTextureExporterD3D12::FTouchTextureExporterD3D12(TSharedRef<FTouchFenceCache> FenceCache)
		: FenceCache(MoveTemp(FenceCache))
	{
		CommandQueueFence = FenceCache->GetOrCreateOwnedFence_AnyThread(true);
	}

	FTouchTextureExporterD3D12::~FTouchTextureExporterD3D12()
	{
		CommandQueueFence.Reset(); // this will be destroyed or reused by the FenceCache.
	}

	TFuture<FTouchSuspendResult> FTouchTextureExporterD3D12::SuspendAsyncTasks()
	{
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
		
		TFuture<FTouchSuspendResult> FinishRenderingTasks = FTouchTextureExporter::SuspendAsyncTasks();
		// Once all the rendering tasks have finished using the copying textures, they can be released.
		FinishRenderingTasks.Next([this, Promise = MoveTemp(Promise)](auto) mutable
		{
			ReleaseTextures().Next([Promise = MoveTemp(Promise)](auto) mutable
			{
				UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[FTouchTextureExporterD3D12::SuspendAsyncTasks] is done with ReleaseTextures")) //todo: remove once bug of textures not being release by TE is fixed
				Promise.SetValue({});
			});
		});

		return Future;
	}

	void FTouchTextureExporterD3D12::FinalizeExportsToTouchEngine_AnyThread(const FTouchEngineInputFrameData& FrameData)
	{
		TexturePoolMaintenance();
		
		if (TextureExports.IsEmpty())
		{
			return;
		}
		
		ENQUEUE_RENDER_COMMAND(AccessTexture)([WeakThis = SharedThis(this).ToWeakPtr(), TextureExports = MoveTemp(TextureExports), Fence = CommandQueueFence](FRHICommandListImmediate& RHICmdList) mutable
		{
			const TSharedPtr<FTouchTextureExporterD3D12> ThisPin = WeakThis.Pin();
			if (!ThisPin)
			{
				return;
			}
			
			// 1. Schedule a wait for the textures to be released by TouchEngine. RHIWaitManualFence close the current command list, so we want to enqueue all the waits at the same time
			struct FFenceData
			{
				const Microsoft::WRL::ComPtr<ID3D12Fence> NativeFence;
				uint64 WaitValue;
			};
			TArray<FFenceData> Transfers;
			for (const FExportCopyParams& CopyParams : TextureExports)
			{
				if (CopyParams.ExportParams.TETextureTransfer.Result == TEResultSuccess)
				{
					if (const Microsoft::WRL::ComPtr<ID3D12Fence> NativeFence = ThisPin->FenceCache->GetOrCreateSharedFence(CopyParams.ExportParams.TETextureTransfer.Semaphore))
					{
						Transfers.Add({NativeFence, CopyParams.ExportParams.TETextureTransfer.WaitValue});
					}
				}
			}
			RHICmdList.EnqueueLambda([Transfers, FenceCache = ThisPin->FenceCache](FRHICommandListImmediate& RHICommandList)
			{
				ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
				for (const FFenceData& FenceData : Transfers)
				{
					RHI->RHIWaitManualFence(RHICommandList, FenceData.NativeFence.Get(), FenceData.WaitValue);
				}
			});
			
			// 2. Enqueue the Copy
			for (const FExportCopyParams& CopyParams : TextureExports)
			{
				const FTextureRHIRef SourceRHI = CopyParams.DestinationTETexture->GetStableRHIOfTextureToCopy();
				if (ensureMsgf(SourceRHI, TEXT("No Stable RHI from `%s` to copy onto `%s`. %s"),
				               *GetNameSafe(CopyParams.ExportParams.Texture), *CopyParams.DestinationTETexture->DebugName, *CopyParams.ExportParams.GetDebugDescription()))
				{
					RHICmdList.CopyTexture(SourceRHI, CopyParams.DestinationTETexture->GetSharedTextureRHI(), FRHICopyTextureInfo());
					UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[RHI] Texture has a valid stable RHI: %s"), *CopyParams.ExportParams.GetDebugDescription())
				}
				else
				{
					UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[RHI] Texture has no stable RHI: %s"), *CopyParams.ExportParams.GetDebugDescription())
				}
			}
   
			// 3. Signal to TE
			RHICmdList.EnqueueLambda([TextureExports = MoveTemp(TextureExports), Fence](FRHICommandListImmediate& RHICommandList)
			{
				ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
				RHI->RHISignalManualFence(RHICommandList, Fence->NativeFence.Get(), ++Fence->LastValue);
			});
		});

		// Reset the texture exports now that they have been copied to the Render Thread
		TextureExports.Reset();
	}

	TEResult FTouchTextureExporterD3D12::AddTETextureTransfer(FTouchExportParameters& Params, const TSharedPtr<FExportedTextureD3D12>& Texture)
	{
		CommandQueueFence->DebugName = FString::Printf(TEXT("Fence_%lld"), Params.FrameData.FrameID);
		const uint64 WaitValue = CommandQueueFence->LastValue + 1; //  we need to wait until that fence value is reached
		UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[ExportTexture_AnyThread[%s]] Fence `%s` last value `%llu` (GetCompletedValue(): `%lld`) for frame `%lld`"),
			*GetCurrentThreadStr(), *CommandQueueFence->DebugName, CommandQueueFence->LastValue, CommandQueueFence->NativeFence->GetCompletedValue(), Params.FrameData.FrameID)
		return TEInstanceAddTextureTransfer(Params.Instance, Texture->GetTouchRepresentation(), CommandQueueFence->TouchFence, WaitValue);
	}

	void FTouchTextureExporterD3D12::FinaliseExportAndEnqueueCopy_AnyThread(FTouchExportParameters& Params, TSharedPtr<FExportedTextureD3D12>& Texture)
	{
		// Due to some synchronisation issues encountered with DirectX, we end up directly calling the DX12 RHI at the end of the frame, so we enqueue the needed details for now.
		TextureExports.Add(FExportCopyParams{MoveTemp(Params), Texture});
	}
}
