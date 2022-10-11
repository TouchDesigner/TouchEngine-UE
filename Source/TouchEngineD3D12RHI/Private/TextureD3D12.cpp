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

#include "TextureD3D12.h"

#include "Windows/PreWindowsApi.h"
#include "D3D12RHIPrivate.h"
#include "Windows/PostWindowsApi.h"

#include "D3D12TouchUtils.h"
#include "TouchEngine/TED3D.h"

namespace UE::TouchEngine::D3DX12
{
	TSharedPtr<FTextureD3D12> FTextureD3D12::CreateTexture(ID3D12Device* Device, TED3DSharedTexture* Shared)
	{
		HANDLE h = TED3DSharedTextureGetHandle(Shared);
		check(TED3DSharedTextureGetHandleType(Shared) == TED3DHandleTypeD3D12ResourceNT);
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
		HRESULT SharedHandle = Device->OpenSharedHandle(h, IID_PPV_ARGS(&Resource));
		if (FAILED(SharedHandle))
		{
			return nullptr;
		}

		const EPixelFormat Format = ConvertD3FormatToPixelFormat(Resource->GetDesc().Format);
		FD3D12DynamicRHI* DynamicRHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
		const FTexture2DRHIRef SrcRHI = DynamicRHI->RHICreateTexture2DFromResource(Format, TexCreate_Shared, FClearValueBinding::None, Resource.Get()).GetReference();
		return MakeShareable<FTextureD3D12>(new FTextureD3D12(Resource, SrcRHI));
	}

	FTextureD3D12::FTextureD3D12(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, FTexture2DRHIRef TextureRHI)
		: Resource(MoveTemp(InResource))
		, Width(Resource->GetDesc().Width)
		, Height(Resource->GetDesc().Height)
		, PixelFormat(ConvertD3FormatToPixelFormat(Resource->GetDesc().Format))
		, TextureRHI(MoveTemp(TextureRHI))
	{}
}
