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

#include "RhiIncludeHelper.h"
#include "ID3D12DynamicRHI.h"
#include "D3D12RHI/Private/D3D12RHIPrivate.h"
#include "Engine/TEDebug.h"
#include "TouchEngine/TED3D.h"
#include "Util/TouchHelpers.h"
#include "D3D12Util.h"
#include "Engine/Util/TouchVariableManager.h"
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
	/** Debug function to get the top left pixel color of a Texture. This calls ID3D12DynamicRHI::RHILockTexture2D which ends up flushing the graphic commands first*/
	bool GetRHITopLeftPixelColor(FRHITexture2D* RHI, FColor& Color)
	{
		bool bResult = false;
		if (RHI)
		{
			uint32 stride;
			ID3D12DynamicRHI* DynRHI = static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
			if (void* RawData = DynRHI->RHILockTexture2D(RHI,0,RLM_ReadOnly,stride,false))
			{
				if (RHI->GetDesc().Format == PF_B8G8R8A8)
				{
					const uint8* Data = static_cast<uint8*>(RawData);
					Color = FColor(Data[2], Data[1], Data[0], Data[3]);
					bResult = true;
				}
				else
				{
					UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT(" Unknown format! => %s"), GPixelFormats[RHI->GetDesc().Format].Name)
				}
			}
			DynRHI->RHIUnlockTexture2D(RHI,0,false);
		}
		return bResult;
	}
	
	FRHICOMMAND_MACRO(FRHICopyFromUnrealToVulkanAndSignalFence)
	{
		TRefCountPtr<ID3D12CommandQueue> D3DCommandQueue;
		TSharedPtr<FTouchFenceCache::FFenceData> Fence;
		TArray<FTouchTextureExporterD3D12::FExportCopyParams> TextureExports;
		int64 FrameID;

		FRHICopyFromUnrealToVulkanAndSignalFence(const TRefCountPtr<ID3D12CommandQueue>& InD3DCommandQueue,const TSharedPtr<FTouchFenceCache::FFenceData>& InFence,
			TArray<FTouchTextureExporterD3D12::FExportCopyParams>&& InTextureExports, const int64 InFrameID)
			: D3DCommandQueue(InD3DCommandQueue)
			, Fence(InFence)
			, TextureExports(MoveTemp(InTextureExports))
			, FrameID (InFrameID)
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
			
			const uint64 WaitValue = Fence->LastValue + 1;
			TArray<TRefCountPtr<ID3D12GraphicsCommandList>> CopyCommandLists;
			TArray<ID3D12CommandList*> RawCopyCommandLists;
			TArray<TRefCountPtr<ID3D12CommandAllocator>> CommandAllocatorLists;
			// TRefCountPtr<ID3D12GraphicsCommandList>  CopyCommandList;
			// TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;

			ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();// static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
			check(RHI);
			const uint32 GPUIndex = CmdList.GetGPUMask().ToIndex();
			ID3D12Device* Device = RHI->RHIGetDevice(GPUIndex); // Just reusing the current device to be sure
			const D3D12_COMMAND_LIST_TYPE QueueType = D3DCommandQueue->GetDesc().Type; // D3D12_COMMAND_LIST_TYPE_COPY; // Context->GraphicsCommandList()->GetType();
			
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.a [RHI] Cook Frame - RHI - Create Command List"), STAT_TE_I_B_4_a_D3D, STATGROUP_TouchEngine);
				
				for (const FTouchTextureExporterD3D12::FExportCopyParams& CopyParams : TextureExports)
				{
					TRefCountPtr<ID3D12GraphicsCommandList> CopyCommandList;
					TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;
					// inspired by FD3D12CommandList::FD3D12CommandList
					CHECK_HR_DEFAULT(Device->CreateCommandAllocator(QueueType, IID_PPV_ARGS(CommandAllocator.GetInitReference())));
					CHECK_HR_DEFAULT(Device->CreateCommandList(CmdList.GetGPUMask().GetNative(), QueueType, CommandAllocator, nullptr, IID_PPV_ARGS(CopyCommandList.GetInitReference())));

					ID3D12Resource* Source = static_cast<ID3D12Resource*>(CopyParams.SourceRHITexture->GetNativeResource());
					ID3D12Resource* Destination = static_cast<ID3D12Resource*>(CopyParams.DestinationTETexture->GetSharedTextureRHI()->GetNativeResource());

					CopyCommandList->CopyResource(Destination, Source);;
					UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("   [FRHICopyFromUnrealToVulkanAndSignalFence[%s]] Resource Copy enqueued for param `%s` for frame `%lld`"),
						*GetCurrentThreadStr(), *CopyParams.ExportParams.ParameterName.ToString(), FrameID);
					CopyCommandList->Close();

					RawCopyCommandLists.Add(CopyCommandList.GetReference());
					CopyCommandLists.Emplace(MoveTemp(CopyCommandList));
					CommandAllocatorLists.Emplace(MoveTemp(CommandAllocator));
				}

				// ppCommandLists = { CopyCommandList };
				// ID3D12CommandList* ppCommandLists[] = { CopyCommandList };//
			}

			const HANDLE mFenceEventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
			Fence->NativeFence->SetEventOnCompletion(WaitValue, mFenceEventHandle);
			
			UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("   [FRHICopyFromUnrealToVulkanAndSignalFence[%s]] About to execute CommandList, fence `%s` current value is `%llu` (GetCompletedValue(): `%lld`) for frame `%lld`"),
				*GetCurrentThreadStr(), *Fence->DebugName, Fence->LastValue, Fence->NativeFence->GetCompletedValue(), FrameID)
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.4.b [RHI] Cook Frame - RHI - Execute Command List"), STAT_TE_I_B_4_b_D3D, STATGROUP_TouchEngine);
				// ID3D12CommandList* ppCommandLists[] = CopyCommandLists.GetData(); // { CopyCommandList };
				// D3DCommandQueue->ExecuteCommandLists(1, ppCommandLists);
				
				// ID3D12CommandList** ppCommandLists = RawCopyCommandLists.GetData();
				D3DCommandQueue->ExecuteCommandLists(RawCopyCommandLists.Num(), RawCopyCommandLists.GetData());
				D3DCommandQueue->Signal(Fence->NativeFence.Get(), WaitValue); // asynchronous call
			}
			
			{
				// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("I.b.4.c [RHI] Cook Frame - RHI - Signal Fence"), STAT_TE_I_b_4_c, STATGROUP_TouchEngine);
				// D3DCommandQueue->Wait(Fence->NativeFence.Get(), WaitValue); // this just add a wait at the end of the command queue, but does not actually waits for it

				// WaitForSingleObjectEx(mFenceEventHandle, INFINITE, false); // todo: do we actually need to wait?
				CloseHandle(mFenceEventHandle);
			
				Fence->LastValue = WaitValue;
				UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("   [FRHICopyFromUnrealToVulkanAndSignalFence[%s]] CommandList executed and fence `%s` signalled with value `%llu` (GetCompletedValue(): `%lld`) for frame `%lld`"),
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
				Promise.SetValue({});
			});
		});

		return Future;
	}

	TSharedPtr<FExportedTextureD3D12> FTouchTextureExporterD3D12::CreateTexture(const FTouchExportParameters& Params) const
	{
		const FRHITexture2D* SourceRHI = GetRHIFromTexture(Params.Texture);
		return FExportedTextureD3D12::Create(*SourceRHI, SharedResourceSecurityAttributes);
	}
	
	// bool FTouchTextureExporterD3D12::GetNextOrAllocPooledTETexture_Internal(const FTouchExportParameters& TouchExportParameters, bool& bIsNewTexture, bool& bIsUsedByOtherTexture, TouchObject<TETexture>& OutTexture)
	// {
	// 	return GetNextOrAllocPooledTETexture(TouchExportParameters, bIsNewTexture, bIsUsedByOtherTexture, OutTexture);
	// }

	TouchObject<TETexture> FTouchTextureExporterD3D12::ExportTexture_AnyThread(const FTouchExportParameters& ParamsConst, TEGraphicsContext* GraphicsContext)
	{
		// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Cook Frame - FTouchTextureExporterD3D12::ExportTexture_AnyThread"), STAT_FTouchResourceProviderExportTexture_AnyThread, STATGROUP_TouchEngine);

		// 1. We get a Dx12 Texture to copy onto
		bool bIsNewTexture;
		bool bTextureNeedsCopy;
		TSharedPtr<FExportedTextureD3D12> TextureData;
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    I.B.1 [GT] Cook Frame - GetOrCreateTexture"), STAT_TE_I_B_1_D3D, STATGROUP_TouchEngine);
			TextureData = GetOrCreateTexture(ParamsConst, bIsNewTexture, bTextureNeedsCopy);
		}
		if (!TextureData)
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[ExportTexture_AnyThread[%s]] ETouchExportErrorCode::InternalGraphicsDriverError for parameter `%s` for frame %lld"), *UE::TouchEngine::GetCurrentThreadStr(), *ParamsConst.ParameterName.ToString(), ParamsConst.FrameData.FrameID);
			return nullptr;
		}
		UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[ExportTexture_AnyThread[%s]] GetNextOrAllocPooledTexture returned %s `%s` for parameter `%s` for frame %lld"),
		       *UE::TouchEngine::GetCurrentThreadStr(), bIsNewTexture ? TEXT("a NEW texture") : TEXT("the EXISTING texture"), *TextureData->DebugName, *ParamsConst.ParameterName.ToString(), ParamsConst.FrameData.FrameID);

		const TouchObject<TETexture>& TouchTexture = TextureData->GetTouchRepresentation();
		FTouchExportParameters Params{ParamsConst};
		Params.GetTextureTransferSemaphore = nullptr;
		Params.GetTextureTransferWaitValue = 0;
		Params.GetTextureTransferResult = TEResultNoMatchingEntity;

		// 2. If this is not a new texture, transfer ownership if needed
		if (!bTextureNeedsCopy) // if the texture is already ready
		{
			// 	const bool bHasTransfer = TEInstanceHasTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation());
			UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[ExportTexture[%s]] Reusing existing texture for `%s` as it was already used by other parameters."),
				*GetCurrentThreadStr(), *Params.ParameterName.ToString());
			return TouchTexture;
		}
		if (!bIsNewTexture) // If this is a pre-existing texture, ensure we have ownership (which we should due to how the pool currently works)
		{
			if (TEInstanceHasTextureTransfer(Params.Instance, TouchTexture))
			{
				Params.GetTextureTransferResult = TEInstanceGetTextureTransfer(Params.Instance, TouchTexture, Params.GetTextureTransferSemaphore.take(), &Params.GetTextureTransferWaitValue); // request an ownership transfer from TE to UE, will be processed below
				if (Params.GetTextureTransferResult != TEResultSuccess && Params.GetTextureTransferResult != TEResultNoMatchingEntity) //TEResultNoMatchingEntity would be raised if there is no texture transfer waiting
				{
					UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[ExportTexture] TEInstanceGetTextureTransfer returned `%s` for parameter `%s` for frame %lld"), *TEResultToString(Params.GetTextureTransferResult), *Params.ParameterName.ToString(), ParamsConst.FrameData.FrameID);
					return nullptr;
				}
			}
		}
		// const bool bHasTextureTransfer = TEInstanceHasTextureTransfer_Debug(Params.Instance, TextureData->GetTouchRepresentation());

		// 3. Add a texture transfer 
		CommandQueueFence->DebugName = FString::Printf(TEXT("Fence_%lld"), Params.FrameData.FrameID);
		const uint64 WaitValue = CommandQueueFence->LastValue + 1; //  we need to wait until that fence value is reached
		UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("[ExportTexture_AnyThread[%s]] Fence `%s` last value `%llu` (GetCompletedValue(): `%lld`) for frame `%lld`"),
			*GetCurrentThreadStr(), *CommandQueueFence->DebugName, CommandQueueFence->LastValue, CommandQueueFence->NativeFence->GetCompletedValue(), Params.FrameData.FrameID)
		const TEResult TransferResult = TEInstanceAddTextureTransfer(Params.Instance, TouchTexture, CommandQueueFence->TouchFence, WaitValue);

		if (TransferResult != TEResultSuccess)
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[ExportTexture] TEInstanceAddTextureTransfer returned `%s` for parameter `%s` for frame %lld"), *TEResultToString(TransferResult), *Params.ParameterName.ToString(), Params.FrameData.FrameID);
			return nullptr;
		}
		
		TextureExports.Add(FExportCopyParams{MoveTemp(Params), GetRHIFromTexture(Params.Texture), TextureData});
		
		return TouchTexture;
	}

	void FTouchTextureExporterD3D12::FinalizeExportsToTouchEngine_AnyThread(const FTouchEngineInputFrameData& FrameData)
	{
		TexturePoolMaintenance();
		
		if (TextureExports.IsEmpty())
		{
			return;
		}
		
		ENQUEUE_RENDER_COMMAND(AccessTexture)([StrongThis = SharedThis(this), TextureExports = TextureExports, Fence = CommandQueueFence, FrameData](FRHICommandListImmediate& RHICmdList) mutable
		{
			// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("I.B.3 [RT] Cook Frame - AccessTextureForExport"), STAT_TE_I_B_3_D3D, STATGROUP_TouchEngine);
			// 1. Schedule a wait for the textures to be released by TouchEngine
			const TSharedRef<FTouchTextureExporterD3D12>& Exporter = StrongThis;

			for (FExportCopyParams& CopyParams : TextureExports)
			{
				const TEResult& GetTextureTransferResult = CopyParams.ExportParams.GetTextureTransferResult;
				if (GetTextureTransferResult == TEResultSuccess)
				{
					Exporter->ScheduleWaitFence(CopyParams.ExportParams.GetTextureTransferSemaphore, CopyParams.ExportParams.GetTextureTransferWaitValue);
				}
				else if (GetTextureTransferResult != TEResultNoMatchingEntity) // TE does not have ownership
				{
					UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[FTouchTextureExporterD3D12::ExportTexture_AnyThread:AccessTexture] Failed to transfer ownership of pooled texture back from Touch Engine"));
				}
			}
			
			// 3. Copy and Transfer to TE
			// We cannot call directly RHICmdList.CopyTexture(SourceRHI, TextureData->GetSharedTextureRHI(), FRHICopyTextureInfo()); as this is an asynchronous call and we don't know when it is finished
			ALLOC_COMMAND_CL(RHICmdList, FRHICopyFromUnrealToVulkanAndSignalFence)(StrongThis->D3DCommandQueue, MoveTemp(Fence), MoveTemp(TextureExports), FrameData.FrameID);
		});
		TextureExports.Reset();
	}

	void FTouchTextureExporterD3D12::ScheduleWaitFence(const TouchObject<TESemaphore>& AcquireSemaphore, uint64 AcquireValue) const
	{
		// Is nullptr if there is nothing to wait on
		if (!AcquireSemaphore)
		{
			return;
		}
		
		if (const Microsoft::WRL::ComPtr<ID3D12Fence> NativeFence = FenceCache->GetOrCreateSharedFence(AcquireSemaphore))
		{
			// ID3D12DynamicRHI* RHI = static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
			// ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetCommandQueue();
			// NativeCmdQ->Wait(NativeFence.Get(), AcquireValue);
			D3DCommandQueue->Wait(NativeFence.Get(), AcquireValue);
		}
		else
		{
			UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("Failed to get shared ID3D12Fence for input texture that is still in use. Texture will be overwriten without access synchronization!"));
		}
	}
}

#undef CHECK_HR_DEFAULT