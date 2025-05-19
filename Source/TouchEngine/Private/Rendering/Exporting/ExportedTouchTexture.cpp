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

#include "Rendering/Exporting/ExportedTouchTexture.h"

#include "Logging.h"
#include "Engine/TEDebug.h"
#include "Rendering/TouchResourceProvider.h"
#include "TouchEngine/TEInstance.h"
#include "Util/TouchHelpers.h"

namespace UE::TouchEngine
{
	FExportedTouchTexture::~FExportedTouchTexture()
	{
		// We must wait for TouchEngine to stop using the texture - FExportedTouchTexture::Release implements that logic.
		const bool bDestroyedBeforeTouchRelease = bIsInUseByTouchEngine;
		ensure(!bDestroyedBeforeTouchRelease);
		UE_CLOG(bDestroyedBeforeTouchRelease, LogTouchEngine, Error, TEXT("You didn't let the destruction be handled by FExportedTouchTexture::Release. We are causing undefined behavior for TouchEngine..."));

		FScopeLock Lock(&TouchEngineMutex);
		if (!ensureMsgf(!ReleasePromise.IsSet(), TEXT("FExportedTouchTexture being destroyed before receiving a TEObjectEventRelease event")))
		{
			// we are not supposed to delete the texture manually but to wait for the TEObjectEventRelease event
			UE_LOG(LogTouchEngine, Error, TEXT("[~FExportedTouchTexture] The FExportedTouchTexture was destroyed before receiving a TEObjectEventRelease event."));
			ReleasePromise->SetValue({});
			ReleasePromise.Reset();
		}
		if (!ensureMsgf(bReceivedReleaseEvent, TEXT("FExportedTouchTexture is being destroyed but we haven't received the TEObjectEventRelease")))
		{
			UE_LOG(LogTouchEngine, Error, TEXT("[~FExportedTouchTexture] The FExportedTouchTexture was destroyed before receiving a TEObjectEventRelease event."));
		}
		bDestroyed = true;
	}

	bool FExportedTouchTexture::CanFitTexture(UTexture* TextureToFit) const
	{
		if (!ensure(bIsCreatedOnRenderThread))
		{
			return false;
		}

		const FTextureRHIRef TextureToFitRHI = FTouchResourceProvider::GetStableRHIFromTexture(TextureToFit);
		return ensure(TextureToFitRHI)
			&& TextureToFitRHI->GetSizeXY() == GetSharedTextureRHI_RenderThread()->GetSizeXY()
			&& TextureToFitRHI->GetFormat() == GetSharedTextureRHI_RenderThread()->GetFormat()
			&& TextureToFitRHI->GetNumMips() == GetSharedTextureRHI_RenderThread()->GetNumMips()
			&& TextureToFitRHI->GetNumSamples() == GetSharedTextureRHI_RenderThread()->GetNumSamples()
			&& EnumHasAnyFlags(TextureToFitRHI->GetFlags(), ETextureCreateFlags::SRGB) == EnumHasAnyFlags(GetSharedTextureRHI_RenderThread()->GetFlags(), ETextureCreateFlags::SRGB);
	}

