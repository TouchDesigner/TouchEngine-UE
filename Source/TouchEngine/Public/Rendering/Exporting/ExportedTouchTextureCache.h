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
	template<typename TExportedTouchTexture, typename TCrtp> // curiously recurring template pattern
	class TExportedTouchTextureCache
	{
		TCrtp* This() { return static_cast<TCrtp*>(this); }
		const TCrtp* This() const { return static_cast<const TCrtp*>(this); }
		
		struct FTexturePool // this doesn't really seem to be a pool anymore. How would we want it to work?
		{
			FString DebugName;
			TSharedPtr<TExportedTouchTexture> ExportedPlatformTexture;
			TSet<FName> ParametersInUsage;
		};
	public:

		virtual ~TExportedTouchTextureCache()
		{
			checkf(
				LastTextureForParam.IsEmpty() && CachedTexturePools.IsEmpty(),
				TEXT("ReleaseTextures was either not called or did not clean up the exported textures correctly.")
			);
		}

		/** @return Whether GetNextOrAllocPooledTexture would allocate a new texture */
		TSharedPtr<TExportedTouchTexture> GetNextFromPool(const FTouchExportParameters& Params)
		{
			FScopeLock Lock(&PooledTextureMutex);
			
			FTexturePool* TexturePool = CachedTexturePools.Find(Params.Texture);
			// Note that we do not call FExportedTouchTexture::CanFitTexture here:
			// bReuseExistingTexture promises that the texture has changed and if it has, it's a API usage error by the caller.
			if (TexturePool && Params.bReuseExistingTexture && ensure(TexturePool->ExportedPlatformTexture))
			{
				TexturePool->ParametersInUsage.Add(Params.ParameterName);
				return TexturePool->ExportedPlatformTexture;
			}
			
			return TexturePool
				? GetFromPool(*TexturePool, Params)
				: nullptr;
		}
		
		bool GetNextOrAllocPooledTETexture(const FTouchExportParameters& TouchExportParameters, bool& bIsNewTexture, TouchObject<TETexture>& OutTexture)
		{
			const TSharedPtr<TExportedTouchTexture> TextureData = GetNextOrAllocPooledTexture(TouchExportParameters, bIsNewTexture);
			if (!TextureData)
			{
				OutTexture = TouchObject<TETexture>();
				return false;
			}
			OutTexture = TextureData->GetTouchRepresentation();
			return true;
		}
		
		/** Gets an existing texture, if it can still fit the FTouchExportParameters, or allocates a new one (deleting the old texture, if any, once TouchEngine is done with it). */
		TSharedPtr<TExportedTouchTexture> GetNextOrAllocPooledTexture(const FTouchExportParameters& Params, bool& bIsNewTexture)
		{
			FScopeLock Lock(&PooledTextureMutex);
			UE_LOG(LogTemp, Log, TEXT("[GetNextOrAllocPooledTexture] for param `%s` and texture `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture));

			// todo: would we want to check this even if Params.bReuseExistingTexture is true? see GetNextFromPool
			// 1. we check if we have sent the same texture for the same parameter
			UTexture** PreviousTexture = LastTextureForParam.Find(Params.ParameterName);
			const bool bHasTextureChanged = PreviousTexture && *PreviousTexture != Params.Texture;
			UE_LOG(LogTemp, Log, TEXT("[GetNextOrAllocPooledTexture] bHasChangedParameter? %s"), bHasTextureChanged? TEXT("TRUE") : TEXT("FALSE"));
			if (bHasTextureChanged)
			{
				// if we are sending a new texture, we can get rid of previously cached references
				RemoveTextureParameterDependency(Params.ParameterName);
			}

			// 2. check if we already have a TexturePool for the given texture
			FTexturePool* TexturePool = CachedTexturePools.Find(Params.Texture);
			
			// 3. If we want to reuse the Existing texture, just send it back.
			// Note that we do not call FExportedTouchTexture::CanFitTexture here:
			// bReuseExistingTexture promises that the texture has changed and if it has, it's a API usage error by the caller.
			if (TexturePool && Params.bReuseExistingTexture && ensure(TexturePool->ExportedPlatformTexture))
			{
				UE_LOG(LogTemp, Log, TEXT("[GetNextOrAllocPooledTexture] Reusing existing texture for param `%s` and texture `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture));
				LastTextureForParam.Add(Params.ParameterName, Params.Texture);
				TexturePool->ParametersInUsage.Add(Params.ParameterName);
				bIsNewTexture = false;
				return TexturePool->ExportedPlatformTexture;
			}

			// 4. if we have an existing pool, try to get it from there
			if (TexturePool)
			{
				return GetFromPoolOrAllocNew(*TexturePool, Params, bIsNewTexture);
			}

			//5. Otherwise, we just create a new one
			bIsNewTexture = true;
			return ShareTexture(Params); 
		}
		
		/** Waits for TouchEngine to release the textures and then proceeds to destroy them. */
		TFuture<FTouchSuspendResult> ReleaseTextures()
		{
			FScopeLock Lock(&PooledTextureMutex);
			// Important: Do not copy iterate LastTextureForParam and do not copy LastTextureForParam...
			// Otherwise we'll get a failing ensure in RemoveTextureParameterDependency
			TArray<FName> UsedParameterNames;
			LastTextureForParam.GenerateKeyArray(UsedParameterNames);
			
			for (FName ParameterName : UsedParameterNames)
			{
				RemoveTextureParameterDependency(ParameterName);
			}

			check(LastTextureForParam.IsEmpty());
			
			// Sometimes CachedTexturePools mis-align with LastTextureForParam if the texture is still in use in RemoveTextureParameterDependency, so we cannot be sure the CachedTexturePools would be empty at this point
			// todo: check if there is a better way to remove the textures as soon as they are not used, if the if statement in RemoveTextureParameterDependency returns early
			TArray<UTexture*> UsedTextures;
			CachedTexturePools.GenerateKeyArray(UsedTextures);
			for (UTexture* TextureKey : UsedTextures)
			{
				FTexturePool TexturePool;
				CachedTexturePools.RemoveAndCopyValue(TextureKey, TexturePool);
				ReleaseTexture(TexturePool.ExportedPlatformTexture);
				TexturePool.ExportedPlatformTexture.Reset();
			}
			check(CachedTexturePools.IsEmpty());

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
		TSharedPtr<TExportedTouchTexture> ShareTexture(const FTouchExportParameters& Params)
		{
			UE_LOG(LogTemp, Log, TEXT("[ShareTexture] Creating new Texture for param `%s` and texture `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture));
			TSharedPtr<TExportedTouchTexture> ExportedTexture = This()->CreateTexture(Params);
			if (ensure(ExportedTexture))
			{
				if(UTexture** TextureFound = LastTextureForParam.Find(Params.ParameterName))
				{
					UTexture* Texture = *TextureFound;
					UE_LOG(LogTemp, Error, TEXT("[ShareTexture] LastTextureForParam `%s` still holds a previous texture `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture));
					if (auto Pool = CachedTexturePools.Find(Texture))
					{
						if (Pool->ParametersInUsage.Contains(Params.ParameterName))
						{
							UE_LOG(LogTemp, Error, TEXT("[ShareTexture] CachedTexturePool `%s` still holds a a reference to the parameter `%s`"), *Pool->DebugName, *Params.ParameterName.ToString());
						}
					}
				}
				LastTextureForParam.Add(Params.ParameterName, Params.Texture);
				
				FTexturePool& TexturePool = CachedTexturePools.FindOrAdd(Params.Texture);
				ReleaseTexture(TexturePool.ExportedPlatformTexture); // make sure there is not an existing texture
				
				TexturePool.DebugName = GetNameSafe(Params.Texture);
				TexturePool.ExportedPlatformTexture = ExportedTexture;
				TexturePool.ParametersInUsage.Add(Params.ParameterName);
				return ExportedTexture;
			}
			return nullptr;
		}
		
		TSharedPtr<TExportedTouchTexture> GetFromPoolOrAllocNew(FTexturePool& Pool, const FTouchExportParameters& Params, bool& bIsNewTexture)
		{
			UE_LOG(LogTemp, Log, TEXT("[GetFromPoolOrAllocNew] for param `%s` and texture `%s` from pool `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture), *Pool.DebugName);
			if (const TSharedPtr<TExportedTouchTexture> TextureFromPool = GetFromPool(Pool, Params))
			{
				bIsNewTexture = false;
				return TextureFromPool;
			}

			// The texture has changed structurally - release the old shared texture and create a new one
			bIsNewTexture = true;
			RemoveTextureParameterDependency(Params.ParameterName); // we might have already been removing the dependency for this parameter if the texture had changed for example
			return ShareTexture(Params);
		}
		
		TSharedPtr<TExportedTouchTexture> GetFromPool(FTexturePool& Pool, const FTouchExportParameters& Params)
		{
			// The FTexturePool used to have an array of TExportedTouchTexture but the array was never filled so this was simplified.
			// How would we want this to work?
			UE_LOG(LogTemp, Log, TEXT("[GetFromPool] for param `%s` and texture `%s` from pool `%s`"), *Params.ParameterName.ToString(), *GetNameSafe(Params.Texture), *Pool.DebugName);
			if (Pool.ExportedPlatformTexture)
			{
				if (!Pool.ExportedPlatformTexture->CanFitTexture(Params) && !Pool.ExportedPlatformTexture->IsInUseByTouchEngine())
				{
					ReleaseTexture(Pool.ExportedPlatformTexture);
					Pool.ExportedPlatformTexture.Reset();
				}
				else if (Pool.ExportedPlatformTexture->CanFitTexture(Params))
				{
					if (!Pool.ExportedPlatformTexture->IsInUseByTouchEngine())
					{
						Pool.ParametersInUsage.Add(Params.ParameterName);
						return Pool.ExportedPlatformTexture;
					}
					const bool Result = TEInstanceHasTextureTransfer(Params.Instance, Pool.ExportedPlatformTexture->TouchRepresentation);
					UE_LOG(LogTemp, Warning, TEXT("[TExportedTouchTextureCache::GetFromPool] was not able to return existing texture for param `%s` because it is still in use by TouchEngine. TEInstanceHasTextureTransfer returned %s"),
						*Params.ParameterName.ToString(), Result ? TEXT("TRUE") : TEXT("FALSE"))
				}
			}
			return nullptr;
		}
		
		void RemoveTextureParameterDependency(FName TextureParam)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RemoveTextureParameterDependency] for param `%s`"), *TextureParam.ToString());
			UTexture* OldExportedTexture = nullptr;
			if (!LastTextureForParam.RemoveAndCopyValue(TextureParam, OldExportedTexture))
			{
				// If we try to remove a parameter and its name is linked to a texture
				if (FTexturePool* TexturePool = CachedTexturePools.Find(OldExportedTexture))
				{
					TexturePool->ParametersInUsage.Remove(TextureParam);
					if (TexturePool->ParametersInUsage.IsEmpty())
					{
						ReleaseTexture(TexturePool->ExportedPlatformTexture);
						TexturePool->ExportedPlatformTexture.Reset();
						CachedTexturePools.Remove(OldExportedTexture);
					}
				}
			}

			// due to the few ways we can arrive to this point, the below is currently the only reliable way we clean up properly.
			// This should be looked at further
			TArray<UTexture*> UsedTextures;
			CachedTexturePools.GenerateKeyArray(UsedTextures);
			for (UTexture* TextureKey : UsedTextures)
			{
				FTexturePool& TexturePool = CachedTexturePools[TextureKey];
				TexturePool.ParametersInUsage.Remove(TextureParam);
				if (TexturePool.ParametersInUsage.IsEmpty())
				{
					ReleaseTexture(TexturePool.ExportedPlatformTexture);
					TexturePool.ExportedPlatformTexture.Reset();
					CachedTexturePools.Remove(OldExportedTexture); //todo: should we be deleting OldExportedTexture?
				}
			}
		}

		void ReleaseTexture(TSharedPtr<TExportedTouchTexture> Texture)
		{
			// This will keep the Texture valid for as long as TE is using the texture
			if (Texture)
			{
				Texture->Release()
					.Next([this, Texture, TaskToken = PendingTextureReleases.StartTask()](auto){});
			}
		}
		
		/** Associates texture objects with the resource shared with TE. */
		TMap<UTexture*, FTexturePool> CachedTexturePools;
		/** Maps to texture last bound to this parameter name. */
		TMap<FName, UTexture*> LastTextureForParam;

		/** Tracks the tasks of releasing textures. */
		FTaskSuspender PendingTextureReleases;

		
		FCriticalSection PooledTextureMutex;
	};

}
