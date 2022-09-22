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

#include "Engine/TouchEngine.h"

#include "Misc/Paths.h"

DECLARE_STATS_GROUP(TEXT("TouchEngine"), STATGROUP_TouchEngine, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("VarSet"), STAT_StatsVarSet, STATGROUP_TouchEngine);
DECLARE_CYCLE_STAT(TEXT("VarGet"), STAT_StatsVarGet, STATGROUP_TouchEngine);

UTouchEngineInfo::UTouchEngineInfo()
  : Super()
{
	Engine = NewObject<UTouchEngine>();
}

FString UTouchEngineInfo::GetToxPath() const
{
	if (Engine)
	{
		return Engine->GetToxPath();
	}
	return {};
}

bool UTouchEngineInfo::SetCookMode(bool IsIndependent)
{
	return Engine->SetCookMode(IsIndependent);
}

bool UTouchEngineInfo::SetFrameRate(int64 FrameRate)
{
	return Engine->SetFrameRate(FrameRate);
}

bool UTouchEngineInfo::Load(const FString& AbsoluteOrRelativeToxPath)
{
	FString AbsoluteFilePath = AbsoluteOrRelativeToxPath;
	if (!FPaths::FileExists(AbsoluteOrRelativeToxPath))
	{
		AbsoluteFilePath = FPaths::ProjectContentDir() + AbsoluteOrRelativeToxPath;
		if (!FPaths::FileExists(AbsoluteOrRelativeToxPath))
		{
			// file does not exist
			Engine->OutputError(FString::Printf(TEXT("Invalid file path - %s"), *AbsoluteOrRelativeToxPath));
			Engine->OnLoadFailed.Broadcast("Invalid file path");
			return false;
		}
	}

	const bool bIsNewPath = Engine->GetToxPath() != AbsoluteFilePath; 
	if (bIsNewPath)
	{
		Engine->LoadTox(AbsoluteFilePath);
	}

	// Sometimes we destroy engine on failure notifications
	return Engine
		? Engine->GetDidLoad()
		: false;
}

bool UTouchEngineInfo::Unload()
{
	if (Engine)
	{
		Engine->Unload();
	}

	Engine->OnLoadFailed.Clear();
	Engine->OnParametersLoaded.Clear();
	return true;
}

void UTouchEngineInfo::Clear()
{
	if (Engine)
	{
		Engine->Clear();
	}
}

void UTouchEngineInfo::Destroy()
{
	Clear();
	Engine = nullptr;
}

FTouchCHOPFull UTouchEngineInfo::GetCHOPOutputSingleSample(const FString& Identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	
	return Engine
		? Engine->GetCHOPOutputs(Identifier)
		: FTouchCHOPFull();
}

void UTouchEngineInfo::SetCHOPInputSingleSample(const FString& Identifier, const FTouchCHOPSingleSample& Chop)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	if (Engine)
	{
		return Engine->SetCHOPInputSingleSample(Identifier, Chop);
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

void UTouchEngineInfo::SetTOPInput(const FString& Identifier, UTexture* Texture)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	if (Engine)
	{
		Engine->SetTOPInput(Identifier, Texture);
	}
}

TTouchVar<bool> UTouchEngineInfo::GetBooleanOutput(const FString& Identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetBooleanOutput(Identifier);
}

void UTouchEngineInfo::SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetBooleanInput(Identifier, Op);
}

TTouchVar<double> UTouchEngineInfo::GetDoubleOutput(const FString& Identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetDoubleOutput(Identifier);
}

void UTouchEngineInfo::SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetDoubleInput(Identifier, Op);
}

TTouchVar<int32> UTouchEngineInfo::GetIntegerOutput(const FString& Identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetIntegerOutput(Identifier);
}

void UTouchEngineInfo::SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32>>& Op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	Engine->SetIntegerInput(Identifier, Op);
}

TTouchVar<TEString*> UTouchEngineInfo::GetStringOutput(const FString& Identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return Engine->GetStringOutput(Identifier);
}

void UTouchEngineInfo::SetStringInput(const FString& Identifier, TTouchVar<char*>& Op)
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

void UTouchEngineInfo::CookFrame_GameThread(int64 FrameTime_Mill)
{
	check(IsInGameThread());
	
	if (Engine)
	{
		if (Engine->MyTimeMode == TETimeExternal && Engine->MyNumInputTexturesQueued)
		{
			// need to stall until all input textures have been sent
			FGenericPlatformProcess::ConditionalSleep([this, FrameTime_Mill]()
				{
					if (!Engine->MyNumInputTexturesQueued)
					{
						Engine->CookFrame_GameThread(FrameTime_Mill);
						return true;
					}
					else
					{
						return false;
					}
				}, .0001f);
		}
		else
		{
			Engine->CookFrame_GameThread(FrameTime_Mill);
		}
	}
}

bool UTouchEngineInfo::IsLoaded() const
{
	return Engine->GetDidLoad();
}

bool UTouchEngineInfo::IsLoading() const
{
	return Engine && (Engine->GetIsLoading());
}

bool UTouchEngineInfo::IsCookComplete() const
{
	return !Engine || (Engine->MyNumOutputTexturesQueued == 0 && !Engine->MyCooking);
}

bool UTouchEngineInfo::HasFailedLoad() const
{
	return Engine->GetFailedLoad();
}

void UTouchEngineInfo::LogTouchEngineError(const FString& Error)
{
	Engine->OutputError(Error);
}

bool UTouchEngineInfo::IsRunning() const
{
	return Engine != nullptr; 
}

FTouchOnLoadFailed* UTouchEngineInfo::GetOnLoadFailedDelegate()
{
	return &Engine->OnLoadFailed;
}

FTouchOnParametersLoaded* UTouchEngineInfo::GetOnParametersLoadedDelegate()
{
	return &Engine->OnParametersLoaded;
}

FString UTouchEngineInfo::GetFailureMessage() const
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

TArray<FString> UTouchEngineInfo::GetCHOPChannelNames(const FString& Identifier) const
{
	if (Engine)
	{
		if (FTouchCHOPFull* FullChop = Engine->MyCHOPFullOutputs.Find(Identifier))
		{
			TArray<FString> RetVal;

			for (int32 i = 0; i < FullChop->SampleData.Num(); i++)
			{
				RetVal.Add(FullChop->SampleData[i].ChannelName);
			}
			return RetVal;
		}
	}

	return TArray<FString>();
}
