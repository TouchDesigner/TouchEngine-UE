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
#include "D3D12RHI/Private/D3D12RHIPrivate.h"
#include "TouchEngine/TED3D.h"
#include "Util/TouchHelpers.h"
#include "Util/TouchEngineStatsGroup.h"

// macro to deal with COM calls inside a function that returns on failure
#define CHECK_HR_DEFAULT(COM_call)\
	{\
		HRESULT Res = COM_call;\
		if (FAILED(Res))\
		{\
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("`" #COM_call "` failed: 0x%X - %s"), Res, *GetComErrorDescription(Res)); \
			return;\
		}\
	}

namespace UE::TouchEngine::D3DX12
{
	FRHICOMMAND_MACRO(FRHICopyFromUnrealToVulkanAndSignalFence)
	{
		TRefCountPtr<ID3D12CommandQueue> D3DCommandQueue;
		TSharedPtr<FTouchFenceCache::FFenceData> Fence;
		TArray<FTouchTextureExporterD3D12::FExportCopyParams> TextureExports;
		TSharedRef<FTouchFenceCache> FenceCache;

		FRHICopyFromUnrealToVulkanAndSignalFence(const TRefCountPtr<ID3D12CommandQueue>& InD3DCommandQueue,const TSharedPtr<FTouchFenceCache::FFenceData>& InFence,
			TArray<FTouchTextureExporterD3D12::FExportCopyParams>&& InTextureExports, const TSharedRef<FTouchFenceCache>& InFenceCache)
			: D3DCommandQueue(InD3DCommandQueue)
			, Fence(InFence)
			, TextureExports(MoveTemp(InTextureExports))
			, FenceCache (InFenceCache)
		{
		}

		void Execute(FRHICommandListBase& CmdList)
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    I.B.4 [RHI] Cook Frame - RHI Export Copy"), STAT_TE_I_B_4_D3D, STATGROUP_TouchEngine);
			
			// The goal is just to copy the texture so early implementations were just calling CmdList.GetContext().RHICopyTexture(SourceTexture, DestTexture->GetSharedTextureRHI(), CopyInfo);
			// The problem is that this ends up calling ID3D12GraphicsCommandList::CopyResource, which is an asynchronous call, and we might end up signalling before the copy is actually done, which became obvious in Synchronised mode
			// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-copyresource
			// To make this work, we looked for a way to call ID3D12CommandQueue::ExecuteCommandLists without interfering with the UE implementation.
			// This made us directly call some of the DirectX12 functions and create our own queue
			uint64 FrameID = -1;
			
			const uint64 WaitValue = Fence->LastValue + 1;
			TArray<TRefCountPtr<ID3D12GraphicsCommandList>> CopyCommandLists;
			TArray<ID3D12CommandList*> RawCopyCommandLists;
			TArray<TRefCountPtr<ID3D12CommandAllocator>> CommandAllocatorLists;

			const ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
			check(RHI);
			const uint32 GPUIndex = CmdList.GetGPUMask().ToIndex();
			ID3D12Device* Device = RHI->RHIGetDevice(GPUIndex); // Just reusing the current device to be sure
			const D3D12_COMMAND_LIST_TYPE QueueType = D3DCommandQueue->GetDesc().Type; // D3D12_COMMAND_LIST_TYPE_COPY;
			
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.a [RHI] Cook Frame - RHI - Create Command List"), STAT_TE_I_B_4_a_D3D, STATGROUP_TouchEngine);
				
				for (const FTouchTextureExporterD3D12::FExportCopyParams& CopyParams : TextureExports)
				{
					FrameID = CopyParams.ExportParams.FrameData.FrameID;
					
					const FTextureRHIRef SourceRHI = CopyParams.DestinationTETexture->GetStableRHIOfTextureToCopy();
					if(ensureMsgf(SourceRHI, TEXT("No Stable RHI from `%s` to copy onto `%s`. %s"),
						*GetNameSafe(CopyParams.ExportParams.Texture), *CopyParams.DestinationTETexture->DebugName, *CopyParams.ExportParams.GetDebugDescription()))
					{
						// First, we add a wait to the command queue
						const TEResult& GetTextureTransferResult = CopyParams.ExportParams.GetTextureTransferResult;
						if (GetTextureTransferResult == TEResultSuccess)
						{
							if (const Microsoft::WRL::ComPtr<ID3D12Fence> NativeFence = FenceCache->GetOrCreateSharedFence(CopyParams.ExportParams.GetTextureTransferSemaphore))
							{
								D3DCommandQueue->Wait(NativeFence.Get(), CopyParams.ExportParams.GetTextureTransferWaitValue);
							}
						}
						else if (!ensure(GetTextureTransferResult == TEResultNoMatchingEntity)) // ensure TE does not have ownership
						{
							UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("Failed to transfer ownership of pooled texture `%s` back from Touch Engine. %s"), *CopyParams.DestinationTETexture->DebugName, *CopyParams.ExportParams.GetDebugDescription());
						}

						// Then, we enqueue a CopyResource command
						UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[RHI] Texture has a valid stable RHI: %s"), *CopyParams.ExportParams.GetDebugDescription())
						TRefCountPtr<ID3D12GraphicsCommandList> CopyCommandList;
						TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;
						// inspired by FD3D12CommandList::FD3D12CommandList
						CHECK_HR_DEFAULT(Device->CreateCommandAllocator(QueueType, IID_PPV_ARGS(CommandAllocator.GetInitReference())));
						CHECK_HR_DEFAULT(Device->CreateCommandList(CmdList.GetGPUMask().GetNative(), QueueType, CommandAllocator, nullptr, IID_PPV_ARGS(CopyCommandList.GetInitReference())));

						ID3D12Resource* Source = static_cast<ID3D12Resource*>(SourceRHI->GetNativeResource());
						ID3D12Resource* Destination = static_cast<ID3D12Resource*>(CopyParams.DestinationTETexture->GetSharedTextureRHI()->GetNativeResource());

						CopyCommandList->CopyResource(Destination, Source);;
						UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("   [FRHICopyFromUnrealToVulkanAndSignalFence[%s]] Resource Copy enqueued for param `%s` for frame `%lld`"),
							*GetCurrentThreadStr(), *CopyParams.ExportParams.ParameterName.ToString(), FrameID);
						CopyCommandList->Close();
						RawCopyCommandLists.Add(CopyCommandList.GetReference());
						CopyCommandLists.Emplace(MoveTemp(CopyCommandList));
						CommandAllocatorLists.Emplace(MoveTemp(CommandAllocator));
					}
					else
					{
						UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[RHI] Texture has no stable RHI: %s"), *CopyParams.ExportParams.GetDebugDescription())
					}
				}
			}

			const HANDLE mFenceEventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
			Fence->NativeFence->SetEventOnCompletion(WaitValue, mFenceEventHandle);
			
			UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("   [FRHICopyFromUnrealToVulkanAndSignalFence[%s]] About to execute CommandList, fence `%s` current value is `%llu` (GetCompletedValue(): `%lld`) for frame `%lld`"),
				*GetCurrentThreadStr(), *Fence->DebugName, Fence->LastValue, Fence->NativeFence->GetCompletedValue(), FrameID)
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.b [RHI] Cook Frame - RHI - Execute Command List"), STAT_TE_I_B_4_b_D3D, STATGROUP_TouchEngine);
				D3DCommandQueue->ExecuteCommandLists(RawCopyCommandLists.Num(), RawCopyCommandLists.GetData());
				D3DCommandQueue->Signal(Fence->NativeFence.Get(), WaitValue); // asynchronous call
			}

			{
				// we don't really need to wait, but we want to be sure that we are keeping the Fence and the textures alive as long as they are needed. Might be better to find another way to achieve this
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("I.b.4.c [RHI] Cook Frame - RHI - Wait for Copy"), STAT_TE_I_b_4_c_D3D, STATGROUP_TouchEngine);
				
				WaitForSingleObjectEx(mFenceEventHandle, INFINITE, false); // we need to wait to ensure the fence and the RHI are still alive for the copy
				CloseHandle(mFenceEventHandle);
			
				Fence->LastValue = WaitValue;
				UE_LOG(LogTouchEngineD3D12RHI, Display, TEXT("   [FRHICopyFromUnrealToVulkanAndSignalFence[%s]] CommandList executed and fence `%s` signalled with value `%llu` (GetCompletedValue(): `%lld`) for frame `%lld`"),
					*GetCurrentThreadStr(), *Fence->DebugName, Fence->LastValue, Fence->NativeFence->GetCompletedValue(), FrameID)

			}
		}
	};
	
	FTouchTextureExporterD3D12::FTouchTextureExporterD3D12(TSharedRef<FTouchFenceCache> FenceCache)
		: FenceCache(MoveTemp(FenceCache))
	{
		constexpr D3D12_COMMAND_LIST_TYPE QueueType = D3D12_COMMAND_LIST_TYPE_COPY;
		ID3D12Device* Device = (ID3D12Device*)GDynamicRHI->RHIGetNativeDevice();
		ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
		ID3D12CommandQueue* DefaultCommandQueue = RHI->RHIGetCommandQueue();
		
		// inspired by FD3D12Queue::FD3D12Queue
		D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
		CommandQueueDesc.Type = QueueType; // GetD3DCommandListType((ED3D12QueueType)QueueType);
		CommandQueueDesc.Priority = 0;
		CommandQueueDesc.NodeMask = DefaultCommandQueue->GetDesc().NodeMask;;
		CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		CHECK_HR_DEFAULT(Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(D3DCommandQueue.GetInitReference())));
		CHECK_HR_DEFAULT(D3DCommandQueue->SetName(*FString::Printf(TEXT("FTouchTextureExporterD3D12 (GPU %d)"), CommandQueueDesc.NodeMask)))

		CommandQueueFence = FenceCache->GetOrCreateOwnedFence_AnyThread(true);
	}

	FTouchTextureExporterD3D12::~FTouchTextureExporterD3D12()
	{
		D3DCommandQueue = nullptr; // this should release the command queue
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

	TSharedPtr<FExportedTextureD3D12> FTouchTextureExporterD3D12::CreateTexture(const FTouchExportParameters& Params, const FRHITexture2D* ParamTextureRHI) const
	{
		return FExportedTextureD3D12::Create(*ParamTextureRHI, SharedResourceSecurityAttributes);
	}

	void FTouchTextureExporterD3D12::FinalizeExportsToTouchEngine_AnyThread(const FTouchEngineInputFrameData& FrameData)
	{
		TexturePoolMaintenance();
		
		if (TextureExports.IsEmpty())
		{
			return;
		}

		// for (FExportCopyParams& CopyParams : TextureExports) //todo: to remove
		// {
		// 	const FTextureRHIRef SourceRHI = CopyParams.DestinationTETexture->GetStableRHIOfTextureToCopy();
		// 	if(!ensureMsgf(SourceRHI, TEXT("No Stable RHI from `%s` to copy onto `%s`. %s"),
		// 		*GetNameSafe(CopyParams.ExportParams.Texture), *CopyParams.DestinationTETexture->DebugName, *CopyParams.ExportParams.GetDebugDescription()))
		// 	{
		// 		UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[GT] Texture has no stable RHI: %s"),*CopyParams.ExportParams.GetDebugDescription())
		// 	}
		// 	else
		// 	{
		// 		UE_LOG(LogTouchEngineD3D12RHI, Display, TEXT("[GT] Texture has a valid stable RHI: %s"),*CopyParams.ExportParams.GetDebugDescription())
		// 	}
		// }
		
		ENQUEUE_RENDER_COMMAND(AccessTexture)([WeakThis = SharedThis(this).ToWeakPtr(), TextureExports = TextureExports, Fence = CommandQueueFence, FrameData](FRHICommandListImmediate& RHICmdList) mutable
		{
			// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("I.B.3 [RT] Cook Frame - AccessTextureForExport"), STAT_TE_I_B_3_D3D, STATGROUP_TouchEngine);
			// 1. Schedule a wait for the textures to be released by TouchEngine
			const TSharedPtr<FTouchTextureExporterD3D12> ThisPin = WeakThis.Pin();
			if (!ThisPin)
			{
				return;
			}

			// for (FExportCopyParams& CopyParams : TextureExports) //todo: to remove
			// {
			// 	const FTextureRHIRef SourceRHI = CopyParams.DestinationTETexture->GetStableRHIOfTextureToCopy();
			// 	if(!ensureMsgf(SourceRHI, TEXT("No Stable RHI from `%s` to copy onto `%s`. %s"),
			// 		*GetNameSafe(CopyParams.ExportParams.Texture), *CopyParams.DestinationTETexture->DebugName, *CopyParams.ExportParams.GetDebugDescription()))
			// 	{
			// 		UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[RT] Texture has no stable RHI: %s"),*CopyParams.ExportParams.GetDebugDescription())
			// 	}
			// 	else
			// 	{
			// 		UE_LOG(LogTouchEngineD3D12RHI, Display, TEXT("[RT] Texture has a valid stable RHI: %s"),*CopyParams.ExportParams.GetDebugDescription())
			// 	}
			// }
			
			// 3. Copy and Transfer to TE
			// We cannot call directly RHICmdList.CopyTexture(); as this is an asynchronous call and we still need to signal the fence but we don't know when it is finished
			ALLOC_COMMAND_CL(RHICmdList, FRHICopyFromUnrealToVulkanAndSignalFence)(ThisPin->D3DCommandQueue, MoveTemp(Fence), MoveTemp(TextureExports), ThisPin->FenceCache);
		});
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

	// void FTouchTextureExporterD3D12::ScheduleWaitFence(const TouchObject<TESemaphore>& AcquireSemaphore, uint64 AcquireValue) const
	// {
	// 	// Is nullptr if there is nothing to wait on
	// 	if (!AcquireSemaphore)
	// 	{
	// 		return;
	// 	}
	// 	
	// 	if (const Microsoft::WRL::ComPtr<ID3D12Fence> NativeFence = FenceCache->GetOrCreateSharedFence(AcquireSemaphore))
	// 	{
	// 		D3DCommandQueue->Wait(NativeFence.Get(), AcquireValue);
	// 	}
	// 	else
	// 	{
	// 		UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("Failed to get shared ID3D12Fence for input texture that is still in use. Texture will be overwriten without access synchronization!"));
	// 	}
	// }
}

#undef CHECK_HR_DEFAULT