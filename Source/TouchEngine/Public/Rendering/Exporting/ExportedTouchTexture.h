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
#include "RHIResources.h"
#include "TouchExportParams.h"
#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TEObject.h"
#include "TouchEngine/TETexture.h"
#include "TouchEngine/TouchObject.h"

namespace UE::TouchEngine
{
	class FTouchTextureExporter;
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
		friend class FTouchTextureExporter;
		friend class FTouchFrameCooker;
	public:

		virtual ~FExportedTouchTexture();

		/** Checks whether the internal resource is compatible with the passed in texture */
		virtual bool CanFitTexture(UTexture* TextureToFit) const;

		const FTextureRHIRef& GetSharedTextureRHI_RenderThread() const { return SharedTextureRHI_RenderThread; }
		const TouchObject<TETexture>& GetTouchRepresentation_RenderThread() const { return TouchRepresentation_RenderThread; }
		
		bool IsCreatedOnRenderThread() const { return bIsCreatedOnRenderThread; }
		bool IsUsedInCurrentCook() const { return bIsUsedInCurrentCook; }
		bool WasEverUsedByTouchEngine() const { return bWasEverUsedByTouchEngine; }
		bool IsInUseByTouchEngine() const { return bIsInUseByTouchEngine; }
		bool ReceivedReleaseEvent() const { return bReceivedReleaseEvent; }
		
		virtual bool EnqueueTextureCopy(UTexture* SrcTexture);
		
		bool IsInUseByDynVars() const { return bIsInUsedByDynVars; }
		void SetInUseByDynVars() { bIsInUsedByDynVars = true; }
		void ReleasedByDynVars() { bIsInUsedByDynVars = false; }
		
		const FTouchTextureTransfer& GetTETextureTransferBackToUE() { return TETextureTransfer; }

		FString DebugName;

		struct FOnTouchReleaseTexture {};
		TFuture<FOnTouchReleaseTexture> Release();

		void GetTextureBackFromTE(const TouchObject<TEInstance>& Instance);
	protected:
		void SetTextureRHI_RenderThread(const FTextureRHIRef& SharedTextureRHI);
		void SetTouchRepresentation_RenderThread(TouchObject<TETexture>&& InTouchRepresentation, const TFunctionRef<void(const TouchObject<TETexture>&)>& InRegisterTouchCallback);
		
		void OnTouchTextureUseUpdate(void* Handle, TEObjectEvent Event, void* Info);
		static void OnSemaphoreUsageChangedForTextureTransferFromTE(void* Semaphore, TEObjectEvent Event, void* Info);
		virtual void SetSemaphoreCallbackForTextureTransferFromTE(TouchObject<TESemaphore> Semaphore) {}

		std::atomic_bool bIsCreatedOnRenderThread = false;

		void ResetTETextureTransferBackToUE() { TETextureTransfer = {}; }
	private:
		/** Shared between Unreal and TE. Access must be synchronized. */
		FTextureRHIRef SharedTextureRHI_RenderThread;
		TouchObject<TETexture> TouchRepresentation_RenderThread;
		
		std::atomic_bool bIsUsedInCurrentCook = false;
		std::atomic_bool bWasEverUsedByTouchEngine = false;
		std::atomic_bool bIsInUsedByDynVars = false;
		std::atomic_bool bIsInUseByTouchEngine = false;
		std::atomic_bool bReceivedReleaseEvent = false;

		FTouchTextureTransfer TETextureTransfer;
		TouchObject<TEInstance> TouchInstance;
		
		/** You must acquire this in order to ReleasePromise. */
		FCriticalSection TouchEngineMutex;
		TOptional<TPromise<FOnTouchReleaseTexture>> ReleasePromise;
		
		// In rare case, there is possibility that the object got destroyed and the callbacks still fire.
		bool bDestroyed = false;
	};
}
