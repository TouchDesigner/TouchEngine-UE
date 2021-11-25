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

#include "TouchEngineInfo.h"
#include "UTouchEngine.h"
#include "Misc/DateTime.h"

DECLARE_STATS_GROUP(TEXT("TouchEngine"), STATGROUP_TouchEngine, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("VarSet"), STAT_StatsVarSet, STATGROUP_TouchEngine);
DECLARE_CYCLE_STAT(TEXT("VarGet"), STAT_StatsVarGet, STATGROUP_TouchEngine);


UTouchEngineInfo::UTouchEngineInfo() : Super()
{
	Engine = NewObject<UTouchEngine>();
}

FString
UTouchEngineInfo::GetToxPath() const
{
	if (Engine)
	{
		// engine has been created
		return Engine->GetToxPath();
	}
	else
	{
		// engine has not been loaded, return empty
		return FString();
	}

}

bool UTouchEngineInfo::SetCookMode(bool IsIndependent)
{
	return Engine->SetCookMode(IsIndependent);
}

bool UTouchEngineInfo::SetFrameRate(int64 FrameRate)
{
	return Engine->SetFrameRate(FrameRate);
}

bool UTouchEngineInfo::PreLoad()
{
	Engine->PreLoad();
	return true;
}

bool UTouchEngineInfo::PreLoad(FString ToxPath)
{
	Engine->PreLoad(ToxPath);
	return true;
}

bool UTouchEngineInfo::Load(FString ToxPath)
{
	// ensure file exists
	if (!FPaths::FileExists(ToxPath))
	{
		// try scoping to content directory if we can't find it
		ToxPath = FPaths::ProjectContentDir() + ToxPath;

		if (!FPaths::FileExists(ToxPath))
		{
			// file does not exist
			Engine->OutputError(FString::Printf(TEXT("Invalid file path - %s"), *ToxPath));
			Engine->OnLoadFailed.Broadcast("Invalid file path");
			return false;
		}
	}

	if (Engine->GetToxPath() != ToxPath)
	{
		Engine->LoadTox(ToxPath);
	}

	// sometimes we destroy engine on failure notifications, make sure it's still valid
	if (Engine)
	{
		return Engine->GetDidLoad();
	}
	else
	{
		// engine->loadTox failed and engine was destroyed
		return false;
	}
}

bool UTouchEngineInfo::Unload()
{
	if (Engine)
		Engine->Unload();

	Engine->OnLoadFailed.Clear();
	Engine->OnParametersLoaded.Clear();

	return true;
}

void UTouchEngineInfo::Clear()
{
	if (Engine)
		Engine->Clear();
}

void UTouchEngineInfo::Destroy()
{
	Clear();
	Engine = nullptr;
}

FTouchCHOPFull UTouchEngineInfo::GetCHOPOutputSingleSample(const FString& Identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);

	if (Engine)
	{
		return Engine->GetCHOPOutputs(Identifier);
	}
	else
	{
		return FTouchCHOPFull();
	}
}

void UTouchEngineInfo::SetCHOPInputSingleSample(const FString& Identifier, const FTouchCHOPSingleSample& chop)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	if (Engine)
	{
		return Engine->SetCHOPInputSingleSample(Identifier, chop);
	}
}

FTouchTOP UTouchEngineInfo::GetTOPOutput(const FString& Identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);

	if (Engine)
	{
		return Engine->GetTOPOutput(Identifier);
	}
	else
	{
		return FTouchTOP();
	}
}

void UTouchEngineInfo::SetTOPInput(const FString& Identifier, UTexture* texture)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	if (Engine)
		Engine->SetTOPInput(Identifier, texture);
}

FTouchVar<bool> UTouchEngineInfo::GetBooleanOutput(const FString& Identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetBooleanOutput(Identifier);
}

void UTouchEngineInfo::SetBooleanInput(const FString& Identifier, FTouchVar<bool>& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetBooleanInput(Identifier, Op);
}

FTouchVar<double> UTouchEngineInfo::GetDoubleOutput(const FString& Identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetDoubleOutput(Identifier);
}

void UTouchEngineInfo::SetDoubleInput(const FString& Identifier, FTouchVar<TArray<double>>& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetDoubleInput(Identifier, Op);
}

FTouchVar<int32_t> UTouchEngineInfo::GetIntegerOutput(const FString& Identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetIntegerOutput(Identifier);
}

void UTouchEngineInfo::SetIntegerInput(const FString& Identifier, FTouchVar<TArray<int32_t>>& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetIntegerInput(Identifier, Op);
}

FTouchVar<TEString*> UTouchEngineInfo::GetStringOutput(const FString& Identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetStringOutput(Identifier);
}

void UTouchEngineInfo::SetStringInput(const FString& Identifier, FTouchVar<char*>& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetStringInput(Identifier, Op);
}

FTouchDATFull UTouchEngineInfo::GetTableOutput(const FString& Identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetTableOutput(Identifier);
}

void UTouchEngineInfo::SetTableInput(const FString& Identifier, FTouchDATFull& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetTableInput(Identifier, Op);
}



void UTouchEngineInfo::CookFrame(int64 FrameTime_Mill)
{
	if (Engine)
	{
		CookStartFrame = FDateTime::Now().GetTicks();

		return Engine->CookFrame(FrameTime_Mill);
	}
}

bool UTouchEngineInfo::IsLoaded()
{
	return Engine->GetDidLoad();
}

bool UTouchEngineInfo::IsLoading()
{
	return Engine && (Engine->GetIsLoading());
}

bool UTouchEngineInfo::IsCookComplete()
{
	if (FDateTime::Now().GetTicks() - CookStartFrame > 60)
		return true;

	if (!Engine)
		return true;

	return !Engine->MyCooking;
}

bool UTouchEngineInfo::HasFailedLoad()
{
	return Engine->GetFailedLoad();
}

void UTouchEngineInfo::LogTouchEngineError(FString Error)
{
	Engine->OutputError(Error);
}

bool UTouchEngineInfo::IsRunning()
{
	if (!Engine)
	{
		return false;
	}
	else
	{
		return true;
	}
}

FTouchOnLoadFailed* UTouchEngineInfo::GetOnLoadFailedDelegate()
{
	return &Engine->OnLoadFailed;
}

FTouchOnParametersLoaded* UTouchEngineInfo::GetOnParametersLoadedDelegate()
{
	return &Engine->OnParametersLoaded;
}

FString UTouchEngineInfo::GetFailureMessage()
{
	if (Engine)
	{
		return Engine->FailureMessage;
	}
	else
	{
		return FString();
	}
}

TArray<FString> UTouchEngineInfo::GetCHOPChannelNames(FString Identifier)
{
	FTouchCHOPFull* FullChop = Engine->MyCHOPFullOutputs.Find(Identifier);

	if (FullChop)
	{
		TArray<FString> RetVal;

		for (int i = 0; i < FullChop->SampleData.Num(); i++)
		{
			RetVal.Add(FullChop->SampleData[i].ChannelName);
		}
		return RetVal;
	}
	return TArray<FString>();
}
