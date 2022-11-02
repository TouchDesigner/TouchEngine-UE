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
	public:

		struct FTextureCreationArgs : TCustomBuildArgs, FTouchExportParameters
		{};
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
		
		/** Gets an existing texture, if it can still fit the FTouchExportParameters, or allocates a new one (deleting the old texture, if any, once TouchEngine is done with it). */
		TSharedPtr<TExportedTouchTexture> GetOrTryCreateTexture(const FTextureCreationArgs& Params)
		{
			return CachedTextureData.Contains(Params.Texture)
				? ReallocateTextureIfNeeded(Params)
				: ShareTexture(Params);
		}
		
		/** Waits for TouchEngine to release the textures and then proceeds to destroy them. */
		TFuture<FTouchSuspendResult> ReleaseTextures()
		{
			// Important: Do not copy iterate ParamNameToTexture and do not copy ParamNameToTexture... otherwise we'll keep
			// the contained shared pointers alive and RemoveTextureParameterDependency won't end up scheduling any kill commands.
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
				ParamNameToTexture.Add(Params.ParameterName, FTextureDependency{ Params.Texture, ExportedTexture });
				return CachedTextureData.Add(Params.Texture, ExportedTexture);
			}
			return nullptr;
		}
		
		TSharedPtr<TExportedTouchTexture> ReallocateTextureIfNeeded(const FTextureCreationArgs& Params)
		{
			if (const TSharedPtr<TExportedTouchTexture>* Existing = CachedTextureData.Find(Params.Texture);
				// This will hit, e.g. when somebody sets the same UTexture as parameter twice.
				// If the texture has not changed structurally (i.e. resolution or format), we can reuse the shared handle and copy into the shared texture.
				Existing && Existing->Get()->CanFitTexture(Params))
			{
				return *Existing;
			}

			// The texture has changed structurally - release the old shared texture and create a new one
			RemoveTextureParameterDependency(Params.ParameterName);
			return ShareTexture(Params);
		}
		
		void RemoveTextureParameterDependency(FName TextureParam)
		{
			FTextureDependency OldExportedTexture;
			ParamNameToTexture.RemoveAndCopyValue(TextureParam, OldExportedTexture);
			CachedTextureData.Remove(OldExportedTexture.UnrealTexture);

			// If this was the last parameter that used this UTexture, then OldExportedTexture should be the last one referencing the exported texture.
			if (OldExportedTexture.ExportedTexture.IsUnique())
			{
				// This will keep the data valid for as long as TE is using the texture
				const TSharedPtr<TExportedTouchTexture> KeepAlive = OldExportedTexture.ExportedTexture;
				KeepAlive->Release()
					.Next([this, KeepAlive, TaskToken = PendingTextureReleases.StartTask()](auto){});
			}
		}

		struct FTextureDependency
		{
			TWeakObjectPtr<UTexture> UnrealTexture;
			TSharedPtr<TExportedTouchTexture> ExportedTexture;
		};

		/** Associates texture objects with the resource shared with TE. */
		TMap<TWeakObjectPtr<UTexture>, TSharedPtr<TExportedTouchTexture>> CachedTextureData;
		/** Maps to texture last bound to this parameter name. */
		TMap<FName, FTextureDependency> ParamNameToTexture;

		/** Tracks the tasks of releasing textures. */
		FTaskSuspender PendingTextureReleases;
	};

}
