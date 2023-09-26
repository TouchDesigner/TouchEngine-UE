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
		// Firstly, we check if there is an export in progress and if yes, we start a task to wait for the fence to be signalled before we suspend all work
		if (CommandQueueFence && CommandQueueFence->NativeFence.Get())
		{
			if (LastSignalValue >= CommandQueueFence->NativeFence->GetCompletedValue())
			{
				UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[FTouchTextureExporterD3D12::SuspendAsyncTasks] About to start waiting for CommandQueueFence..."))
				// If we are waiting on an export
				UE::Tasks::Launch(UE_SOURCE_LOCATION,
				[Fence = CommandQueueFence, TargetSignalValue = LastSignalValue, TextureExports = TextureExports, TaskToken = StartAsyncTask()]()
				{
					double StartTime = FPlatformTime::Seconds(); 
					FPlatformProcess::ConditionalSleep([Fence, TargetSignalValue, StartTime, TaskToken]()
					{
						if (FPlatformTime::Seconds() < StartTime + 5.0) // 5s should be more than enough time, we would be expecting this next tick
						{
							if (Fence && Fence->NativeFence.Get())
							{
								if (Fence->NativeFence->GetCompletedValue() >= TargetSignalValue)
								{
									UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[FTouchTextureExporterD3D12::SuspendAsyncTasks] ... Successfully waited for the CommandQueueFence '%s' to be signalled to %lld (TargetValue: %lld) after %f s"),
										*Fence->DebugName, Fence->NativeFence->GetCompletedValue(), TargetSignalValue, FPlatformTime::Seconds() - StartTime)
								}
								return Fence->NativeFence->GetCompletedValue() >= TargetSignalValue; //stop sleeping if the real fence value is more than the signalled one
							}
						}
						return true; // stop sleeping if any of those is invalid or if we timed out
					}, 0.1f);
				}, LowLevelTasks::ETaskPriority::BackgroundLow);
			}
		}

		
		// Then we are ready to suspend the tasks
		UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[FTouchTextureExporterD3D12::SuspendAsyncTasks] About to suspends the async tasks..."))
		TFuture<FTouchSuspendResult> FinishRenderingTasks = FTouchTextureExporter::SuspendAsyncTasks();

		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();

		// Once all the rendering tasks have finished using the copying textures, they can be released.
		FinishRenderingTasks.Next([this, Promise = MoveTemp(Promise)](auto) mutable
		{
			UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[FTouchTextureExporterD3D12::SuspendAsyncTasks] ... Done suspending the Async Tasks. About to release textures..."))
			TextureExports.Empty(); // We empty the TextureExports after the rendering tasks are done to be sure we don't hold any reference before trying to release them
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
		TextureExports.Reset(); // We only clear them at the start of a new cook because at this point, we are sure the textures have been exported
	}

	void FTouchTextureExporterD3D12::FinalizeExportsToTouchEngine_GameThread(const FTouchEngineInputFrameData& FrameData)
	{
		TexturePoolMaintenance();
		
		if (TextureExports.IsEmpty())
		{
			return;
		}
		
		LastSignalValue = CommandQueueFence->LastValue + 1; // We need to remember which value we are trying to signal to be able to wait on it before destruction
		
		ENQUEUE_RENDER_COMMAND(AccessTexture)([WeakThis = SharedThis(this).ToWeakPtr(), TextureExports = TextureExports, Fence = CommandQueueFence, SignalValue = LastSignalValue](FRHICommandListImmediate& RHICmdList) mutable
		{
			const TSharedPtr<FTouchTextureExporterD3D12> ThisPin = WeakThis.Pin();
			if (!ThisPin || ThisPin->IsSuspended())
			{
				// Even if we suspended, we still want to signal the fence that we might be waiting for by SuspendAsyncTasks
				RHICmdList.EnqueueLambda([Fence, SignalValue](FRHICommandListImmediate& RHICommandList)
				{
					ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
					if (ensure(RHI) && ensure(Fence && Fence->NativeFence.Get()))
					{
						RHI->RHISignalManualFence(RHICommandList, Fence->NativeFence.Get(), SignalValue); // this is an async function, will not be signalled when this lambda is exited
						Fence->LastValue = SignalValue; // Technically not yet the value of the fence as the above function is asynchronous
					}
				});
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
			RHICmdList.EnqueueLambda([Transfers](FRHICommandListImmediate& RHICommandList)
			{
				ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
				for (const FFenceData& FenceData : Transfers)
				{
					if (FenceData.NativeFence.Get() && ensure(RHI))
					{
						RHI->RHIWaitManualFence(RHICommandList, FenceData.NativeFence.Get(), FenceData.WaitValue);
					}
				}
			});
			
			// 2. Enqueue the Copy
			for (const FExportCopyParams& CopyParams : TextureExports)
			{
				const FTextureRHIRef SourceRHI = CopyParams.DestinationTETexture->GetStableRHIOfTextureToCopy();
				if (SourceRHI) //, TEXT("No Stable RHI from `%s` to copy onto `%s`. %s"), *GetNameSafe(CopyParams.ExportParams.Texture), *CopyParams.DestinationTETexture->DebugName, *CopyParams.ExportParams.GetDebugDescription()))
				{
					RHICmdList.CopyTexture(SourceRHI, CopyParams.DestinationTETexture->GetSharedTextureRHI(), FRHICopyTextureInfo());
					UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[RHI] Texture has a valid stable RHI: %s"), *CopyParams.ExportParams.GetDebugDescription())
				}
				else // This can now happen if the frame is cancelled
				{
					UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("[RHI] Texture has no stable RHI, it will not be copied: %s"), *CopyParams.ExportParams.GetDebugDescription())
				}
			}
   
			// 3. Signal to TE
			RHICmdList.EnqueueLambda([TextureExports = MoveTemp(TextureExports), Fence, Transfers, SignalValue](FRHICommandListImmediate& RHICommandList)
			{
				ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
				if (ensure(RHI) && ensure(Fence && Fence->NativeFence.Get()))
				{
					RHI->RHISignalManualFence(RHICommandList, Fence->NativeFence.Get(), SignalValue); // this is an async function, will not be signalled when this lambda is exited
					Fence->LastValue = SignalValue; // Technically not yet the value of the fence as the above function is asynchronous
				}
			});
		});
	}

	TEResult FTouchTextureExporterD3D12::AddTETextureTransfer(FTouchExportParameters& Params, const TSharedPtr<FExportedTextureD3D12>& Texture)
	{
		check(Texture);
		CommandQueueFence->DebugName = FString::Printf(TEXT("Fence_%lld"), Params.FrameData.FrameID);
		const uint64 WaitValue = CommandQueueFence->LastValue + 1; //  we need to wait until that fence value is reached
		UE_LOG(LogTouchEngineTECalls, Verbose, TEXT("TEInstanceAddTextureTransfer[%s] for texture '%s', fence '%s' with WaitValue '%lld' (last completed value: %lld)"),
			*GetCurrentThreadStr(), *Texture->DebugName, *CommandQueueFence->DebugName, WaitValue, CommandQueueFence->NativeFence->GetCompletedValue());
		return TEInstanceAddTextureTransfer(Params.Instance, Texture->GetTouchRepresentation(), CommandQueueFence->TouchFence, WaitValue);
	}

	void FTouchTextureExporterD3D12::FinaliseExportAndEnqueueCopy_AnyThread(FTouchExportParameters& Params, TSharedPtr<FExportedTextureD3D12>& Texture)
	{
		// Due to some synchronisation issues encountered with DirectX, we end up directly calling the DX12 RHI at the end of the frame, so we enqueue the needed details for now.
		TextureExports.Add(FExportCopyParams{MoveTemp(Params), Texture});
	}
}
