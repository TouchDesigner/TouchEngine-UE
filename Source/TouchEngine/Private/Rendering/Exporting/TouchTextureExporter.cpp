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
#include "Logging.h"
#include "RenderingThread.h"
#include "TextureResource.h"
#include "Engine/TEDebug.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Rendering/TouchResourceProvider.h"
#include "Rendering/Exporting/TouchExportParams.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/TouchHelpers.h"

namespace UE::TouchEngine
{
	TFuture<TouchObject<TETexture>> FTouchTextureExporter::ExportTextureToTouchEngine_AnyThread(const FTouchExportParameters& ParamsConst)
	{
		if (TaskSuspender.IsSuspended())
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("[ExportTextureToTouchEngine_AnyThread[%s]] FTouchTextureExporter is suspended. Your task will be ignored."), *GetCurrentThreadStr());
			return MakeFulfilledPromise<TouchObject<TETexture>>(nullptr).GetFuture();
		}

		ParamsConst.TextureToBeExported->bIsUsedInCurrentCook = true;

		TPromise<TouchObject<TETexture>> Promise;
		TFuture<TouchObject<TETexture>> Future = Promise.GetFuture();

		// 1. We get a Texture to copy onto
		EnqueueShareTexture(ParamsConst).Next([Promise = MoveTemp(Promise), ParamsConst, WeakThis = AsWeak()]
			(TSharedPtr<FExportedTouchTexture> ExportedTexture) mutable
		{
			// We are now supposed to be in render thread. It is technically possible that this runs in GameThread
			// if the ENQUEUE_RENDER_COMMAND was processed before the .Next, in which case the Future would be already set.
			// We know though that the values we need would be set on whichever thread we are on at this point

			const TSharedPtr<FTouchTextureExporter> This = WeakThis.Pin();
			if (!This)
			{
				Promise.SetValue(nullptr);
				return;
			}
			UE_LOG(LogTouchEngine, Verbose, TEXT("[ExportTextureToTE_AnyThread[%s]] EnqueueShareTexture(ParamsConst).Next => returned texture '%s' for input '%s' on frame %lld"), *GetCurrentThreadStr(), *ExportedTexture->DebugName, *ParamsConst.ParameterName.ToString(), ParamsConst.FrameData.FrameID)

			if (!ExportedTexture)
			{
				UE_LOG(LogTouchEngine, Error, TEXT("[ExportTextureToTE_AnyThread[%s]] Unable to share the Texture. %s"), *GetCurrentThreadStr(), *ParamsConst.GetDebugDescription());
				Promise.SetValue(nullptr);
				return;
			}

			UE_LOG(LogTouchEngine, Log, TEXT("[ExportTextureToTE_AnyThread[%s]] GetOrCreateTexture returned the texture '%s'. %s"),
				*GetCurrentThreadStr(),
				*ExportedTexture->DebugName, *ParamsConst.GetDebugDescription());

			const TouchObject<TETexture>& TouchTexture = ExportedTexture->GetTouchRepresentation_RenderThread();
			check(TouchTexture);

			const FTouchExportParameters Params{ParamsConst};
			{
				// Add a texture transfer
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    I.B.3 [GT] Cook Frame - AddTextureTransfer"), STAT_TE_I_B_3, STATGROUP_TouchEngine);
				const TEResult TransferResult = This->AddTETextureTransfer_RenderThread(Params, ExportedTexture.ToSharedRef());
				if (TransferResult != TEResultSuccess)
				{
					UE_LOG(LogTouchEngineTECalls, Error, TEXT("[ExportTextureToTE_AnyThread[%s]] TEInstanceAddTextureTransfer `%s` returned `%s`. %s"), *GetCurrentThreadStr(), *ExportedTexture->DebugName, *TEResultToString(TransferResult), *Params.GetDebugDescription());
					Promise.SetValue(nullptr);
					return;
				}
			}

			This->FinaliseExport_RenderThread(Params, ExportedTexture.ToSharedRef());

			// Finally return the texture that will be passed to TEInstanceLinkSetTextureValue in FTouchVariableManager::SetTOPInput
			Promise.SetValue(TouchTexture);
		});

		return Future;
	}

	TSharedPtr<FExportedTouchTexture> FTouchTextureExporter::GetOrCreateTexture(UTexture* InTexture)
	{
		if (!IsValid(InTexture))
		{
			return nullptr;
		}
			
		FScopeLock Lock(&PooledTextureMutex);

		TSharedPtr<FExportedTouchTexture> ExportedPlatformTexture;
		UE_LOG(LogTouchEngine, Verbose, TEXT("[TExportedTouchTextureCache::GetOrCreateTexture] Overall Pool Size: %d   Pool: %d   Cached: %d   Future: %d"),
			TexturePool.Num() + CachedInputTextures.Num() + FutureTexturesToPool.Num(), TexturePool.Num(), CachedInputTextures.Num(), FutureTexturesToPool.Num());

		bool bIsNewTexture = false;
		// If we have an existing pool, try to get it from there
		if (TSharedPtr<FTextureData> TextureData = FindSuitableTextureFromPool(InTexture))
		{
			check(!TextureData->ExportedPlatformTexture->IsInUseByTouchEngine())
			ExportedPlatformTexture = TextureData->ExportedPlatformTexture;
		}
		else
		{
			// Otherwise, we just create a new one
			ExportedPlatformTexture = CreatePooledTexture(InTexture)->ExportedPlatformTexture;
			bIsNewTexture = true;
		}

		if (!ExportedPlatformTexture)
		{
			UE_LOG(LogTouchEngine, Error, TEXT("[TExportedTouchTextureCache::GetOrCreateTexture] Unable to get or create a pooled texture for `%s`"), *InTexture->GetFullName());
			return nullptr;
		}

		UE_LOG(LogTouchEngine, Verbose, TEXT("[TExportedTouchTextureCache::GetOrCreateTexture] for texture `%s` returned %s pool texture '%s'"), *InTexture->GetFullName(), bIsNewTexture ? TEXT("NEW") : TEXT("EXISTING"), *ExportedPlatformTexture->DebugName);
		ExportedPlatformTexture->EnqueueTextureCopy(InTexture);

		return ExportedPlatformTexture;
	}

	void FTouchTextureExporter::TexturePoolMaintenance()
	{
		TArray<TSharedRef<FTextureData>> TexturesToPool;
		TArray<TSharedRef<FTextureData>> TexturesToWait;
		TArray<TSharedRef<FTextureData>> TexturesToRelease;

		// 1.  we check the CachedInputTextureData which holds input textures
		for (auto It = CachedInputTextures.CreateIterator(); It; ++It)
		{
			TSharedRef<FTextureData>& TextureData = *It;
			if (TextureData->ExportedPlatformTexture->IsInUseByDynVars() || TextureData->ExportedPlatformTexture->IsUsedInCurrentCook())
			{
				continue;
			}
			
			if (TextureData->ExportedPlatformTexture->WasEverUsedByTouchEngine())
			{
				if (const TSharedPtr<FTouchResourceProvider> Provider = WeakProvider.Pin())
				{
					TextureData->ExportedPlatformTexture->GetTextureBackFromTE(Provider->GetInstance());
				}
			}
			
			if (!TextureData->CanBeReused())
			{
				TexturesToWait.AddUnique(TextureData); // if it is still in use, we cannot reuse it right away.
			}
			else
			{
				TexturesToPool.AddUnique(TextureData); // otherwise we'll add it to the pool
			}
			It.RemoveCurrent();
		}

		// 2. Then we ensure our pool is healthy
		for (auto It = TexturePool.CreateIterator(); It; ++It)
		{
			TSharedRef<FTextureData>& TextureData = *It;
			if (!ensure(TextureData->CanBeReused()))
			{
				CachedInputTextures.AddUnique(TextureData);
				It.RemoveCurrent();
			}
		}

		// 3. Check if the textures are still in use by TouchEngine and add the new ones
		for (auto It = FutureTexturesToPool.CreateIterator(); It; ++It)
		{
			TSharedRef<FTextureData>& TextureData = *It;
			if (TextureData->CanBeReused()) // if freed up, we can add it to the Pool
			{
				TexturesToPool.AddUnique(TextureData);
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
		for (const TSharedRef<FTextureData>& TextureData : TexturesToRelease)
		{
			ReleaseTexture(TextureData->ExportedPlatformTexture);
		}

		SET_DWORD_STAT(STAT_TE_ExportedTexturePool_NbTexturesPool, TexturePool.Num())
	}

	TFuture<FTouchSuspendResult> FTouchTextureExporter::ReleaseTextures()
	{
		FScopeLock Lock(&PooledTextureMutex);
			
		for (TSharedRef<FTextureData>& TextureData : CachedInputTextures)
		{
			ReleaseTexture(TextureData->ExportedPlatformTexture);
		}
		CachedInputTextures.Empty();
		check(CachedInputTextures.IsEmpty());

		for (const TSharedRef<FTextureData>& TextureData : FutureTexturesToPool)
		{
			ReleaseTexture(TextureData->ExportedPlatformTexture);
		}
		FutureTexturesToPool.Empty();
		check(FutureTexturesToPool.IsEmpty());
			
		for (const TSharedRef<FTextureData>& TextureData : TexturePool)
		{
			ReleaseTexture(TextureData->ExportedPlatformTexture);
		}
		TexturePool.Empty();
		check(TexturePool.IsEmpty());
			
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
		// Once all the texture clean-ups are done, we can tell whoever is waiting that the rendering resources have been cleared up.
		// From this point forward we're ready to be destroyed.
		PendingTextureReleases.Suspend().Next([Promise = MoveTemp(Promise)](auto) mutable
		{
			Promise.SetValue({});
		});
		return Future;
	}

	TFuture<TSharedPtr<FExportedTouchTexture>> FTouchTextureExporter::EnqueueShareTexture(const FTouchExportParameters& ParamsConst)
	{
		TPromise<TSharedPtr<FExportedTouchTexture>> Promise;
		TFuture<TSharedPtr<FExportedTouchTexture>> Future = Promise.GetFuture();
		
		ENQUEUE_RENDER_COMMAND(ShareExportedTexture)([Promise = MoveTemp(Promise), WeakThis = AsWeak(), ParamsConst](FRHICommandListImmediate& RHICmdList) mutable
		{
			const TSharedPtr<FTouchTextureExporter> This = WeakThis.Pin();
			if (!This)
			{
				Promise.SetValue(nullptr);
				return;
			}

			const TSharedPtr<FTouchResourceProvider> Provider = This->GetWeakProvider().Pin();
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
			
			if (!Provider->CanExportPixelFormat(*ParamsConst.Instance.get(), ParamsConst.TextureToBeExported->GetSharedTextureRHI_RenderThread()->GetFormat()))
			{
				UE_LOG(LogTouchEngine, Error, TEXT("EPixelFormat `%s` is not supported for export to TouchEngine. %s"), GetPixelFormatString(ParamsConst.TextureToBeExported->GetSharedTextureRHI_RenderThread()->GetFormat()), *ParamsConst.TextureToBeExported->DebugName);
				Promise.SetValue(nullptr);
				return;
			}
			
			UE_LOG(LogTouchEngine, Verbose, TEXT("[EnqueueShareTexture[%s]] ShareExportedTexture => about to share texture '%s' for input '%s' on frame %lld"), *GetCurrentThreadStr(), *ParamsConst.TextureToBeExported->DebugName, *ParamsConst.ParameterName.ToString(), ParamsConst.FrameData.FrameID)
			
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

	TSharedPtr<FTouchTextureExporter::FTextureData> FTouchTextureExporter::CreatePooledTexture(UTexture* InTexture)
	{
		check(InTexture)

		UE_LOG(LogTemp, Verbose, TEXT("[TExportedTouchTextureCache::ShareTexture] for texture `%s`"), *InTexture->GetName());

		const TSharedPtr<FExportedTouchTexture> ExportedTexture = CreateTexture(InTexture);
		if (!ensure(ExportedTexture))
		{
			return nullptr;
		}

		ExportedTexture->DebugName = FString::Printf(TEXT("%s__%s"), *GetNameSafe(InTexture), *FDateTime::Now().ToIso8601());
		TSharedRef<FTextureData> NewTextureData = MakeShared<FTextureData>(ExportedTexture.ToSharedRef());
		NewTextureData->DebugName = ExportedTexture->DebugName;

		CachedInputTextures.Add(NewTextureData);
		INC_DWORD_STAT(STAT_TE_ExportedTexturePool_NbTexturesTotal)

		return NewTextureData;
	}

	TSharedPtr<FTouchTextureExporter::FTextureData> FTouchTextureExporter::FindSuitableTextureFromPool(UTexture* InTexture)
	{
		for (auto It = TexturePool.CreateIterator(); It; ++It)
		{
			TSharedRef<FTextureData>& TextureData = *It;
			if (ensure(TextureData->CanBeReused()) && TextureData->ExportedPlatformTexture->CanFitTexture(InTexture))
			{
				UE_LOG(LogTouchEngine, Verbose, TEXT("[TExportedTouchTextureCache::FindSuitableTextureFromPool] reusing pooled texture '%s' for UTexture `%s`"), *TextureData->ExportedPlatformTexture->DebugName, *InTexture->GetFullName());

				TextureData->DebugName = TextureData->ExportedPlatformTexture->DebugName;
				TSharedPtr<FTextureData> SuitableTextureFromPool = TextureData;

				CachedInputTextures.Add(TextureData);
				It.RemoveCurrent();
				return SuitableTextureFromPool;
			}
		}
		return nullptr;
	}

	void FTouchTextureExporter::ReleaseTexture(TSharedRef<FExportedTouchTexture>& Texture)
	{
		// This will keep the Texture valid for as long as TE is using the texture, which is why we pass it to the lambda capture
		Texture->Release()
			.Next([this, Texture, TaskToken = PendingTextureReleases.StartTask()](auto)
			{
				UE_LOG(LogTouchEngine, Verbose, TEXT("[ReleaseTexture] Done Releasing texture `%s`"), *Texture->DebugName)
				DEC_DWORD_STAT(STAT_TE_ExportedTexturePool_NbTexturesTotal)
			});
	}
}
