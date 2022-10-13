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
		struct FTextureCallbackContext
		{
			TWeakPtr<FExportedTextureD3D12> Instance;
		};
	}
	
	TSharedPtr<FExportedTextureD3D12> FExportedTextureD3D12::Create(const FRHITexture2D& SourceRHI, const FTextureShareD3D12SharedResourceSecurityAttributes& SharedResourceSecurityAttributes)
	{
		using namespace Private;
		const FGuid ResourceId = FGuid::NewGuid();
		const FString ResourceIdString = GenerateIdentifierString(ResourceId);
		
		FRHIResourceCreateInfo CreateInfo(*FString::Printf(TEXT("TouchCopy_%s_%s"), *SourceRHI.GetName().ToString(), *ResourceIdString));
		const FTexture2DRHIRef SharedTextureRHI = GDynamicRHI->RHICreateTexture2D(
			SourceRHI.GetSizeX(), SourceRHI.GetSizeY(), SourceRHI.GetFormat(), SourceRHI.GetNumMips(), SourceRHI.GetNumSamples(), TexCreate_Shared, ERHIAccess::CopyDest, CreateInfo
			);
		
		ID3D12Resource* ResolvedTexture = (ID3D12Resource*)SharedTextureRHI->GetNativeResource();
		ID3D12Device* Device = (ID3D12Device*)GDynamicRHI->RHIGetNativeDevice();
		HANDLE ResourceSharingHandle;
		CHECK_HR_DEFAULT(Device->CreateSharedHandle(ResolvedTexture, *SharedResourceSecurityAttributes, GENERIC_ALL, *ResourceIdString, &ResourceSharingHandle));

		// We need to pass in an info object below - and we only want to construct with a valid state (no failing in constructor) so we need indirection using FExportedTextureD3D12
		const TSharedRef<FTextureCallbackContext> CallbackContext = MakeShared<FTextureCallbackContext>();
		TED3DSharedTexture* SharedTexture = TED3DSharedTextureCreate(
			ResourceSharingHandle,
			TED3DHandleTypeD3D12ResourceNT,
			ToTypedDXGIFormat(SharedTextureRHI->GetFormat()),
			SharedTextureRHI->GetSizeX(),
			SharedTextureRHI->GetSizeY(),
			TETextureOriginTopLeft,
			kTETextureComponentMapIdentity,
			TouchTextureCallback,
			&CallbackContext.Get()
			);
		if (!SharedTexture)
		{
			return {}; 
		}
		
		TouchObject<TED3DSharedTexture> TouchRepresentation;
		TouchRepresentation.set(SharedTexture);
		const TSharedRef<FExportedTextureD3D12> Result = MakeShared<FExportedTextureD3D12>(SharedTextureRHI, ResourceId, ResourceSharingHandle, TouchRepresentation, CallbackContext);
		CallbackContext->Instance = Result;
		return Result;
	}

	FExportedTextureD3D12::FExportedTextureD3D12(FTexture2DRHIRef SharedTextureRHI, const FGuid& ResourceId, void* ResourceSharingHandle, TouchObject<TED3DSharedTexture> TouchRepresentation, TSharedRef<Private::FTextureCallbackContext> CallbackContext)
		: SharedTextureRHI(MoveTemp(SharedTextureRHI))
		, ResourceId(ResourceId)
		, ResourceSharingHandle(ResourceSharingHandle)
		, TouchRepresentation(MoveTemp(TouchRepresentation))
		, CallbackContext(MoveTemp(CallbackContext))
	{}
	
	void FExportedTextureD3D12::TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info)
	{
		
	}
}
