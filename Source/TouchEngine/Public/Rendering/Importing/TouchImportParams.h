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
#include "Blueprint/TouchEngineInputFrameData.h"

class UTexture2D;

namespace UE::TouchEngine
{
	enum class EPendingTextureImportType
	{
		/** The texture can be imported directly */
		CanBeImported,
		/** The texture need a new UTexture Created */
		NeedUTextureCreated,
		/** The request was cancelled because a new request was made before the current request was started. */
		Cancelled,
		/** Some internal error. Look at log. */
		Failure,
	};

	static FString EPendingTextureImportTypeToString(EPendingTextureImportType PendingTextureImportType)
	{
		switch (PendingTextureImportType)
		{
		case EPendingTextureImportType::CanBeImported: return TEXT("CanBeImported");
		case EPendingTextureImportType::NeedUTextureCreated: return TEXT("NeedUTextureCreated");
		case EPendingTextureImportType::Cancelled: return TEXT("Cancelled");;
		case EPendingTextureImportType::Failure: return TEXT("Failure");
		default: return TEXT("[default]");
		}
	}

	struct FTextureFormat
	{
		EPendingTextureImportType PendingTextureImportType;
		FName Identifier;
		int32 SizeX;
		int32 SizeY;
		EPixelFormat PixelFormat = PF_B8G8R8A8;
		
		UTexture2D* UnrealTexture = nullptr;
		TSharedPtr<TPromise<UTexture2D*>> OnTextureCreated = nullptr;
	};

	struct FTouchImportParameters
	{
		/** The instance from which the link request originates */
		TouchObject<TEInstance> Instance;

		/** The parameter name of the output texture */
		FName ParameterName;
		/** The output texture as retrieved using TEInstanceLinkGetTextureValue */
		TouchObject<TETexture> Texture;
		
		TSharedPtr<TPromise<FTextureFormat>> PendingTextureImportType;

		mutable TouchObject<TESemaphore> GetTextureTransferSemaphore;
		mutable uint64 GetTextureTransferWaitValue;
		mutable TEResult GetTextureTransferResult;
	};

	enum class EImportResultType
	{
		/** The requested was fulfilled successfully */
		Success,
		
		/**
		 * The request was cancelled because a new request was made before the current request was started.
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

		/** Some internal error. Look at log. */
		Failure,
	};

	static FString EImportResultTypeToString(EImportResultType ImportResultType)
	{
		switch (ImportResultType)
		{
		case EImportResultType::Success: return TEXT("Success");
		case EImportResultType::Cancelled: return TEXT("Cancelled");
		case EImportResultType::UnsupportedOperation: return TEXT("UnsupportedOperation");;
		case EImportResultType::Failure: return TEXT("Failure");
		default: return TEXT("[default]");
		}
	}
	
	struct FTouchTextureImportResult
	{
		EImportResultType ResultType;

		/** Only valid if ResultType == Success */
		TOptional<UTexture2D*> ConvertedTextureObject;

		static FTouchTextureImportResult MakeCancelled() { return { EImportResultType::Cancelled }; }
		static FTouchTextureImportResult MakeFailure() { return { EImportResultType::Failure }; }
		static FTouchTextureImportResult MakeSuccessful(UTexture2D* Texture) { return { EImportResultType::Success, Texture }; }
	};
		
	struct FTouchTexturesReady
	{
		EImportResultType Result;
		FTouchEngineInputFrameData FrameData;
		TMap<FName, FTouchTextureImportResult> ImportResults;
	};

}
