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
#include "Modules/ModuleManager.h"
#include "ThirdParty/Windows/DirectX/include/dxgiformat.h"
#include "TouchEngine/TEInstance.h"

namespace UE::TouchEngine::D3DX12
{
	/** Convert DXGI_FORMAT to EPixelFormat */
	EPixelFormat ConvertD3FormatToPixelFormat(DXGI_FORMAT Format);

	/** Convert EPixelFormat to DXGI_FORMAT */
	DXGI_FORMAT ToTypedDXGIFormat(EPixelFormat Format);

	/**
	 *	Input e.g. DXGI_FORMAT_R16G16B16A16_TYPELESS and output contains
	 *	- DXGI_FORMAT_R16G16B16A16_TYPELESS
	 *	- DXGI_FORMAT_R16G16B16A16_FLOAT
	 *	- DXGI_FORMAT_R16G16B16A16_UNORM
	 *	- DXGI_FORMAT_R16G16B16A16_UINT
	 *	- DXGI_FORMAT_R16G16B16A16_SNORM
	 *	- DXGI_FORMAT_R16G16B16A16_SINT
	 */
	TArray<DXGI_FORMAT> GetFormatsInSameFormatGroup(DXGI_FORMAT Format);

	/** Gets all pixel formats supported by TE. */
	TArray<DXGI_FORMAT> GetTypesSupportedByTouchEngine(TEInstance& Instance);
	
	/**
	 * Converts the given format to one supported by Touch Engine, if possible.
	 * For example, TE may not support DXGI_FORMAT_R16G16B16A16_TYPELESS but could support DXGI_FORMAT_R16G16B16A16_FLOAT instead.
	 */
	TArray<DXGI_FORMAT> GetAlternateFormatsInGroupWhichAreSupportedByTouchEngine(DXGI_FORMAT Format, const TArray<DXGI_FORMAT>& TypesSupportedByTE);
	inline TArray<DXGI_FORMAT> GetAlternateFormatsInGroupWhichAreSupportedByTouchEngine(DXGI_FORMAT Format, TEInstance& Instance)
	{
		return GetAlternateFormatsInGroupWhichAreSupportedByTouchEngine(Format, GetTypesSupportedByTouchEngine(Instance));
	}

	TOptional<DXGI_FORMAT> ConvertFormatToFormatSupportedByTouchEngine(DXGI_FORMAT Format, TEInstance& Instance);
}
