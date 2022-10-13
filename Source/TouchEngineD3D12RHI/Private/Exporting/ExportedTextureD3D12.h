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

#include "TouchEngine/TouchObject.h"

namespace UE::TouchEngine::D3DX12
{
	namespace Private
	{
		struct FTextureCallbackContext;
	}

	class FTextureShareD3D12SharedResourceSecurityAttributes;

	class FExportedTextureD3D12 : public TSharedFromThis<FExportedTextureD3D12>
	{
	public:

		static TSharedPtr<FExportedTextureD3D12> Create(const FRHITexture2D& SourceRHI, const FTextureShareD3D12SharedResourceSecurityAttributes& SharedResourceSecurityAttributes);
		static FString GenerateIdentifierString(const FGuid& ResourceId) { return ResourceId.ToString(EGuidFormats::DigitsWithHyphensInBraces); }
		
		FExportedTextureD3D12(FTexture2DRHIRef SharedTextureRHI, const FGuid& ResourceId, void* ResourceSharingHandle, TouchObject<TED3DSharedTexture> TouchRepresentation, TSharedRef<Private::FTextureCallbackContext> CallbackContext);

		const FTexture2DRHIRef& GetSharedTextureRHI() const { return SharedTextureRHI; }
		const TouchObject<TED3DSharedTexture>& GetTouchRepresentation() const { return TouchRepresentation; }
		
	private:
		
		/** Tracks how many parameters use this texture. Once it reaches 0, it can be released. */
		int32 NumParametersUsing = 0;

		/** Shared between Unreal and TE. Access must be synchronized. */
		FTexture2DRHIRef SharedTextureRHI;

		/** Used to handle the ID of the resource */
		FGuid ResourceId;
		/** Handle to the shared resource */
		void* ResourceSharingHandle;
			
		/** Result of passing SharedTexture to TED3DSharedTextureCreate */
		TouchObject<TED3DSharedTexture> TouchRepresentation;
		/** Keep alive while */
		TSharedRef<Private::FTextureCallbackContext> CallbackContext;

		static void TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info);;
	};

}

