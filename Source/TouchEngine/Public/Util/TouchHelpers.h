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

namespace UE::TouchEngine
{
	inline FString GetCurrentThreadStr()
	{
		if (IsInGameThread())
		{
			return FString(TEXT("GameThread"));
		}
		else if (IsInParallelGameThread())
		{
			return FString(TEXT("ParallelGameThread"));
		}
		else if (IsInRenderingThread())
		{
			return FString(TEXT("RenderThread"));
		}
		switch (ETaskTag TaskTag = FTaskTagScope::GetCurrentTag()) {
			case ETaskTag::ENone: return FString(TEXT("ENone"));
			case ETaskTag::EStaticInit: return FString(TEXT("EStaticInit"));
			case ETaskTag::EGameThread: return FString(TEXT("EGameThread"));
			case ETaskTag::ESlateThread: return FString(TEXT("ESlateThread"));
			// case ETaskTag::EAudioThread: return FString(TEXT("EAudioThread"));
			case ETaskTag::ERenderingThread: return FString(TEXT("ERenderingThread"));
			case ETaskTag::ERhiThread: return FString(TEXT("ERhiThread"));
			case ETaskTag::EAsyncLoadingThread: return FString(TEXT("EAsyncLoadingThread"));
			case ETaskTag::ENamedThreadBits: return FString(TEXT("ENamedThreadBits"));
			case ETaskTag::EParallelThread: return FString(TEXT("EParallelThread"));
			case ETaskTag::EWorkerThread: return FString(TEXT("EWorkerThread"));
			case ETaskTag::EParallelRenderingThread: return FString(TEXT("EParallelRenderingThread"));
			case ETaskTag::EParallelGameThread: return FString(TEXT("EParallelGameThread"));
			case ETaskTag::EParallelRhiThread: return FString(TEXT("EParallelRhiThread"));
			default: return FString(TEXT("UnknownThread"));
		};
		
	};
}


