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

#include "TouchTextureExporterD3D11.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include "d3d11.h"
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "D3D11TouchUtils.h"
#include "Rendering/Exporting/TouchExportParams.h"
#include "TouchEngine/TED3D11.h"
#include "TouchTextureImporterD3D11.h"

namespace UE::TouchEngine
{
	TFuture<FTouchExportResult> FTouchTextureExporterD3D11::ExportTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTouchExportParameters& Params)
	{
		FRHITexture2D* TextureRHI = GetRHIFromTexture(Params.Texture);
		const EPixelFormat Format = TextureRHI->GetFormat();
		ID3D11Texture2D* D3D11Texture = static_cast<ID3D11Texture2D*>(TextureRHI->GetNativeResource());
		const DXGI_FORMAT TypedDXGIFormat = D3DX11::ToTypedDXGIFormat(Format);
    
		D3D11_TEXTURE2D_DESC Desc;
		D3D11Texture->GetDesc(&Desc);

		TouchObject<TETexture> Result;
		if (D3DX11::IsTypeless(Desc.Format))
		{
			TED3D11Texture* ResultTexture = TED3D11TextureCreateTypeless(D3D11Texture, TETextureOriginTopLeft, kTETextureComponentMapIdentity, TypedDXGIFormat, nullptr, nullptr);
			Result.set(ResultTexture);
		}
		else
		{
			TED3D11Texture* ResultTexture = TED3D11TextureCreate(D3D11Texture, TETextureOriginTopLeft, kTETextureComponentMapIdentity, nullptr, nullptr);
			Result.set(ResultTexture);
		}

		return MakeFulfilledPromise<FTouchExportResult>(FTouchExportResult{ ETouchExportErrorCode::Success, Result }).GetFuture();
	}
}

