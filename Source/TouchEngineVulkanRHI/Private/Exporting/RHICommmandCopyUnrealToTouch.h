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

namespace UE::TouchEngine
{
	struct FTouchExportParameters;
}

namespace UE::TouchEngine::Vulkan
{
	class FExportedTextureVulkan;

	bool CopyUnrealToTouchRHICommand(
		FRHICommandListImmediate& RHICmdList,
		const TouchObject<TEInstance>& Instance,
		const FTextureRHIRef& InSrcTextureStableRHI,
		const TSharedRef<FExportedTextureVulkan>& InDestTexture
	);

	// todo: This is a test to force signal the Wait value to Vulkan when cancelling, doesn't seem to work though
	bool ForceSignalWaitValues(
		FRHICommandListImmediate& RHICmdList,
		const TSharedRef<FExportedTextureVulkan>& InDestTexture
	);
}
