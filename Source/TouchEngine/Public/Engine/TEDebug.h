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

#define CASE_TO_FSTRING(Value) case Value: return FString(TEXT(#Value));

inline FString TEEventToString(const TEEvent Event)
{
	switch (Event)
	{
	CASE_TO_FSTRING(TEEventGeneral);
	CASE_TO_FSTRING(TEEventInstanceReady);
	CASE_TO_FSTRING(TEEventInstanceDidLoad);
	CASE_TO_FSTRING(TEEventInstanceDidUnload);
	CASE_TO_FSTRING(TEEventFrameDidFinish);
	default: return FString(TEXT("UNKNOWN TEEvent"));
	}
}

inline FString TELinkEventToString(const TELinkEvent Event)
{
	switch (Event)
	{
	CASE_TO_FSTRING( TELinkEventAdded);
	CASE_TO_FSTRING( TELinkEventRemoved);
	CASE_TO_FSTRING( TELinkEventModified);
	CASE_TO_FSTRING( TELinkEventMoved);
	CASE_TO_FSTRING( TELinkEventStateChange);
	CASE_TO_FSTRING( TELinkEventChildChange);
	CASE_TO_FSTRING( TELinkEventValueChange);
	default: return FString(TEXT("UNKNOWN TELinkEvent"));
	}
}

inline FString TEObjectEventToString(const TEObjectEvent Event)
{
	switch (Event)
	{
	CASE_TO_FSTRING(TEObjectEventBeginUse);
	CASE_TO_FSTRING(TEObjectEventEndUse);
	CASE_TO_FSTRING(TEObjectEventRelease);
	default: return FString(TEXT("UNKNOWN TEObjectEvent"));
	}
}

inline FString TEResultToString(const TEResult Result)
{
	switch (Result)
	{
	CASE_TO_FSTRING(TEResultSuccess);
	CASE_TO_FSTRING(TEResultInsufficientMemory);
	CASE_TO_FSTRING(TEResultGPUAllocationFailed);
	CASE_TO_FSTRING(TEResultTextureFormatNotSupported);
	CASE_TO_FSTRING(TEResultTextureComponentMapNotSupported);
	CASE_TO_FSTRING(TEResultExecutableError);
	CASE_TO_FSTRING(TEResultInternalError);
	CASE_TO_FSTRING(TEResultMissingResource);
	CASE_TO_FSTRING(TEResultDroppedSamples);
	CASE_TO_FSTRING(TEResultMissedSamples);
	CASE_TO_FSTRING(TEResultBadUsage);
	CASE_TO_FSTRING(TEResultNoMatchingEntity);
	CASE_TO_FSTRING(TEResultCancelled);
	CASE_TO_FSTRING(TEResultExpiredKey);
	CASE_TO_FSTRING(TEResultNoKey);
	CASE_TO_FSTRING(TEResultKeyError);
	CASE_TO_FSTRING(TEResultFileError);
	CASE_TO_FSTRING(TEResultNewerFileVersion);
	CASE_TO_FSTRING(TEResultTouchEngineNotFound);
	CASE_TO_FSTRING(TEResultTouchEngineBadPath);
	CASE_TO_FSTRING(TEResultFailedToLaunchTouchEngine);
	CASE_TO_FSTRING(TEResultFeatureNotSupportedBySystem);
	CASE_TO_FSTRING(TEResultOlderEngineVersion);
	CASE_TO_FSTRING(TEResultIncompatibleEngineVersion);
	CASE_TO_FSTRING(TEResultPermissionDenied);
	CASE_TO_FSTRING(TEResultComponentErrors);
	CASE_TO_FSTRING(TEResultComponentWarnings);
	default: return FString(TEXT("UNKNOWN TEResult"));
	}
}

inline FString TESeverityToString(const TESeverity Severity)
{
	switch (Severity)
	{
	CASE_TO_FSTRING(TESeverityNone)
	CASE_TO_FSTRING(TESeverityWarning)
	CASE_TO_FSTRING(TESeverityError)
	default: return FString(TEXT("UNKNOWN TESeverity"));
	}
}

inline FString TELinkTypeToString(const TELinkType LinkType)
{
	switch (LinkType)
	{
	CASE_TO_FSTRING(TELinkTypeGroup)
	CASE_TO_FSTRING(TELinkTypeComplex)
	CASE_TO_FSTRING(TELinkTypeSequence)
	CASE_TO_FSTRING(TELinkTypeBoolean)
	CASE_TO_FSTRING(TELinkTypeDouble)
	CASE_TO_FSTRING(TELinkTypeInt)
	CASE_TO_FSTRING(TELinkTypeString)
	CASE_TO_FSTRING(TELinkTypeTexture)
	CASE_TO_FSTRING(TELinkTypeFloatBuffer)
	CASE_TO_FSTRING(TELinkTypeStringData)
	CASE_TO_FSTRING(TELinkTypeSeparator)
	default: return FString(TEXT("UNKNOWN TELinkType"));
	}
}

inline FString TEScopeToString(const TEScope Scope)
{
	switch (Scope)
	{
	CASE_TO_FSTRING(TEScopeInput)
	CASE_TO_FSTRING(TEScopeOutput)
	default: return FString(TEXT("UNKNOWN TEScope"));
	}
}

#undef CASE_TO_FSTRING