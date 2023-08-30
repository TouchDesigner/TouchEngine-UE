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
#include "RHI.h"
#include "TextureResource.h"
#include "RHICommandList.h"
#include "TouchImportParams.h"

#include "Async/Future.h"
#include "PixelFormat.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"

namespace UE::TouchEngine
{
	class FTouchTextureImporter;

	struct FTouchCopyTextureArgs
	{
		FTouchImportParameters RequestParams;
		FRHICommandListImmediate& RHICmdList;
		FTexture2DRHIRef TargetRHI;
	};

	struct FTextureMetaData
	{
		uint32 SizeX;
		uint32 SizeY;
		EPixelFormat PixelFormat;
		bool IsSRGB;
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
		bool CanCopyInto(const FTexture2DRHIRef& Target) const
		{
			if (Target)
			{
				const FTextureMetaData SrcInfo = GetTextureMetaData();
				return SrcInfo.SizeX == Target->GetSizeX()
					&& SrcInfo.SizeY == Target->GetSizeY()
					&& SrcInfo.PixelFormat == Target->GetFormat(); //do we need to check for sRGB?
			}
			return false;
		}
		
		virtual ECopyTouchToUnrealResult CopyNativeToUnrealRHI_RenderThread(const FTouchCopyTextureArgs& CopyArgs, TSharedRef<FTouchTextureImporter> Importer) = 0;
	};
}
