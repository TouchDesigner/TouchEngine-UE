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

#include "ExportedTextureD3D12.h"

#include "Engine/Util/TouchErrorLog.h"
#include "Rendering/Exporting/TouchExportParams.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
THIRD_PARTY_INCLUDES_START
#include "d3d12.h"
THIRD_PARTY_INCLUDES_END
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "D3D12TouchUtils.h"
#include "Logging.h"
#include "TextureShareD3D12PlatformWindows.h"

#include "Engine/Texture.h"
#include "Engine/Texture2D.h"

#include "TouchEngine/TED3D.h"

// macro to deal with COM calls inside a function that returns {} on failure
#define CHECK_HR_DEFAULT(COM_call)\
	{\
		HRESULT Res = COM_call;\
		if (FAILED(Res))\
		{\
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("`" #COM_call "` failed: 0x%X - %s"), Res, *GetComErrorDescription(Res)); \
			return {};\
		}\
	}

namespace UE::TouchEngine::D3DX12
{
	namespace Private
	{
		static FString GenerateIdentifierString(const FGuid& ResourceId)
		{
			return ResourceId.ToString(EGuidFormats::DigitsWithHyphensInBraces);
		}
	}
	
	TSharedPtr<FExportedTextureD3D12> FExportedTextureD3D12::Create(const FRHITexture2D& SourceRHI, const FTextureShareD3D12SharedResourceSecurityAttributes& SharedResourceSecurityAttributes)
	{
		using namespace Private;
		const FGuid ResourceId = FGuid::NewGuid();
		const FString ResourceIdString = GenerateIdentifierString(ResourceId);

		// Name should start with Global or Local https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createsharedhandle
		FRHIResourceCreateInfo CreateInfo(*FString::Printf(TEXT("Global %s %s"), *SourceRHI.GetName().ToString(), *ResourceIdString));
		const int32 SizeX = SourceRHI.GetSizeX();
		const int32 SizeY = SourceRHI.GetSizeY();
		const EPixelFormat Format = SourceRHI.GetFormat();
		const int32 NumMips = SourceRHI.GetNumMips();
		const int32 NumSamples = SourceRHI.GetNumSamples();
		const FTexture2DRHIRef SharedTextureRHI = RHICreateTexture2D(
			SizeX, SizeY, Format, NumMips, NumSamples, TexCreate_Shared | TexCreate_ResolveTargetable, CreateInfo
			);
		if (!SharedTextureRHI.IsValid() || !SharedTextureRHI->IsValid())
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("Failed to allocate RHI texture (X: %d, Y: %d, Format: %d, NumMips: %d, NumSamples: %d)"), SizeX, SizeY, static_cast<int32>(Format), NumMips, NumSamples);
			return nullptr;
		}
		
		ID3D12Resource* ResolvedTexture = (ID3D12Resource*)SharedTextureRHI->GetTexture2D()->GetNativeResource();
		ID3D12Device* Device = (ID3D12Device*)GDynamicRHI->RHIGetNativeDevice();
		HANDLE ResourceSharingHandle;
		CHECK_HR_DEFAULT(Device->CreateSharedHandle(ResolvedTexture, *SharedResourceSecurityAttributes, GENERIC_ALL, *ResourceIdString, &ResourceSharingHandle));
		
		TED3DSharedTexture* SharedTexture = TED3DSharedTextureCreate(
			ResourceSharingHandle,
			TED3DHandleTypeD3D12ResourceNT,
			ToTypedDXGIFormat(SharedTextureRHI->GetFormat()),
			SharedTextureRHI->GetSizeX(),
			SharedTextureRHI->GetSizeY(),
			TETextureOriginTopLeft,
			kTETextureComponentMapIdentity,
			nullptr,
			nullptr
			);
		if (!SharedTexture)
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("TED3DSharedTextureCreate failed"));
			return nullptr; 
		}
		
		TouchObject<TED3DSharedTexture> TouchRepresentation;
		TouchRepresentation.take(SharedTexture);
		return MakeShared<FExportedTextureD3D12>(SharedTextureRHI, ResourceId, ResourceSharingHandle, TouchRepresentation);
	}

	FExportedTextureD3D12::FExportedTextureD3D12(FTexture2DRHIRef SharedTextureRHI, const FGuid& ResourceId, void* ResourceSharingHandle, TouchObject<TED3DSharedTexture> TouchRepresentation)
		: FExportedTouchTexture(TouchRepresentation, [this](const TouchObject<TETexture>& Texture)
		{
			TED3DSharedTexture* Casted = static_cast<TED3DSharedTexture*>(Texture.get());
			TED3DSharedTextureSetCallback(Casted, TouchTextureCallback, this);
		})
		, SharedTextureRHI(MoveTemp(SharedTextureRHI))
		, ResourceId(ResourceId)
		, ResourceSharingHandle(ResourceSharingHandle)
	{}

	bool FExportedTextureD3D12::CanFitTexture(const FTouchExportParameters& Params) const
	{
		const FRHITexture2D& SourceRHI = *Params.Texture->GetResource()->TextureRHI->GetTexture2D();
		return SourceRHI.GetSizeXY() == SharedTextureRHI->GetSizeXY()
			&& SourceRHI.GetFormat() == SharedTextureRHI->GetFormat()
			&& SourceRHI.GetNumMips() == SharedTextureRHI->GetNumMips()
			&& SourceRHI.GetNumSamples() == SharedTextureRHI->GetNumSamples();
	}

	void FExportedTextureD3D12::TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info)
	{
		FExportedTextureD3D12* ExportedTexture = static_cast<FExportedTextureD3D12*>(Info);
		ExportedTexture->OnTouchTextureUseUpdate(Event);
	}
}
