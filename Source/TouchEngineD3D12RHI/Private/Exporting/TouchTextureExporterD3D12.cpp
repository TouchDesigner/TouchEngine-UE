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
#include "Util/TouchFenceCache.h"

#include "RhiIncludeHelper.h"
#include "ID3D12DynamicRHI.h"

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
	FRHICOMMAND_MACRO(FRHIIncrementAndSignalFence)
	{
		const TSharedRef<FTouchTextureExporterD3D12> Exporter;

		FRHICopyTextureInfo CopyInfo;
		FRHITexture* SourceTexture;
		FRHITexture* DestTexture;
		FString Identifier;

		FRHIIncrementAndSignalFence(TSharedRef<FTouchTextureExporterD3D12> Exporter, FRHITexture* InSourceTexture, FRHITexture* InDestTexture, const FRHICopyTextureInfo& InCopyInfo, const FString& Identifier)
			: Exporter(MoveTemp(Exporter)),
			CopyInfo(InCopyInfo)
			, SourceTexture(InSourceTexture)
			, DestTexture(InDestTexture)
			,Identifier (Identifier)
		{
			ensure(SourceTexture);
			ensure(DestTexture);
		}

		void Execute(FRHICommandListBase& CmdList)
		{
			RHISTAT(CopyTexture);
			CmdList.GetContext().RHICopyTexture(SourceTexture, DestTexture, CopyInfo); // from FRHICommandCopyTexture::Execute
			// FPlatformProcess::Sleep(2);
			UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("   [FRHICopyFromUnrealToVulkanCommand[%s]] Texture copy enqueue to render thread for param `%s`. Enqueuing signal to TouchEngine..."),
				*GetCurrentThreadStr(),
				*Identifier)

			const uint64 WaitValue = Exporter->IncrementAndSignalFence();
			UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("   [FRHIIncrementAndSignalFence[%s]] WaitValue `%llu`"),
					*GetCurrentThreadStr(),
					WaitValue)
		}
	};

	FRHICOMMAND_MACRO(FRHICopyFromUnrealToVulkanCommand)
	{
		FTouchExportParameters Params;
		const TSharedRef<FTouchTextureExporterD3D12> Exporter;
		const TSharedRef<FExportedTextureD3D12> TextureData;
		// TPromise<FTouchExportResult> Promise; // , TPromise<FTouchExportResult>&& Promise

		FRHICopyFromUnrealToVulkanCommand(FTouchExportParameters&& Params, TSharedRef<FTouchTextureExporterD3D12> Exporter, TSharedRef<FExportedTextureD3D12> TextureData)
			: Params(MoveTemp(Params))
			  , Exporter(MoveTemp(Exporter))
			  , TextureData(MoveTemp(TextureData))
			  // , Promise(MoveTemp(Promise))
		{
		}

		void Execute(FRHICommandListBase& CmdList)
		{
			// 1. If TE still has ownership of it, schedule a wait operation
			// const bool bNeedsOwnershipTransfer = TextureData->WasEverUsedByTouchEngine();
			// if (bNeedsOwnershipTransfer)
			// {
			// 	TouchObject<TESemaphore>& AcquireSemaphore = Params.GetTextureTransferSemaphore;
			// 	const uint64& AcquireValue = Params.GetTextureTransferWaitValue;
			//
			// 	const bool bIsInUseByTouchEngine = TextureData->IsInUseByTouchEngine();
			// 	const TouchObject<TEInstance>& Inst = Params.Instance;
			// 	const TouchObject<TETexture>& Text = TextureData->GetTouchRepresentation();
			// 	//todo: check why this crashes now, probably something with the instance?
			// 	const bool bHasTextureTransfer = bIsInUseByTouchEngine && TEInstanceHasTextureTransfer(Inst, Text);
			//
			// 	const TEResult& GetTextureTransferResult = Params.GetTextureTransferResult;
			// 	if (GetTextureTransferResult == TEResultSuccess)
			// 	{
			// 		Exporter->ScheduleWaitFence(AcquireSemaphore, AcquireValue);
			// 	}
			// 	else if (GetTextureTransferResult == TEResultNoMatchingEntity) // TE does not have ownership
			// 	{
			// 	}
			// 	else
			// 	{
			// 		UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("bIsInUseByTouchEngine: %s  bHasTextureTransfer: %s  GetTextureTransferResult: %s"),
			// 		       bIsInUseByTouchEngine ? TEXT("TRUE") : TEXT("FALSE"), bHasTextureTransfer ? TEXT("TRUE") : TEXT("FALSE"),
			// 		       GetTextureTransferResult == TEResultSuccess ? TEXT("SUCCESS") : TEXT("FAIL"));
			// 		UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("Failed to transfer ownership of pooled texture back from Touch Engine"));
			// 	}
			// 	TERelease(&AcquireSemaphore);
			// 	AcquireSemaphore = nullptr;
			// }
			const TEResult& GetTextureTransferResult = Params.GetTextureTransferResult;
			if (GetTextureTransferResult == TEResultSuccess)
			{
				Exporter->ScheduleWaitFence(Params.GetTextureTransferSemaphore, Params.GetTextureTransferWaitValue);
			}
			else if (GetTextureTransferResult != TEResultNoMatchingEntity) // TE does not have ownership
			{
				UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("Failed to transfer ownership of pooled texture back from Touch Engine"));
			}
			TERelease(&Params.GetTextureTransferSemaphore);
			Params.GetTextureTransferSemaphore = nullptr;

			// 2. 
			FRHITexture2D* SourceRHI = Exporter->GetRHIFromTexture(Params.Texture);
			// CmdList.GetContext().RHICopyTexture(SourceRHI, TextureData->GetSharedTextureRHI(), FRHICopyTextureInfo());
			// 3. Transfer to TE
			// UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("   [FRHICopyFromUnrealToVulkanCommand[%s]] Texture copy enqueue to render thread for param `%s`. Enqueuing signal to TouchEngine..."),
			// 		*GetCurrentThreadStr(),
			// 		*Params.ParameterName.ToString())
			ALLOC_COMMAND_CL(CmdList, FRHIIncrementAndSignalFence)(Exporter, SourceRHI, TextureData->GetSharedTextureRHI(), FRHICopyTextureInfo(), Params.ParameterName.ToString());
			// const uint64 WaitValue = Exporter->IncrementAndSignalFence();
			// const TEResult TransferResult = TEInstanceAddTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation(), Exporter->FenceTE, WaitValue);
			// if (TransferResult != TEResultSuccess)
			// { //todo: bad usage given here for some reason! Params.Instance is null for some reason
			// 	UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("   [FRHICopyFromUnrealToVulkanCommand] TEInstanceAddTextureTransfer error code: %d"), static_cast<int32>(TransferResult));
			// 	Promise.SetValue(FTouchExportResult{ETouchExportErrorCode::FailedTextureTransfer});
			// }
			// else
			// {
			// 	UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("   [FRHICopyFromUnrealToVulkanCommand[%s]] Successfully added Texture transfer to Unreal Engine for parameter [%s]"),
			// 		*GetCurrentThreadStr(),
			// 		*Params.ParameterName.ToString())
			// 	Promise.SetValue(FTouchExportResult{ETouchExportErrorCode::Success, TextureData->GetTouchRepresentation()});
			// }
			// Promise.SetValue(FTouchExportResult{ETouchExportErrorCode::Success, TextureData->GetTouchRepresentation()});
		}
	};
	
	TSharedPtr<FTouchTextureExporterD3D12> FTouchTextureExporterD3D12::Create(ID3D12Device* Device, TSharedRef<FTouchFenceCache> FenceCache)
	{
		Microsoft::WRL::ComPtr<ID3D12Fence> FenceNative;
		CHECK_HR_DEFAULT(Device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&FenceNative)))

		HANDLE SharedFenceHandle;
		CHECK_HR_DEFAULT(Device->CreateSharedHandle(FenceNative.Get(), nullptr, GENERIC_ALL, nullptr, &SharedFenceHandle));

		TouchObject<TED3DSharedFence> FenceTE;
		FenceTE.take(TED3DSharedFenceCreate(SharedFenceHandle, [](HANDLE handle, TEObjectEvent event, void * TE_NULLABLE info)
		{
			switch (event)
			{
			case TEObjectEventBeginUse:
				UE_LOG(LogTouchEngineD3D12RHI, Display, TEXT("    [TED3DSharedFenceCallback] TEObjectEventBeginUse"));
				break;
			case TEObjectEventEndUse:
				UE_LOG(LogTouchEngineD3D12RHI, Display, TEXT("    [TED3DSharedFenceCallback] TEObjectEventEndUse"));
				break;
			case TEObjectEventRelease:
				UE_LOG(LogTouchEngineD3D12RHI, Display, TEXT("    [TED3DSharedFenceCallback] TEObjectEventRelease"));
				break;
			default:
				UE_LOG(LogTouchEngineD3D12RHI, Display, TEXT("    [TED3DSharedFenceCallback] default"));
				break;
			}
		}, nullptr));
        // TouchEngine duplicates the handle, so close it now
		CloseHandle(SharedFenceHandle);
		if (!FenceTE)
		{
			return nullptr;
		}

		return MakeShared<FTouchTextureExporterD3D12>(MoveTemp(FenceCache), MoveTemp(FenceNative), MoveTemp(FenceTE));
	}

	FTouchTextureExporterD3D12::FTouchTextureExporterD3D12(TSharedRef<FTouchFenceCache> FenceCache, Microsoft::WRL::ComPtr<ID3D12Fence> FenceNative, TouchObject<TED3DSharedFence> FenceTE)
		: FenceCache(MoveTemp(FenceCache))
		, FenceNative(MoveTemp(FenceNative))
		, FenceTE(MoveTemp(FenceTE))
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

	TSharedPtr<FExportedTextureD3D12> FTouchTextureExporterD3D12::CreateTexture(const FTextureCreationArgs& Params)
	{
		const FRHITexture2D* SourceRHI = GetRHIFromTexture(Params.Texture);
		return FExportedTextureD3D12::Create(*SourceRHI, SharedResourceSecurityAttributes);
	}

	bool FTouchTextureExporterD3D12::GetNextOrAllocPooledTETexture_Internal(const FTouchExportParameters& TouchExportParameters, bool& bIsNewTexture, TouchObject<TETexture>& OutTexture)
	{
		return GetNextOrAllocPooledTETexture(TouchExportParameters, bIsNewTexture, OutTexture);
	}

	TouchObject<TETexture> FTouchTextureExporterD3D12::ExportTexture_GameThread(const FTouchExportParameters& ParamsConst, TEGraphicsContext* GraphicsContext)
	{
		bool bIsNewTexture;
		const TSharedPtr<FExportedTextureD3D12> TextureData = GetNextOrAllocPooledTexture(MakeTextureCreationArgs(ParamsConst), bIsNewTexture);
		if (!TextureData)
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[ExportTexture] ETouchExportErrorCode::InternalGraphicsDriverError for parameter %s"), *ParamsConst.ParameterName.ToString());
			return nullptr;
		}

		const TouchObject<TETexture>& TouchTexture = TextureData->GetTouchRepresentation();

		FTouchExportParameters Params {ParamsConst};
		
		// We link first as we have access to the texture and we might leave early
		const auto AnsiString = StringCast<ANSICHAR>(*Params.ParameterName.ToString());
		const char* IdentifierAsCStr = AnsiString.Get();
		
		Params.GetTextureTransferSemaphore = nullptr;
		Params.GetTextureTransferWaitValue = 0;
		Params.GetTextureTransferResult = TEResultNoMatchingEntity;
		if (!bIsNewTexture) // If this is a pre-existing texture
		{
			if (Params.bReuseExistingTexture) //todo: would we still have wanted an ownership transfer if we were doing this? shouldn't we need a copy regardless?
			{
				TEInstanceLinkSetTextureValue(Params.Instance, IdentifierAsCStr, TouchTexture, GraphicsContext);
				UE_LOG(LogTouchEngineD3D12RHI, Display, TEXT("  TEInstanceLinkSetTextureValue[%s]:  %s"), *GetCurrentThreadStr(), *Params.ParameterName.ToString());
				UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[FTouchTextureExporterD3D12::ExportTexture_GameThread] Params.bReuseExistingTexture was true for %s"), *Params.ParameterName.ToString());
				return TouchTexture;
			}
			
			const TouchObject<TEInstance>& Instance = Params.Instance;
			
			Params.GetTextureTransferResult = TEInstanceGetTextureTransfer(Instance, TouchTexture, Params.GetTextureTransferSemaphore.take(), &Params.GetTextureTransferWaitValue); // request an ownership transfer from TE to UE, will be processed below
			if (Params.GetTextureTransferResult != TEResultSuccess && Params.GetTextureTransferResult != TEResultNoMatchingEntity) //TEResultNoMatchingEntity would be raised if there is no texture transfer waiting
			{
				UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("[ExportTexture] ETouchExportErrorCode::FailedTextureTransfer for parameter %s"), *Params.ParameterName.ToString());
				//todo: would we have wanted to call TEInstanceLinkSetTextureValue in this case?
				return nullptr;
			}
		}

		const bool bHasTextureTransfer = TEInstanceHasTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation());
		if (bHasTextureTransfer && bIsNewTexture)
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("bHasTextureTransfer is %s"), bHasTextureTransfer ? TEXT("True") : TEXT("False"));
		}

		TEInstanceLinkSetTextureValue(Params.Instance, IdentifierAsCStr, TouchTexture, nullptr);
		UE_LOG(LogTouchEngineD3D12RHI, Display, TEXT("  TEInstanceLinkSetTextureValue[%s]:  %s"), *GetCurrentThreadStr(), *Params.ParameterName.ToString());

		NextFenceValue = 0; //todo: try creating a new fence every time
		const uint64 WaitValue = IncrementAndSignalFence() + 1; // ++NextFenceValue+1; // we need to wait until that fence value is reached
		UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("   [ExportTexture_GameThread] About to call TEInstanceAddTextureTransfer with wait value: %lld"), WaitValue);
		const TEResult TransferResult = TEInstanceAddTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation(), FenceTE, WaitValue);
		// if (TransferResult != TEResultSuccess)
		// {
		// 	UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("   [ExportTexture_GameThread] TEInstanceAddTextureTransfer error code: %d"), static_cast<int32>(TransferResult));
		// 	return nullptr;
		// }
		// else
		// {
		// 	UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("   [ExportTexture_GameThread[%s]] Successfully added Texture transfer to Unreal Engine for parameter [%s]"),
		// 		*GetCurrentThreadStr(),
		// 		*Params.ParameterName.ToString())
		// }




		// TPromise<FTouchExportResult> Promise;
		// TFuture<FTouchExportResult> Future = Promise.GetFuture(); //todo: need to do something of this, or find a way to get rid of it , Promise = MoveTemp(Promise)
		ENQUEUE_RENDER_COMMAND(AccessTexture)([StrongThis = SharedThis(this), Params = MoveTemp(Params), SharedTextureData = TextureData.ToSharedRef(), GraphicsContext](FRHICommandListImmediate& RHICmdList) mutable
		{
			const bool bBecameInvalidSinceRenderEnqueue = !IsValid(Params.Texture);
			if (bBecameInvalidSinceRenderEnqueue)
			{
				return;
			}
			
			// ALLOC_COMMAND_CL(RHICmdList, FRHICopyFromUnrealToVulkanCommand)(MoveTemp(Params), StrongThis, MoveTemp(SharedTextureData));
			const TSharedRef<FExportedTextureD3D12>& TextureData = SharedTextureData;
			const TSharedRef<FTouchTextureExporterD3D12>& Exporter = StrongThis;
			
			const TEResult& GetTextureTransferResult = Params.GetTextureTransferResult;
			if (GetTextureTransferResult == TEResultSuccess)
			{
				Exporter->ScheduleWaitFence(Params.GetTextureTransferSemaphore, Params.GetTextureTransferWaitValue);
			}
			else if (GetTextureTransferResult != TEResultNoMatchingEntity) // TE does not have ownership
			{
				UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("Failed to transfer ownership of pooled texture back from Touch Engine"));
			}
			TERelease(&Params.GetTextureTransferSemaphore);
			Params.GetTextureTransferSemaphore = nullptr;
			
			// 2. 
			FRHITexture2D* SourceRHI = Exporter->GetRHIFromTexture(Params.Texture);
			// RHICmdList.CopyTexture(SourceRHI, TextureData->GetSharedTextureRHI(), FRHICopyTextureInfo());
			// 3. Transfer to TE
			// UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("   [FRHICopyFromUnrealToVulkanCommand[%s]] Texture copy enqueued to render thread for param `%s`. Signalling to TouchEngine..."),
			// 		*GetCurrentThreadStr(),
			// 		*Params.ParameterName.ToString())
			// RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread); //todo: should not be here!
			ALLOC_COMMAND_CL(RHICmdList, FRHIIncrementAndSignalFence)(Exporter, SourceRHI, TextureData->GetSharedTextureRHI(), FRHICopyTextureInfo(), Params.ParameterName.ToString());
			// const uint64 WaitValue = Exporter->IncrementAndSignalFence();
			// auto Fence = RHICmdList.CreateGPUFence(TEXT("TEstFence"));
			
			// ENQUEUE_RENDER_COMMAND(DoneSignalling)([WaitValue](FRHICommandListImmediate& RHICmdList) mutable
			// {
			// 	UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("   [FRHICopyFromUnrealToVulkanCommand[%s]] DoneSignalling the wait value %lld"),*GetCurrentThreadStr(), WaitValue)
			// });
			// auto buffer = RHICmdList.GetNativeCommandBuffer();
			// ID3D12DynamicRHI* GQueue = static_cast<ID3D12DynamicRHI*>(RHICmdList.GetNativeGraphicsQueue());
			// ID3D12DynamicRHI* CQueue = static_cast<ID3D12DynamicRHI*>(RHICmdList.GetNativeComputeQueue());
			// if (GQueue) UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("   [FRHICopyFromUnrealToVulkanCommand] GetNativeGraphicsQueue"));
			// if (CQueue) UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("   [FRHICopyFromUnrealToVulkanCommand] GetNativeComputeQueue"));
			// ID3D12DynamicRHI* RHI = static_cast<ID3D12DynamicRHI*>(GQueue ? GQueue : CQueue);// static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
			// ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetCommandQueue();
			// NativeCmdQ->Wait(Exporter->FenceNative.Get(), WaitValue);
			// UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("   [FRHICopyFromUnrealToVulkanCommand[%s]] DoneSignalling the wait value %lld"),*GetCurrentThreadStr(), WaitValue)
			// UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("   [FRHICopyFromUnrealToVulkanCommand[%s]] Signalled to TouchEngine the wait value %lld"),*GetCurrentThreadStr(), WaitValue)
			// Promise.SetValue(FTouchExportResult{ETouchExportErrorCode::Success, TextureData->GetTouchRepresentation()});
		});
		// Promise.SetValue(FTouchExportResult{ETouchExportErrorCode::Success, TextureData->GetTouchRepresentation()});
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

	uint64 FTouchTextureExporterD3D12::IncrementAndSignalFence()
	{
		ID3D12DynamicRHI* RHI = static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
		// ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetCommandQueue();
		++NextFenceValue;
		FenceNative->Signal(NextFenceValue);
		// NativeCmdQ->Signal(FenceNative.Get(), NextFenceValue);
		return NextFenceValue;
	}
}

#undef CHECK_HR_DEFAULT