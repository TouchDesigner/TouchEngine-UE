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

#include "D3D11TouchUtils.h"

namespace UE::TouchEngine::D3DX11
{
	EPixelFormat ConvertD3FormatToPixelFormat(DXGI_FORMAT Format)
	{
#define MAP_FORMAT(DX_Format, UE_Format) \
	case DX_Format: return UE_Format;

		switch (Format)
		{
			MAP_FORMAT(DXGI_FORMAT_R8_UNORM,           PF_G8)
			MAP_FORMAT(DXGI_FORMAT_R16_UNORM,          PF_G16)
			MAP_FORMAT(DXGI_FORMAT_R32G32B32A32_FLOAT, PF_A32B32G32R32F)
			MAP_FORMAT(DXGI_FORMAT_B8G8R8A8_UNORM,     PF_B8G8R8A8)
			MAP_FORMAT(DXGI_FORMAT_R16G16B16A16_FLOAT, PF_FloatRGBA)
			MAP_FORMAT(DXGI_FORMAT_R32_FLOAT,          PF_R32_FLOAT)
			MAP_FORMAT(DXGI_FORMAT_R16G16_UNORM,       PF_G16R16)
			MAP_FORMAT(DXGI_FORMAT_R16G16_FLOAT,       PF_G16R16F)
			MAP_FORMAT(DXGI_FORMAT_R32G32_FLOAT,       PF_G32R32F)
			MAP_FORMAT(DXGI_FORMAT_R10G10B10A2_UNORM,  PF_A2B10G10R10)
			MAP_FORMAT(DXGI_FORMAT_R16_FLOAT,          PF_R16F)
			MAP_FORMAT(DXGI_FORMAT_R11G11B10_FLOAT,    PF_FloatR11G11B10)
			MAP_FORMAT(DXGI_FORMAT_R8G8B8A8_UNORM,     PF_R8G8B8A8)
			MAP_FORMAT(DXGI_FORMAT_R8G8_UNORM,         PF_R8G8)
			MAP_FORMAT(DXGI_FORMAT_R16G16B16A16_UNORM, PF_R16G16B16A16_UNORM)

			// Compressed types
			MAP_FORMAT(DXGI_FORMAT_B8G8R8A8_TYPELESS,  PF_B8G8R8A8)
			MAP_FORMAT(DXGI_FORMAT_BC1_TYPELESS,       PF_DXT1)
			MAP_FORMAT(DXGI_FORMAT_BC2_TYPELESS,       PF_DXT3)
			MAP_FORMAT(DXGI_FORMAT_BC3_TYPELESS,       PF_DXT5)
			MAP_FORMAT(DXGI_FORMAT_BC4_UNORM,          PF_BC4)

			default:
				return PF_Unknown;
		}

#undef MAP_FORMAT
	}
	
	DXGI_FORMAT ToTypedDXGIFormat(EPixelFormat Format)
	{
#define MAP_FORMAT(UE_Format, DX_Format) \
	case UE_Format: return DX_Format;

		switch (Format)
		{
			MAP_FORMAT(PF_G8,                 DXGI_FORMAT_R8_UNORM)
			MAP_FORMAT(PF_G16,                DXGI_FORMAT_R16_UNORM)
			MAP_FORMAT(PF_A32B32G32R32F,      DXGI_FORMAT_R32G32B32A32_FLOAT)
			MAP_FORMAT(PF_B8G8R8A8,           DXGI_FORMAT_B8G8R8A8_UNORM)
			MAP_FORMAT(PF_FloatRGB,           DXGI_FORMAT_R16G16B16A16_FLOAT)
			MAP_FORMAT(PF_FloatRGBA,          DXGI_FORMAT_R16G16B16A16_FLOAT)
			MAP_FORMAT(PF_R32_FLOAT,          DXGI_FORMAT_R32_FLOAT)
			MAP_FORMAT(PF_G16R16,             DXGI_FORMAT_R16G16_UNORM)
			MAP_FORMAT(PF_G16R16F,            DXGI_FORMAT_R16G16_FLOAT)
			MAP_FORMAT(PF_G32R32F,            DXGI_FORMAT_R32G32_FLOAT)
			MAP_FORMAT(PF_A2B10G10R10,        DXGI_FORMAT_R10G10B10A2_UNORM)
			MAP_FORMAT(PF_R16F,               DXGI_FORMAT_R16_FLOAT)
			MAP_FORMAT(PF_FloatR11G11B10,     DXGI_FORMAT_R11G11B10_FLOAT)
			MAP_FORMAT(PF_R8G8B8A8,           DXGI_FORMAT_R8G8B8A8_UNORM)
			MAP_FORMAT(PF_R8G8,               DXGI_FORMAT_R8G8_UNORM)
			MAP_FORMAT(PF_A16B16G16R16,       DXGI_FORMAT_R16G16B16A16_UNORM)
			MAP_FORMAT(PF_R16G16B16A16_UNORM, DXGI_FORMAT_R16G16B16A16_UNORM)

			// Compressed types
			MAP_FORMAT(PF_DXT1,               DXGI_FORMAT_BC1_TYPELESS)
			MAP_FORMAT(PF_DXT3,               DXGI_FORMAT_BC2_TYPELESS)
			MAP_FORMAT(PF_DXT5,               DXGI_FORMAT_BC3_TYPELESS)
			MAP_FORMAT(PF_BC4,                DXGI_FORMAT_BC4_UNORM)

	   default:
		return DXGI_FORMAT_UNKNOWN;
		}

#undef MAP_FORMAT
	}

	bool IsTypeless(DXGI_FORMAT Format)
	{
		switch (Format)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC7_TYPELESS:
			return true;
		default:
			return false;
		}
	}
}