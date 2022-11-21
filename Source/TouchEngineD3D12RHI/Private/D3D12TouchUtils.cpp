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

#include "D3D12TouchUtils.h"

namespace UE::TouchEngine::D3DX12
{
	EPixelFormat ConvertD3FormatToPixelFormat(DXGI_FORMAT Format)
	{
		switch (Format)
		{
			case DXGI_FORMAT_R8_UNORM:
				return PF_G8;
			case DXGI_FORMAT_R16_UNORM:
				return PF_G16;
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
				return PF_A32B32G32R32F;
			case DXGI_FORMAT_B8G8R8A8_UNORM:
				return PF_B8G8R8A8;
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			   return PF_FloatRGBA;
			case DXGI_FORMAT_R32_FLOAT:
				return PF_R32_FLOAT;
			case DXGI_FORMAT_R16G16_UNORM:
				return PF_G16R16;
			case DXGI_FORMAT_R16G16_FLOAT:
				return PF_G16R16F;
			case DXGI_FORMAT_R32G32_FLOAT:
				return PF_G32R32F;
			case DXGI_FORMAT_R10G10B10A2_UNORM:
				return PF_A2B10G10R10;
			case DXGI_FORMAT_R16_FLOAT:
				return PF_R16F;
			case DXGI_FORMAT_R11G11B10_FLOAT:
				return PF_FloatR11G11B10;
			case DXGI_FORMAT_R8G8B8A8_UNORM:
				return PF_R8G8B8A8;
			case DXGI_FORMAT_R8G8_UNORM:
				return PF_R8G8;
			case DXGI_FORMAT_R16G16B16A16_UNORM:
				return PF_R16G16B16A16_UNORM;
			default:
				return PF_Unknown;
		}
	}
	
	DXGI_FORMAT ToTypedDXGIFormat(EPixelFormat Format)
	{
 		switch (Format)
 		{
 		case PF_DXT1:
		case PF_DXT3:
		case PF_DXT5:
		default:
			return DXGI_FORMAT_UNKNOWN;
 		case PF_G8:
 			return DXGI_FORMAT_R8_UNORM;
 		case PF_G16:
 			return DXGI_FORMAT_R16_UNORM;
 		case PF_A32B32G32R32F:
 			return DXGI_FORMAT_R32G32B32A32_FLOAT;
 		case PF_B8G8R8A8:
 			return DXGI_FORMAT_B8G8R8A8_UNORM;
 		case PF_FloatRGB:
 		case PF_FloatRGBA:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
 		case PF_R32_FLOAT:
 			return DXGI_FORMAT_R32_FLOAT;
 		case PF_G16R16:
 			return DXGI_FORMAT_R16G16_UNORM;
 		case PF_G16R16F:
 			return DXGI_FORMAT_R16G16_FLOAT;
 		case PF_G32R32F:
 			return DXGI_FORMAT_R32G32_FLOAT;
 		case PF_A2B10G10R10:
 			return DXGI_FORMAT_R10G10B10A2_UNORM;
 		case PF_A16B16G16R16:
 			return DXGI_FORMAT_R16G16B16A16_UNORM;
 		case PF_R16F:
 			return DXGI_FORMAT_R16_FLOAT;
 		case PF_FloatR11G11B10:
 			return DXGI_FORMAT_R11G11B10_FLOAT;
 		case PF_R8G8B8A8:
 			return DXGI_FORMAT_R8G8B8A8_UNORM;
 		case PF_R8G8:
 			return DXGI_FORMAT_R8G8_UNORM;
 		case PF_R16G16B16A16_UNORM:
 			return DXGI_FORMAT_R16G16B16A16_UNORM;
 		}
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