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
#include "ITouchImportTexture.h"

#include "Util/TaskSuspender.h"

#include "Async/TaskGraphInterfaces.h"

class FRHICommandListImmediate;
class FRHICommandList;
class FRHICommandListBase;
class UTexture2D;

namespace UE::TouchEngine
{
	class ITouchImportTexture;
	struct FTouchTextureImportResult;
	struct FTouchSuspendResult;
	class FTouchFrameCooker;
	
	struct FTouchTextureLinkData
	{
		/** Whether a task is currently in progress */
		bool bIsInProgress;
		
		UTexture* UnrealTexture;
	};
	
	/** Util for importing a TouchEngine texture into a UTexture2D */
	class TOUCHENGINE_API FTouchTextureImporter : public TSharedFromThis<FTouchTextureImporter>
	{
	public:

		virtual ~FTouchTextureImporter();

		/** @return A future that executes once the UTexture2D has been updated (if successful) */
		TFuture<FTouchTextureImportResult> ImportTexture_AnyThread(const FTouchImportParameters& LinkParams, const TSharedPtr<FTouchFrameCooker>& FrameCooker);

		/** Prevents further async tasks from being enqueued, cancels running tasks where possible, and executes the future once all tasks are done. */
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks();
		bool IsSuspended() const { return TaskSuspender.IsSuspended(); }
		
		static bool CanCopyIntoUTexture(const FTextureMetaData& Source, const UTexture* Target);

		/** The maximum size of the Importing texture pool */
		int32 PoolSize = 10;
		/**
		 * Ensure the number of available textures in the pool is less than the PoolSize.
		 * We could have more textures in the pool than the PoolSize as we are not removing textures recently added to the pool.
		 */
		void TexturePoolMaintenance(const FTouchEngineInputFrameData& FrameData);

		/**
		 * Remove a UTexture from the pool, so its lifetime will not be managed by the Importer anymore. Returns true if the Texture was found and the operation successful.
		 */
		bool RemoveUTextureFromPool(UTexture2D* Texture);
		
		void PrepareForNewCook(const FTouchEngineInputFrameData& FrameData)
		{
			RemoveUnusedAliveTextures();
		}
	
	protected:

		/** Acquires the shared texture (possibly waiting) and creates a platform texture from it. */
		virtual TSharedPtr<ITouchImportTexture> CreatePlatformTexture_RenderThread(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture) = 0;

		/** Fill the size and picture format of the received TE Texture. Does NOT require a wait on the texture usage */
		virtual FTextureMetaData GetTextureMetaData(const TouchObject<TETexture>& Texture) const = 0;
		
		/** Subclasses can use this when the enqueue more rendering tasks on which must be waited when SuspendAsyncTasks is called. */
		FTaskSuspender::FTaskTracker StartRenderThreadTask() { return TaskSuspender.StartTask(); }

		/** Initiates a texture transfer by calling the appropriate TEInstanceGetTextureTransfer */
		virtual FTouchTextureTransfer GetTextureTransfer(const FTouchImportParameters& ImportParams);
		
		virtual void CopyNativeToUnreal_RenderThread(const TSharedPtr<ITouchImportTexture>& TETexture, const FTouchCopyTextureArgs& CopyArgs);

		void RemoveUnusedAliveTextures();
	private:
		
		/** Tracks running tasks and helps us execute an event when all tasks are done (once they've been suspended). */
		FTaskSuspender TaskSuspender;
		FCriticalSection LinkDataMutex;
		TMap<FName, FTouchTextureLinkData> LinkData;

		struct FImportedTexturePoolData
		{
			/** The FrameID at which the texture was added to the pool. To avoid issues with the texture being possibly still in use,
			 * a texture can only be reused on a frame after they have been added to the pool */
			int64 PooledFrameID;
			TObjectPtr<UTexture2D> UETexture;
		};
		FCriticalSection TexturePoolMutex;
		/** The texture pool itself, keeping hold of the temporary UTexture created to reuse them when an import is needed, saving the need to go back to GameThread to create a new one */
		TArray<FImportedTexturePoolData> TexturePool;

		FCriticalSection KeepTexturesAliveMutex;
		/** Array of textures to keep alive while we are copying them */
		TArray<TPair<TSharedPtr<ITouchImportTexture>, FTextureRHIRef>> KeepTexturesAliveForCopy;
		
		/**
		 * @brief 
		 * @param Promise The Promise to return when the UTexture is ready
		 * @param LinkParams 
		 * @param FrameCooker 
		 */
		void ExecuteLinkTextureRequest_AnyThread(TPromise<FTouchTextureImportResult>&& Promise, const FTouchImportParameters& LinkParams, const TSharedPtr<FTouchFrameCooker>& FrameCooker);
		
		UTexture2D* GetOrCreateUTextureMatchingMetaData(const FTextureMetaData& TETextureMetadata, const FTouchImportParameters& LinkParams, bool& bOutAccessRHIViaReferenceTexture);
		UTexture2D* FindPoolTextureMatchingMetadata(const FTextureMetaData& TETextureMetadata, const FTouchEngineInputFrameData& FrameData);
	};
}


