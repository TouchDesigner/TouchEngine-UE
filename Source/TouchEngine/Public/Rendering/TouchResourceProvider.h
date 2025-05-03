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

#pragma once

#include "CoreMinimal.h"

#include "Exporting/TouchExportParams.h"
#include "Importing/TouchImportParams.h"
#include "PixelFormat.h"
#include "TouchSuspendResult.h"

#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TouchObject.h"

#include "Async/Future.h"
#include "RHIResources.h"
#include "RHITextureReference.h" 	
#include "TextureResource.h"
#include "RenderResource.h"
#include "Exporting/TouchTextureExporter.h"
#include "Importing/TouchTextureImporter.h"

class FTexture2DResource;
struct FTouchTOP;
typedef void FTouchEngineDevice;

namespace UE::TouchEngine
{
	class FTouchVariableManager;
	class FTouchFrameCooker;

	struct TOUCHENGINE_API FTouchLoadInstanceResult
	{
		TOptional<FString> Error;

		static FTouchLoadInstanceResult MakeSuccess() { return {}; }
		static FTouchLoadInstanceResult MakeFailure(FString ErrorMessage) { return { { MoveTemp(ErrorMessage) } }; }
		
		bool IsSuccess() const { return !Error.IsSet(); }
		bool IsFailure() const { return !IsSuccess(); }

		FORCEINLINE operator bool() const
		{
			return IsSuccess();
		}
	};
	
	/** Common interface for rendering API implementations */
	class TOUCHENGINE_API FTouchResourceProvider : public TSharedFromThis<FTouchResourceProvider>
	{
	public:
		
		/** Configures the instance to work together with this resource provider. TEInstanceAssociateGraphicsContext has already been called with GetContext() as parameter. */
		virtual void ConfigureInstance(const TouchObject<TEInstance>& InInstance);
		
		virtual TEGraphicsContext* GetContext() const = 0;

		/**
		 * Called when TEEventInstanceDidLoad event is received and the result is successful.
		 * @return Whether this instance is compatible with UE or not.
		 */
		virtual FTouchLoadInstanceResult ValidateLoadedTouchEngine() = 0;
		
		/** Whether the given pixel format can be used for textures passed to ExportTextureToTouchEngine_AnyThread. Must be called after the TE instance has sent TEEventInstanceDidLoad event. */
		bool CanExportPixelFormat(TEInstance& InInstance, EPixelFormat Format) { return GetExportablePixelTypes(InInstance).Contains(Format); }
		virtual TSet<EPixelFormat> GetExportablePixelTypes(TEInstance& InInstance) = 0;

		/** Converts an Unreal texture to a TE texture so it can be used as input to TE. Would be called zero or more times after PrepareForExportToTouchEngine_AnyThread and before FinalizeExportToTouchEngine_AnyThread */
		TFuture<TouchObject<TETexture>> ExportTextureToTouchEngine_AnyThread(const FTouchExportParameters& Params);
		
		virtual void PrepareForNewCook(const FTouchEngineInputFrameData& FrameData);
		void InitializeExportsToTouchEngine_GameThread(const FTouchEngineInputFrameData& FrameData)
		{
			GetTextureExporter().InitializeExportsToTouchEngine_GameThread(FrameData);
		}
		void FinalizeExportsToTouchEngine_AnyThread(const FTouchEngineInputFrameData& FrameData)
		{
			GetTextureExporter().FinalizeExportsToTouchEngine_AnyThread(FrameData);
		}

		/** Converts a TE texture received from TE to an Unreal texture. */
		TFuture<FTouchTextureImportResult> ImportTextureToUnrealEngine_AnyThread(const FTouchImportParameters& LinkParams, const TSharedPtr<FTouchFrameCooker>& FrameCooker)
		{
			return GetTextureImporter().ImportTexture_AnyThread(LinkParams, FrameCooker);
		}

		/**
		 * Prevents further async tasks from being enqueued, cancels running tasks where possible, and executes the future once all tasks are done.
		 * This is not a replacement of ~FTouchResourceProvider. Actively running tasks may reference this resource provider thus preventing its destruction.
		 * This function allows the tasks to complete on the CPU and GPU. After this is possible to destroy FTouchResourceProvider in a defined state. 
		 */
		virtual TFuture<FTouchSuspendResult> SuspendAsyncTasks_GameThread() = 0;
		
		virtual ~FTouchResourceProvider() = default;

		virtual FTouchTextureImporter& GetTextureImporter() = 0;
		virtual FTouchTextureExporter& GetTextureExporter() = 0;

		void SetExportedTexturePoolSize(int ExportedTexturePoolSize);
		void SetImportedTexturePoolSize(int ImportedTexturePoolSize);
		
		/**
		 * Returns a stable RHI for the given texture. The texture needs to not be null.
		 * In this case, stable means that it will not change between GameThread and RenderThread, which could otherwise be the case for a UTexture2D if not retrieved properly.
		 */
		static FTextureRHIRef GetStableRHIFromTexture(const UTexture* Texture)
		{
			check(Texture)

			// A Texture2D is actually composed of 2 RHI resources, one only accessible in GameThread, and another one only accessible in RenderThread.
			// The RHI is access via UTexture2D::GetResource() on one thread is not guaranteed to match on the other thread, especially as they might be retrieved at different times.
			// This is an issue with Texture Streaming as the size of the RHI retrieved on GameThread which we use to create a shared texture sometimes varies with the one we retrieve on
			// RenderThread where we try to enqueue the copy.
			// Instead of using UTexture2D::GetResource(), we retrieve and store a stable RHI via UTexture::TextureReference, and we make sure to return the referenced texture
			if (Texture->TextureReference.TextureReferenceRHI.IsValid())
			{
				return FTextureRHIRef(Texture->TextureReference.TextureReferenceRHI->GetReferencedTexture());
			}
			// else // The below seems to crash in certain condition, but it is also hard to know from which thread this should be called as this rarely happens
			// {
			// 	FTextureRHIRef Ref;
			// 	// Hack to allow us to access the GameThread RHI texture from this thread.
			// 	const ETaskTag PreviousTagScope = FTaskTagScope::SwapTag(ETaskTag::ENone);
			// 	{
			// 		FOptionalTaskTagScope Scope(ETaskTag::EParallelGameThread);
			// 		Ref = Texture->GetResource()->TextureRHI;
			// 	}
			// 	FTaskTagScope::SwapTag(PreviousTagScope);
			//
			// 	return Ref;
			// }
			return nullptr;
		}
		/**
		 * Returns the pixel format from the given texture. The texture needs to not be null.
		 * To work with most texture types, we are retrieving the EPixelFormat from the Texture RHI returned by GetStableRHIFromTexture
		 */
		static EPixelFormat GetPixelFormat(const UTexture* Texture)
		{
			if (!IsValid(Texture))
			{
				return EPixelFormat::PF_Unknown;
			}
			const FTextureRHIRef RHI = GetStableRHIFromTexture(Texture);
			return RHI.IsValid() ? RHI->GetDesc().Format : EPixelFormat::PF_Unknown;
		}

		const TouchObject<TEInstance>& GetInstance() const { return Instance; }
		void ClearSavedInstance();
	private:
		TouchObject<TEInstance> Instance;
	};
}
