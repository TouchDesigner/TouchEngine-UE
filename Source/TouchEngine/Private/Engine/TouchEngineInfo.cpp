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

#include "Engine/TouchEngineInfo.h"

#include "Logging.h"
#include "Engine/TouchEngine.h"
#include "Engine/Util/CookFrameData.h"

#include "Misc/Paths.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/TouchFrameCooker.h"

DECLARE_CYCLE_STAT(TEXT("DynVar - Set"), STAT_StatsVarSet, STATGROUP_TouchEngine);
DECLARE_CYCLE_STAT(TEXT("DynVar - Get"), STAT_StatsVarGet, STATGROUP_TouchEngine);

UTouchEngineInfo::UTouchEngineInfo()
  : Super(), Engine(MakeShared<UE::TouchEngine::FTouchEngine>())
{
}

bool UTouchEngineInfo::GetSupportedPixelFormats(TSet<TEnumAsByte<EPixelFormat>>& SupportedPixelFormat) const
{
	return Engine->GetSupportedPixelFormat(SupportedPixelFormat);
}

void UTouchEngineInfo::CancelCurrentAndNextCooks_GameThread(ECookFrameResult CookFrameResult)
{
	return Engine->CancelCurrentAndNextCooks_GameThread(CookFrameResult);
}

bool UTouchEngineInfo::CancelCurrentFrame_GameThread(int64 FrameID, ECookFrameResult CookFrameResult)
{
	return Engine->CancelCurrentFrame_GameThread(FrameID, CookFrameResult);
}

bool UTouchEngineInfo::CheckIfCookTimedOut_GameThread(double CookTimeoutInSeconds)
{
	return Engine->CheckIfCookTimedOut_GameThread(CookTimeoutInSeconds);
}

TFuture<UE::TouchEngine::FTouchLoadResult> UTouchEngineInfo::LoadTox(const FString& AbsolutePath, UTouchEngineComponentBase* Component, double TimeoutInSeconds)
{
	using namespace UE::TouchEngine;
	return Engine->LoadTox_GameThread(AbsolutePath, Component, TimeoutInSeconds);
}

bool UTouchEngineInfo::Unload()
{
	Engine->Unload_GameThread();
	return true;
}

void UTouchEngineInfo::Destroy()
{
	Engine->DestroyTouchEngine_GameThread();
}

FTouchEngineCHOP UTouchEngineInfo::GetCHOPOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetCHOPOutput(Identifier);
}

UTexture2D* UTouchEngineInfo::GetTOPOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetTOPOutput(Identifier);
}

bool UTouchEngineInfo::GetBooleanOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetBooleanOutput(Identifier);
}

double UTouchEngineInfo::GetDoubleOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetDoubleOutput(Identifier);
}

int32 UTouchEngineInfo::GetIntegerOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetIntegerOutput(Identifier);
}

TouchObject<TEString> UTouchEngineInfo::GetStringOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetStringOutput(Identifier);
}

int64 UTouchEngineInfo::GetFrameLastUpdatedForParameter(const FString& Identifier) const
{
	return Engine->GetFrameLastUpdatedForParameter(Identifier);
}

FTouchDATFull UTouchEngineInfo::GetTableOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetTableOutput(Identifier);
}

TFuture<UE::TouchEngine::FCookFrameResult> UTouchEngineInfo::CookFrame_GameThread(UE::TouchEngine::FCookFrameRequest&& CookFrameRequest, int32 InputBufferLimit)
{
	check(IsInGameThread());
	return Engine->CookFrame_GameThread(MoveTemp(CookFrameRequest), InputBufferLimit);
}

bool UTouchEngineInfo::ExecuteNextPendingCookFrame_GameThread() const
{
	check(IsInGameThread());
	return Engine->ExecuteNextPendingCookFrame_GameThread();
}

bool UTouchEngineInfo::IsCookingFrame() const
{
	if (Engine->TouchResources.FrameCooker)
	{
		return Engine->TouchResources.FrameCooker->IsCookingFrame();
	}
	return false;
}

void UTouchEngineInfo::LogTouchEngineWarning(const FString& Message, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription) const
{
	if (!Engine->TouchResources.ErrorLog)
	{
		UE_LOG(LogTouchEngine, Error, TEXT("UTouchEngineInfo warning (no error log) - '%s' Variable '%s' FunctionName '%s': %s"), *Message, *VarName, *FunctionName.ToString(), *AdditionalDescription);
		return;
	}
	
	Engine->TouchResources.ErrorLog->AddWarning(Message, VarName, FunctionName, AdditionalDescription);
}

void UTouchEngineInfo::LogTouchEngineWarning(UE::TouchEngine::FTouchErrorLog::EErrorType ErrorType, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription) const
{
	if (!Engine->TouchResources.ErrorLog)
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("UTouchEngineInfo warning (no error log) - [%d] Variable '%s' FunctionName '%s': %s"), ErrorType, *VarName, *FunctionName.ToString(), *AdditionalDescription);
		return;
	}
	
	Engine->TouchResources.ErrorLog->AddWarning(ErrorType, VarName, FunctionName, AdditionalDescription);
}

void UTouchEngineInfo::LogTouchEngineError(const FString& Message, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription) const
{
	if (!Engine->TouchResources.ErrorLog)
	{
		UE_LOG(LogTouchEngine, Error, TEXT("UTouchEngineInfo error (no error log) - '%s' Variable '%s' FunctionName '%s': %s"), *Message, *VarName, *FunctionName.ToString(), *AdditionalDescription);
		return;
	}
	
	Engine->TouchResources.ErrorLog->AddError(Message, VarName, FunctionName, AdditionalDescription);
}

void UTouchEngineInfo::LogTouchEngineError(UE::TouchEngine::FTouchErrorLog::EErrorType ErrorType, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription) const
{
	if (!Engine->TouchResources.ErrorLog)
	{
		UE_LOG(LogTouchEngine, Error, TEXT("UTouchEngineInfo error (no error log) - [%d] Variable '%s' FunctionName '%s': %s"), ErrorType, *VarName, *FunctionName.ToString(), *AdditionalDescription);
		return;
	}
	
	Engine->TouchResources.ErrorLog->AddError(ErrorType, VarName, FunctionName, AdditionalDescription);
}
