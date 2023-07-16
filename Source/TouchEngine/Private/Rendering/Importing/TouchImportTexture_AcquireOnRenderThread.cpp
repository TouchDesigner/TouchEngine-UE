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

#include "Rendering/Importing/TouchImportTexture_AcquireOnRenderThread.h"

#include "Logging.h"
#include "TouchEngine/TEInstance.h"

namespace UE::TouchEngine
{
	TFuture<ECopyTouchToUnrealResult> FTouchImportTexture_AcquireOnRenderThread::CopyNativeToUnreal_RenderThread(const FTouchCopyTextureArgs& CopyArgs, TSharedRef<FTouchTextureImporter> Importer)
	{
		const TouchObject<TEInstance>& Instance = CopyArgs.RequestParams.Instance;
		const TouchObject<TETexture>& SharedTexture = CopyArgs.RequestParams.TETexture;
		
		if (SharedTexture ) // && TEInstanceHasTextureTransfer(Instance, SharedTexture))
		{
			//todo this could be updated to do all of this on an other command queue, like FTouchTextureExporterD3D12::FinalizeExportsToTouchEngine_AnyThread
			const TouchObject<TESemaphore>& Semaphore = CopyArgs.RequestParams.GetTextureTransferSemaphore; // todo: this works but is it the best way?
			const uint64& WaitValue = CopyArgs.RequestParams.GetTextureTransferWaitValue;
			// const TEResult ResultCode = TEInstanceGetTextureTransfer(Instance, SharedTexture, Semaphore.take(), &WaitValue);
			// CopyArgs.RequestParams.GetTextureTransferResult = TEInstanceGetTextureTransfer(Instance, SharedTexture, Semaphore.take(), &WaitValue);
			const TEResult& ResultCode =  CopyArgs.RequestParams.GetTextureTransferResult;
			if (ResultCode != TEResultSuccess && ResultCode != TEResultNoMatchingEntity) // TEResultNoMatchingEntity would mean that we would already have ownership
			{
				return MakeFulfilledPromise<ECopyTouchToUnrealResult>(ECopyTouchToUnrealResult::Failure).GetFuture();
			}
			
			const bool bSuccess = AcquireMutex(CopyArgs, Semaphore, WaitValue);
			if (!bSuccess)
			{
				return MakeFulfilledPromise<ECopyTouchToUnrealResult>(ECopyTouchToUnrealResult::Failure).GetFuture();
			}

			if (const FTexture2DRHIRef SourceTexture = ReadTextureDuringMutex())
			{
				check(CopyArgs.Target);
				const FTexture2DRHIRef& DestTexture{CopyArgs.Target->TextureReference.TextureReferenceRHI}; //CopyArgs.Target->GetResource()->TextureRHI->GetTexture2D();
				CopyTexture_RenderThread(CopyArgs.RHICmdList, SourceTexture, DestTexture, Importer);
			}
			else
			{
				UE_LOG(LogTouchEngine, Warning, TEXT("Failed to copy texture to Unreal."))
			}
			
			ReleaseMutex(CopyArgs, Semaphore, WaitValue);
			return MakeFulfilledPromise<ECopyTouchToUnrealResult>(ECopyTouchToUnrealResult::Success).GetFuture();
		}

		return MakeFulfilledPromise<ECopyTouchToUnrealResult>(ECopyTouchToUnrealResult::Failure).GetFuture();
	}

	ECopyTouchToUnrealResult FTouchImportTexture_AcquireOnRenderThread::CopyNativeToUnrealRHI_RenderThread(const FTouchCopyTextureArgs& CopyArgs, TSharedRef<FTouchTextureImporter> Importer)
	{
		const TouchObject<TEInstance>& Instance = CopyArgs.RequestParams.Instance;
		
		if (CopyArgs.RequestParams.TETexture)
		{
			//todo this could be updated to do all of this on an other command queue, like FTouchTextureExporterD3D12::FinalizeExportsToTouchEngine_AnyThread
			const TouchObject<TESemaphore>& Semaphore = CopyArgs.RequestParams.GetTextureTransferSemaphore; // todo: this works but is it the best way?
			const uint64& WaitValue = CopyArgs.RequestParams.GetTextureTransferWaitValue;
			const TEResult& ResultCode =  CopyArgs.RequestParams.GetTextureTransferResult;
			if (ResultCode != TEResultSuccess && ResultCode != TEResultNoMatchingEntity) // TEResultNoMatchingEntity would mean that we would already have ownership
			{
				return ECopyTouchToUnrealResult::Failure;
			}
			
			const bool bSuccess = AcquireMutex(CopyArgs, Semaphore, WaitValue);
			if (!bSuccess)
			{
				return ECopyTouchToUnrealResult::Failure;
			}

			if (const FTexture2DRHIRef SourceTexture = ReadTextureDuringMutex())
			{
				check(CopyArgs.TargetRHI);
				const FTexture2DRHIRef DestTexture = CopyArgs.TargetRHI;
				CopyTexture_RenderThread(CopyArgs.RHICmdList, SourceTexture, DestTexture, Importer);
			}
			else
			{
				UE_LOG(LogTouchEngine, Warning, TEXT("Failed to copy texture to Unreal."))
			}
			
			ReleaseMutex(CopyArgs, Semaphore, WaitValue);
			return ECopyTouchToUnrealResult::Success;
		}

		return ECopyTouchToUnrealResult::Failure;
	}
}
