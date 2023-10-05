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
  : Super()
{
	Engine = MakeShared<UE::TouchEngine::FTouchEngine>();
}

bool UTouchEngineInfo::GetSupportedPixelFormats(TSet<TEnumAsByte<EPixelFormat>>& SupportedPixelFormat) const
{
	if (Engine)
	{
		return Engine->GetSupportedPixelFormat(SupportedPixelFormat);
	}

	return false;
}

void UTouchEngineInfo::CancelCurrentAndNextCooks_GameThread(ECookFrameResult CookFrameResult)
{
	if (Engine)
	{
		return Engine->CancelCurrentAndNextCooks_GameThread(CookFrameResult);
	}
}

bool UTouchEngineInfo::CancelCurrentFrame_GameThread(int64 FrameID, ECookFrameResult CookFrameResult)
{
	if (Engine)
	{
		return Engine->CancelCurrentFrame_GameThread(FrameID, CookFrameResult);
	}
	return false;
}

bool UTouchEngineInfo::CheckIfCookTimedOut_GameThread(double CookTimeoutInSeconds)
{
	if (Engine)
	{
		return Engine->CheckIfCookTimedOut_GameThread(CookTimeoutInSeconds);
	}
	return false;
}

TFuture<UE::TouchEngine::FTouchLoadResult> UTouchEngineInfo::LoadTox(const FString& AbsolutePath, UTouchEngineComponentBase* Component, double TimeoutInSeconds)
{
	using namespace UE::TouchEngine;
	return Engine
		? Engine->LoadTox_GameThread(AbsolutePath, Component, TimeoutInSeconds)
		: MakeFulfilledPromise<FTouchLoadResult>(FTouchLoadResult::MakeFailure(TEXT("No active engine instance"))).GetFuture();
}

bool UTouchEngineInfo::Unload()
{
	if (!Engine)
	{
		return false;
	}

	Engine->Unload_GameThread();
	return true;
}

void UTouchEngineInfo::Destroy()
{
	if (Engine)
	{
		Engine->DestroyTouchEngine_GameThread();
	}
}

FTouchEngineCHOP UTouchEngineInfo::GetCHOPOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	
	return Engine
		? Engine->GetCHOPOutput(Identifier)
		: FTouchEngineCHOP();
}

void UTouchEngineInfo::SetCHOPChannelInput(const FString& Identifier, const FTouchEngineCHOPChannel& Chop)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	if (Engine)
	{
		Engine->SetCHOPChannelInput(Identifier, Chop);
	}
}

void UTouchEngineInfo::SetCHOPInput(const FString& Identifier, const FTouchEngineCHOP& Chop)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	if (Engine)
	{
		Engine->SetCHOPInput(Identifier, Chop);
	}
}

UTexture2D* UTouchEngineInfo::GetTOPOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);

	return Engine
		? Engine->GetTOPOutput(Identifier)
		: nullptr;
}

void UTouchEngineInfo::SetTOPInput(const FString& Identifier, UTexture* Texture, const FTouchEngineInputFrameData& FrameData)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	if (Engine)
	{
		Engine->SetTOPInput(Identifier, Texture, FrameData);
	}
}

bool UTouchEngineInfo::GetBooleanOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetBooleanOutput(Identifier);
}

void UTouchEngineInfo::SetBooleanInput(const FString& Identifier, bool& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetBooleanInput(Identifier, Op);
}

double UTouchEngineInfo::GetDoubleOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetDoubleOutput(Identifier);
}

void UTouchEngineInfo::SetDoubleInput(const FString& Identifier, const TArray<double>& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetDoubleInput(Identifier, Op);
}

int32 UTouchEngineInfo::GetIntegerOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetIntegerOutput(Identifier);
}

void UTouchEngineInfo::SetIntegerInput(const FString& Identifier, const TArray<int32>& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetIntegerInput(Identifier, Op);
}

TouchObject<TEString> UTouchEngineInfo::GetStringOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetStringOutput(Identifier);
}

