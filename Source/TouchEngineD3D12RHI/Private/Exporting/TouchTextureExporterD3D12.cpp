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

#include "Exporting/TouchTextureExporterD3D12.h"

#include "Windows/PreWindowsApi.h"
#include "D3D12RHIPrivate.h"
#include "ExportedTextureD3D12.h"
#include "Windows/PostWindowsApi.h"

#include "Rendering/TouchExportParams.h"

#include "Engine/Texture.h"
#include "TouchEngine/TED3D.h"

namespace UE::TouchEngine::D3DX12
{
	FTouchTextureExporterD3D12::~FTouchTextureExporterD3D12()
	{
		checkf(
			ParamNameToTexture.IsEmpty() && CachedTextureData.IsEmpty(),
			TEXT("SuspendAsyncTasks was either not called or did not clean up the exported textures correctly. ~FExportedTextureD3D12 will now proceed to fatally crash.")
			);
	}

	TFuture<FTouchSuspendResult> FTouchTextureExporterD3D12::SuspendAsyncTasks()
	{
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();
		
		TFuture<FTouchSuspendResult> FinishRenderingTasks = FTouchTextureExporter::SuspendAsyncTasks();
		// Once all the rendering tasks have finished using the copying textures, they can be released.
		FinishRenderingTasks.Next([this, Promise = MoveTemp(Promise)](auto) mutable
		{
			// Important: Do not copy iterate ParamNameToTexture and do not copy ParamNameToTexture... otherwise we'll keep
			// the contained shared pointers alive and RemoveTextureParameterDependency won't end up scheduling any kill commands.
			TArray<FName> UsedParameterNames;
			ParamNameToTexture.GenerateKeyArray(UsedParameterNames);
			for (FName ParameterName : UsedParameterNames)
			{
				RemoveTextureParameterDependency(ParameterName);
			}

			check(ParamNameToTexture.IsEmpty());
			check(CachedTextureData.IsEmpty());

			// Once all the texture clean-ups are done, we can tell whomever is waiting that the rendering resources have been cleared up.
			// From this point forward we're ready to be destroyed.
			PendingTextureReleases.Suspend().Next([Promise = MoveTemp(Promise)](auto) mutable
			{
				Promise.SetValue({});
			});
		});

		return Future;
	}

	FTouchExportResult FTouchTextureExporterD3D12::ExportTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTouchExportParameters& Params)
	{
		const TSharedPtr<FExportedTextureD3D12> TextureData = TryGetTexture(Params);
		if (!TextureData)
		{
			return FTouchExportResult{ ETouchExportErrorCode::InternalD3D12Error };
		}
		
		FRHITexture2D* SourceRHI = GetRHIFromTexture(Params.Texture);
		RHICmdList.CopyTexture(SourceRHI, TextureData->GetSharedTextureRHI(), FRHICopyTextureInfo());
		// TODO: Synchronize
		
		return FTouchExportResult{ ETouchExportErrorCode::Success, TextureData->GetTouchRepresentation() };
	}

	TSharedPtr<FExportedTextureD3D12> FTouchTextureExporterD3D12::TryGetTexture(const FTouchExportParameters& Params)
	{
		return CachedTextureData.Contains(Params.Texture)
			? ReallocateTextureIfNeeded(Params)
			: ShareTexture(Params);
	}

	TSharedPtr<FExportedTextureD3D12> FTouchTextureExporterD3D12::ShareTexture(const FTouchExportParameters& Params)
	{
		const FRHITexture2D* TextureRHI = GetRHIFromTexture(Params.Texture);
		TSharedPtr<FExportedTextureD3D12> ExportedTexture = FExportedTextureD3D12::Create(*TextureRHI, SharedResourceSecurityAttributes);
		if (ensure(ExportedTexture))
		{
			ParamNameToTexture.Add(Params.ParameterName, FTextureDependency{ Params.Texture, ExportedTexture });
			return CachedTextureData.Add(Params.Texture, ExportedTexture);
		}
		return nullptr;
	}

	TSharedPtr<FExportedTextureD3D12> FTouchTextureExporterD3D12::ReallocateTextureIfNeeded(const FTouchExportParameters& Params)
	{
		const FRHITexture2D* InputRHI = GetRHIFromTexture(Params.Texture);
		if (!ensure(LIKELY(InputRHI)))
		{
			RemoveTextureParameterDependency(Params.ParameterName);
			return nullptr;
		}
		
		if (const TSharedPtr<FExportedTextureD3D12>* Existing = CachedTextureData.Find(Params.Texture);
			// This will hit, e.g. when somebody sets the same UTexture as parameter twice.
			// If the texture has not changed structurally (i.e. resolution or format), we can reuse the shared handle and copy into the shared texture.
			Existing && Existing->Get()->CanFitTexture(*InputRHI))
		{
			return *Existing;
		}

		// The texture has changed structurally - release the old shared texture and create a new one
		RemoveTextureParameterDependency(Params.ParameterName);
		return ShareTexture(Params);
	}

	void FTouchTextureExporterD3D12::RemoveTextureParameterDependency(FName TextureParam)
	{
		FTextureDependency OldExportedTexture;
		ParamNameToTexture.RemoveAndCopyValue(TextureParam, OldExportedTexture);
		CachedTextureData.Remove(OldExportedTexture.UnrealTexture);

		// If this was the last parameter that used this UTexture, then OldExportedTexture should be the last one referencing the exported texture.
		if (OldExportedTexture.ExportedTexture.IsUnique())
		{
			// This will keep the data valid for as long as TE is using the texture
			const TSharedPtr<FExportedTextureD3D12> KeepAlive = OldExportedTexture.ExportedTexture;
			KeepAlive->Release()
				.Next([this, KeepAlive, TaskToken = PendingTextureReleases.StartTask()](auto){});
		}
	}
}

#undef CHECK_HR_DEFAULT