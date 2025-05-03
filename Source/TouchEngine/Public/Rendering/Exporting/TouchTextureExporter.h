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

#pragma once

#include "CoreMinimal.h"
#include "ExportedTouchTexture.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchExportParams.h"
#include "TouchTextureExporter.h"
#include "Async/Future.h"
#include "Engine/TEDebug.h"
#include "TouchEngine/TouchObject.h"
#include "TouchEngine/Public/Logging.h"
#include "Util/TaskSuspender.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/TouchHelpers.h"

class FRHICommandListImmediate;
class FRHICommandList;
class FRHICommandListBase;
class UTexture;

namespace UE::TouchEngine
{
	class FTouchVariableManager;
	struct FTouchExportResult;
	struct FTouchExportParameters;
	struct FTouchSuspendResult;

	/** Util for exporting textures from Unreal to TouchEngine */
	class TOUCHENGINE_API FTouchTextureExporter : public TSharedFromThis<FTouchTextureExporter>
	{
	public:
		virtual ~FTouchTextureExporter()
		{
			checkf(
				CachedTextureData.IsEmpty(),
				TEXT("ReleaseTextures was either not called or did not clean up the exported textures correctly.")
			);
		}
		void Initialize(const TSharedRef<FTouchResourceProvider>& Provider)
		{
			WeakProvider = Provider;
		}

		TFuture<TouchObject<TETexture>> ExportTextureToTouchEngine_AnyThread(const FTouchExportParameters& Params);
		
		/** Prevents further async tasks from being enqueued, cancels running tasks where possible, and executes the future once all tasks are done. */
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks() { return TaskSuspender.Suspend(); }
		virtual FTaskSuspender::FTaskTracker StartAsyncTask() { return TaskSuspender.StartTask(); }
		bool IsSuspended() const { return TaskSuspender.IsSuspended(); }
		
		virtual void InitializeExportsToTouchEngine_GameThread(const FTouchEngineInputFrameData& FrameData) {};
		virtual void FinalizeExportsToTouchEngine_AnyThread(const FTouchEngineInputFrameData& FrameData) {};

		const TWeakPtr<FTouchResourceProvider>& GetWeakProvider() { return WeakProvider; }
	private:

		/** Tracks running tasks and helps us execute an event when all tasks are done (once they've been suspended). */
		FTaskSuspender TaskSuspender;
		TWeakPtr<FTouchResourceProvider> WeakProvider;
		
	public: // Texture Cache
		struct FTextureData
		{
			FString DebugName;
			TSharedPtr<FExportedTouchTexture> ExportedPlatformTexture;

			bool CanBeReused() const 
			{
				return ExportedPlatformTexture && ExportedPlatformTexture->IsCreatedOnRenderThread() && !ExportedPlatformTexture->IsInUseByDynVars() &&
					(!ExportedPlatformTexture->WasEverUsedByTouchEngine() || !ExportedPlatformTexture->IsInUseByTouchEngine());
			}
		};
		
	public:
		int32 PoolSize = 20;

		TSharedPtr<FExportedTouchTexture> GetOrCreateTexture(UTexture* InTexture)
		{
			if (!IsValid(InTexture))
			{
				return nullptr;
			}
			
			FScopeLock Lock(&PooledTextureMutex);

			TSharedPtr<FExportedTouchTexture> ExportedPlatformTexture;
			UE_LOG(LogTouchEngine, Verbose, TEXT("[TExportedTouchTextureCache::GetOrCreateTexture] Overall Pool Size: %d   Pool: %d   Cached: %d   Future: %d"),
				TexturePool.Num() + CachedTextureData.Num() + FutureTexturesToPool.Num(), TexturePool.Num(), CachedTextureData.Num(), FutureTexturesToPool.Num());
			// 4. if we have an existing pool, try to get it from there
			if (TSharedPtr<FTextureData> TextureData = FindSuitableTextureFromPool(InTexture))
			{
				check(!TextureData->ExportedPlatformTexture->IsInUseByTouchEngine())
				ExportedPlatformTexture = TextureData->ExportedPlatformTexture;
			}
			else
			{
				//5. Otherwise, we just create a new one
				ExportedPlatformTexture = CreatePooledTexture(InTexture)->ExportedPlatformTexture; 
			}

			if (!ExportedPlatformTexture)
			{
				UE_LOG(LogTouchEngine, Error, TEXT("[TExportedTouchTextureCache::GetOrCreateTexture] Unable to get or create a pooled texture for `%s`"), *InTexture->GetFullName());
				return nullptr;
			}

			UE_LOG(LogTouchEngine, Verbose, TEXT("[TExportedTouchTextureCache::GetOrCreateTexture] for texture `%s` returned pool texture '%s'"), *InTexture->GetFullName(), *ExportedPlatformTexture->DebugName);
			ExportedPlatformTexture->EnqueueTextureCopy(InTexture, AsShared());

			return ExportedPlatformTexture;
		}
		
