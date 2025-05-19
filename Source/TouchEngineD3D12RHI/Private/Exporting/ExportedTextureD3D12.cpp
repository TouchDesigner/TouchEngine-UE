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

#include "RenderingThread.h"
#include "ID3D12DynamicRHI.h"
#include "RHI.h"
#include "TextureResource.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchTextureExporterD3D12.h"
#include "Engine/TEDebug.h"
#include "TouchEngine/Public/Logging.h"

#include "Util/TouchEngineStatsGroup.h"
#include "Windows/AllowWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_START
#include "d3d12.h"
THIRD_PARTY_INCLUDES_END
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
			return false;\
		}\
	}

namespace UE::TouchEngine::D3DX12
{
	
	TSharedPtr<FExportedTextureD3D12> FExportedTextureD3D12::Create(const TSharedRef<FTouchTextureExporterD3D12>& InExporter, UTexture* InTexture)
	{
		if (!IsValid(InTexture))
		{
			return nullptr;
		}

		TSharedRef<FExportedTextureD3D12> ExportedTexture = MakeShared<FExportedTextureD3D12>(InExporter);

		FTextureResource* SourceTextureResource = InTexture->GetResource();
		
		ENQUEUE_RENDER_COMMAND(ExportedTextureD3D12CreateTexture)([SourceTextureResource, WeakThis = ExportedTexture->AsWeak()](FRHICommandListImmediate& RHICmdList) mutable
		{
			TSharedPtr<FExportedTextureD3D12> This = StaticCastSharedPtr<FExportedTextureD3D12>(WeakThis.Pin());
			if (!This)
			{
				return;
			}
			
			const FTextureRHIRef& SourceTextureRHI = SourceTextureResource->GetTextureRHI();
			if (!SourceTextureRHI.IsValid())
			{
				UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("Failed to retrieve RHI of texture '%s'"), *This->DebugName);
				return;
			}
			
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.a [GT] Cook Frame - D3D12::CreateTexture"), STAT_TE_I_B_1_a_D3D, STATGROUP_TouchEngine);
			
			const FRHITextureDesc& ExistingTextureDesc = SourceTextureResource->GetTextureRHI()->GetDesc();

			FRHITextureCreateDesc TextureDesc = FRHITextureCreateDesc::Create2D(
				*FString::Printf(TEXT("ExportedTextureD3D12 %s"), *This->DebugName),
				ExistingTextureDesc.Extent.X, ExistingTextureDesc.Extent.Y,
				ExistingTextureDesc.Format
			)
				.SetNumMips(ExistingTextureDesc.NumMips)
				.SetNumSamples(ExistingTextureDesc.NumSamples)
				.SetFlags(TexCreate_Shared | TexCreate_RenderTargetable | TexCreate_External);
			
			if (EnumHasAnyFlags(ExistingTextureDesc.Flags, ETextureCreateFlags::SRGB))
			{
				TextureDesc.AddFlags(ETextureCreateFlags::SRGB);
			}

			FTextureRHIRef NewRHI = RHICmdList.CreateTexture(TextureDesc);
			This->SetTextureRHI_RenderThread(NewRHI);
		});
		
