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
#include "TouchEngine/TEObject.h"
#include "TouchEngine/TETexture.h"
#include "TouchEngine/TouchObject.h"
#include "RHIResources.h"
#include "TouchExportParams.h"

namespace UE::TouchEngine
{
	/**
	 * Intended to be used with TExportedTouchTextureCache.
	 * 
	 * Keeps hold of TouchObject<TETexture> for as long as we need it.
	 * Once it is no longer needed (determined by TExportedTouchTextureCache), Release is called:
	 * this object will notify TouchEngine and wait for it to stop using this texture.
	 * Once TouchEngine has stopped using this texture, this object is destroyed.
	 */
	class TOUCHENGINE_API FExportedTouchTexture : public TSharedFromThis<FExportedTouchTexture>
	{
		template<typename A, typename C>
		friend class TExportedTouchTextureCache;
	public:

		FExportedTouchTexture(TouchObject<TETexture> InTouchRepresentation, const TFunctionRef<void(const TouchObject<TETexture>&)>& RegisterTouchCallback);
		virtual ~FExportedTouchTexture();

		/** Checks whether the internal resource is compatible with the passed in texture */
		virtual bool CanFitTexture(const FRHITexture* TextureToFit) const = 0;

		const TouchObject<TETexture>& GetTouchRepresentation() const { return TouchRepresentation; }
		bool IsInUseByTouchEngine() const { return bIsInUseByTouchEngine; }
		bool WasEverUsedByTouchEngine() const { return bWasEverUsedByTouchEngine; }
		bool ReceivedReleaseEvent() const { return bReceivedReleaseEvent; }
		
		const FTextureRHIRef& GetStableRHIOfTextureToCopy()
		{
			return RHIOfTextureToCopy;
		}
		void SetStableRHIOfTextureToCopy(const FTextureRHIRef& InRHI)
		{
			RHIOfTextureToCopy = InRHI;
		}
		void SetStableRHIOfTextureToCopy(FTextureRHIRef&& InRHI)
		{
			RHIOfTextureToCopy = InRHI;
		}
		void ClearStableRHI(const FTouchExportParameters& ExportParams) //todo: remove export params once bug of textures not being release by TE is fixed
		{
			// UE_LOG(LogTemp, Log, TEXT("FExportedTouchTexture::ClearStableRHI called for Texture %s : %s"), *DebugName, *ExportParams.GetDebugDescription())
			RHIOfTextureToCopy = nullptr;
		}

		FString DebugName;
		TouchObject<TEInstance> TEInstance; //todo: remove TEInstance once bug of textures not being release by TE is fixed
	protected:
		
		void OnTouchTextureUseUpdate(TEObjectEvent Event);
		virtual void RemoveTextureCallback() = 0;
		
	private:

		struct FOnTouchReleaseTexture {};
		
		TouchObject<TETexture> TouchRepresentation;
		std::atomic_bool bIsInUseByTouchEngine = false;
		bool bWasEverUsedByTouchEngine = false;
		bool bReceivedReleaseEvent = false;

		FTextureRHIRef RHIOfTextureToCopy;
		
		/** You must acquire this in order to ReleasePromise. */
		FCriticalSection TouchEngineMutex;
		TOptional<TPromise<FOnTouchReleaseTexture>> ReleasePromise;
		
		TFuture<FOnTouchReleaseTexture> Release();

		// In rare case, there is possibility that the object got destroyed and the callbacks still fire.
		bool bDestroyed = false;
	};
}
