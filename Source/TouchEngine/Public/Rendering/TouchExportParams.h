// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"
#include "TouchEngine/TouchObject.h"

class UTexture;

namespace UE::TouchEngine
{
	struct FTouchExportParameters
	{
		const TETextureComponentMap& kTETextureComponentMapIdentity;
		
		/** The parameter name of the texture */
		FName ParameterName;

		/** The texture to export */
		UTexture* Texture;
	};

	enum class ETouchExportErrorCode
	{
		Success,
		Cancelled,
		
		/** Passed in UTexture is not of supported type */
		UnsupportedTextureObject,
		UnsupportedPixelFormat,

		Count
	};
	
	struct FTouchExportResult
	{
		ETouchExportErrorCode ErrorCode;
		TouchObject<TETexture> Texture;
	};
}