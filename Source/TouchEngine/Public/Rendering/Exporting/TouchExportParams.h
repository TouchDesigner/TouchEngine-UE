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

class UTexture;

namespace UE::TouchEngine
{
	struct FTouchExportParameters
	{
		/** The instance from which the link request originates */
		TouchObject<TEInstance> Instance;
		
		/** The parameter name of the texture */
		FName ParameterName;

		/**
		 * If Texture was used as parameter in the past, it is safe to reuse that data.
		 * In that case, we will skip allocating a new texture resource and copying Texture into it:
		 * we'll just return the existing resource.
		 *
		 * Set to true if you never change the content of Texture (e.g. the pixels).
		 */
		bool bReuseExistingTexture = true;

		/** The texture to export */
		UTexture* Texture;
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