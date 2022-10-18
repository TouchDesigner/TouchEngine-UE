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
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("TED3DSharedTextureCreate failed"));
			return nullptr; 
		}
		
		TouchObject<TED3DSharedTexture> TouchRepresentation;
		TouchRepresentation.set(SharedTexture);
		TSharedPtr<FExportedTextureD3D12> Result = MakeShared<FExportedTextureD3D12>(SharedTextureRHI, ResourceId, ResourceSharingHandle, TouchRepresentation, CallbackContext);
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

	FExportedTextureD3D12::~FExportedTextureD3D12()
	{
		// We must wait for TouchEngine to stop using the texture - FExportedTextureD3D12::Release implements that logic.
		UE_CLOG(CallbackContext->Instance != nullptr, LogTouchEngineD3D12RHI, Fatal, TEXT("You didn't let the destruction be handled by FExportedTextureD3D12::Release. TouchEngine callback will crash later because CallbackContext will have been cleared."));
	}

	TFuture<FExportedTextureD3D12::FOnTouchReleaseTexture> FExportedTextureD3D12::Release()
	{
		FScopeLock Lock(&TouchEngineMutex);
		if (!bIsInUseByTouchEngine)
		{
			return MakeFulfilledPromise<FOnTouchReleaseTexture>(FOnTouchReleaseTexture{}).GetFuture();
		}

		checkf(!ReleasePromise.IsSet(), TEXT("Release called twice."));
		TPromise<FOnTouchReleaseTexture> Promise;
		TFuture<FOnTouchReleaseTexture> Future = Promise.GetFuture();
		ReleasePromise = MoveTemp(Promise);
		return Future;
	}

	bool FExportedTextureD3D12::CanFitTexture(const FRHITexture2D& SourceRHI) const
	{
		return SourceRHI.GetSizeXY() == SharedTextureRHI->GetSizeXY()
			&& SourceRHI.GetFormat() == SharedTextureRHI->GetFormat()
			&& SourceRHI.GetNumMips() == SharedTextureRHI->GetNumMips()
			&& SourceRHI.GetNumSamples() == SharedTextureRHI->GetNumSamples();
	}

	void FExportedTextureD3D12::TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info)
	{
		using namespace Private;

		FTextureCallbackContext* CallbackContext = static_cast<FTextureCallbackContext*>(Info);
		TSharedPtr<FExportedTextureD3D12> ExportedTexture = CallbackContext->Instance.Pin();
		check(ExportedTexture);

		FScopeLock Lock(&ExportedTexture->TouchEngineMutex);
		switch (Event)
		{
		case TEObjectEventBeginUse:
			ExportedTexture->bIsInUseByTouchEngine = true;
			break;
			
		case TEObjectEventRelease:
		case TEObjectEventEndUse:
			ExportedTexture->bIsInUseByTouchEngine = false;
			if (ExportedTexture->ReleasePromise.IsSet())
			{
				ExportedTexture->ReleasePromise->SetValue({});
				ExportedTexture->ReleasePromise.Reset();
			}
			break;
		default: checkNoEntry(); break;
		}
	}
}
