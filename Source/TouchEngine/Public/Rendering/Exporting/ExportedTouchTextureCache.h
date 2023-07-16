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
#include "Util/TouchEngineStatsGroup.h"


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
		int32 PoolSize = 20;
		
		virtual ~TExportedTouchTextureCache()
		{
			checkf(
				CachedTextureData.IsEmpty(),
				TEXT("ReleaseTextures was either not called or did not clean up the exported textures correctly.")
			);
		}
		
		// bool GetNextOrAllocPooledTETexture(const FTouchExportParameters& TouchExportParameters, bool& bIsNewTexture, bool& bIsUsedByOtherTexture, TouchObject<TETexture>& OutTexture)
		// {
		// 	const TSharedPtr<TExportedTouchTexture> TextureData = GetOrCreateTexture(TouchExportParameters, bIsNewTexture, bIsUsedByOtherTexture);
		// 	if (!TextureData)
		// 	{
		// 		OutTexture = TouchObject<TETexture>();
		// 		return false;
		// 	}
		// 	OutTexture = TextureData->GetTouchRepresentation();
		// 	return true;
		// }
		
		/** Gets an existing texture, if it can still fit the FTouchExportParameters, or allocates a new one (deleting the old texture, if any, once TouchEngine is done with it). */
		TSharedPtr<TExportedTouchTexture> GetOrCreateTexture(const FTouchExportParameters& Params, bool& bIsNewTexture, bool& bTextureNeedsCopy)
		{
			FScopeLock Lock(&PooledTextureMutex);
			UE_LOG(LogTemp, Verbose, TEXT("[TExportedTouchTextureCache::GetOrCreateTexture] for param `%s` and texture `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture));

			// // 1. we check if we have sent the same texture for the same parameter

			// 2. check if we already have a TextureData for the given UTexture
			TSharedPtr<FTextureData>* FoundTextureData = CachedTextureData.Find(Params.Texture);
			
			// 3. If we do, just send it back.
			if (FoundTextureData)
			{
				if (TSharedPtr<FTextureData>& TextureData = *FoundTextureData)
				{
					if (ensure(TextureData->IsExportedPlatformTextureHealthy()))
					{
						check(TextureData->ExportedPlatformTexture)
						UE_LOG(LogTemp, Verbose, TEXT("[TExportedTouchTextureCache::GetNextOrAllocPooledTexture] Reusing existing texture for param `%s` and texture `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture));
						bTextureNeedsCopy = false;
						TextureData->ParametersInUsage.Add(Params.ParameterName);
						bIsNewTexture = false;
						return TextureData->ExportedPlatformTexture;
					}
					else // This is not supposed to happen but if it does, we need to release it now as the CachedTextureData will be overriden for this UTexture
					{
						ReleaseTexture(TextureData->ExportedPlatformTexture);
						CachedTextureData.Remove(Params.Texture);
					}
				}
			}

			bTextureNeedsCopy = true;
			
			// 4. if we have an existing pool, try to get it from there
			if (TSharedPtr<FTextureData> TextureData = FindSuitableTextureFromPool(Params))
			{
				bIsNewTexture = false;
				return TextureData->ExportedPlatformTexture;
			}

			//5. Otherwise, we just create a new one
			bIsNewTexture = true;
			return ShareTexture(Params)->ExportedPlatformTexture; 
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
			// Important: Do not copy iterate LastTextureForParam and do not copy LastTextureForParam...
			// Otherwise we'll get a failing ensure in RemoveTextureParameterDependency
			TArray<FName> UsedParameterNames;
			// LastTextureForParam.GenerateKeyArray(UsedParameterNames);
			//
			// for (FName ParameterName : UsedParameterNames)
			// {
			// 	RemoveTextureParameterDependency(ParameterName);
			// }

			// check(LastTextureForParam.IsEmpty());
			
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

	private:
		/** Create a texture and add it to the different internal pools */
		TSharedPtr<FTextureData> ShareTexture(const FTouchExportParameters& Params)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[TExportedTouchTextureCache::ShareTexture] Creating new Texture for param `%s` and texture `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture));
			TSharedPtr<TExportedTouchTexture> ExportedTexture = This()->CreateTexture(Params);
			if (ensure(ExportedTexture))
			{
				INC_DWORD_STAT(STAT_TE_ExportedTexturePool_NbTexturesTotal)
				ExportedTexture->DebugName = FString::Printf(TEXT("%s__frame%lld__%s"), *GetNameSafe(Params.Texture), Params.FrameData.FrameID, *Params.ParameterName.ToString());
				TSharedPtr<FTextureData> NewTextureData = MakeShared<FTextureData>();
				NewTextureData->DebugName = GetNameSafe(Params.Texture);
				NewTextureData->UETexture = Params.Texture;
				NewTextureData->ExportedPlatformTexture = ExportedTexture;
				NewTextureData->ParametersInUsage = {Params.ParameterName};
				
				CachedTextureData.Add(Params.Texture, NewTextureData);
				return NewTextureData;
			}
			return nullptr;
		}

		TSharedPtr<FTextureData> FindSuitableTextureFromPool(const FTouchExportParameters& Params)
		{
			TSharedPtr<FTextureData> SuitableTextureFromPool;
			
			for (TSharedPtr<FTextureData>& TextureData : TexturePool)
			{
				if (ensure(TextureData && TextureData->IsExportedPlatformTextureHealthy()) && TextureData->ExportedPlatformTexture->CanFitTexture(Params))
				{
					TextureData->ExportedPlatformTexture->DebugName = FString::Printf(TEXT("%s__frame%lld__%s"), *GetNameSafe(Params.Texture), Params.FrameData.FrameID, *Params.ParameterName.ToString());
					TextureData->DebugName = GetNameSafe(Params.Texture);
					TextureData->UETexture = Params.Texture;
					TextureData->ParametersInUsage = {Params.ParameterName};
					
					CachedTextureData.Add(Params.Texture, TextureData);

					SuitableTextureFromPool = TextureData;
					// TSharedPtr<FTextureData> TextureDataCopy = TextureData; // to ensure we do not delete the last reference when we remove it from the pool
					// TexturePool.Remove(TextureData); // remove from the pool
					// return TextureDataCopy;
					break;
				}
			}
			
			if (SuitableTextureFromPool)
			{
				TexturePool.Remove(SuitableTextureFromPool);
			}
			return SuitableTextureFromPool;
		}

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

		TArray<TSharedPtr<FTextureData>> TexturePool;
		TArray<TSharedPtr<FTextureData>> FutureTexturesToPool;
		/** Maps to texture last bound to this parameter name. */
		// TMap<FName, UTexture*> LastTextureForParam;

		/** Tracks the tasks of releasing textures. */
		FTaskSuspender PendingTextureReleases;

		
		FCriticalSection PooledTextureMutex;
	};

}
