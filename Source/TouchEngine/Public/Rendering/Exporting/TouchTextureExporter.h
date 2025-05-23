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
#include "Util/TaskSuspender.h"

class FRHICommandListImmediate;
class FRHICommandList;
class FRHICommandListBase;
class UTexture;

namespace UE::TouchEngine
{
	class FTouchResourceProvider;
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
				CachedInputTextures.IsEmpty(),
				TEXT("ReleaseTextures was either not called or did not clean up the exported textures correctly.")
			);
		}
		void Initialize(const TSharedRef<FTouchResourceProvider>& Provider)
		{
			WeakProvider = Provider;
		}

		TFuture<TouchObject<TETexture>> ExportTextureToTouchEngine_AnyThread(const FTouchExportParameters& ParamsConst);
		
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
			FTextureData(const TSharedRef<FExportedTouchTexture>& InExportedPlatformTexture)
				: ExportedPlatformTexture(InExportedPlatformTexture)
			{}
			
			FString DebugName;
			TSharedRef<FExportedTouchTexture> ExportedPlatformTexture;

			bool CanBeReused() const 
			{
				return ExportedPlatformTexture->IsCreatedOnRenderThread() &&
					!ExportedPlatformTexture->IsInUseByDynVars() &&
					!ExportedPlatformTexture->IsUsedInCurrentCook() &&
					(!ExportedPlatformTexture->WasEverUsedByTouchEngine() || !ExportedPlatformTexture->IsInUseByTouchEngine());
			}
		};
		
	public:
		int32 PoolSize = 20;

		TSharedPtr<FExportedTouchTexture> GetOrCreateTexture(UTexture* InTexture);
		
		void TexturePoolMaintenance();

		/** Waits for TouchEngine to release the textures and then proceeds to destroy them. */
		TFuture<FTouchSuspendResult> ReleaseTextures();

		virtual bool ShareTexture_RenderThread(const FTouchExportParameters& ParamsConst) = 0;
		
		TFuture<TSharedPtr<FExportedTouchTexture>> EnqueueShareTexture(const FTouchExportParameters& ParamsConst);
		
	protected:
		virtual TSharedPtr<FExportedTouchTexture> CreateTexture(UTexture* InTexture) = 0;
		/** Handles the creation of the semaphore and the call to TEInstanceAddTextureTransfer for each RHI */
		virtual TEResult AddTETextureTransfer_RenderThread(const FTouchExportParameters& Params, const TSharedRef<FExportedTouchTexture>& Texture) = 0;
		/** Called at the end of ExportTexture_AnyThread */
		virtual void FinaliseExport_RenderThread(const FTouchExportParameters& Params, const TSharedRef<FExportedTouchTexture>& Texture) {};

	private:
		/**
		 * Create a texture and add it to the different internal pools 
		 * @param InTexture The texture we are trying to match
		 */
		TSharedPtr<FTextureData> CreatePooledTexture(UTexture* InTexture);

		/**
		 * Look in the texture pool for any texture that would match the size and pixel format as the export parameters.
		 * If found, the Texture is removed from the pool and cached for this parameter
		 * @param InTexture The texture we are trying to match
		 */
		TSharedPtr<FTextureData> FindSuitableTextureFromPool(UTexture* InTexture);

		/** Release the texture, ensuring it has been released by TouchEngine before we let it be destroyed */
		void ReleaseTexture(TSharedRef<FExportedTouchTexture>& Texture);

		/** list of input textures that are either used by DynVars and/or used are currently being cooked */
		TArray<TSharedRef<FTextureData>> CachedInputTextures;
		/** The Texture Pool of textures not yet available for reuse. Their availability will be checked in TexturePoolMaintenance and they will be moved to the Texture Pool once ready */
		TArray<TSharedRef<FTextureData>> FutureTexturesToPool;
		/** The pool of available textures to be reused. Managed and trimmed in TexturePoolMaintenance */
		TArray<TSharedRef<FTextureData>> TexturePool;

		/** Tracks the tasks of releasing textures. */
		FTaskSuspender PendingTextureReleases;
		
		FCriticalSection PooledTextureMutex;
	};
}
