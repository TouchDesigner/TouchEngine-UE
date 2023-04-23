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
	FRHICOMMAND_MACRO(FRHICopyFromUnrealToVulkanCommand)
	{
		const FTouchExportParameters Params;
		const TSharedRef<FTouchTextureExporterD3D12> Exporter;
		const TSharedRef<FExportedTextureD3D12> TextureData;
		FRHICommandListImmediate& RHICmdList;
		TPromise<FTouchExportResult> Promise;

		FRHICopyFromUnrealToVulkanCommand(const FTouchExportParameters& Params, TSharedRef<FTouchTextureExporterD3D12> Exporter, TSharedRef<FExportedTextureD3D12> TextureData, TPromise<FTouchExportResult>&& Promise, FRHICommandListImmediate& InRHICmdList)
			: Params(Params)
			  , Exporter(MoveTemp(Exporter))
			  , TextureData(MoveTemp(TextureData))
			  , RHICmdList(InRHICmdList)
			  , Promise(MoveTemp(Promise))
		{
		}
		
		void Execute(FRHICommandListBase& CmdList)
		{
		    // 1. If TE still has ownership of it, schedule a wait operation
            const bool bNeedsOwnershipTransfer = TextureData->WasEverUsedByTouchEngine(); 
            if (bNeedsOwnershipTransfer)
            {
                TouchObject<TESemaphore>& AcquireSemaphore = Params.GetTextureTransferSemaphore;
                uint64& AcquireValue = Params.GetTextureTransferWaitValue;

                const bool bIsInUseByTouchEngine = TextureData->IsInUseByTouchEngine();
                const bool bHasTextureTransfer = bIsInUseByTouchEngine && TEInstanceHasTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation());

				// if (bHasTextureTransfer)
				// {
					// const TEResult GetTextureTransferResult = TEInstanceGetTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation(), AcquireSemaphore.take(), &AcquireValue);
					const TEResult& GetTextureTransferResult = Params.GetTextureTransferResult;
					if (GetTextureTransferResult == TEResultSuccess)
					{
						Exporter->ScheduleWaitFence(AcquireSemaphore, AcquireValue);
					}
					else if (GetTextureTransferResult == TEResultNoMatchingEntity) // TE does not have ownership
					{
					}
					else
					{
						UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("bIsInUseByTouchEngine: %s  bHasTextureTransfer: %s  GetTextureTransferResult: %s"),
						       bIsInUseByTouchEngine ? TEXT("TRUE") : TEXT("FALSE"), bHasTextureTransfer ? TEXT("TRUE") : TEXT("FALSE"),
						       GetTextureTransferResult == TEResultSuccess ? TEXT("SUCCESS") : TEXT("FAIL"));
						UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("Failed to transfer ownership of pooled texture back from Touch Engine"));
					}
            	TERelease(&AcquireSemaphore);
            	AcquireSemaphore= nullptr;
				// }
				// else
				// {
				// 	UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("bIsInUseByTouchEngine: %s  bHasTextureTransfer: %s"),
				// 		bIsInUseByTouchEngine ? TEXT("TRUE") : TEXT("FALSE"), bHasTextureTransfer ? TEXT("TRUE") : TEXT("FALSE"));
				// 	UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("Texture was transferred to TouchEngine at least once, is no longer in use, but TouchEngine refuses to transfer it back"));
				// }
            }

            // 2. 
            FRHITexture2D* SourceRHI = Exporter->GetRHIFromTexture(Params.Texture);
            RHICmdList.GetContext().RHICopyTexture(SourceRHI, TextureData->GetSharedTextureRHI(), FRHICopyTextureInfo());
			RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
		    // 3. Transfer to TE
			const uint64 WaitValue = Exporter->IncrementAndSignalFence();
			const TEResult TransferResult = TEInstanceAddTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation(), Exporter->FenceTE, WaitValue);
			if (TransferResult != TEResultSuccess)
			{
				UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("TEInstanceAddTextureTransfer error code: %d"), static_cast<int32>(TransferResult));
				Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::FailedTextureTransfer }); 
			}
			else
			{
				Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::Success, TextureData->GetTouchRepresentation() });
			}
		}
	};
	
	TSharedPtr<FTouchTextureExporterD3D12> FTouchTextureExporterD3D12::Create(ID3D12Device* Device, TSharedRef<FTouchFenceCache> FenceCache)
	{
		Microsoft::WRL::ComPtr<ID3D12Fence> FenceNative;
		CHECK_HR_DEFAULT(Device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&FenceNative)))

		HANDLE SharedFenceHandle;
		CHECK_HR_DEFAULT(Device->CreateSharedHandle(FenceNative.Get(), nullptr, GENERIC_ALL, nullptr, &SharedFenceHandle));

		TouchObject<TED3DSharedFence> FenceTE;
		FenceTE.take(TED3DSharedFenceCreate(SharedFenceHandle, nullptr, nullptr));
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

	TFuture<FTouchExportResult> FTouchTextureExporterD3D12::ExportTexture_GameThread(const FTouchExportParameters& Params, TouchObject<TETexture>& OutTexture)
	{
		bool bIsNewTexture;
		const TSharedPtr<FExportedTextureD3D12> TextureData = GetNextOrAllocPooledTexture(MakeTextureCreationArgs(Params), bIsNewTexture);
		if (!TextureData)
		{
			OutTexture = TouchObject<TETexture>();
			return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::InternalGraphicsDriverError }).GetFuture();
		}

		const TouchObject<TETexture>& TouchTexture = TextureData->GetTouchRepresentation();
		Params.GetTextureTransferSemaphore = nullptr;
		Params.GetTextureTransferWaitValue = 0;
		Params.GetTextureTransferResult = TEResultNoMatchingEntity;
		if (!bIsNewTexture) // If this is a pre-existing texture
		{
			if (Params.bReuseExistingTexture) //todo: would we still have wanted an ownership transfer if we were doing this?
			{
				OutTexture = TouchTexture;
				return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::Success, OutTexture }).GetFuture();
			}
			
			const TouchObject<TEInstance>& Instance = Params.Instance;
			
			// TouchObject<TESemaphore> Semaphore; //todo: do I need to release this? => need to pass them around
			// uint64 WaitValue;
			Params.GetTextureTransferResult = TEInstanceGetTextureTransfer(Instance, TouchTexture, Params.GetTextureTransferSemaphore.take(), &Params.GetTextureTransferWaitValue); // request an ownership transfer from TE to UE, will be processed below
			if (Params.GetTextureTransferResult != TEResultSuccess && Params.GetTextureTransferResult != TEResultNoMatchingEntity) //TEResultNoMatchingEntity would be raised if there is no texture transfer waiting
			{
				OutTexture = TouchObject<TETexture>();
				return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::FailedTextureTransfer }).GetFuture(); // return MakeFulfilledPromise<ECopyTouchToUnrealResult>(ECopyTouchToUnrealResult::Failure).GetFuture();
			}
		}
		
		TPromise<FTouchExportResult> Promise;
		TFuture<FTouchExportResult> Future = Promise.GetFuture();
		ENQUEUE_RENDER_COMMAND(AccessTexture)([StrongThis = SharedThis(this), Params, Promise = MoveTemp(Promise), SharedTextureData = TextureData.ToSharedRef()](FRHICommandListImmediate& RHICmdList) mutable
		{
			const bool bBecameInvalidSinceRenderEnqueue = !IsValid(Params.Texture);
			if (bBecameInvalidSinceRenderEnqueue)
			{
				Promise.SetValue(FTouchExportResult{ ETouchExportErrorCode::UnsupportedTextureObject });
				return;
			}

			//todo: why in `ALLOC_COMMAND_CL` try to?
			// ALLOC_COMMAND_CL(RHICmdList, FRHICopyFromUnrealToVulkanCommand)(Params, StrongThis, MoveTemp(SharedTextureData), MoveTemp(Promise), RHICmdList);

			{
				TSharedRef<FExportedTextureD3D12>& TextureData = SharedTextureData;
				TSharedRef<FTouchTextureExporterD3D12>& Exporter = StrongThis;
				
				// 1. If TE still has ownership of it, schedule a wait operation
				const bool bNeedsOwnershipTransfer = TextureData->WasEverUsedByTouchEngine();
				if (bNeedsOwnershipTransfer)
				{
					TouchObject<TESemaphore>& AcquireSemaphore = Params.GetTextureTransferSemaphore;
					uint64& AcquireValue = Params.GetTextureTransferWaitValue;

					const bool bIsInUseByTouchEngine = TextureData->IsInUseByTouchEngine();
					const bool bHasTextureTransfer = bIsInUseByTouchEngine && TEInstanceHasTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation());

					// if (bHasTextureTransfer)
					// {
					// const TEResult GetTextureTransferResult = TEInstanceGetTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation(), AcquireSemaphore.take(), &AcquireValue);
					const TEResult& GetTextureTransferResult = Params.GetTextureTransferResult;
					if (GetTextureTransferResult == TEResultSuccess)
					{
						Exporter->ScheduleWaitFence(AcquireSemaphore, AcquireValue);
					}
					else if (GetTextureTransferResult == TEResultNoMatchingEntity) // TE does not have ownership
					{
					}
					else
					{
						UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("bIsInUseByTouchEngine: %s  bHasTextureTransfer: %s  GetTextureTransferResult: %s"),
						       bIsInUseByTouchEngine ? TEXT("TRUE") : TEXT("FALSE"), bHasTextureTransfer ? TEXT("TRUE") : TEXT("FALSE"),
						       GetTextureTransferResult == TEResultSuccess ? TEXT("SUCCESS") : TEXT("FAIL"));
						UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("Failed to transfer ownership of pooled texture back from Touch Engine"));
					}
					TERelease(&AcquireSemaphore);
					AcquireSemaphore = nullptr;
					// }
					// else
					// {
					// 	UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("bIsInUseByTouchEngine: %s  bHasTextureTransfer: %s"),
					// 		bIsInUseByTouchEngine ? TEXT("TRUE") : TEXT("FALSE"), bHasTextureTransfer ? TEXT("TRUE") : TEXT("FALSE"));
					// 	UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("Texture was transferred to TouchEngine at least once, is no longer in use, but TouchEngine refuses to transfer it back"));
					// }
				}

				// 2. 
				FRHITexture2D* SourceRHI = Exporter->GetRHIFromTexture(Params.Texture);
				RHICmdList.CopyTexture(SourceRHI, TextureData->GetSharedTextureRHI(), FRHICopyTextureInfo());
				RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
				// 3. Transfer to TE
				const uint64 WaitValue = Exporter->IncrementAndSignalFence();
				const TEResult TransferResult = TEInstanceAddTextureTransfer(Params.Instance, TextureData->GetTouchRepresentation(), Exporter->FenceTE, WaitValue);
				if (TransferResult != TEResultSuccess)
				{
					UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("TEInstanceAddTextureTransfer error code: %d"), static_cast<int32>(TransferResult));
					Promise.SetValue(FTouchExportResult{ETouchExportErrorCode::FailedTextureTransfer});
				}
				else
				{
					Promise.SetValue(FTouchExportResult{ETouchExportErrorCode::Success, TextureData->GetTouchRepresentation()});
				}
			}
		});
		
		OutTexture = TouchTexture;
		return Future;
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
		ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetCommandQueue();
		++NextFenceValue;
		NativeCmdQ->Signal(FenceNative.Get(), NextFenceValue);
		return NextFenceValue;
	}
}

#undef CHECK_HR_DEFAULT