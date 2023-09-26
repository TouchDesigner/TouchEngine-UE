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
#include "TextureResource.h"
#include "Logging.h"
#include "TouchEngine/TEInstance.h"

namespace UE::TouchEngine
{
	ECopyTouchToUnrealResult FTouchImportTexture_AcquireOnRenderThread::CopyNativeToUnrealRHI_RenderThread(const FTouchCopyTextureArgs& CopyArgs, TSharedRef<FTouchTextureImporter> Importer)
	{
		if (CopyArgs.RequestParams.TETexture)
		{
			if (CopyArgs.RequestParams.TETextureTransfer.Result != TEResultSuccess && CopyArgs.RequestParams.TETextureTransfer.Result != TEResultNoMatchingEntity) // TEResultNoMatchingEntity would mean that we would already have ownership
			{
				return ECopyTouchToUnrealResult::Failure;
			}

			if (CopyArgs.RequestParams.TETextureTransfer.Result == TEResultSuccess)
			{
				const bool bSuccess = AcquireMutex(CopyArgs, CopyArgs.RequestParams.TETextureTransfer.Semaphore, CopyArgs.RequestParams.TETextureTransfer.WaitValue);
				if (!bSuccess)
				{
					return ECopyTouchToUnrealResult::Failure;
				}
			}

			FTexture2DRHIRef SourceTexture = ReadTextureDuringMutex();
			if (SourceTexture)
			{
				check(CopyArgs.TargetRHI);
				const FTexture2DRHIRef DestTexture = CopyArgs.TargetRHI;
				CopyTexture_RenderThread(CopyArgs.RHICmdList, SourceTexture, DestTexture, Importer);
			}
			else
			{
				UE_LOG(LogTouchEngine, Warning, TEXT("Failed to copy texture to Unreal."))
			}

			if (CopyArgs.RequestParams.TETextureTransfer.Result == TEResultSuccess)
			{
				ReleaseMutex_RenderThread(CopyArgs, CopyArgs.RequestParams.TETextureTransfer.Semaphore, CopyArgs.RequestParams.TETextureTransfer.WaitValue, SourceTexture);
			}
			
			return ECopyTouchToUnrealResult::Success;
		}

		return ECopyTouchToUnrealResult::Failure;
	}
}
