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
#include "TouchExportParams.h"
#include "TouchTextureExporter.h"
#include "Engine/TEDebug.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/TouchHelpers.h"
#include "TouchEngine/Public/Logging.h"


namespace UE::TouchEngine
{
	/**
	 * Keeps track of textures that are being or have been exported from Unreal to TouchEngine.
	 * 
	 * The intended use case is setting input textures. This class handles the lifecycles of shared textures:
	 * after the shared texture are no longer needed, they can only be released after TE has stopped using them.
	 *
	 * Subclasses must implement:
	 *  - TSharedPtr<TExportedTouchTexture> CreateTexture(const FTextureCreationArgs& Params)
	 */
	template<typename TExportedTouchTexture, typename TCrtp>
	class TExportedTouchTextureCache
	{
		TCrtp* This() { return static_cast<TCrtp*>(this); }
		const TCrtp* This() const { return static_cast<const TCrtp*>(this); }
		
		struct FTextureData
		{
			FString DebugName;
			UTexture* UETexture;
			TSharedPtr<TExportedTouchTexture> ExportedPlatformTexture;
			TSet<FName> ParametersInUsage;

			bool IsExportedPlatformTextureHealthy()
			{
				return ExportedPlatformTexture && !ExportedPlatformTexture->ReceivedReleaseEvent();
			}
		};
		
	public:
		int32 PoolSize = 20; //todo: make this accessible from BP
		
		virtual ~TExportedTouchTextureCache()
		{
			checkf(
				CachedTextureData.IsEmpty(),
				TEXT("ReleaseTextures was either not called or did not clean up the exported textures correctly.")
			);
		}

		/**
		 * Returns a stable RHI for the given texture.
		 * In this case, stable means that it will not change between GameThread and RenderThread, which could otherwise be the case for a UTexture2D if not retrieved properly.
		 */
		static FTextureRHIRef GetStableRHIFromTexture(const UTexture* Texture)
		{
			check(Texture)
			
			// A Texture2D is actually composed of 2 RHI resources, one only accessible in GameThread, and another one only accessible in RenderThread.
			// The RHI is access via UTexture2D::GetResource() on one thread is not guaranteed to match on the other thread, especially as they might be retrieved at different times.
			// This is an issue with Texture Streaming as the size of the RHI retrieved on GameThread which we use to create a shared texture sometimes varies with the one we retrieve on
			// RenderThread where we try to enqueue the copy.
			// Instead of using UTexture2D::GetResource(), we retrieve and store a stable RHI via UTexture::TextureReference, and we make sure to return the referenced texture
			return FTextureRHIRef(Texture->TextureReference.TextureReferenceRHI->GetReferencedTexture());
		}
		
