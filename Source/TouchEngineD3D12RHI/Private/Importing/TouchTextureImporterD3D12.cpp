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

#include "TouchTextureImporterD3D12.h"

#include "D3D12TouchUtils.h"
#include "Logging.h"
#include "TouchImportTextureD3D12.h"
#include "TouchEngine/TED3D.h"
#include "Exporting/TextureShareD3D12PlatformWindows.h"

// macro to deal with COM calls inside a function that returns on failure
#define CHECK_HR_DEFAULT(COM_call)\
	{\
		HRESULT Res = COM_call;\
		if (FAILED(Res))\
		{\
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("`" #COM_call "` failed: 0x%X - %s"), Res, *GetComErrorDescription(Res)); \
			return;\
		}\
	}

namespace UE::TouchEngine::D3DX12
{
	FTouchTextureImporterD3D12::FTouchTextureImporterD3D12(ID3D12Device* Device, TSharedRef<FTouchFenceCache> FenceCache)
		: Device(Device)
		, FenceCache(MoveTemp(FenceCache))
	{
	}

	TSharedPtr<ITouchImportTexture> FTouchTextureImporterD3D12::CreatePlatformTexture_RenderThread(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture)
	{
		check(TETextureGetType(SharedTexture) == TETextureTypeD3DShared);
		TED3DSharedTexture* Shared = static_cast<TED3DSharedTexture*>(SharedTexture.get());
		const HANDLE Handle = TED3DSharedTextureGetHandle(Shared);
		if (const TSharedPtr<FTouchImportTextureD3D12> Existing = GetSharedTexture(Handle))
		{
			return Existing;
		}

		const TSharedPtr<FTouchImportTextureD3D12> NewTexture = FTouchImportTextureD3D12::CreateTexture_RenderThread(Device, Shared, FenceCache);
		if (!NewTexture)
		{
			return nullptr;
		}

		TED3DSharedTextureSetCallback(Shared, TextureCallback, this);
		CachedTextures.Add(Handle, NewTexture.ToSharedRef());

		return StaticCastSharedPtr<ITouchImportTexture>(NewTexture);
	}

	FTextureMetaData FTouchTextureImporterD3D12::GetTextureMetaData(const TouchObject<TETexture>& Texture) const
	{
		const TED3DSharedTexture* Source = static_cast<TED3DSharedTexture*>(Texture.get());
		const DXGI_FORMAT Format = TED3DSharedTextureGetFormat(Source);
		FTextureMetaData Result;
		Result.SizeX = TED3DSharedTextureGetWidth(Source);
		Result.SizeY = TED3DSharedTextureGetHeight(Source);
		Result.PixelFormat = ConvertD3FormatToPixelFormat(Format, Result.IsSRGB);
		// UE_LOG(LogTouchEngineD3D12RHI, Verbose, TEXT("TextureFormat: %s [%d]"), GetD3D12TextureFormatString(Format), Format);
		return Result;
	}
	
	TSharedPtr<FTouchImportTextureD3D12> FTouchTextureImporterD3D12::GetSharedTexture(HANDLE Handle) const
	{
		const TSharedRef<FTouchImportTextureD3D12>* Result = CachedTextures.Find(Handle);
		return Result
			? *Result
			: TSharedPtr<FTouchImportTextureD3D12>{ nullptr };
	}

	void FTouchTextureImporterD3D12::TextureCallback(HANDLE Handle, TEObjectEvent Event, void* Info)
	{
		if (Event == TEObjectEventRelease)
		{
			FTouchTextureImporterD3D12* This = static_cast<FTouchTextureImporterD3D12*>(Info);
			This->CachedTextures.Remove(Handle);
		}
	}
}