	bool FExportedTouchTexture::EnqueueTextureCopy(UTexture* SrcTexture)
	{
		if (!IsValid(SrcTexture))
		{
			return false;
		}
		
		ENQUEUE_RENDER_COMMAND(ExportedTouchTextureCopy)([SourceTextureResource = SrcTexture->GetResource(), WeakThis = AsWeak()](FRHICommandListImmediate& RHICmdList)
		{
			const TSharedPtr<FExportedTouchTexture> This = WeakThis.Pin();
			if (!This)
			{
				return;
			}

			RHICmdList.Transition(FRHITransitionInfo(SourceTextureResource->GetTextureRHI(), ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.Transition(FRHITransitionInfo(This->GetSharedTextureRHI_RenderThread(), ERHIAccess::Unknown, ERHIAccess::CopyDest));
			RHICmdList.CopyTexture(SourceTextureResource->GetTextureRHI(), This->GetSharedTextureRHI_RenderThread(), FRHICopyTextureInfo());
		});
		
		return true;
	}

	TFuture<FExportedTouchTexture::FOnTouchReleaseTexture> FExportedTouchTexture::Release()
	{
		TouchRepresentation_RenderThread.reset();
		
		if (!bIsInUseByTouchEngine && bReceivedReleaseEvent)
		{
			return MakeFulfilledPromise<FOnTouchReleaseTexture>(FOnTouchReleaseTexture{}).GetFuture();
		}
		
		FScopeLock Lock(&TouchEngineMutex);
		checkf(!ReleasePromise.IsSet(), TEXT("Release called twice."));
		TPromise<FOnTouchReleaseTexture> Promise;
		TFuture<FOnTouchReleaseTexture> Future = Promise.GetFuture();
		ReleasePromise = MoveTemp(Promise);
		return Future;
	}

	void FExportedTouchTexture::GetTextureBackFromTE(const TouchObject<TEInstance>& Instance)
	{
		TETextureTransfer = {};
		const TouchObject<TETexture> TouchTexture = GetTouchRepresentation_RenderThread();
		
		if (TouchTexture && Instance && TEInstanceHasTextureTransfer(Instance, TouchTexture)) // If this is a pre-existing texture
		{
			// Here we can use a regular TEInstanceGetTextureTransfer even for Vulkan because the contents of the texture can be discarded
			// as noted https://github.com/TouchDesigner/TouchEngine-Windows#vulkan
			TETextureTransfer.Result = TEInstanceGetTextureTransfer(Instance, TouchTexture, TETextureTransfer.Semaphore.take(), &TETextureTransfer.WaitValue); // request an ownership transfer from TE to UE, will be processed below
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceGetTextureTransfer(TEInstance: '%p', texture: '%p' ['%s'], semaphore&: '%p', waitValue&: '%lld') [Thread: '%s']  =>  Returned '%s'"),
				Instance.get(),
				TouchTexture.get(),
				*DebugName,
				TETextureTransfer.Semaphore.get(),
				TETextureTransfer.WaitValue,
				*GetCurrentThreadStr(),
				*TEResultToString(TETextureTransfer.Result)
			)
			if (TETextureTransfer.Result != TEResultSuccess && TETextureTransfer.Result != TEResultNoMatchingEntity) //TEResultNoMatchingEntity would be raised if there is no texture transfer waiting
			{
				UE_LOG(LogTouchEngine, Error, TEXT("[GetTextureBackFromTE[%s]] TEInstanceGetTextureTransfer returned `%s`."), *GetCurrentThreadStr(), *TEResultToString(TETextureTransfer.Result));
			}

			if (TETextureTransfer.Semaphore)
			{
				SetSemaphoreCallbackForTextureTransferFromTE(TETextureTransfer.Semaphore);
			}
		}
	}

	void FExportedTouchTexture::SetTextureRHI_RenderThread(const FTextureRHIRef& SharedTextureRHI)
	{
		SharedTextureRHI_RenderThread = SharedTextureRHI;
		bIsCreatedOnRenderThread = true;
	}

	void FExportedTouchTexture::SetTouchRepresentation_RenderThread(TouchObject<TETexture>&& InTouchRepresentation, const TFunctionRef<void(const TouchObject<TETexture>&)>& InRegisterTouchCallback)
	{
		TouchRepresentation_RenderThread = MoveTemp(InTouchRepresentation);
		InRegisterTouchCallback(TouchRepresentation_RenderThread);
	}

	void FExportedTouchTexture::OnTouchTextureUseUpdate(void* Handle, TEObjectEvent Event, void* Info)
	{
		if (!ensureMsgf(!bDestroyed, TEXT("FExportedTouchTexture is already destroyed but still receiving TEObjectEvent")))
		{
			return;
		}

		UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEVulkanTextureCallback(textureHandle: '%p' [TE: '%p', UE: '%s'], event: '%s', info: '%p') [Thread: '%s']"),
			Handle,
			TouchRepresentation_RenderThread.get(),
			*DebugName,
			*TEObjectEventToString(Event),
			Info,
			*GetCurrentThreadStr()
		)
		
		switch (Event)
		{
		case TEObjectEventBeginUse:
			bWasEverUsedByTouchEngine = true;
			bIsInUseByTouchEngine = true;
			break;

		case TEObjectEventRelease:
			{
				bIsInUseByTouchEngine = false;
				bReceivedReleaseEvent = true;
				TOptional<TPromise<FOnTouchReleaseTexture>> Promise;
				{
					FScopeLock Lock(&TouchEngineMutex);
					// we want to reset first the ReleasePromise as SetValue might end up calling the destructor which will want to acquire a lock, so we are moving it to a temporary object.
					Promise = MoveTemp(ReleasePromise);
					ReleasePromise.Reset();
				}
				if (Promise.IsSet())
				{
					Promise->SetValue({});
				}
				break;
			}
		case TEObjectEventEndUse:
			{
				bIsInUseByTouchEngine = false;
				break;
			}
		default: checkNoEntry();
			break;
		}
	}

	void FExportedTouchTexture::OnSemaphoreUsageChangedForTextureTransferFromTE(void* Semaphore, TEObjectEvent Event, void* Info)
	{
		const FExportedTouchTexture* This = static_cast<FExportedTouchTexture*>(Info);
		UE_LOG(LogTouchEngine, Verbose, TEXT("[FExportedTouchTexture::OnSemaphoreUsageChangedForTextureTransferFromTE[%s]] Event `%s` for semaphore '%p' from texture `%s`"), *GetCurrentThreadStr(), *TEObjectEventToString(Event), Semaphore, *(This ? This->DebugName : TEXT("")))
	}
}
