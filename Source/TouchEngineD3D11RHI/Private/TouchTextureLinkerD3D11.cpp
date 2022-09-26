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

#include "TouchTextureLinkerD3D11.h"

#include "D3D11TouchUtils.h"
#include "TouchEngine/TED3D.h"

namespace UE::TouchEngine::D3DX11
{
	FTouchTextureLinkerD3D11::FTouchTextureLinkerD3D11()
	{
	}

	int32 FTouchTextureLinkerD3D11::GetSharedTextureWidth(TETexture* Texture) const
	{
		const TED3DSharedTexture* TextureDX = static_cast<TED3DSharedTexture*>(Texture);
		return TED3DSharedTextureGetWidth(TextureDX);
	}

	int32 FTouchTextureLinkerD3D11::GetSharedTextureHeight(TETexture* Texture) const
	{
		const TED3DSharedTexture* TextureDX = static_cast<TED3DSharedTexture*>(Texture);
		return TED3DSharedTextureGetHeight(TextureDX);
	}

	EPixelFormat FTouchTextureLinkerD3D11::GetSharedTexturePixelFormat(TETexture* Texture) const
	{
		const TED3DSharedTexture* TextureDX = static_cast<TED3DSharedTexture*>(Texture);
		const DXGI_FORMAT FormatDX = TED3DSharedTextureGetFormat(TextureDX);
		return ConvertD3FormatToPixelFormat(FormatDX);
	}
}
