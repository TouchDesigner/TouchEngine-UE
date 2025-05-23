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
#include "TouchEngine/TouchObject.h"
#include "Blueprint/TouchEngineInputFrameData.h"
#include "Engine/Texture.h"
#include "Rendering/TouchTextureTransfer.h"

class UTexture;

namespace UE::TouchEngine
{
	class FExportedTouchTexture;
	struct FTextureRHIStruct;

	struct FTouchExportParameters
	{
		/** The instance from which the link request originates */
		TouchObject<TEInstance> Instance;
		
		/** The parameter name of the texture */
		FName ParameterName;
		
		/** The texture to export */
		TSharedRef<UE::TouchEngine::FExportedTouchTexture> TextureToBeExported;

		FTouchEngineInputFrameData FrameData;

		FString GetDebugDescription() const
		{
			return FString::Printf(TEXT("[Instance: '%p', Input: '%s', Frame: '%lld']"), Instance.get(), *ParameterName.ToString(), FrameData.FrameID);
		}
	};

	enum class ETouchExportErrorCode
	{
		Success,
		Cancelled,
		
		/** Passed in UTexture is not of supported type */
		UnsupportedTextureObject,
		/** Pixel format of UTexture is not supported */
		UnsupportedPixelFormat,

		/** Could not create the resource required to share with TE. Usually some DX12 / Vulkan error. */
		InternalGraphicsDriverError,

		/** Failed to transfer the texture to TE (TEInstanceAddTextureTransfer failed) */
		FailedTextureTransfer,
		
		UnsupportedOperation,

		UnknownFailure,
		Count
	};
	
	struct FTouchExportResult
	{
		ETouchExportErrorCode ErrorCode;
		TouchObject<TETexture> Texture;
	};
}