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

#include "Windows/AllowWindowsPlatformTypes.h"
#include "TouchEngine/TED3D.h"
#include "Windows/HideWindowsPlatformTypes.h"

namespace UE::TouchEngine::D3DX12
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
	
	TArray<DXGI_FORMAT> GetFormatsInSameFormatGroup(DXGI_FORMAT Format)
	{
		switch (Format)
		{
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return { DXGI_FORMAT_BC1_TYPELESS, DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM_SRGB };

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
			return { DXGI_FORMAT_BC2_TYPELESS, DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM_SRGB };

		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			return { DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_TYPELESS, DXGI_FORMAT_BC3_UNORM_SRGB };

		case DXGI_FORMAT_BC4_SNORM:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_TYPELESS:
			return { DXGI_FORMAT_BC4_SNORM, DXGI_FORMAT_BC4_UNORM, DXGI_FORMAT_BC4_TYPELESS};

		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return { DXGI_FORMAT_R32G32B32A32_TYPELESS, DXGI_FORMAT_R32G32B32A32_SINT, DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_FLOAT };

		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
			return { DXGI_FORMAT_R16G16B16A16_SINT, DXGI_FORMAT_R16G16B16A16_UINT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_SNORM, DXGI_FORMAT_R16G16B16A16_UNORM, DXGI_FORMAT_R16G16B16A16_TYPELESS };
			
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return { DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_TYPELESS, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB };

		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_TYPELESS:
			return { DXGI_FORMAT_R16G16_SINT, DXGI_FORMAT_R16G16_UINT, DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_R16G16_SNORM, DXGI_FORMAT_R16G16_UNORM, DXGI_FORMAT_R16G16_TYPELESS };

		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_TYPELESS:
			return { DXGI_FORMAT_R32G32_SINT, DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32_TYPELESS };

		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
			return { DXGI_FORMAT_R10G10B10A2_UINT, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_TYPELESS };

		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_TYPELESS:
			return { DXGI_FORMAT_R16_SINT, DXGI_FORMAT_R16_SINT, DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16_SNORM, DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16_TYPELESS };

		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };

		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_TYPELESS:
			return { DXGI_FORMAT_R8G8_SINT, DXGI_FORMAT_R8G8_UINT, DXGI_FORMAT_R8G8_SNORM, DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_R8G8_TYPELESS };
			
		default:
			return {};
		}
	}
	
	TArray<DXGI_FORMAT> GetTypesSupportedByTouchEngine(TEInstance& Instance)
	{
		int32 Count = 0;
		const TEResult ResultGettingCount = TEInstanceGetSupportedD3DFormats(&Instance, nullptr, &Count);
		if (ResultGettingCount != TEResultInsufficientMemory)
		{
			return {};
		}

		TArray<DXGI_FORMAT> SupportedTypes;
		SupportedTypes.SetNumZeroed(Count);
		const TEResult ResultGettingTypes = TEInstanceGetSupportedD3DFormats(&Instance, SupportedTypes.GetData(), &Count);
		if (ResultGettingTypes != TEResultSuccess)
		{
			return {};
		}
		return SupportedTypes;
	}
	
	TArray<DXGI_FORMAT> GetAlternateFormatsInGroupWhichAreSupportedByTouchEngine(DXGI_FORMAT Format, const TArray<DXGI_FORMAT>& TypesSupportedByTE)
	{
		const TArray<DXGI_FORMAT> FormatsInSameGroup = GetFormatsInSameFormatGroup(Format);
		if (FormatsInSameGroup.Num() > 0)
		{
			TArray<DXGI_FORMAT> Result;
			Result.Reserve(FormatsInSameGroup.Num());
			for (DXGI_FORMAT FormatInGroup : FormatsInSameGroup)
			{
				if (TypesSupportedByTE.Contains(FormatInGroup))
				{
					Result.Add(FormatInGroup);
				}
			}
			return Result;
		}

		return {};
	}

	TOptional<DXGI_FORMAT> ConvertFormatToFormatSupportedByTouchEngine(DXGI_FORMAT Format, TEInstance& Instance)
	{
		if (const EPixelFormat SupportedOutOfTheBox = ConvertD3FormatToPixelFormat(Format); SupportedOutOfTheBox != PF_Unknown)
		{
			return Format;
		}

		const TArray<DXGI_FORMAT> AlternateFormatsInGroup = GetAlternateFormatsInGroupWhichAreSupportedByTouchEngine(Format, Instance);
		return AlternateFormatsInGroup.IsEmpty()
			? TOptional<DXGI_FORMAT>{}
			// We could prioritise different formats but for now let's just take the first one that is supported
			: AlternateFormatsInGroup[0];
	}
}
