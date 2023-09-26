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

#include "Importing/TouchImportTextureD3D12.h"

#include "D3D12TouchUtils.h"
#include "ID3D12DynamicRHI.h"
#include "Logging.h"
#include "TouchTextureImporterD3D12.h"
#include "Tasks/Task.h"
#include "TouchEngine/TED3D.h"
#include "Util/TouchEngineStatsGroup.h"

namespace UE::TouchEngine::D3DX12
{
	TSharedPtr<FTouchImportTextureD3D12> FTouchImportTextureD3D12::CreateTexture_RenderThread(ID3D12Device* Device, const TED3DSharedTexture* Shared, TSharedRef<FTouchFenceCache> FenceCache)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      III.A.2.a [RT] Link Texture Import - CreateTexture"), STAT_TE_III_A_2_a_D3D, STATGROUP_TouchEngine);
		const HANDLE Handle = TED3DSharedTextureGetHandle(Shared);
		check(TED3DSharedTextureGetHandleType(Shared) == TED3DHandleTypeD3D12ResourceNT);
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("        III.A.2.a.1 [RT] Link Texture Import - CreateTexture - OpenSharedHandle"), STAT_TE_III_A_2_a_1_D3D, STATGROUP_TouchEngine);
			const HRESULT SharedHandleResult = Device->OpenSharedHandle(Handle, IID_PPV_ARGS(&Resource));
			if (FAILED(SharedHandleResult))
			{
				return nullptr;
			}
		}

		FTexture2DRHIRef SrcRHI;
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("        III.A.2.a.2 [RT] Link Texture Import - CreateTexture - RHICreateTexture2DFromResource"), STAT_TE_III_A_2_a_2_D3D, STATGROUP_TouchEngine);
			bool IsSRGB;
			const EPixelFormat Format = ConvertD3FormatToPixelFormat(Resource->GetDesc().Format, IsSRGB);
			ID3D12DynamicRHI* DynamicRHI = static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
			ETextureCreateFlags Flags = TexCreate_Shared;
			if (IsSRGB)
			{
				Flags |= ETextureCreateFlags::SRGB;
			}
			SrcRHI = DynamicRHI->RHICreateTexture2DFromResource(Format, Flags, FClearValueBinding::None, Resource.Get()).GetReference();
		}
		
		const TSharedPtr<FTouchFenceCache::FFenceData> ReleaseMutexSemaphore = FenceCache->GetOrCreateOwnedFence_AnyThread();
		if (!ReleaseMutexSemaphore)
		{
			return nullptr;
		}

		return MakeShared<FTouchImportTextureD3D12>(
			SrcRHI,
			MoveTemp(Resource),
			MoveTemp(FenceCache),
			ReleaseMutexSemaphore.ToSharedRef()
		);
	}

	FTouchImportTextureD3D12::FTouchImportTextureD3D12(
		const FTexture2DRHIRef& TextureRHI,
		Microsoft::WRL::ComPtr<ID3D12Resource> SourceResource,
		TSharedRef<FTouchFenceCache> FenceCache,
		TSharedRef<FTouchFenceCache::FFenceData> ReleaseMutexSemaphore
		)
		: DestTextureRHI(TextureRHI)
		, SourceResource(MoveTemp(SourceResource))
		, FenceCache(MoveTemp(FenceCache))
		, ReleaseMutexSemaphore(MoveTemp(ReleaseMutexSemaphore))
	{}

	FTextureMetaData FTouchImportTextureD3D12::GetTextureMetaData() const
	{
		const D3D12_RESOURCE_DESC TextureDesc = SourceResource->GetDesc();
		FTextureMetaData Result;
		Result.SizeX = TextureDesc.Width;
		Result.SizeY = TextureDesc.Height;
		Result.PixelFormat = ConvertD3FormatToPixelFormat(TextureDesc.Format, Result.IsSRGB);
		return Result;
	}

	bool FTouchImportTextureD3D12::AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
		if (const TComPtr<ID3D12Fence> Fence = FenceCache->GetOrCreateSharedFence(Semaphore))
		{
			const ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
			ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetCommandQueue();
			NativeCmdQ->Wait(Fence.Get(), WaitValue);
			return true;
		}

		UE_LOG(LogTouchEngineD3D12RHI, Warning, TEXT("FTouchPlatformTextureD3D12: Failed to wait on ID3D12Fence"));
		return false;
	}

	void FTouchImportTextureD3D12::ReleaseMutex_RenderThread(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue, FTexture2DRHIRef& SourceTexture)
	{
		const uint64 SignalValue = WaitValue + 1;
		//todo: implement a similar system than FTouchTextureExporterD3D12 to keep hold of references
		
		CopyArgs.RHICmdList.EnqueueLambda([CopyArgs, Fence = ReleaseMutexSemaphore.ToSharedPtr(), SignalValue, SourceTexture, DestTexture = CopyArgs.TargetRHI](FRHICommandListImmediate& RHICommandList)
		{
			ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
			if (Fence && Fence->NativeFence.Get() && RHI)
			{
				UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("ReleaseMutex_RenderThread  => NativeFence Valid? %s , Address: %p, WaitValue: %llu"), Fence->NativeFence.Get() ? TEXT("Non Null") : TEXT("NULL"), Fence->NativeFence.GetAddressOf(), Fence->LastValue+1);
				RHI->RHISignalManualFence(RHICommandList, Fence->NativeFence.Get(), SignalValue);
				TEInstanceAddTextureTransfer(CopyArgs.RequestParams.Instance, CopyArgs.RequestParams.TETexture.get(), Fence->TouchFence, SignalValue);

				UE::Tasks::Launch(UE_SOURCE_LOCATION, [Fence, SignalValue]()
				{
					FPlatformProcess::ConditionalSleep([Fence, SignalValue]()
					{
						return Fence && Fence->NativeFence.Get() && Fence->NativeFence->GetCompletedValue() > SignalValue;
					}, 0.1f);
				}, LowLevelTasks::ETaskPriority::BackgroundLow);
			}
		});
		
	}

	void FTouchImportTextureD3D12::CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef SrcTexture, const FTexture2DRHIRef DstTexture, TSharedRef<FTouchTextureImporter> Importer)
	{
		check(SrcTexture.IsValid() && DstTexture.IsValid());
		check(SrcTexture->GetFormat() == DstTexture->GetFormat());
		
		RHICmdList.Transition(FRHITransitionInfo(SrcTexture, ERHIAccess::Unknown, ERHIAccess::CopySrc));
		RHICmdList.Transition(FRHITransitionInfo(DstTexture, ERHIAccess::Unknown, ERHIAccess::CopyDest));
		RHICmdList.CopyTexture(SrcTexture, DstTexture, FRHICopyTextureInfo());
		RHICmdList.Transition(FRHITransitionInfo(DstTexture, ERHIAccess::CopyDest, ERHIAccess::SRVMask));
		RHICmdList.EnqueueLambda([SrcTexture, DstTexture](FRHICommandListImmediate& RHICommandList)
		{
			//to keep SrcTexture and DstTexture Alive for longer
		});
	}
}
