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
#include "TouchEngine/TEInstance.h"

inline FString TEEventToString(const TEEvent Event)
{
	switch (Event)
	{
	case TEEventGeneral: return FString(TEXT("TEEventGeneral"));
	case TEEventInstanceReady: return FString(TEXT("TEEventInstanceReady"));
	case TEEventInstanceDidLoad: return FString(TEXT("TEEventInstanceDidLoad"));
	case TEEventInstanceDidUnload: return FString(TEXT("TEEventInstanceDidUnload"));
	case TEEventFrameDidFinish: return FString(TEXT("TEEventFrameDidFinish"));
	default: return FString(TEXT("default"));
	}
}

inline FString TELinkEventToString(const TELinkEvent Event)
{
	switch (Event)
	{
	case TELinkEventAdded: return FString(TEXT("TELinkEventAdded"));
	case TELinkEventRemoved: return FString(TEXT("TELinkEventRemoved"));
	case TELinkEventModified: return FString(TEXT("TELinkEventModified"));
	case TELinkEventMoved: return FString(TEXT("TELinkEventMoved"));
	case TELinkEventStateChange: return FString(TEXT("TELinkEventStateChange"));
	case TELinkEventChildChange: return FString(TEXT("TELinkEventChildChange"));
	case TELinkEventValueChange: return FString(TEXT("TELinkEventValueChange"));
	default: return FString(TEXT("default"));
	}
}

inline FString TEObjectEventToString(const TEObjectEvent Event)
{
	switch (Event)
	{
	case TEObjectEventBeginUse: return FString(TEXT("TEObjectEventBeginUse"));
	case TEObjectEventEndUse: return FString(TEXT("TEObjectEventEndUse"));
	case TEObjectEventRelease: return FString(TEXT("TEObjectEventRelease"));
	default: return FString(TEXT("default"));
	}
}

inline FString TEResultToString(const TEResult Result)
{
	switch (Result)
	{
	case TEResultSuccess: return FString(TEXT("TEResultSuccess"));
	case TEResultInsufficientMemory: return FString(TEXT("TEResultInsufficientMemory"));
	case TEResultGPUAllocationFailed: return FString(TEXT("TEResultGPUAllocationFailed"));
	case TEResultTextureFormatNotSupported: return FString(TEXT("TEResultTextureFormatNotSupported"));
	case TEResultTextureComponentMapNotSupported: return FString(TEXT("TEResultTextureComponentMapNotSupported"));
	case TEResultExecutableError: return FString(TEXT("TEResultExecutableError"));
	case TEResultInternalError: return FString(TEXT("TEResultInternalError"));
	case TEResultMissingResource: return FString(TEXT("TEResultMissingResource"));
	case TEResultDroppedSamples: return FString(TEXT("TEResultDroppedSamples"));
	case TEResultMissedSamples: return FString(TEXT("TEResultMissedSamples"));
	case TEResultBadUsage: return FString(TEXT("TEResultBadUsage"));
	case TEResultNoMatchingEntity: return FString(TEXT("TEResultNoMatchingEntity"));
	case TEResultCancelled: return FString(TEXT("TEResultCancelled"));
	case TEResultExpiredKey: return FString(TEXT("TEResultExpiredKey"));
	case TEResultNoKey: return FString(TEXT("TEResultNoKey"));
	case TEResultKeyError: return FString(TEXT("TEResultKeyError"));
	case TEResultFileError: return FString(TEXT("TEResultFileError"));
	case TEResultNewerFileVersion: return FString(TEXT("TEResultNewerFileVersion"));
	case TEResultTouchEngineNotFound: return FString(TEXT("TEResultTouchEngineNotFound"));
	case TEResultTouchEngineBadPath: return FString(TEXT("TEResultTouchEngineBadPath"));
	case TEResultFailedToLaunchTouchEngine: return FString(TEXT("TEResultFailedToLaunchTouchEngine"));
	case TEResultFeatureNotSupportedBySystem: return FString(TEXT("TEResultFeatureNotSupportedBySystem"));
	case TEResultOlderEngineVersion: return FString(TEXT("TEResultOlderEngineVersion"));
	case TEResultIncompatibleEngineVersion: return FString(TEXT("TEResultIncompatibleEngineVersion"));
	case TEResultPermissionDenied: return FString(TEXT("TEResultPermissionDenied"));
	case TEResultComponentErrors: return FString(TEXT("TEResultComponentErrors"));
	case TEResultComponentWarnings: return FString(TEXT("TEResultComponentWarnings"));
	default: return FString(TEXT("default"));
	}
}