		void TexturePoolMaintenance()
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
			for (TSharedPtr<FTextureData>& TextureData : TexturesToRelease)
			{
				ReleaseTexture(TextureData->ExportedPlatformTexture);
			}

			SET_DWORD_STAT(STAT_TE_ExportedTexturePool_NbTexturesPool, TexturePool.Num())
		}

		/** Waits for TouchEngine to release the textures and then proceeds to destroy them. */
		TFuture<FTouchSuspendResult> ReleaseTextures()
		{
			FScopeLock Lock(&PooledTextureMutex);
			
			for (const TSharedPtr<FTextureData>& TextureData : CachedTextureData)
			{
				ReleaseTexture(TextureData->ExportedPlatformTexture);
				TextureData->ExportedPlatformTexture.Reset();
			}
			CachedTextureData.Empty();
			check(CachedTextureData.IsEmpty());

			for (const TSharedPtr<FTextureData>& TextureData : FutureTexturesToPool)
			{
				ReleaseTexture(TextureData->ExportedPlatformTexture);
				TextureData->ExportedPlatformTexture.Reset();
			}
			FutureTexturesToPool.Empty();
			check(FutureTexturesToPool.IsEmpty());
			
			for (const TSharedPtr<FTextureData>& TextureData : TexturePool)
			{
				ReleaseTexture(TextureData->ExportedPlatformTexture);
				TextureData->ExportedPlatformTexture.Reset();
			}
			TexturePool.Empty();
			check(TexturePool.IsEmpty());
			
			TPromise<FTouchSuspendResult> Promise;
			TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
			// Once all the texture clean-ups are done, we can tell whomever is waiting that the rendering resources have been cleared up.
			// From this point forward we're ready to be destroyed.
			PendingTextureReleases.Suspend().Next([Promise = MoveTemp(Promise)](auto) mutable
			{
				Promise.SetValue({});
			});
			return Future;
		}

		virtual bool ShareTexture_RenderThread(const FTouchExportParameters& ParamsConst) = 0;
		
		TFuture<TSharedPtr<FExportedTouchTexture>> EnqueueShareTexture(const FTouchExportParameters& ParamsConst);
		/** Exports the given texture to TouchEngine. Called by FTouchTextureExporter::ExportTextureToTouchEngine_AnyThread */
		TFuture<TouchObject<TETexture>> ExportTextureToTE_AnyThread(const FTouchExportParameters& ParamsConst)
		{
			check(ParamsConst.TextureToBeExported)

			TPromise<TouchObject<TETexture>> Promise;
			TFuture<TouchObject<TETexture>> Future = Promise.GetFuture();

			// 1. We get a Texture to copy onto
			EnqueueShareTexture(ParamsConst).Next([Promise = MoveTemp(Promise), ParamsConst, WeakThis = AsWeak()](TSharedPtr<FExportedTouchTexture> ExportedTexture) mutable
			{
				// We are now supposed to be in render thread. It is technically possible that this runs in GameThread
				// if the ENQUEUE_RENDER_COMMAND was processed before the .Next, in which case the Future would be already set.
				// We know though that the values we need would be set on whichever thread we are on at this point
				
				TSharedPtr<FTouchTextureExporter> This = WeakThis.Pin();
				if (!This)
				{
					Promise.SetValue(nullptr);
					return;
				}
				UE_LOG(LogTouchEngine, Warning, TEXT("[ExportTextureToTE_AnyThread[%s]] EnqueueShareTexture(ParamsConst).Next => returned texture '%s' for input '%s' on frame %lld"), *GetCurrentThreadStr(), *ExportedTexture->DebugName, *ParamsConst.ParameterName.ToString(), ParamsConst.FrameData.FrameID)
				
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
				
				// 2.b ...Otherwise, if this is not a new texture, transfer ownership if needed
				FTouchExportParameters Params{ParamsConst};

				// 3. Add a texture transfer
				{
					DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    I.B.3 [GT] Cook Frame - AddTextureTransfer"), STAT_TE_I_B_3, STATGROUP_TouchEngine);
					const TEResult TransferResult = This->AddTETextureTransfer_RenderThread(Params, ExportedTexture);
					UE_CLOG(TransferResult == TEResultSuccess, LogTouchEngineTECalls, Log, TEXT("[ExportTextureToTE_AnyThread[%s]] TEInstanceAddTextureTransfer `%s` returned `%s`. %s"), *GetCurrentThreadStr(), *ExportedTexture->DebugName, *TEResultToString(TransferResult), *Params.GetDebugDescription());
					if (TransferResult != TEResultSuccess)
					{
						UE_LOG(LogTouchEngineTECalls, Error, TEXT("[ExportTextureToTE_AnyThread[%s]] TEInstanceAddTextureTransfer `%s` returned `%s`. %s"), *GetCurrentThreadStr(), *ExportedTexture->DebugName, *TEResultToString(TransferResult), *Params.GetDebugDescription());
						Promise.SetValue(nullptr);
						return;
					}
				}

				// 4. Finalise the export and enqueue the copy of the texture on RenderThread
				This->FinaliseExport_RenderThread(Params, ExportedTexture);
				
				// 5. Finally return the texture that will be passed to TEInstanceLinkSetTextureValue in FTouchVariableManager::SetTOPInput
				Promise.SetValue(TouchTexture);
			});

			return Future;
		}
		
