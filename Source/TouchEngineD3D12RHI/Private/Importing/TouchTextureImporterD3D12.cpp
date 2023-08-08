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

#include "TouchTextureImporterD3D12.h"

#include "D3D12TouchUtils.h"
#include "ID3D12DynamicRHI.h"
#include "TouchImportTextureD3D12.h"
#include "TouchEngine/TED3D.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Logging.h"
#include "Exporting/TextureShareD3D12PlatformWindows.h"

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
	FTouchTextureImporterD3D12::FTouchTextureImporterD3D12(ID3D12Device* Device, TSharedRef<FTouchFenceCache> FenceCache)
		: Device(Device)
		, FenceCache(MoveTemp(FenceCache))
	{
		constexpr D3D12_COMMAND_LIST_TYPE QueueType = D3D12_COMMAND_LIST_TYPE_COPY;
		ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
		ID3D12CommandQueue* DefaultCommandQueue = RHI->RHIGetCommandQueue();
		
		// inspired by FD3D12Queue::FD3D12Queue
		D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
		CommandQueueDesc.Type = QueueType; // GetD3DCommandListType((ED3D12QueueType)QueueType);
		CommandQueueDesc.Priority = 0;
		CommandQueueDesc.NodeMask = DefaultCommandQueue->GetDesc().NodeMask;;
		CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		CHECK_HR_DEFAULT(Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(D3DCommandQueue.GetInitReference())));
		CHECK_HR_DEFAULT(D3DCommandQueue->SetName(*FString::Printf(TEXT("FTouchTextureImporterD3D12 (GPU %d)"), CommandQueueDesc.NodeMask)))

		CommandQueueFence = FenceCache->GetOrCreateOwnedFence_AnyThread(true);
	}

	FTouchTextureImporterD3D12::~FTouchTextureImporterD3D12()
	{
		D3DCommandQueue = nullptr; // this should release the command queue
		CommandQueueFence.Reset(); // this will be destroyed or reused by the FenceCache.
	}

	// void FTouchTextureImporterD3D12::CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef SrcTexture, const FTexture2DRHIRef DstTexture)
	// {
	// 	TWeakPtr<FTouchTextureImporter> ThisWeak = AsWeak();
	// 	// RHICmdList.EnqueueLambda([ThisWeak, SrcTexture, DstTexture, D3DCommandQueue = D3DCommandQueue, CommandQueueFence = CommandQueueFence](FRHICommandListBase& CmdList)
	// 	// {
	// 		TSharedPtr<FTouchTextureImporter> PinThis = ThisWeak.Pin();
	// 		if (!PinThis)
	// 		{
	// 			return;
	// 		}
	// 		
	// 		const uint64 WaitValue = CommandQueueFence->LastValue + 1;
	// 		TRefCountPtr<ID3D12GraphicsCommandList>  CopyCommandList;
	// 		TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;
	// 		const FRHIGPUMask GPUMask = RHICmdList.GetGPUMask(); //  CmdList.GetGPUMask();
	// 		// TRefCountPtr<ID3D12CommandQueue> D3DCommandQueue;
	// 		// std::vector<ID3D12CommandList*> ppCommandLists;
	//
	// 		ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();// static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
	// 		check(RHI);
	// 		const uint32 GPUIndex = GPUMask.ToIndex();
	// 		ID3D12Device* D3DDevice = RHI->RHIGetDevice(GPUIndex); // Just reusing the current device to be sure
	// 		const D3D12_COMMAND_LIST_TYPE QueueType = D3DCommandQueue->GetDesc().Type; // D3D12_COMMAND_LIST_TYPE_COPY; // Context->GraphicsCommandList()->GetType();
	// 		
	// 		{
	// 			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("RHI Import Copy - 1. Create Command List"), STAT_RHIExportCopyCommandList, STATGROUP_TouchEngine);
	// 			
	// 			// inspired by FD3D12CommandList::FD3D12CommandList
	// 			CHECK_HR_DEFAULT(D3DDevice->CreateCommandAllocator(QueueType, IID_PPV_ARGS(CommandAllocator.GetInitReference())));
	// 			CHECK_HR_DEFAULT(D3DDevice->CreateCommandList(GPUMask.GetNative(), QueueType, CommandAllocator, nullptr, IID_PPV_ARGS(CopyCommandList.GetInitReference())));
	//
	// 			ID3D12Resource* Source = static_cast<ID3D12Resource*>(SrcTexture->GetNativeResource());
	// 			ID3D12Resource* Destination = static_cast<ID3D12Resource*>(DstTexture->GetNativeResource());
	//
	// 			CopyCommandList->CopyResource(Destination, Source);;
	// 			
	// 			CopyCommandList->Close();
	// 		}
	//
	// 		const HANDLE mFenceEventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
	// 		CommandQueueFence->NativeFence->SetEventOnCompletion(WaitValue, mFenceEventHandle);
	// 		
	// 		// UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("   [FRHICopyFromUnrealToVulkanAndSignalFence[%s]] About to execute CommandList, fence `%s` current value is `%llu` (GetCompletedValue(): `%lld`) for frame `%lld`"),
	// 		// 	*GetCurrentThreadStr(), *CommandQueueFence->DebugName, CommandQueueFence->LastValue, CommandQueueFence->NativeFence->GetCompletedValue(), FrameID)
	// 		{
	// 			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("RHI Import Copy - 2. Execute Command List"), STAT_RHIExportCopyExecuteCommandList, STATGROUP_TouchEngine);
	// 			ID3D12CommandList* ppCommandLists[] = { CopyCommandList };
	// 			D3DCommandQueue->ExecuteCommandLists(1, ppCommandLists);
	// 		}
	// 		
	// 		{
	// 			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("RHI Import Copy - 3. Wait"), STAT_RHIExportCopyWait, STATGROUP_TouchEngine);
	// 			D3DCommandQueue->Signal(CommandQueueFence->NativeFence.Get(), WaitValue); // asynchronous call
	// 			// D3DCommandQueue->Wait(CommandQueueFence->NativeFence.Get(), WaitValue); // this just add a wait at the end of the command queue, but does not actually waits for it
	//
	// 			WaitForSingleObjectEx(mFenceEventHandle, INFINITE, false);
	// 			CloseHandle(mFenceEventHandle);
	// 		
	// 			CommandQueueFence->LastValue = WaitValue;
	// 			// UE_LOG(LogTouchEngineD3D12RHI, Log, TEXT("   [FRHICopyFromUnrealToVulkanAndSignalFence[%s]] CommandList executed and fence `%s` signalled with value `%llu` (GetCompletedValue(): `%lld`) for frame `%lld`"),
	// 			// 	*GetCurrentThreadStr(), *Fence->DebugName, Fence->LastValue, Fence->NativeFence->GetCompletedValue(), FrameID)
	// 		}
	// 		
	// 		CopyCommandList.SafeRelease();
	// 		CommandAllocator.SafeRelease();
	// 	// });
	// }

	TSharedPtr<ITouchImportTexture> FTouchTextureImporterD3D12::CreatePlatformTexture_RenderThread(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture)
	{
		check(TETextureGetType(SharedTexture) == TETextureTypeD3DShared);
		TED3DSharedTexture* Shared = static_cast<TED3DSharedTexture*>(SharedTexture.get());
		const HANDLE Handle = TED3DSharedTextureGetHandle(Shared);
		if (const TSharedPtr<FTouchImportTextureD3D12> Existing = GetSharedTexture(Handle))
		{
			return Existing;
		}

		const TSharedPtr<FTouchImportTextureD3D12> NewTexture = FTouchImportTextureD3D12::CreateTexture_RenderThread(Device, Shared, FenceCache);
		if (!NewTexture)
		{
			return nullptr;
		}

		TED3DSharedTextureSetCallback(Shared, TextureCallback, this);
		CachedTextures.Add(Handle, NewTexture.ToSharedRef());

		return StaticCastSharedPtr<ITouchImportTexture>(NewTexture);
	}

	FTextureMetaData FTouchTextureImporterD3D12::GetTextureMetaData(const TouchObject<TETexture>& Texture) const
	{
		const TED3DSharedTexture* Source = static_cast<TED3DSharedTexture*>(Texture.get());
		const DXGI_FORMAT Format = TED3DSharedTextureGetFormat(Source);
		FTextureMetaData Result;
		Result.SizeX = TED3DSharedTextureGetWidth(Source);
		Result.SizeY = TED3DSharedTextureGetHeight(Source);
		Result.PixelFormat = ConvertD3FormatToPixelFormat(Format, Result.IsSRGB);
		return Result;
	}

	void FTouchTextureImporterD3D12::CopyNativeToUnreal_RenderThread(const TSharedPtr<ITouchImportTexture>& TETexture, const FTouchCopyTextureArgs& CopyArgs)
	{
		FTouchTextureImporter::CopyNativeToUnreal_RenderThread(TETexture, CopyArgs);
	}

	TSharedPtr<FTouchImportTextureD3D12> FTouchTextureImporterD3D12::GetSharedTexture(HANDLE Handle) const
	{
		const TSharedRef<FTouchImportTextureD3D12>* Result = CachedTextures.Find(Handle);
		return Result
			? *Result
			: TSharedPtr<FTouchImportTextureD3D12>{ nullptr };
	}

	void FTouchTextureImporterD3D12::TextureCallback(HANDLE Handle, TEObjectEvent Event, void* Info)
	{
		if (Event == TEObjectEventRelease)
		{
			FTouchTextureImporterD3D12* This = static_cast<FTouchTextureImporterD3D12*>(Info);
			This->CachedTextures.Remove(Handle);
		}
	}
}

