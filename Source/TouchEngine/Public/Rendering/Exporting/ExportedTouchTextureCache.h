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
#include "Rendering/TouchResourceProvider.h"
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
			int64 FrameCreated; //The frame ID at which this texture was created

			bool IsExportedPlatformTextureHealthy()
			{
				return ExportedPlatformTexture && !ExportedPlatformTexture->ReceivedReleaseEvent();
			}
		};
		
	public:
		int32 PoolSize = 20;
		
		virtual ~TExportedTouchTextureCache()
		{
			checkf(
				CachedTextureData.IsEmpty(),
				TEXT("ReleaseTextures was either not called or did not clean up the exported textures correctly.")
			);
		}
		
		/** Gets an existing texture, if it can still fit the FTouchExportParameters, or allocates a new one (deleting the old texture, if any, once TouchEngine is done with it). */
		TSharedPtr<TExportedTouchTexture> GetOrCreateTexture(const FTouchExportParameters& Params, bool& bIsNewTexture, bool& bTextureNeedsCopy)
		{
			check(Params.Texture)
			
			FScopeLock Lock(&PooledTextureMutex);
			UE_LOG(LogTemp, Verbose, TEXT("[TExportedTouchTextureCache::GetOrCreateTexture] for param `%s` and texture `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture));

			// // 1. we check if we have sent the same texture for the same parameter
			FTextureRHIRef ParamTextureRHI = FTouchResourceProvider::GetStableRHIFromTexture(Params.Texture);
			
			// 2. check if we already have a TextureData for the given UTexture
			TSharedPtr<FTextureData>* FoundTextureData = CachedTextureData.Find(Params.Texture);
			
			// 3. If we do, just send it back.
			if (FoundTextureData)
			{
				if (TSharedPtr<FTextureData>& TextureData = *FoundTextureData)
				{
					if (ensure(TextureData->IsExportedPlatformTextureHealthy()) && TextureData->ExportedPlatformTexture->CanFitTexture(ParamTextureRHI)
						 && (!TextureData->ExportedPlatformTexture->IsInUseByTouchEngine() || TextureData->FrameCreated == Params.FrameData.FrameID))
					{
						// As we could try to be sending the same texture to different inputs, the texture might validly be in use by TouchEngine.
						// To differentiate it from a case where we shouldn't use the texture this frame, we are storing the current frame ID to the texture.
						// If another texture exported it this frame, we can just reuse it,
						// otherwise the texture from a previous cook is still in use, so we need to create a new one
						check(TextureData->ExportedPlatformTexture)
						UE_LOG(LogTemp, Verbose, TEXT("[TExportedTouchTextureCache::GetNextOrAllocPooledTexture] Reusing existing texture for param `%s` and texture `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture));
						bTextureNeedsCopy = TextureData->ParametersInUsage.IsEmpty(); // if other parameters are using this texture, they are already taking care of the copy
						TextureData->ParametersInUsage.Add(Params.ParameterName);
						bIsNewTexture = false;
						TextureData->ExportedPlatformTexture->SetStableRHIOfTextureToCopy(MoveTemp(ParamTextureRHI));
						TextureData->FrameCreated = Params.FrameData.FrameID; // If we are able to use this texture this frame, make sure its frameID is updated so other parameters can use it
						return TextureData->ExportedPlatformTexture;
					}
					else // We need to release it now as the CachedTextureData will be overriden for this UTexture
					{
						// if we are here we should be the only parameter still using this texture, so the texture should be added to the TexturePool which is what we are checking here
						check(ForceReturnTextureToPool(TextureData->ExportedPlatformTexture, Params));
					}
				}
			}

			bTextureNeedsCopy = true;
			
			// 4. if we have an existing pool, try to get it from there
			if (TSharedPtr<FTextureData> TextureData = FindSuitableTextureFromPool(Params, ParamTextureRHI))
			{
				check(!TextureData->ExportedPlatformTexture->IsInUseByTouchEngine())
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
					TexturesToRelease.AddUnique(CachedTextureData.FindAndRemoveChecked(Key));
					continue;
				}

				TSharedPtr<FTextureData>& TextureData = CachedTextureData[Key];
				if (ensure(TextureData && TextureData->IsExportedPlatformTextureHealthy()))
				{
					if (TextureData->ParametersInUsage.IsEmpty()) // if we processed all the parameters and none are using this texture
					{
						TextureData->UETexture = nullptr;
						if (TextureData->ExportedPlatformTexture->IsInUseByTouchEngine())
						{
							TexturesToWait.AddUnique(CachedTextureData.FindAndRemoveChecked(Key)); // if it is still in use, we cannot reuse it right away.
						}
						else
						{
							TexturesToPool.AddUnique(CachedTextureData.FindAndRemoveChecked(Key)); // otherwise we'll add it to the pool
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
					TexturesToRelease.AddUnique(CachedTextureData.FindAndRemoveChecked(Key));
				}
			}

			// 2. Then we ensure our pool is healthy
			for (int i = 0; i < TexturePool.Num(); ++i)
			{
				TSharedPtr<FTextureData>& TextureData = TexturePool[i];
				if (!ensure(TextureData && TextureData->IsExportedPlatformTextureHealthy()))
				{
					TexturesToRelease.AddUnique(TextureData);
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
						TexturesToPool.AddUnique(TextureData);
						FutureTexturesToPool.RemoveAt(i);
						--i;
					}
				}
				else
				{
					TexturesToRelease.AddUnique(TextureData);
					FutureTexturesToPool.RemoveAt(i);
					--i;
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

		/**
		 * If something happened and the texture returned from GetOrCreateTexture did not get copied into,
		 * we need to ensure it is not reused and we add it back to the pool.
		 * Returns true if the Texture was actually returned to the pool, or false otherwise (if null, or if other parameters are using this texture)
		 */
		bool ForceReturnTextureToPool(TSharedPtr<TExportedTouchTexture>& ExportedTexture, const FTouchExportParameters& Params)
		{
			if (!ExportedTexture)
			{
				return false;
			}
			
			if (TSharedPtr<FTextureData>* TextureDataPtr = CachedTextureData.Find(Params.Texture))
			{
				TSharedPtr<FTextureData>& TextureData = *TextureDataPtr;
				if (TextureData && TextureData->ExportedPlatformTexture == ExportedTexture)
				{
					TextureData->ParametersInUsage.Remove(Params.ParameterName);
					if (TextureData->ParametersInUsage.IsEmpty())
					{
						UE_LOG(LogTouchEngine, Log, TEXT("FExportedTouchTexture::ForceReturnTextureToPool called for Texture %s : %s"), *ExportedTexture->DebugName, *Params.GetDebugDescription())
						TextureData->ExportedPlatformTexture->ClearStableRHI();
						FutureTexturesToPool.AddUnique(CachedTextureData.FindAndRemoveChecked(Params.Texture));
						return true;
					}
				}
			}
			return false;
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
					UE_LOG(LogTouchEngine, Error, TEXT("[ExportTextureToTE_AnyThread[%s]] Unable to Get or Create a Texture to export onto. %s"), *GetCurrentThreadStr(), *ParamsConst.GetDebugDescription());
					return nullptr;
				}
			}
			
			UE_LOG(LogTouchEngine, Log, TEXT("[ExportTextureToTE_AnyThread[%s]] GetOrCreateTexture returned %s `%s` (%sneeding a copy). %s"),
			       *GetCurrentThreadStr(), bIsNewTexture ? TEXT("a NEW texture") : TEXT("the EXISTING texture"),
			       bTextureNeedsCopy ? TEXT("") : TEXT("NOT "),
			       *ExportedTexture->DebugName, *ParamsConst.GetDebugDescription());

			const TouchObject<TETexture>& TouchTexture = ExportedTexture->GetTouchRepresentation();

			// 2.a If we don't need to copy because the copy is already enqueued by another parameter, return early...
			if (!bTextureNeedsCopy) // if the texture is already ready
			{
				UE_LOG(LogTouchEngine, Log, TEXT("[ExportTextureToTE_AnyThread[%s]] Reusing existing texture as it was already used by other parameters. %s"),
					*GetCurrentThreadStr(), *ParamsConst.GetDebugDescription());
				return TouchTexture;
			}
			
			// 2.b ...Otherwise, if this is not a new texture, transfer ownership if needed
			FTouchExportParameters Params{ParamsConst};
			Params.TETextureTransfer.Result = TEResultNoMatchingEntity;
			
			if (!bIsNewTexture && TEInstanceHasTextureTransfer(Params.Instance, TouchTexture)) // If this is a pre-existing texture
			{
				// Here we can use a regular TEInstanceGetTextureTransfer even for Vulkan because the contents of the texture can be discarded
				// as noted https://github.com/TouchDesigner/TouchEngine-Windows#vulkan
				Params.TETextureTransfer.Result = TEInstanceGetTextureTransfer(Params.Instance, TouchTexture, Params.TETextureTransfer.Semaphore.take(), &Params.TETextureTransfer.WaitValue); // request an ownership transfer from TE to UE, will be processed below
				if (Params.TETextureTransfer.Result != TEResultSuccess && Params.TETextureTransfer.Result != TEResultNoMatchingEntity) //TEResultNoMatchingEntity would be raised if there is no texture transfer waiting
				{
					UE_LOG(LogTouchEngine, Error, TEXT("[ExportTextureToTE_AnyThread[%s]] TEInstanceGetTextureTransfer returned `%s`. %s"), *GetCurrentThreadStr(), *TEResultToString(Params.TETextureTransfer.Result), *Params.GetDebugDescription());
					ExportedTexture->ClearStableRHI(); // Not needed as we are not copying
					ForceReturnTextureToPool(ExportedTexture, Params); // Not needed as we are not copying
					return nullptr;
				}
			}

			// 3. Add a texture transfer
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    I.B.3 [GT] Cook Frame - AddTextureTransfer"), STAT_TE_I_B_3, STATGROUP_TouchEngine);
				const TEResult TransferResult = AddTETextureTransfer(Params, ExportedTexture);
				UE_CLOG(TransferResult == TEResultSuccess, LogTouchEngineTECalls, Log, TEXT("[ExportTextureToTE_AnyThread[%s]] TEInstanceAddTextureTransfer `%s` returned `%s`. %s"), *GetCurrentThreadStr(), *ExportedTexture->DebugName, *TEResultToString(TransferResult), *Params.GetDebugDescription());
				if (TransferResult != TEResultSuccess)
				{
					UE_LOG(LogTouchEngineTECalls, Error, TEXT("[ExportTextureToTE_AnyThread[%s]] TEInstanceAddTextureTransfer `%s` returned `%s`. %s"), *GetCurrentThreadStr(), *ExportedTexture->DebugName, *TEResultToString(TransferResult), *Params.GetDebugDescription());
					ExportedTexture->ClearStableRHI(); // Not needed as we are not copying
					ForceReturnTextureToPool(ExportedTexture, Params); // Not needed as we are not copying
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
				NewTextureData->FrameCreated = Params.FrameData.FrameID;

				if (!ensure(!CachedTextureData.Contains(Params.Texture)))
				{
					FutureTexturesToPool.Add(CachedTextureData.FindAndRemoveChecked(Params.Texture)); // just to be sure we keep track of this texture
				}
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
				if (ensure(TextureData && TextureData->IsExportedPlatformTextureHealthy()) && !TextureData->ExportedPlatformTexture->IsInUseByTouchEngine() && TextureData->ExportedPlatformTexture->CanFitTexture(ParamTextureRHI))
				{
					TextureData->ExportedPlatformTexture->DebugName = FString::Printf(TEXT("%s__frame%lld__%s"), *GetNameSafe(Params.Texture), Params.FrameData.FrameID, *Params.ParameterName.ToString());
					TextureData->DebugName = GetNameSafe(Params.Texture);
					TextureData->UETexture = Params.Texture;
					TextureData->ParametersInUsage = {Params.ParameterName};
					TextureData->FrameCreated = Params.FrameData.FrameID;

					if (!ensure(!CachedTextureData.Contains(Params.Texture)))
					{
						FutureTexturesToPool.Add(CachedTextureData.FindAndRemoveChecked(Params.Texture)); // just to be sure we keep track of this texture
					}
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
