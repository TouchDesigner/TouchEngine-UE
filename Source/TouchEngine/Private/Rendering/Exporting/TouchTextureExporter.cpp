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

#include "Rendering/Exporting/TouchTextureExporter.h"
#include "RenderingThread.h"
#include "TextureResource.h"

#include "Logging.h"
#include "Rendering/Exporting/TouchExportParams.h"

#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Rendering/TouchResourceProvider.h"
#include "Util/TouchHelpers.h"

namespace UE::TouchEngine
{
	TFuture<TouchObject<TETexture>> FTouchTextureExporter::ExportTextureToTouchEngine_AnyThread(const FTouchExportParameters& Params)
	{
		if (TaskSuspender.IsSuspended())
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("[ExportTextureToTouchEngine_AnyThread[%s]] FTouchTextureExporter is suspended. Your task will be ignored."), *GetCurrentThreadStr());
			return MakeFulfilledPromise<TouchObject<TETexture>>(nullptr).GetFuture();
		}
		
		check(Params.TextureToBeExported)
		
		return ExportTextureToTE_AnyThread(Params);
	}

	void FTouchTextureExporter::TexturePoolMaintenance()
	{
		TArray<TSharedPtr<FTextureData>> TexturesToPool;
		TArray<TSharedPtr<FTextureData>> TexturesToWait;
		TArray<TSharedPtr<FTextureData>> TexturesToRelease;

		// todo: cached texture data and future texture pool have a similar usage, should they be combined?
		// 1.  we check the CachedTextureData which holds the recent textures exported
		for (auto It = CachedTextureData.CreateIterator(); It; ++It)
		{
			TSharedPtr<FTextureData>& TextureData = *It;
			if (!ensure(TextureData || TextureData->ExportedPlatformTexture)) // if it is null for some reason
			{
				TexturesToRelease.AddUnique(TextureData);
			}
			else if (TextureData->ExportedPlatformTexture->IsInUseByDynVars())
			{
				continue;
			}
			else if (TextureData->ExportedPlatformTexture->WasEverUsedByTouchEngine())
			{
				// if (const TSharedPtr<FTouchResourceProvider> Provider = WeakProvider.Pin()) //todo: should this be done here instead of TEObjectEventEndUse?
				// {
				// 	TextureData->ExportedPlatformTexture->GetTextureBackFromTE(Provider->GetInstance());
				// }
				if (!TextureData->CanBeReused())
				{
					TexturesToWait.AddUnique(TextureData); // if it is still in use, we cannot reuse it right away.
				}
				else
				{
					TexturesToPool.AddUnique(TextureData); // otherwise we'll add it to the pool
				}
			}
			else
			{
				TexturesToPool.AddUnique(TextureData);
			}
			It.RemoveCurrent();
		}

		// 2. Then we ensure our pool is healthy
		for (auto It = TexturePool.CreateIterator(); It; ++It)
		{
			TSharedPtr<FTextureData>& TextureData = *It;
			if (!ensure(TextureData && TextureData->ExportedPlatformTexture))
			{
				if (!ensure(TextureData->CanBeReused()))
				{
					CachedTextureData.AddUnique(TextureData);
				}
				else
				{
					TexturesToRelease.AddUnique(TextureData);
				}
				It.RemoveCurrent();
			}
		}

		// 3. Check if the textures are still in use by TouchEngine and add the new ones
		for (auto It = FutureTexturesToPool.CreateIterator(); It; ++It)
		{
			TSharedPtr<FTextureData>& TextureData = *It;
			if (ensure(TextureData && TextureData->ExportedPlatformTexture))
			{
				if (TextureData->CanBeReused()) // if freed up, we can add it to the Pool
				{
					TexturesToPool.AddUnique(TextureData);
					It.RemoveCurrent();
				}
			}
			else
			{
				TexturesToRelease.AddUnique(TextureData);
				It.RemoveCurrent();
			}
		}
		FutureTexturesToPool.Append(TexturesToWait);

		// 4. We add to the pool the ones that can be added and we ensure the pool is not too big
		TexturePool.Append(TexturesToPool);
		while (TexturePool.Num() > PoolSize)
		{
			TexturesToRelease.AddUnique(TexturePool[0]); // we remove from the front as they have been here the longest
			TexturePool.RemoveAt(0);
		}

		// 5. And finally we release the textures
		for (const TSharedPtr<FTextureData>& TextureData : TexturesToRelease)
		{
			ReleaseTexture(TextureData->ExportedPlatformTexture);
		}

		SET_DWORD_STAT(STAT_TE_ExportedTexturePool_NbTexturesPool, TexturePool.Num())
	}
	
	TFuture<TSharedPtr<FExportedTouchTexture>> FTouchTextureExporter::EnqueueShareTexture(const FTouchExportParameters& ParamsConst)
	{
		TPromise<TSharedPtr<FExportedTouchTexture>> Promise;
		TFuture<TSharedPtr<FExportedTouchTexture>> Future = Promise.GetFuture();
		
		ENQUEUE_RENDER_COMMAND(ShareExportedTexture)([Promise = MoveTemp(Promise), WeakThis = AsWeak(), ParamsConst](FRHICommandListImmediate& RHICmdList) mutable
		{
			TSharedPtr<FTouchTextureExporter> This = WeakThis.Pin();
			if (!This || !ParamsConst.TextureToBeExported)
			{
				Promise.SetValue(nullptr);
				return;
			}

			TSharedPtr<FTouchResourceProvider> Provider = This->GetWeakProvider().Pin();
			if (!Provider)
			{
				Promise.SetValue(nullptr);
				return;
			}
			
			if (!ParamsConst.TextureToBeExported->IsCreatedOnRenderThread())
			{
				UE_LOG(LogTouchEngine, Error, TEXT("RHI has not yet been created for '%s'"), *ParamsConst.TextureToBeExported->DebugName);
				Promise.SetValue(nullptr);
				return;
			}
			
			if (! Provider->CanExportPixelFormat(*ParamsConst.Instance.get(), ParamsConst.TextureToBeExported->GetSharedTextureRHI_RenderThread()->GetFormat()))
			{
				UE_LOG(LogTouchEngine, Error, TEXT("EPixelFormat `%s` is not supported for export to TouchEngine. %s"), GetPixelFormatString(ParamsConst.TextureToBeExported->GetSharedTextureRHI_RenderThread()->GetFormat()), *ParamsConst.TextureToBeExported->DebugName);
				Promise.SetValue(nullptr);
				return;
			}
			
			UE_LOG(LogTouchEngine, Warning, TEXT("[EnqueueShareTexture[%s]] ShareExportedTexture => about to share texture '%s' for input '%s' on frame %lld"), *GetCurrentThreadStr(), *ParamsConst.TextureToBeExported->DebugName, *ParamsConst.ParameterName.ToString(), ParamsConst.FrameData.FrameID)
			
			if (This->ShareTexture_RenderThread(ParamsConst))
			{
				Promise.SetValue(ParamsConst.TextureToBeExported);
			}
			else
			{
				Promise.SetValue(nullptr);
			}
		});

		return Future;
	}
}
