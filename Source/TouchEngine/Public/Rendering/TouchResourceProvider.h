// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "PixelFormat.h"
#include "TouchEngine/TETexture.h"
#include "TouchEngine/TouchObject.h"

class FTexture2DResource;
class FRHITexture2D;
struct FTouchTOP;
typedef void FTouchEngineDevice;

namespace UE::TouchEngine
{
	struct FTouchLinkParameters
	{
		FName ParameterName;
		TED3DSharedTexture* Texture;
	};

	enum class ELinkResultType
	{
		/** The requested was fulfilled successfully */
		Success,
		
		/**
		 * The request was cancelled because there a new request was made before the current request was started.
		 * 
		 * Example scenario:
		 * 1. You make a request > starts execution
		 * 2. Tick > Make a new request
		 * 3. Tick > Make yet another request
		 *
		 * If step 3 happens before step finishes execution, then step 2 will be cancelled. Once 1 finishes, 3 will be executed.
		 */
		Cancelled,

		/** Some internal error. Log at log. */
		Failure,
	};
	
	struct FTouchLinkResult
	{
		ELinkResultType ResultType;

		/** Only valid if ResultType == Success */
		TOptional<UTexture2D*> ConvertedTextureObject;

		static FTouchLinkResult MakeCancelled() { return { ELinkResultType::Cancelled }; }
		static FTouchLinkResult MakeFailure() { return { ELinkResultType::Failure }; }
		static FTouchLinkResult MakeSuccessful(UTexture2D* Texture) { return { ELinkResultType::Success, Texture }; }
	};
	
	
	/** Common interface for rendering API implementations */
	class FTouchResourceProvider
	{
	public:

		virtual TEGraphicsContext* GetContext() const = 0;
		virtual TEResult CreateTexture(TETexture* Src, TETexture*& Dst) = 0;

		virtual TETexture* CreateTextureWithFormat(FRHITexture2D* Texture, TETextureOrigin Origin, TETextureComponentMap Map, EPixelFormat Format) = 0;

		/** Converts a texture received from the touch engine to an Unreal Engine texture object. */
		virtual TFuture<FTouchLinkResult> LinkTexture(const FTouchLinkParameters& LinkParams) = 0;

		virtual void ReleaseTexture(FTouchTOP& Texture) = 0;
		virtual void ReleaseTextures_RenderThread(bool bIsFinalClean = false) = 0;
		virtual void QueueTextureRelease_RenderThread(TETexture* Texture) = 0;

		/** Returns the texture resource in it's native format. */
		virtual FTexture2DResource* GetTexture(const TETexture* Texture) = 0;

		virtual bool IsSupportedFormat(EPixelFormat Format) = 0;
		
		virtual ~FTouchResourceProvider() = default;
	};
}