		/** Gets an existing texture, if it can still fit the FTouchExportParameters, or allocates a new one (deleting the old texture, if any, once TouchEngine is done with it). */
		TSharedPtr<TExportedTouchTexture> GetOrCreateTexture(const FTouchExportParameters& Params, bool& bIsNewTexture, bool& bTextureNeedsCopy)
		{
			check(Params.Texture)
			
			FScopeLock Lock(&PooledTextureMutex);
			UE_LOG(LogTemp, Verbose, TEXT("[TExportedTouchTextureCache::GetOrCreateTexture] for param `%s` and texture `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture));

			// // 1. we check if we have sent the same texture for the same parameter
			FTextureRHIRef ParamTextureRHI = GetStableRHIFromTexture(Params.Texture);
			
			// 2. check if we already have a TextureData for the given UTexture
			TSharedPtr<FTextureData>* FoundTextureData = CachedTextureData.Find(Params.Texture);
			
			// 3. If we do, just send it back.
			if (FoundTextureData)
			{
				if (TSharedPtr<FTextureData>& TextureData = *FoundTextureData)
				{
					if (ensure(TextureData->IsExportedPlatformTextureHealthy()) && TextureData->ExportedPlatformTexture->CanFitTexture(ParamTextureRHI))
					{
						check(TextureData->ExportedPlatformTexture)
						UE_LOG(LogTemp, Verbose, TEXT("[TExportedTouchTextureCache::GetNextOrAllocPooledTexture] Reusing existing texture for param `%s` and texture `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture));
						bTextureNeedsCopy = false;
						TextureData->ParametersInUsage.Add(Params.ParameterName);
						bIsNewTexture = false;
						TextureData->ExportedPlatformTexture->SetStableRHIOfTextureToCopy(MoveTemp(ParamTextureRHI));
						return TextureData->ExportedPlatformTexture;
					}
					else // We need to release it now as the CachedTextureData will be overriden for this UTexture
					{
						ForceReturnTextureToPool(TextureData->ExportedPlatformTexture, Params);
						// ReleaseTexture(TextureData->ExportedPlatformTexture);
						// CachedTextureData.Remove(Params.Texture);
					}
				}
			}

			bTextureNeedsCopy = true;
			
			// 4. if we have an existing pool, try to get it from there
			if (TSharedPtr<FTextureData> TextureData = FindSuitableTextureFromPool(Params, ParamTextureRHI))
			{
				bIsNewTexture = false;
				TextureData->ExportedPlatformTexture->SetStableRHIOfTextureToCopy(MoveTemp(ParamTextureRHI));
				return TextureData->ExportedPlatformTexture;
			}

			//5. Otherwise, we just create a new one
			bIsNewTexture = true;
			return ShareTexture(Params, MoveTemp(ParamTextureRHI))->ExportedPlatformTexture; 
		}

		void TexturePoolMaintenance()
		{
			// DECLARE_SCOPE_CYCLE_COUNTER(TEXT("I.B.2 [GT] Cook Frame - Pool Maintenance"), STAT_TE_I_B_2, STATGROUP_TouchEngine);

			TArray<TSharedPtr<FTextureData>> TexturesToPool;
			TArray<TSharedPtr<FTextureData>> TexturesToWait;
			TArray<TSharedPtr<FTextureData>> TexturesToRelease;

			// 1.  we check the CachedTextureData which holds the recent textures exported
			TArray<UTexture*> Keys;
			CachedTextureData.GenerateKeyArray(Keys);
			for (UTexture* Key : Keys)
			{
				if (!ensure(Key)) // if the key is null for some reason
				{
					TexturesToRelease.Add(CachedTextureData.FindAndRemoveChecked(Key));
					continue;
				}

				TSharedPtr<FTextureData>& TextureData = CachedTextureData[Key];
				if (ensure(TextureData && TextureData->IsExportedPlatformTextureHealthy()))
				{
					if (TextureData->ParametersInUsage.IsEmpty()) // if we processed all the parameters and none are using this texture
					{
						TextureData->UETexture = nullptr;
						if (TextureData->ExportedPlatformTexture->IsInUseByTouchEngine()) //todo: should the texture transfer actually move here?
						{
							TexturesToWait.Add(CachedTextureData.FindAndRemoveChecked(Key)); // if it is still in use, we cannot reuse it right away.
						}
						else
						{
							TexturesToPool.Add(CachedTextureData.FindAndRemoveChecked(Key)); // otherwise we'll add it to the pool
						}
					}
					else
					{
						TextureData->ParametersInUsage.Empty(); // will keep the texture alive one more frame to avoid deleting it if still in use
					}
				}
				else
				{
					// this is not supposed to happen, but now that we have a pool, lifetime of the texture could be different so to be sure
					TexturesToRelease.Add(CachedTextureData.FindAndRemoveChecked(Key));
				}
			}

			// 2. Then we ensure our pool is healthy
			for (int i = 0; i < TexturePool.Num(); ++i)
			{
				TSharedPtr<FTextureData>& TextureData = TexturePool[i];
				if (!ensure(TextureData && TextureData->IsExportedPlatformTextureHealthy()))
				{
					TexturesToRelease.Add(TextureData);
					TexturePool.RemoveAt(i);
					--i;
				}
			}

			// 3. Check if the textures are still in use by TouchEngine and add the new ones
			for (int i = 0; i < FutureTexturesToPool.Num(); ++i)
			{
				TSharedPtr<FTextureData>& TextureData = FutureTexturesToPool[i];
				if (ensure(TextureData && TextureData->IsExportedPlatformTextureHealthy()))
				{
					if (!TextureData->ExportedPlatformTexture->IsInUseByTouchEngine()) // if freed up, we can add it to the Pool
					{
						TexturesToPool.Add(TextureData);
						FutureTexturesToPool.RemoveAt(i);
						--i;
					}
				}
				else
				{
					TexturesToRelease.Add(TextureData);
					FutureTexturesToPool.RemoveAt(i);
					--i;
				}
			}
			FutureTexturesToPool.Append(TexturesToWait);
			
			// 4. We add to the pool the ones that can be added and we ensure the pool is not too big //todo: expose the pool size as a parameter
			TexturePool.Append(TexturesToPool);
			while (TexturePool.Num() > PoolSize)
			{
				TexturesToRelease.Add(TexturePool[0]); // we remove from the front as they have been here the longest
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
			
			for (auto& Pair : CachedTextureData)
			{
				TSharedPtr<FTextureData>& TextureData = Pair.Value; // CachedTextureData.FindAndRemoveChecked(TextureKey);
				ReleaseTexture(TextureData->ExportedPlatformTexture);
				TextureData->ExportedPlatformTexture.Reset();
			}
			CachedTextureData.Empty();
			check(CachedTextureData.IsEmpty());
			
			while(!FutureTexturesToPool.IsEmpty())
			{
				TSharedPtr<FTextureData> TextureData = FutureTexturesToPool.Pop();
				ReleaseTexture(TextureData->ExportedPlatformTexture);
				TextureData->ExportedPlatformTexture.Reset();
			}
			check(FutureTexturesToPool.IsEmpty());
			
			while(!TexturePool.IsEmpty())
			{
				TSharedPtr<FTextureData> TextureData = TexturePool.Pop();
				ReleaseTexture(TextureData->ExportedPlatformTexture);
				TextureData->ExportedPlatformTexture.Reset();
			}
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

		/** If something happened and the texture returned from GetOrCreateTexture did not get copied into, we need to ensure it is not reused and we add it back to the pool */
		void ForceReturnTextureToPool(TSharedPtr<TExportedTouchTexture>& ExportedTexture, const FTouchExportParameters& Params)
		{
			if (!ExportedTexture)
			{
				return;
			}
			
			if (TSharedPtr<FTextureData>* TextureDataPtr = CachedTextureData.Find(Params.Texture))
			{
				TSharedPtr<FTextureData>& TextureData = *TextureDataPtr;
				if (TextureData && TextureData->ExportedPlatformTexture == ExportedTexture)
				{
					TextureData->ParametersInUsage.Remove(Params.ParameterName);
					if (TextureData->ParametersInUsage.IsEmpty())
					{
						TextureData->ExportedPlatformTexture->ClearStableRHI();
						FutureTexturesToPool.Add(CachedTextureData.FindAndRemoveChecked(Params.Texture));
					}
				}
			}
		}

		/** Exports the given texture to TouchEngine. Called by FTouchTextureExporter::ExportTextureToTouchEngine_AnyThread */
		TouchObject<TETexture> ExportTextureToTE_AnyThread(const FTouchExportParameters& ParamsConst, TEGraphicsContext* GraphicContext)
		{
			check(ParamsConst.Texture)

			// 1. We get a Texture to copy onto
			bool bIsNewTexture, bTextureNeedsCopy;
			TSharedPtr<TExportedTouchTexture> ExportedTexture;
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    I.B.1 [GT] Cook Frame - GetOrCreateTexture"), STAT_TE_I_B_1, STATGROUP_TouchEngine);
				ExportedTexture = GetOrCreateTexture(ParamsConst, bIsNewTexture, bTextureNeedsCopy);
				if (!ExportedTexture)
				{
					UE_LOG(LogTouchEngine, Error, TEXT("[ExportTexture_AnyThread[%s]] Unable to Get or Create a Texture to export onto for parameter `%s` for frame %lld"), *UE::TouchEngine::GetCurrentThreadStr(), *ParamsConst.ParameterName.ToString(), ParamsConst.FrameData.FrameID);
					return nullptr;
				}
			}

			UE_LOG(LogTouchEngine, Log, TEXT("[ExportTexture_AnyThread[%s]] GetOrCreateTexture returned %s `%s` (%sneeding a copy) for parameter `%s` for frame %lld"),
			       *UE::TouchEngine::GetCurrentThreadStr(), bIsNewTexture ? TEXT("a NEW texture") : TEXT("the EXISTING texture"),
			       bTextureNeedsCopy ? TEXT("") : TEXT("NOT "),
			       *ExportedTexture->DebugName, *ParamsConst.ParameterName.ToString(), ParamsConst.FrameData.FrameID);

			const TouchObject<TETexture>& TouchTexture = ExportedTexture->GetTouchRepresentation();

			// 2.a If we don't need to copy because the copy is already enqueued by another parameter, return early...
			if (!bTextureNeedsCopy) // if the texture is already ready //todo: to test with video texture
			{
				UE_LOG(LogTouchEngine, Log, TEXT("[ExportTexture[%s]] Reusing existing texture for `%s` as it was already used by other parameters."),
					*GetCurrentThreadStr(), *ParamsConst.ParameterName.ToString());
				ExportedTexture->ClearStableRHI(); // Not needed as we are not copying
				return TouchTexture;
			}
			
			// 2.b ...Otherwise, if this is not a new texture, transfer ownership if needed
			FTouchExportParameters Params{ParamsConst};
			Params.GetTextureTransferSemaphore = nullptr;
			Params.GetTextureTransferWaitValue = 0;
			Params.GetTextureTransferResult = TEResultNoMatchingEntity;
			
			if (!bIsNewTexture && TEInstanceHasTextureTransfer(Params.Instance, TouchTexture)) // If this is a pre-existing texture
			{
				// Here we can use a regular TEInstanceGetTextureTransfer even for Vulkan because the contents of the texture can be discarded
				// as noted https://github.com/TouchDesigner/TouchEngine-Windows#vulkan
				Params.GetTextureTransferResult = TEInstanceGetTextureTransfer(Params.Instance, TouchTexture, Params.GetTextureTransferSemaphore.take(), &Params.GetTextureTransferWaitValue); // request an ownership transfer from TE to UE, will be processed below
				if (Params.GetTextureTransferResult != TEResultSuccess && Params.GetTextureTransferResult != TEResultNoMatchingEntity) //TEResultNoMatchingEntity would be raised if there is no texture transfer waiting
				{
					UE_LOG(LogTouchEngine, Error, TEXT("[ExportTexture] TEInstanceGetTextureTransfer returned `%s` for parameter `%s` for frame %lld"), *TEResultToString(Params.GetTextureTransferResult), *Params.ParameterName.ToString(), ParamsConst.FrameData.FrameID);
					ExportedTexture->ClearStableRHI(); // Not needed as we are not copying
					ForceReturnTextureToPool(ExportedTexture, Params); // Not needed as we are not copying
					return nullptr;
				}
			}

			// 3. Add a texture transfer
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    I.B.3 [GT] Cook Frame - AddTextureTransfer"), STAT_TE_I_B_3, STATGROUP_TouchEngine);
				const TEResult TransferResult = AddTETextureTransfer(Params, ExportedTexture);
				UE_LOG(LogTouchEngine, Log, TEXT("[ExportTexture] TEInstanceAddTextureTransfer `%s` returned `%s` for parameter `%s` for frame %lld"), *ExportedTexture->DebugName, *TEResultToString(TransferResult), *Params.ParameterName.ToString(), Params.FrameData.FrameID);
				if (TransferResult != TEResultSuccess)
				{
					UE_LOG(LogTouchEngine, Error, TEXT("[ExportTexture] TEInstanceAddTextureTransfer `%s` returned `%s` for parameter `%s` for frame %lld"), *ExportedTexture->DebugName, *TEResultToString(TransferResult), *Params.ParameterName.ToString(), Params.FrameData.FrameID);
					return nullptr;
				}
			}

			// 4. Finalise the export and enqueue the copy of the texture on RenderThread
			FinaliseExportAndEnqueueCopy_AnyThread(Params, ExportedTexture);
			
			// 5. Finally return the texture that will be passed to TEInstanceLinkSetTextureValue in FTouchVariableManager::SetTOPInput
			return TouchTexture;
		}
	protected:
		/** Handles the creation of the semaphore and the call to TEInstanceAddTextureTransfer for each RHI */
		virtual TEResult AddTETextureTransfer(FTouchExportParameters& Params, const TSharedPtr<TExportedTouchTexture>& Texture) = 0;
		/** Called at the end of ExportTexture_AnyThread once the texture is ready to be copied into */
		virtual void FinaliseExportAndEnqueueCopy_AnyThread(FTouchExportParameters& Params, TSharedPtr<TExportedTouchTexture>& Texture) = 0;

	private:
		/**
		 * Create a texture and add it to the different internal pools 
		 * @param Params The export parameters that this texture needs to match
		 * @param ParamTextureRHI The stable RHI of Params.Texture that we previously retrieved, used to get the size of the current available Mip that will be exported
		 */
		TSharedPtr<FTextureData> ShareTexture(const FTouchExportParameters& Params, const FTextureRHIRef& ParamTextureRHI)
		{
			TSharedPtr<TExportedTouchTexture> ExportedTexture = This()->CreateTexture(Params, ParamTextureRHI);
			if (ensure(ExportedTexture))
			{
				INC_DWORD_STAT(STAT_TE_ExportedTexturePool_NbTexturesTotal)
				ExportedTexture->DebugName = FString::Printf(TEXT("%s__frame%lld__%s"), *GetNameSafe(Params.Texture), Params.FrameData.FrameID, *Params.ParameterName.ToString());
				TSharedPtr<FTextureData> NewTextureData = MakeShared<FTextureData>();
				NewTextureData->DebugName = GetNameSafe(Params.Texture);
				NewTextureData->UETexture = Params.Texture;
				NewTextureData->ExportedPlatformTexture = ExportedTexture;
				NewTextureData->ParametersInUsage = {Params.ParameterName};
				NewTextureData->ExportedPlatformTexture->SetStableRHIOfTextureToCopy(ParamTextureRHI);
				
				CachedTextureData.Add(Params.Texture, NewTextureData);
				return NewTextureData;
			}
			return nullptr;
		}

		/**
		 * Look in the texture pool for any texture that would match the size and pixel format as the export parameters.
		 * If found, the Texture is removed from the pool and cached for this parameter
		 * @param Params The export parameters that this texture needs to match
		 * @param ParamTextureRHI The stable RHI of Params.Texture that we previously retrieved, used to get the size of the current available Mip that will be exported
		 */
		TSharedPtr<FTextureData> FindSuitableTextureFromPool(const FTouchExportParameters& Params, const FTextureRHIRef& ParamTextureRHI)
		{
			TSharedPtr<FTextureData> SuitableTextureFromPool;
			
			for (TSharedPtr<FTextureData>& TextureData : TexturePool)
			{
				if (ensure(TextureData && TextureData->IsExportedPlatformTextureHealthy()) && TextureData->ExportedPlatformTexture->CanFitTexture(ParamTextureRHI))
				{
					TextureData->ExportedPlatformTexture->DebugName = FString::Printf(TEXT("%s__frame%lld__%s"), *GetNameSafe(Params.Texture), Params.FrameData.FrameID, *Params.ParameterName.ToString());
					TextureData->DebugName = GetNameSafe(Params.Texture);
					TextureData->UETexture = Params.Texture;
					TextureData->ParametersInUsage = {Params.ParameterName};
					
					CachedTextureData.Add(Params.Texture, TextureData);

					SuitableTextureFromPool = TextureData;
					break;
				}
			}
			
			if (SuitableTextureFromPool)
			{
				TexturePool.Remove(SuitableTextureFromPool);
			}
			return SuitableTextureFromPool;
		}

		/** Release the texture, ensuring it has been released by TouchEngine before we let it be destroyed */
		void ReleaseTexture(TSharedPtr<TExportedTouchTexture>& Texture)
		{
			// This will keep the Texture valid for as long as TE is using the texture
			if (Texture)
			{
				Texture->Release()
					.Next([this, Texture, TaskToken = PendingTextureReleases.StartTask()](auto)
					{
						DEC_DWORD_STAT(STAT_TE_ExportedTexturePool_NbTexturesTotal)
					});
			}
		}

		
		/** Associates UTexture objects with the resource shared with TE. */
		TMap<UTexture*, TSharedPtr<FTextureData>> CachedTextureData;

		/** The pool of available textures to be reused. Managed and trimmed in TexturePoolMaintenance */
		TArray<TSharedPtr<FTextureData>> TexturePool;
		/** The Texture Pool of textures not yet available for reuse. Their availability will be checked in TexturePoolMaintenance and they will be moved to the Texture Pool once ready */
		TArray<TSharedPtr<FTextureData>> FutureTexturesToPool;

		/** Tracks the tasks of releasing textures. */
		FTaskSuspender PendingTextureReleases;
		
		FCriticalSection PooledTextureMutex;
	};

}
