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
#include "TouchImportParams.h"

#include "Async/Future.h"
#include "PixelFormat.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"

namespace UE::TouchEngine
{
	struct FTouchCopyTextureArgs
	{
		FTouchImportParameters RequestParams;
		
		FRHICommandListImmediate& RHICmdList;
		UTexture2D* Target;
	};

	struct FTextureMetaData
	{
		uint32 SizeX;
		uint32 SizeY;
		EPixelFormat PixelFormat;
	};
	
	enum class ECopyTouchToUnrealResult
	{
		Success,
		Failure
	};

	/** Abstracts a texture that should be imported from Touch Engine to Unreal Engine */
	class ITouchImportTexture
	{
	public:

		virtual ~ITouchImportTexture() = default;

		virtual FTextureMetaData GetTextureMetaData() const = 0;
		bool CanCopyInto(const UTexture* Target) const
		{
			if (const UTexture2D* TargetTexture = Cast<UTexture2D>(Target))
			{
				const FTextureMetaData SrcInfo = GetTextureMetaData();
				return SrcInfo.SizeX == static_cast<uint32>(TargetTexture->GetSizeX())
					&& SrcInfo.SizeY == static_cast<uint32>(TargetTexture->GetSizeY())
					&& SrcInfo.PixelFormat == TargetTexture->GetPixelFormat();
			}
			return false;
		}
		
		virtual TFuture<ECopyTouchToUnrealResult> CopyNativeToUnreal_RenderThread(const FTouchCopyTextureArgs& CopyArgs) = 0;
	};
}
