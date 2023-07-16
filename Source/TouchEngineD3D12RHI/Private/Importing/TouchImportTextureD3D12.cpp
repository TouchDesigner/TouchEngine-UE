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
#include "TouchEngine/TED3D.h"
#include "Util/TouchEngineStatsGroup.h"

namespace UE::TouchEngine::D3DX12
{
	TSharedPtr<FTouchImportTextureD3D12> FTouchImportTextureD3D12::CreateTexture_RenderThread(ID3D12Device* Device, TED3DSharedTexture* Shared, TSharedRef<FTouchFenceCache> FenceCache)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      III.A.2.a [RT] Link Texture Import - CreateTexture"), STAT_TE_III_A_2_a_D3D, STATGROUP_TouchEngine);
		HANDLE Handle = TED3DSharedTextureGetHandle(Shared);
		check(TED3DSharedTextureGetHandleType(Shared) == TED3DHandleTypeD3D12ResourceNT);
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("        III.A.2.a.1 [RT] Link Texture Import - CreateTexture - OpenSharedHandle"), STAT_TE_III_A_2_a_1_D3D, STATGROUP_TouchEngine);
			HRESULT SharedHandleResult = Device->OpenSharedHandle(Handle, IID_PPV_ARGS(&Resource)); //todo: this takes a lot of time, and when is this closed?
			if (FAILED(SharedHandleResult))
			{
				return nullptr;
			}
		}

		FTexture2DRHIRef SrcRHI;
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("        III.A.2.a.2 [RT] Link Texture Import - CreateTexture - RHICreateTexture2DFromResource"), STAT_TE_III_A_2_a_2_D3D, STATGROUP_TouchEngine);
			const EPixelFormat Format = ConvertD3FormatToPixelFormat(Resource->GetDesc().Format);
			ID3D12DynamicRHI* DynamicRHI = static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
			SrcRHI = DynamicRHI->RHICreateTexture2DFromResource(Format, TexCreate_Shared, FClearValueBinding::None, Resource.Get()).GetReference(); //todo: look at using this function when creating the Texture2D from scratch
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
		FTexture2DRHIRef TextureRHI,
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
		D3D12_RESOURCE_DESC TextureDesc = SourceResource->GetDesc();
		FTextureMetaData Result;
		Result.SizeX = TextureDesc.Width;
		Result.SizeY = TextureDesc.Height;
		Result.PixelFormat = ConvertD3FormatToPixelFormat(TextureDesc.Format);
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

	void FTouchImportTextureD3D12::ReleaseMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue)
	{
		const ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
		ID3D12CommandQueue* NativeCmdQ = RHI->RHIGetCommandQueue();

		const uint64 ReleaseValue = WaitValue + 1;
		NativeCmdQ->Signal(ReleaseMutexSemaphore->NativeFence.Get(), ReleaseValue);
		TEInstanceAddTextureTransfer(CopyArgs.RequestParams.Instance, CopyArgs.RequestParams.TETexture.get(), ReleaseMutexSemaphore->TouchFence, ReleaseValue);
	}

	void FTouchImportTextureD3D12::CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef SrcTexture, const FTexture2DRHIRef DstTexture, TSharedRef<FTouchTextureImporter> Importer)
	{
		// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("RHI Import Copy"), STAT_RHIImportCopy, STATGROUP_TouchEngine);
		check(SrcTexture.IsValid() && DstTexture.IsValid());
		check(SrcTexture->GetFormat() == DstTexture->GetFormat());
		
		// TSharedRef<FTouchTextureImporterD3D12> D3D12Importer = StaticCastSharedRef<FTouchTextureImporterD3D12>(Importer);
		// D3D12Importer->CopyTexture_RenderThread(RHICmdList, SrcTexture, DstTexture);
		// return;
		
		RHICmdList.CopyTexture(SrcTexture, DstTexture, FRHICopyTextureInfo());
	}
}
