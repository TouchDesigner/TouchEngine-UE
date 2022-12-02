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
	template<typename TExportedTouchTexture, typename TCustomBuildArgs, typename TCrtp> // curiously recurring template pattern
	class TExportedTouchTextureCache
	{
		TCrtp* This() { return static_cast<TCrtp*>(this); }
		const TCrtp* This() const { return static_cast<const TCrtp*>(this); }
		
		struct FTexturePool
		{
			TSharedPtr<TExportedTouchTexture> CurrentlySetInput;
			TArray<TSharedPtr<TExportedTouchTexture>> PooledTextures;
			TSet<FName> ParametersInUsage;
		};
	public:

		struct FTextureCreationArgs : TCustomBuildArgs, FTouchExportParameters {};
		FTextureCreationArgs MakeTextureCreationArgs(FTouchExportParameters ExportParams, TCustomBuildArgs CustomArgs = {})
		{
			return FTextureCreationArgs{ CustomArgs, ExportParams };
		}

		virtual ~TExportedTouchTextureCache()
		{
			checkf(
				ParamNameToTexture.IsEmpty() && CachedTextureData.IsEmpty(),
				TEXT("ReleaseTextures was either not called or did not clean up the exported textures correctly.")
			);
		}

		/** @return Whether GetNextOrAllocPooledTexture would allocate a new texture */
		TSharedPtr<TExportedTouchTexture> GetNextFromPool(const FTextureCreationArgs& Params)
		{
			FTexturePool* TexturePool = CachedTextureData.Find(Params.Texture);
			// Note that we do not call FExportedTouchTexture::CanFitTexture here:
			// bReuseExistingTexture promises that the texture has changed and if it has, it's a API usage error by the caller.
			if (TexturePool && Params.bReuseExistingTexture && ensure(TexturePool->CurrentlySetInput))
			{
				TexturePool->ParametersInUsage.Add(Params.ParameterName);
				return TexturePool->CurrentlySetInput;
			}

			return TexturePool
				? GetFromPool(*TexturePool, Params)
				: nullptr;
		}
		
		/** Gets an existing texture, if it can still fit the FTouchExportParameters, or allocates a new one (deleting the old texture, if any, once TouchEngine is done with it). */
		TSharedPtr<TExportedTouchTexture> GetNextOrAllocPooledTexture(const FTextureCreationArgs& Params, bool& bIsNewTexture)
		{
			UTexture** OldTextureTextureUse = ParamNameToTexture.Find(Params.ParameterName);
			const bool bHasChangedParameter = OldTextureTextureUse && *OldTextureTextureUse != Params.Texture;
			if (bHasChangedParameter)
			{
				RemoveTextureParameterDependency(Params.ParameterName);
			}

			FTexturePool* TexturePool = CachedTextureData.Find(Params.Texture);
			// Note that we do not call FExportedTouchTexture::CanFitTexture here:
			// bReuseExistingTexture promises that the texture has changed and if it has, it's a API usage error by the caller.
			if (TexturePool && Params.bReuseExistingTexture && ensure(TexturePool->CurrentlySetInput))
			{
				TexturePool->ParametersInUsage.Add(Params.ParameterName);
				bIsNewTexture = false;
				return TexturePool->CurrentlySetInput;
			}

			if (TexturePool)
			{
				return GetFromPoolOrAllocNew(*TexturePool, Params, bIsNewTexture);
			}
			
			bIsNewTexture = true;
			return ShareTexture(Params); 
		}
		
		/** Waits for TouchEngine to release the textures and then proceeds to destroy them. */
		TFuture<FTouchSuspendResult> ReleaseTextures()
		{
			// Important: Do not copy iterate ParamNameToTexture and do not copy ParamNameToTexture...
			// Otherwise we'll get a failing ensure in RemoveTextureParameterDependency
			TArray<FName> UsedParameterNames;
			ParamNameToTexture.GenerateKeyArray(UsedParameterNames);
			for (FName ParameterName : UsedParameterNames)
			{
				RemoveTextureParameterDependency(ParameterName);
			}

			check(ParamNameToTexture.IsEmpty());
			check(CachedTextureData.IsEmpty());

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
		
		TSharedPtr<TExportedTouchTexture> ShareTexture(const FTextureCreationArgs& Params)
		{
			TSharedPtr<TExportedTouchTexture> ExportedTexture = This()->CreateTexture(Params);
			if (ensure(ExportedTexture))
			{
				ParamNameToTexture.Add(Params.ParameterName, Params.Texture);
				FTexturePool& TexturePool = CachedTextureData.FindOrAdd(Params.Texture);
				TexturePool.CurrentlySetInput = ExportedTexture;
				TexturePool.PooledTextures.Add(ExportedTexture);
				TexturePool.ParametersInUsage.Add(Params.ParameterName);
				return ExportedTexture;
			}
			return nullptr;
		}
		
		TSharedPtr<TExportedTouchTexture> GetFromPoolOrAllocNew(FTexturePool& Pool, const FTextureCreationArgs& Params, bool& bIsNewTexture)
		{
			if (const TSharedPtr<TExportedTouchTexture> TextureFromPool = GetFromPool(Pool, Params))
			{
				bIsNewTexture = false;
				return TextureFromPool;
			}

			// The texture has changed structurally - release the old shared texture and create a new one
			bIsNewTexture = true;
			RemoveTextureParameterDependency(Params.ParameterName);
			return ShareTexture(Params);
		}
		
		TSharedPtr<TExportedTouchTexture> GetFromPool(FTexturePool& Pool, const FTextureCreationArgs& Params)
		{
			for (auto It = Pool.PooledTextures.CreateIterator(); It; ++It)
			{
				const TSharedPtr<TExportedTouchTexture>& Texture = *It;
				if (!Texture->CanFitTexture(Params) && !Texture->IsInUseByTouchEngine())
				{
					ReleaseTexture(Texture);
					It.RemoveCurrent();
				}
				else if (Texture->CanFitTexture(Params) && !Texture->IsInUseByTouchEngine())
				{
					Pool.ParametersInUsage.Add(Params.ParameterName);
					Pool.CurrentlySetInput = Texture;
					return Texture;
				}
			}
			return nullptr;
		}
		
		void RemoveTextureParameterDependency(FName TextureParam)
		{
			UTexture* OldExportedTexture;
			ParamNameToTexture.RemoveAndCopyValue(TextureParam, OldExportedTexture);
			FTexturePool& TexturePool = CachedTextureData.FindChecked(OldExportedTexture);
			TexturePool.ParametersInUsage.Remove(TextureParam);
			if (TexturePool.ParametersInUsage.Num() > 0)
			{
				return;
			}
			
			TexturePool.CurrentlySetInput.Reset();
			for (const TSharedPtr<TExportedTouchTexture>& Texture : TexturePool.PooledTextures)
			{
				ReleaseTexture(Texture);
			}

			CachedTextureData.Remove(OldExportedTexture);
		}

		void ReleaseTexture(TSharedPtr<TExportedTouchTexture> Texture)
		{
			// This will keep the Texture valid for as long as TE is using the texture
			Texture->Release()
				.Next([this, Texture, TaskToken = PendingTextureReleases.StartTask()](auto){});
		}
		
		/** Associates texture objects with the resource shared with TE. */
		TMap<UTexture*, FTexturePool> CachedTextureData;
		/** Maps to texture last bound to this parameter name. */
		TMap<FName, UTexture*> ParamNameToTexture;

		/** Tracks the tasks of releasing textures. */
		FTaskSuspender PendingTextureReleases;
	};

}