	protected:
		virtual TSharedPtr<FExportedTouchTexture> CreateTexture(UTexture* InTexture) = 0;
		/** Handles the creation of the semaphore and the call to TEInstanceAddTextureTransfer for each RHI */
		virtual TEResult AddTETextureTransfer_RenderThread(const FTouchExportParameters& Params, const TSharedPtr<FExportedTouchTexture>& Texture) = 0;
		/** Called at the end of ExportTexture_AnyThread once the texture is ready to be copied into */
		virtual void FinaliseExport_RenderThread(const FTouchExportParameters& Params, TSharedPtr<FExportedTouchTexture>& Texture) = 0;

	private:
		/**
		 * Create a texture and add it to the different internal pools 
		 * @param Params The export parameters that this texture needs to match
		 */
		TSharedPtr<FTextureData> CreatePooledTexture(UTexture* InTexture)
		{
			check(InTexture)
			
			UE_LOG(LogTemp, Verbose, TEXT("[TExportedTouchTextureCache::ShareTexture] for texture `%s`"), *InTexture->GetName());

			TSharedPtr<FExportedTouchTexture> ExportedTexture = CreateTexture(InTexture);
			if (!ensure(ExportedTexture))
			{
				return nullptr;
			}
			
			INC_DWORD_STAT(STAT_TE_ExportedTexturePool_NbTexturesTotal)
			ExportedTexture->DebugName = FString::Printf(TEXT("%s__%s"), *GetNameSafe(InTexture), *FDateTime::Now().ToIso8601());
			TSharedPtr<FTextureData> NewTextureData = MakeShared<FTextureData>();
			NewTextureData->ExportedPlatformTexture = ExportedTexture;
			NewTextureData->DebugName = ExportedTexture->DebugName;

			CachedTextureData.Add(NewTextureData);
			
			return NewTextureData;
		}

		/**
		 * Look in the texture pool for any texture that would match the size and pixel format as the export parameters.
		 * If found, the Texture is removed from the pool and cached for this parameter
		 * @param Params The export parameters that this texture needs to match
		 * @param ParamTextureRHI The stable RHI of Params.Texture that we previously retrieved, used to get the size of the current available Mip that will be exported
		 */
		TSharedPtr<FTextureData> FindSuitableTextureFromPool(UTexture* InTexture)
		{
			TSharedPtr<FTextureData> SuitableTextureFromPool;
			
			for (TSharedPtr<FTextureData>& TextureData : TexturePool)
			{
				if (ensure(TextureData && TextureData->CanBeReused()) &&
					TextureData->ExportedPlatformTexture->CanFitTexture(InTexture))
				{
					TextureData->DebugName = TextureData->ExportedPlatformTexture->DebugName;

					CachedTextureData.Add(TextureData);
					SuitableTextureFromPool = TextureData;
					TexturePool.Remove(SuitableTextureFromPool);
					UE_LOG(LogTouchEngine, Verbose, TEXT("[TExportedTouchTextureCache::FindSuitableTextureFromPool] reusing pooled texture '%s' for UTexture `%s`"), *TextureData->ExportedPlatformTexture->DebugName, *InTexture->GetFullName());
					break;
				}
			}
			return SuitableTextureFromPool;
		}

		/** Release the texture, ensuring it has been released by TouchEngine before we let it be destroyed */
		void ReleaseTexture(TSharedPtr<FExportedTouchTexture>& Texture)
		{
			// This will keep the Texture valid for as long as TE is using the texture, which is why we pass it to the lambda capture
			if (Texture)
			{
				Texture->Release()
					.Next([this, Texture, TaskToken = PendingTextureReleases.StartTask()](auto)
					{
						UE_LOG(LogTouchEngine, Verbose, TEXT("[ReleaseTexture] Done Releasing texture `%s`"), *Texture->DebugName)
						DEC_DWORD_STAT(STAT_TE_ExportedTexturePool_NbTexturesTotal)
					});
			}
		}

		mutable FCriticalSection CachedTextureDataMutex; //todo: check if needed
		/** Associates UTexture objects with the resource shared with TE. */
		TArray<TSharedPtr<FTextureData>> CachedTextureData;

		/** The pool of available textures to be reused. Managed and trimmed in TexturePoolMaintenance */
		TArray<TSharedPtr<FTextureData>> TexturePool;
		/** The Texture Pool of textures not yet available for reuse. Their availability will be checked in TexturePoolMaintenance and they will be moved to the Texture Pool once ready */
		TArray<TSharedPtr<FTextureData>> FutureTexturesToPool;

		/** Tracks the tasks of releasing textures. */
		FTaskSuspender PendingTextureReleases;
		
		FCriticalSection PooledTextureMutex;  //todo: check if needed
	};
}
