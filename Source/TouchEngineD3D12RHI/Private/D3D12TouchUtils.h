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
#include "PixelFormat.h"
#include "ThirdParty/Windows/DirectX/include/dxgiformat.h"
#include "RHIResources.h"

namespace UE::TouchEngine::D3DX12
{
	// Copy from D3D12Util.h: GetPlatformTextureResourceFormat
	const TCHAR* GetD3D12TextureFormatString(DXGI_FORMAT TextureFormat);

	/** Convert DXGI_FORMAT to EPixelFormat. bIsSRGB will return true if the passed format was sRGB */
	EPixelFormat ConvertD3FormatToPixelFormat(DXGI_FORMAT Format, bool& bIsSRGB);

	/** Convert EPixelFormat to DXGI_FORMAT. bIsSRGB will return true if the passed format was sRGB */
	DXGI_FORMAT ToTypedDXGIFormat(EPixelFormat Format, bool bIsSRGB);

	/** Is this a typeless DXGI_FORMAT format? */
	bool IsTypeless(DXGI_FORMAT Format);
}