		return ExportedTexture;
	}

	bool FExportedTextureD3D12::ShareTexture_RenderThread(const TSharedRef<FTextureShareD3D12SharedResourceSecurityAttributes>& SharedResourceSecurityAttributes)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.s [GT] Cook Frame - D3D12::ShareTexture"), STAT_TE_I_B_1_s_D3D, STATGROUP_TouchEngine);

		/** Used to handle the ID of the resource */
		const FGuid ResourceId = FGuid::NewGuid();
		const FString ResourceIdString = ResourceId.ToString(EGuidFormats::DigitsWithHyphensInBraces);

		HANDLE SharingHandle;
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.t [GT] Cook Frame - D3D12::ShareTexture - CreateSharedHandle"), STAT_TE_I_B_1_t_D3D, STATGROUP_TouchEngine);
			ID3D12Resource* ResolvedTexture = static_cast<ID3D12Resource*>(GetSharedTextureRHI_RenderThread()->GetTexture2D()->GetNativeResource());
			ID3D12Device* Device = static_cast<ID3D12Device*>(GDynamicRHI->RHIGetNativeDevice());
			{
				CHECK_HR_DEFAULT(Device->CreateSharedHandle(ResolvedTexture, *SharedResourceSecurityAttributes.Get(), GENERIC_ALL, *ResourceIdString, &SharingHandle))
			};
		}

		TED3DSharedTexture* SharedTexture;
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.u [GT] Cook Frame - D3D12::ShareTexture - TED3DSharedTextureCreate"), STAT_TE_I_B_1_u_D3D, STATGROUP_TouchEngine);
			SharedTexture = TED3DSharedTextureCreate(
				SharingHandle,
				TED3DHandleTypeD3D12ResourceNT,
				ToTypedDXGIFormat(GetSharedTextureRHI_RenderThread()->GetFormat(), EnumHasAnyFlags(GetSharedTextureRHI_RenderThread()->GetFlags(), ETextureCreateFlags::SRGB)),
				GetSharedTextureRHI_RenderThread()->GetSizeX(),
				GetSharedTextureRHI_RenderThread()->GetSizeY(),
				TETextureOriginTopLeft,
				kTETextureComponentMapIdentity,
				nullptr,
				nullptr
			);
			const void* NullPointer = nullptr;
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEVulkanTextureCreate(textureHandle: '%p' [UE: '%s'], type: '%d', format: '%d', width: '%d', height: '%d', origin: '%s', map: '%s', callback: '%p', info: '%p') [Thread: '%s']  =>  Returned '%p'"),
				SharingHandle,
				*DebugName,
				TED3DHandleTypeD3D12ResourceNT,
				ToTypedDXGIFormat(GetSharedTextureRHI_RenderThread()->GetFormat(), EnumHasAnyFlags(GetSharedTextureRHI_RenderThread()->GetFlags(), ETextureCreateFlags::SRGB)),
				GetSharedTextureRHI_RenderThread()->GetSizeX(),
				GetSharedTextureRHI_RenderThread()->GetSizeY(),
				TEXT("TETextureOriginTopLeft"),
				*FString::Printf(TEXT("[r: %d, g: %d, b: %d, a: %d]"), kTETextureComponentMapIdentity.r, kTETextureComponentMapIdentity.g, kTETextureComponentMapIdentity.b, kTETextureComponentMapIdentity.a),
				NullPointer,
				NullPointer,
				*GetCurrentThreadStr(),
				SharedTexture
			)
		}

		if (!SharedTexture)
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("TED3DSharedTextureCreate failed"));
			return false;
		}

		TouchObject<TED3DSharedTexture> TouchRepresentation;
		TouchRepresentation.take(SharedTexture);

		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.v [GT] Cook Frame - D3D12::ShareTexture - TED3DSharedTextureSetCallback"), STAT_TE_I_B_1_u_D3D, STATGROUP_TouchEngine);
			ResourceSharingHandle_RenderThread = SharingHandle;
			SetTouchRepresentation_RenderThread(MoveTemp(TouchRepresentation),
				[this](const TouchObject<TETexture>& Texture)
				{
					TED3DSharedTexture* Casted = static_cast<TED3DSharedTexture*>(Texture.get());
					UE_LOG(LogTouchEngineD3D12RHI, VeryVerbose, TEXT("TED3DSharedTextureSetCallback( TETexture: %p, callback, FExportedTouchTexture: %p)"), Casted, this)
					TED3DSharedTextureSetCallback(Casted, TouchTextureCallback, this);
				});
		}
		return true;
	}

	FExportedTextureD3D12::FExportedTextureD3D12(const TSharedRef<FTouchTextureExporterD3D12>& InExporter)
		: WeakExporter(InExporter), CopyCompletedFence(InExporter->GetOrCreateOwnedFence_AnyThread().ToSharedRef())
	{}

	bool FExportedTextureD3D12::EnqueueTextureCopy(UTexture* SrcTexture)
	{
		if (!IsValid(SrcTexture))
		{
			return false;
		}
		
		ENQUEUE_RENDER_COMMAND(ExportedTouchTextureCopy)([SourceTextureResource = SrcTexture->GetResource(), WeakThis = SharedThis(this).ToWeakPtr(), WeakExporter = WeakExporter](FRHICommandListImmediate& RHICmdList)
		{
			const TSharedPtr<FExportedTextureD3D12> This = WeakThis.Pin();
			const TSharedPtr<FTouchTextureExporterD3D12> Exporter = StaticCastSharedPtr<FTouchTextureExporterD3D12>(WeakExporter.Pin());
			if (!This || !Exporter)
			{
				return;
			}
			
			if (This->GetTETextureTransferBackToUE().Result == TEResultSuccess)
			{
				if (const Microsoft::WRL::ComPtr<ID3D12Fence> NativeFence = Exporter->GetOrCreateSharedFence(This->GetTETextureTransferBackToUE().Semaphore))
				{
					RHICmdList.EnqueueLambda([NativeFence = NativeFence, WaitValue = This->GetTETextureTransferBackToUE().WaitValue](FRHICommandListImmediate& RHICommandList)
					{
						GetID3D12DynamicRHI()->RHIWaitManualFence(RHICommandList, NativeFence.Get(), WaitValue);
					});
				}
			}

			RHICmdList.Transition(FRHITransitionInfo(SourceTextureResource->GetTextureRHI(), ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.Transition(FRHITransitionInfo(This->GetSharedTextureRHI_RenderThread(), ERHIAccess::Unknown, ERHIAccess::CopyDest));
			RHICmdList.CopyTexture(SourceTextureResource->GetTextureRHI(), This->GetSharedTextureRHI_RenderThread(), FRHICopyTextureInfo());

			++This->CopyCompletedFence->LastValue;
			RHICmdList.EnqueueLambda([Fence = This->CopyCompletedFence->NativeFence, SignalValue = This->CopyCompletedFence->LastValue](FRHICommandListImmediate& RHICommandList)
			{
				// this is an async function, will not yet be signalled when this lambda is exited
				GetID3D12DynamicRHI()->RHISignalManualFence(RHICommandList, Fence.Get(), SignalValue);
			});
		});
		
		return true;
	}

	void FExportedTextureD3D12::TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info)
	{
		FExportedTextureD3D12* ExportedTexture = static_cast<FExportedTextureD3D12*>(Info);
		ExportedTexture->OnTouchTextureUseUpdate(Handle, Event, Info);
	}
}