int64 UTouchEngineInfo::GetFrameLastUpdatedForParameter(const FString& Identifier) const
{
	check(Engine);
	return Engine->GetFrameLastUpdatedForParameter(Identifier);
}

void UTouchEngineInfo::SetStringInput(const FString& Identifier, const char*& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetStringInput(Identifier, Op);
}

FTouchDATFull UTouchEngineInfo::GetTableOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetTableOutput(Identifier);
}

void UTouchEngineInfo::SetTableInput(const FString& Identifier, FTouchDATFull& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetTableInput(Identifier, Op);
}

TFuture<UE::TouchEngine::FCookFrameResult> UTouchEngineInfo::CookFrame_GameThread(UE::TouchEngine::FCookFrameRequest&& CookFrameRequest, int32 InputBufferLimit)
{
	using namespace UE::TouchEngine;
	check(IsInGameThread());

	if (Engine)
	{
		return Engine->CookFrame_GameThread(MoveTemp(CookFrameRequest), InputBufferLimit);
	}

	return MakeFulfilledPromise<FCookFrameResult>(FCookFrameResult::FromCookFrameRequest(CookFrameRequest, ECookFrameResult::BadRequest, -1)).GetFuture();
}

bool UTouchEngineInfo::ExecuteNextPendingCookFrame_GameThread() const
{
	using namespace UE::TouchEngine;
	check(IsInGameThread());

	return Engine ? Engine->ExecuteNextPendingCookFrame_GameThread() : false;
}

bool UTouchEngineInfo::IsCookingFrame() const
{
	if (Engine && Engine->TouchResources.FrameCooker)
	{
		return Engine->TouchResources.FrameCooker->IsCookingFrame();
	}
	return false;
}

void UTouchEngineInfo::LogTouchEngineWarning(const FString& Message, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription) const
{
	if (!Engine || !Engine->TouchResources.ErrorLog)
	{
		UE_LOG(LogTouchEngine, Error, TEXT("UTouchEngineInfo warning (no error log) - '%s' Variable '%s' FunctionName '%s': %s"), *Message, *VarName, *FunctionName.ToString(), *AdditionalDescription);
		return;
	}
	
	Engine->TouchResources.ErrorLog->AddWarning(Message, VarName, FunctionName, AdditionalDescription);
}

void UTouchEngineInfo::LogTouchEngineWarning(UE::TouchEngine::FTouchErrorLog::EErrorType ErrorType, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription) const
{
	if (!Engine || !Engine->TouchResources.ErrorLog)
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("UTouchEngineInfo warning (no error log) - [%d] Variable '%s' FunctionName '%s': %s"), ErrorType, *VarName, *FunctionName.ToString(), *AdditionalDescription);
		return;
	}
	
	Engine->TouchResources.ErrorLog->AddWarning(ErrorType, VarName, FunctionName, AdditionalDescription);
}

void UTouchEngineInfo::LogTouchEngineError(const FString& Message, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription) const
{
	if (!Engine || !Engine->TouchResources.ErrorLog)
	{
		UE_LOG(LogTouchEngine, Error, TEXT("UTouchEngineInfo error (no error log) - '%s' Variable '%s' FunctionName '%s': %s"), *Message, *VarName, *FunctionName.ToString(), *AdditionalDescription);
		return;
	}
	
	Engine->TouchResources.ErrorLog->AddError(Message, VarName, FunctionName, AdditionalDescription);
}

void UTouchEngineInfo::LogTouchEngineError(UE::TouchEngine::FTouchErrorLog::EErrorType ErrorType, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription) const
{
	if (!Engine || !Engine->TouchResources.ErrorLog)
	{
		UE_LOG(LogTouchEngine, Error, TEXT("UTouchEngineInfo error (no error log) - [%d] Variable '%s' FunctionName '%s': %s"), ErrorType, *VarName, *FunctionName.ToString(), *AdditionalDescription);
		return;
	}
	
	Engine->TouchResources.ErrorLog->AddError(ErrorType, VarName, FunctionName, AdditionalDescription);
}
