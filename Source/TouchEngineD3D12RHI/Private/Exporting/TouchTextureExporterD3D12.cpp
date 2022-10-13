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

#include "Exporting/TouchTextureExporterD3D12.h"

#include "Windows/PreWindowsApi.h"
#include "D3D12RHIPrivate.h"
#include "ExportedTextureD3D12.h"
#include "Windows/PostWindowsApi.h"

#include "Rendering/TouchExportParams.h"

#include "Engine/Texture.h"
#include "TouchEngine/TED3D.h"

namespace UE::TouchEngine::D3DX12
{
	FTouchExportResult FTouchTextureExporterD3D12::ExportTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTouchExportParameters& Params)
	{
		if (const TSharedPtr<FExportedTextureD3D12>* TextureData = CachedTextureData.Find(Params.Texture))
		{
			// TODO DP: We have to delete the old and create the texture anew - for testing we'll skip this step for now
			return FTouchExportResult{ ETouchExportErrorCode::Success, TextureData->Get()->GetTouchRepresentation() };
		}
		
		const TSharedPtr<FExportedTextureD3D12> TextureData = ShareTexture(Params.Texture);
		if (!TextureData)
		{
			return FTouchExportResult{ ETouchExportErrorCode::InternalD3D12Error };
		}
		
		FRHITexture2D* SourceRHI = GetRHIFromTexture(Params.Texture);
		RHICmdList.CopyTexture(SourceRHI, TextureData->GetSharedTextureRHI(), FRHICopyTextureInfo());
		// TODO: Synchronize
		
		return FTouchExportResult{ ETouchExportErrorCode::UnsupportedTextureObject };
	}

	TSharedPtr<FExportedTextureD3D12> FTouchTextureExporterD3D12::ShareTexture(UTexture* Texture)
	{
		const FRHITexture2D* TextureRHI = GetRHIFromTexture(Texture);
		if (!ensure(TextureRHI))
		{
			return {};
		}
		if (const TSharedPtr<FExportedTextureD3D12> Result = FExportedTextureD3D12::Create(*TextureRHI, SharedResourceSecurityAttributes))
		{
			CachedTextureData.Add(Texture, Result);
			return Result;
		}
		
		return {};
	}
}

#undef CHECK_HR_DEFAULT