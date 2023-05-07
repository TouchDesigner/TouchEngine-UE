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
// #include "Util/TouchFenceCache.h"

#include "RhiIncludeHelper.h"
#include "ID3D12DynamicRHI.h"
// THIRD_PARTY_INCLUDES_START
#include "D3D12RHI/Private/D3D12RHIPrivate.h"
// #include "D3D12RHI/Private/D3D12CommandContext.h"
// THIRD_PARTY_INCLUDES_END
// #include "D3D12RHI/Private/D3D12CommandContext.h"
// #include "D3D12RHI/Private/D3D12RHIPrivate.h"
#include "Engine/TEDebug.h"
#include "TouchEngine/TED3D.h"
#include "Util/TouchHelpers.h"

// macro to deal with COM calls inside a function that returns {} on failure
#define CHECK_HR_DEFAULT(COM_call)\
	{\
	HRESULT Res = COM_call;\
	if (FAILED(Res))\
	{\
	UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("`" #COM_call "` failed: 0x%X - %s"), Res, *GetComErrorDescription(Res)); \
	return {};\
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
		// const TSharedRef<FTouchTextureExporterD3D12> Exporter;
		TSharedPtr<FTouchFenceCache::FFenceData> Fence;
		
		// const TSharedRef<FTouchTextureExporterD3D12> Exporter;
		// FRHICopyTextureInfo CopyInfo;
		// TouchObject<TEInstance> Instance;
		FRHITexture* SourceTexture;
		TSharedRef<FExportedTextureD3D12> DestTexture;
		// TRefCountPtr<ID3D12GraphicsCommandList> GraphicCopyCommandList;
		FString Identifier;

		FRHICopyFromUnrealToVulkanAndSignalFence(TSharedPtr<FTouchFenceCache::FFenceData>&& Fence,
			FRHITexture* InSourceTexture, const TSharedRef<FExportedTextureD3D12>& InDestTexture, const FString& Identifier)
			: Fence(MoveTemp(Fence))
			// , Instance(Instance)
			// , Exporter(InExporter)
			// , CopyInfo(InCopyInfo)
			, SourceTexture(InSourceTexture)
			, DestTexture(InDestTexture)
			// , GraphicCopyCommandList(InGraphicCopyCommandList)
			, Identifier (Identifier)
		{
			ensure(SourceTexture);
		}

		void Execute(FRHICommandListBase& CmdList)
		{
			RHISTAT(CopyTextureForExport);
			// The goal is just to copy the texture so early implementations were just calling CmdList.GetContext().RHICopyTexture(SourceTexture, DestTexture->GetSharedTextureRHI(), CopyInfo);
			// The problem is that this ends up calling ID3D12GraphicsCommandList::CopyResource, which is an asynchronous call, and we might end up signalling before the copy is actually done, which became obvious in Synchronised mode
			// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-copyresource
			// To make this work, we looked for a way to call ID3D12CommandQueue::ExecuteCommandLists without interfering with the UE implementation.
			// This made us directly call some of the DirectX12 functions and create our own queue
			
			const uint64 WaitValue = 1;

			ID3D12DynamicRHI* RHI = static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
			check(RHI);
			// FD3D12CommandContext* Context = static_cast<FD3D12CommandContext*>(&CmdList.GetContext());
			// check(Context);
			ID3D12Device* Device = RHI->RHIGetDevice(CmdList.GetGPUMask().ToIndex()); // Context->Device->GetGPUIndex()); // Just reusing the current device to be sure
			
			
			// inspired by FD3D12CommandList::FD3D12CommandList
			TRefCountPtr<ID3D12GraphicsCommandList>  GraphicCommandList;
			TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;
			constexpr D3D12_COMMAND_LIST_TYPE QueueType = D3D12_COMMAND_LIST_TYPE_COPY; // Context->GraphicsCommandList()->GetType();
			//todo: check VERIFYD3D12RESULT
			Device->CreateCommandAllocator(QueueType, IID_PPV_ARGS(CommandAllocator.GetInitReference()));
			Device->CreateCommandList(0, QueueType, CommandAllocator, nullptr, IID_PPV_ARGS(GraphicCommandList.GetInitReference()));

			ID3D12Resource* Source = static_cast<ID3D12Resource*>(SourceTexture->GetNativeResource());
			ID3D12Resource* Destination = static_cast<ID3D12Resource*>(DestTexture->GetSharedTextureRHI()->GetNativeResource());

			GraphicCommandList->CopyResource(Destination, Source);;
			UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("   [FRHICopyFromUnrealToVulkanAndSignalFence[%s]] Resource Copy enqueued for `%s`"), *GetCurrentThreadStr(), *Identifier);
			GraphicCommandList->Close();
			
			ID3D12CommandList* ppCommandLists[] = { GraphicCommandList };//
			
			// inspired by FD3D12Queue::FD3D12Queue
			TRefCountPtr<ID3D12CommandQueue> D3DCommandQueue;
			D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
			CommandQueueDesc.Type = QueueType; // GetD3DCommandListType((ED3D12QueueType)QueueType);
			CommandQueueDesc.Priority = 0;
			// CommandQueueDesc.NodeMask = Device->GetGPUMask().GetNative(); // is this needed?
			CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(D3DCommandQueue.GetInitReference()));
			
			D3DCommandQueue->ExecuteCommandLists(1, ppCommandLists); 
			D3DCommandQueue->Signal(Fence->NativeFence.Get(), WaitValue);
			
			UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("   [FRHICopyFromUnrealToVulkanAndSignalFence[%s]] CommandList executed and fence signalled with value `%llu` for `%s`"),
				*GetCurrentThreadStr(),	WaitValue, *Identifier)
		}
	};
	
	TSharedPtr<FTouchTextureExporterD3D12> FTouchTextureExporterD3D12::Create(TSharedRef<FTouchFenceCache> FenceCache)
	{
		return MakeShared<FTouchTextureExporterD3D12>(MoveTemp(FenceCache));
	}

	FTouchTextureExporterD3D12::FTouchTextureExporterD3D12(TSharedRef<FTouchFenceCache> FenceCache)
		: FenceCache(MoveTemp(FenceCache))
	{}

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

	TSharedPtr<FExportedTextureD3D12> FTouchTextureExporterD3D12::CreateTexture(const FTextureCreationArgs& Params) const
	{
		const FRHITexture2D* SourceRHI = GetRHIFromTexture(Params.Texture);
		return FExportedTextureD3D12::Create(*SourceRHI, SharedResourceSecurityAttributes);
	}

	void FTouchTextureExporterD3D12::PrepareForExportToTouchEngine_AnyThread()
	{
		NumberTextureNeedingCopy = 0;
	}

	void FTouchTextureExporterD3D12::FinalizeExportToTouchEngine_AnyThread()
	{
		if (NumberTextureNeedingCopy  == 0)
		{
			return;
		}
		
		ENQUEUE_RENDER_COMMAND(AccessTexture)([](FRHICommandListImmediate& RHICmdList) mutable
		{
			RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
		});
	}

	bool FTouchTextureExporterD3D12::GetNextOrAllocPooledTETexture_Internal(const FTouchExportParameters& TouchExportParameters, bool& bIsNewTexture, TouchObject<TETexture>& OutTexture)
	{
		return GetNextOrAllocPooledTETexture(TouchExportParameters, bIsNewTexture, OutTexture);
	}

	TouchObject<TETexture> FTouchTextureExporterD3D12::ExportTexture_AnyThread(const FTouchExportParameters& ParamsConst, TEGraphicsContext* GraphicsContext)
	{
		// 1. We get a Dx12 Texture to copy onto
		bool bIsNewTexture;
		const TSharedPtr<FExportedTextureD3D12> TextureData = GetNextOrAllocPooledTexture(MakeTextureCreationArgs(ParamsConst), bIsNewTexture);
		if (!TextureData)
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[ExportTexture_AnyThread[%s]] ETouchExportErrorCode::InternalGraphicsDriverError for parameter %s"), *UE::TouchEngine::GetCurrentThreadStr(), *ParamsConst.ParameterName.ToString());
			return nullptr;
		}
		UE_LOG(LogTouchEngineD3D12RHI, Display, TEXT("[ExportTexture_AnyThread[%s]] GetNextOrAllocPooledTexture returned %s for parameter `%s`"),
		       *UE::TouchEngine::GetCurrentThreadStr(), bIsNewTexture ? TEXT("a NEW texture") : TEXT("an EXISTING texture"), *ParamsConst.ParameterName.ToString());

		const TouchObject<TETexture>& TouchTexture = TextureData->GetTouchRepresentation();
		FTouchExportParameters Params{ParamsConst};
		Params.GetTextureTransferSemaphore = nullptr;
		Params.GetTextureTransferWaitValue = 0;
		Params.GetTextureTransferResult = TEResultNoMatchingEntity;

		// 2. If this is not a new texture, transfer ownership if needed
		if (!bIsNewTexture) // If this is a pre-existing texture
		{
			if (Params.bReuseExistingTexture) //todo: handle this
			{
				UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("[ExportTexture_AnyThread[%s]] Params.bReuseExistingTexture was true for %s"), *GetCurrentThreadStr(), *Params.ParameterName.ToString());
				UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[ExportTexture_AnyThread[%s]] -- Params.bReuseExistingTexture currently not handled. Need to be exposed to BP first"), *GetCurrentThreadStr());
				// return TouchTexture;
			}

			const TouchObject<TEInstance>& Instance = Params.Instance;

			if (TEInstanceHasTextureTransfer_Debug(Params.Instance, TextureData->GetTouchRepresentation()))
			{
				Params.GetTextureTransferResult = TEInstanceGetTextureTransfer_Debug(Instance, TouchTexture, Params.GetTextureTransferSemaphore.take(), &Params.GetTextureTransferWaitValue); // request an ownership transfer from TE to UE, will be processed below
				if (Params.GetTextureTransferResult != TEResultSuccess && Params.GetTextureTransferResult != TEResultNoMatchingEntity) //TEResultNoMatchingEntity would be raised if there is no texture transfer waiting
				{
					UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[ExportTexture_AnyThread[%s]] ETouchExportErrorCode::FailedTextureTransfer for parameter %s"), *GetCurrentThreadStr(), *Params.ParameterName.ToString());
					TERelease(&Params.GetTextureTransferSemaphore);
					//todo: would we have wanted to call TEInstanceLinkSetTextureValue in this case?
					return nullptr;
				}
			}
		}
		// const bool bHasTextureTransfer = TEInstanceHasTextureTransfer_Debug(Params.Instance, TextureData->GetTouchRepresentation());

		// 3. Add a texture transfer 
		TSharedPtr<FTouchFenceCache::FFenceData> Fence = FenceCache->GetOrCreateOwnedFence_AnyThread();
		constexpr uint64 WaitValue = 1; //  we need to wait until that fence value is reached. Currently setup to always be 1
		const TEResult TransferResult = TEInstanceAddTextureTransfer_Debug(Params.Instance, TextureData->GetTouchRepresentation(), Fence->TouchFence, WaitValue);
		++NumberTextureNeedingCopy;

		//4. Enqueue the copy of the texture
		ENQUEUE_RENDER_COMMAND(AccessTexture)([StrongThis = SharedThis(this), Params = MoveTemp(Params), SharedTextureData = TextureData.ToSharedRef(), Fence = MoveTemp(Fence)](FRHICommandListImmediate& RHICmdList) mutable
		{
			const bool bBecameInvalidSinceRenderEnqueue = !IsValid(Params.Texture);
			if (bBecameInvalidSinceRenderEnqueue)
			{
				return;
			}
			
			// 1. Await for the texture to be released bu TouchEngine //todo: should we await on RHI Thread?
			const TSharedRef<FExportedTextureD3D12>& TextureData = SharedTextureData;
			const TSharedRef<FTouchTextureExporterD3D12>& Exporter = StrongThis;
			
			const TEResult& GetTextureTransferResult = Params.GetTextureTransferResult;
			if (GetTextureTransferResult == TEResultSuccess)
			{
				Exporter->ScheduleWaitFence(Params.GetTextureTransferSemaphore, Params.GetTextureTransferWaitValue);
			}
			else if (GetTextureTransferResult != TEResultNoMatchingEntity) // TE does not have ownership
			{
				UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("[FTouchTextureExporterD3D12::ExportTexture_AnyThread:AccessTexture] Failed to transfer ownership of pooled texture back from Touch Engine"));
			}
			TERelease(&Params.GetTextureTransferSemaphore);
			Params.GetTextureTransferSemaphore = nullptr;

			// 2. Debugging information
			FRHITexture2D* SourceRHI = Exporter->GetRHIFromTexture(Params.Texture);
			// RHICmdList.EnqueueLambda([SourceRHI, TextureData](FRHICommandListBase& CmdListLambda)
			// {
			// 	FColor Color;
			// 	bool bResult;
			// 	bResult = GetRHITopLeftPixelColor(TextureData->GetSharedTextureRHI(), Color);
			// 	UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("  Pre Copy GPU Destination Image Color => %s %s"), *Color.ToString(), bResult ? TEXT("") : TEXT("ERROR"))
			// });
			
			// 3. Copy and Transfer to TE
			// We cannot call directly RHICmdList.CopyTexture(SourceRHI, TextureData->GetSharedTextureRHI(), FRHICopyTextureInfo()); as this is an asynchronous call and we don't know when it is finished
			ALLOC_COMMAND_CL(RHICmdList, FRHICopyFromUnrealToVulkanAndSignalFence)(MoveTemp(Fence), SourceRHI, TextureData, Params.ParameterName.ToString());
		});
		
		return TouchTexture;
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
			ID3D12DynamicRHI* RHI = static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
			ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetCommandQueue();
			NativeCmdQ->Wait(NativeFence.Get(), AcquireValue);
		}
		else
		{
			UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("Failed to get shared ID3D12Fence for input texture that is still in use. Texture will be overwriten without access synchronization!"));
		}
	}
}

#undef CHECK_HR_DEFAULT