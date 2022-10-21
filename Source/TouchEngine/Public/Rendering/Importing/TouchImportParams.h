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
#include "Misc/Optional.h"
#include "TouchEngine/TouchObject.h"
#include "TouchEngine/TETexture.h"

class UTexture2D;

namespace UE::TouchEngine
{
	struct FTouchImportParameters
	{
		/** The instance from which the link request originates */
		TouchObject<TEInstance> Instance;

		/** The parameter name of the output texture */
		FName ParameterName;
		/** The output texture as retrieved using TEInstanceLinkGetTextureValue */
		TouchObject<TETexture> Texture;
	};

	enum class EImportResultType
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

		UnsupportedOperation,

		/** Some internal error. Log at log. */
		Failure,
	};
	
	struct FTouchImportResult
	{
		EImportResultType ResultType;

		/** Only valid if ResultType == Success */
		TOptional<UTexture2D*> ConvertedTextureObject;

		static FTouchImportResult MakeCancelled() { return { EImportResultType::Cancelled }; }
		static FTouchImportResult MakeFailure() { return { EImportResultType::Failure }; }
		static FTouchImportResult MakeSuccessful(UTexture2D* Texture) { return { EImportResultType::Success, Texture }; }
	};
}
