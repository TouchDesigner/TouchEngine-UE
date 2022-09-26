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
}