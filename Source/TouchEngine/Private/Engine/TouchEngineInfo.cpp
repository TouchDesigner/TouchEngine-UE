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

DECLARE_STATS_GROUP(TEXT("TouchEngine"), STATGROUP_TouchEngine, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("VarSet"), STAT_StatsVarSet, STATGROUP_TouchEngine);
DECLARE_CYCLE_STAT(TEXT("VarGet"), STAT_StatsVarGet, STATGROUP_TouchEngine);

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

TFuture<UE::TouchEngine::FTouchLoadResult> UTouchEngineInfo::LoadTox(const FString& AbsolutePath)
{
	using namespace UE::TouchEngine;
	return Engine
		? Engine->LoadTox_GameThread(AbsolutePath)
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

FTouchEngineCHOP UTouchEngineInfo::GetCHOPOutput(const FString& Identifier)
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

void UTouchEngineInfo::SetTOPInput(const FString& Identifier, UTexture* Texture, bool bReuseExistingTexture)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	if (Engine)
	{
		Engine->SetTOPInput(Identifier, Texture);
	}
}

TTouchVar<bool> UTouchEngineInfo::GetBooleanOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetBooleanOutput(Identifier);
}

void UTouchEngineInfo::SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetBooleanInput(Identifier, Op);
}

TTouchVar<double> UTouchEngineInfo::GetDoubleOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetDoubleOutput(Identifier);
}

void UTouchEngineInfo::SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetDoubleInput(Identifier, Op);
}

TTouchVar<int32> UTouchEngineInfo::GetIntegerOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetIntegerOutput(Identifier);
}

void UTouchEngineInfo::SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32>>& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetIntegerInput(Identifier, Op);
}

TTouchVar<TEString*> UTouchEngineInfo::GetStringOutput(const FString& Identifier) const
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetStringOutput(Identifier);
}

void UTouchEngineInfo::SetStringInput(const FString& Identifier, TTouchVar<const char*>& Op)
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

TFuture<UE::TouchEngine::FCookFrameResult> UTouchEngineInfo::CookFrame_GameThread(const UE::TouchEngine::FCookFrameRequest& CookFrameRequest, int64& OutFrameNumber)
{
	using namespace UE::TouchEngine;
	check(IsInGameThread());
	
	if (Engine)
	{
		return Engine->CookFrame_GameThread(CookFrameRequest, OutFrameNumber);
	}

	OutFrameNumber = 0;
	return MakeFulfilledPromise<FCookFrameResult>(FCookFrameResult{ ECookFrameErrorCode::BadRequest }).GetFuture();
}

void UTouchEngineInfo::LogTouchEngineError(const FString& Error) const
{
	if (!Engine || !Engine->TouchResources.ErrorLog)
	{
		UE_LOG(LogTouchEngine, Error, TEXT("UTouchEngineInfo error (no error log) - %s"), *Error);
		return;
	}
	
	Engine->TouchResources.ErrorLog->AddError(Error);
}
