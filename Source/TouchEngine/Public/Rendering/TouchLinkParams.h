// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"
#include "TouchEngine/TouchObject.h"
#include "TouchEngine/TETexture.h"

class UTexture2D;

namespace UE::TouchEngine
{
	struct FTouchLinkParameters
	{
		/** The instance from which the link request originates */
		TouchObject<TEInstance> Instance;

		/** The parameter name of the output texture */
		FName ParameterName;
		/** The output texture as retrieved using TEInstanceLinkGetTextureValue */
		TouchObject<TETexture> Texture;
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
}
