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

#include "ID3D12DynamicRHI.h"
#include "RHI.h"
#include "TextureResource.h"

#include "Util/TouchEngineStatsGroup.h"
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
#include "Launch/Resources/Version.h"

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
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.a [GT] Cook Frame - D3D12::CreateTexture"), STAT_TE_I_B_1_a_D3D, STATGROUP_TouchEngine);
		using namespace Private;
		FTexture2DRHIRef SharedTextureRHI;
		
		const FGuid ResourceId = FGuid::NewGuid();
		const FString ResourceIdString = GenerateIdentifierString(ResourceId);

		const int32 SizeX = SourceRHI.GetSizeX();
		const int32 SizeY = SourceRHI.GetSizeY();
		const EPixelFormat Format = SourceRHI.GetFormat();
		const int32 NumMips = SourceRHI.GetNumMips();
		const int32 NumSamples = SourceRHI.GetNumSamples();

		// The code below is to check that the texture is copied properly by outputting the TopLeft pixel color. Check FTouchImportTextureD3D12::CopyTexture_RenderThread
		// ENQUEUE_RENDER_COMMAND(TL)([RHI = const_cast<FRHITexture2D*>(&SourceRHI)](FRHICommandListImmediate& RHICmdList)
		// {
		// 	RHICmdList.EnqueueLambda([RHI](FRHICommandListImmediate& RHICommandList)
		// 	{
		// 		FColor Color;
		// 		GetRHITopLeftPixelColor(RHI, Color);
		// 		UE_LOG(LogTemp, Error, TEXT("Export: TL color:  %s"), *Color.ToString())
		// 	});
		// });
		
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.b [GT] Cook Frame - D3D12::CreateTexture - Create_RHICreateTexture"), STAT_TE_I_B_1_b_D3D, STATGROUP_TouchEngine);

			FRHITextureCreateDesc TextureDesc = FRHITextureCreateDesc::Create2D(*FString::Printf(TEXT("Global %s %s"), *SourceRHI.GetName().ToString(), *ResourceIdString), SizeX, SizeY, Format)
				.SetNumMips(NumMips)
				.SetNumSamples(NumSamples)
				.SetFlags(TexCreate_Shared | TexCreate_ResolveTargetable);
			if (EnumHasAnyFlags(SourceRHI.GetDesc().Flags, ETextureCreateFlags::SRGB))
			{
				TextureDesc.AddFlags(ETextureCreateFlags::SRGB);
			}
			SharedTextureRHI = RHICreateTexture(TextureDesc);
		}
		if (!SharedTextureRHI.IsValid() || !SharedTextureRHI->IsValid())
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("Failed to allocate RHI texture (X: %d, Y: %d, Format: %d, NumMips: %d, NumSamples: %d)"), SizeX, SizeY, static_cast<int32>(Format), NumMips, NumSamples);
			return nullptr;
		}
		
		HANDLE ResourceSharingHandle;
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.c [GT] Cook Frame - D3D12::CreateTexture - CreateSharedHandle"), STAT_TE_I_B_1_c_D3D, STATGROUP_TouchEngine);
			ID3D12Resource* ResolvedTexture = (ID3D12Resource*)SharedTextureRHI->GetTexture2D()->GetNativeResource();
			ID3D12Device* Device = (ID3D12Device*)GDynamicRHI->RHIGetNativeDevice();
			CHECK_HR_DEFAULT(Device->CreateSharedHandle(ResolvedTexture, *SharedResourceSecurityAttributes, GENERIC_ALL, *ResourceIdString, &ResourceSharingHandle));
		}
		
		TED3DSharedTexture* SharedTexture;
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.d [GT] Cook Frame - D3D12::CreateTexture - TED3DSharedTextureCreate"), STAT_TE_I_B_1_d_D3D, STATGROUP_TouchEngine);
			SharedTexture = TED3DSharedTextureCreate(
				ResourceSharingHandle,
				TED3DHandleTypeD3D12ResourceNT,
				ToTypedDXGIFormat(SharedTextureRHI->GetFormat(), EnumHasAnyFlags(SharedTextureRHI->GetFlags(), ETextureCreateFlags::SRGB)),
				SharedTextureRHI->GetSizeX(),
				SharedTextureRHI->GetSizeY(),
				TETextureOriginTopLeft,
				kTETextureComponentMapIdentity,
				nullptr,
				nullptr
			);
		}
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

	bool FExportedTextureD3D12::CanFitTexture(const FRHITexture* TextureToFit) const
	{
		return ensure(TextureToFit)
			&& TextureToFit->GetSizeXY() == SharedTextureRHI->GetSizeXY()
			&& TextureToFit->GetFormat() == SharedTextureRHI->GetFormat()
			&& TextureToFit->GetNumMips() == SharedTextureRHI->GetNumMips()
			&& TextureToFit->GetNumSamples() == SharedTextureRHI->GetNumSamples()
			&& EnumHasAnyFlags(TextureToFit->GetFlags(), ETextureCreateFlags::SRGB) == EnumHasAnyFlags(SharedTextureRHI->GetFlags(), ETextureCreateFlags::SRGB);
	}
	
	void FExportedTextureD3D12::RemoveTextureCallback()
	{
		TED3DSharedTexture* Casted = static_cast<TED3DSharedTexture*>(GetTouchRepresentation().get());
		TED3DSharedTextureSetCallback(Casted, nullptr, nullptr);
	}

	void FExportedTextureD3D12::TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info) //todo: this sometimes gets called after the object got destroyed, but somehow works
	{
		FExportedTextureD3D12* ExportedTexture = static_cast<FExportedTextureD3D12*>(Info);
		ExportedTexture->OnTouchTextureUseUpdate(Event);
	}
}
