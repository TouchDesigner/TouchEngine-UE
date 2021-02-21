// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineInfo.h"

DECLARE_STATS_GROUP(TEXT("TouchEngine"), STATGROUP_TouchEngine, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("VarSet"), STAT_StatsVarSet, STATGROUP_TouchEngine);
DECLARE_CYCLE_STAT(TEXT("VarGet"), STAT_StatsVarGet, STATGROUP_TouchEngine);

#if 0
void
ATouchEngineInstance::BeginPlay()
{
	if (myEngine)
	{
		UTouchEngineSubsystem::destroyEngine(&myEngine);
	}

	myEngine = UTouchEngineSubsystem::createEngine();

}
#endif

UTouchEngineInfo::UTouchEngineInfo() : Super()
{
	engine = NewObject<UTouchEngine>();
}

FString
UTouchEngineInfo::getToxPath() const
{
	if (engine)
	{
		return engine->getToxPath();
	}
	else
	{
		return FString();
	}

}

bool UTouchEngineInfo::setCookMode(bool isIndependent)
{
	return engine->setCookMode(isIndependent);
}

bool
UTouchEngineInfo::load(FString toxPath)
{
	if (!FPaths::FileExists(toxPath) && FPaths::FileExists(FPaths::ProjectContentDir() + toxPath))
	{
		toxPath = FPaths::ProjectContentDir() + toxPath;
	}

	if (engine->getToxPath() != toxPath)
	{
		engine->loadTox(toxPath);
	}

	// sometimes we destroy engine on failure notifications
	if (engine)
	{
		return engine->getDidLoad();
	}
	else
	{
		return false;
	}
}

void
UTouchEngineInfo::clear()
{
	if (engine)
		engine->clear();
}

void
UTouchEngineInfo::destroy()
{
	clear();
	engine = nullptr;
}

FTouchCHOPSingleSample
UTouchEngineInfo::getCHOPOutputSingleSample(const FString& identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);

	if (engine)
	{
		return engine->getCHOPOutputSingleSample(identifier);
	}
	else
	{
		return FTouchCHOPSingleSample();
	}
}

void
UTouchEngineInfo::setCHOPInputSingleSample(const FString& identifier, const FTouchCHOPSingleSample& chop)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	if (engine)
	{
		return engine->setCHOPInputSingleSample(identifier, chop);
	}
}

FTouchTOP
UTouchEngineInfo::getTOPOutput(const FString& identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);

	if (engine)
	{
		return engine->getTOPOutput(identifier);
	}
	else
	{
		return FTouchTOP();
	}
}

void
UTouchEngineInfo::setTOPInput(const FString& identifier, UTexture* texture)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	if (engine)
		engine->setTOPInput(identifier, texture);
}

FTouchVar<bool>
UTouchEngineInfo::getBooleanOutput(const FString& identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return engine->getBooleanOutput(identifier);
}
void
UTouchEngineInfo::setBooleanInput(const FString& identifier, FTouchVar<bool>& op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	engine->setBooleanInput(identifier, op);
}
FTouchVar<double>
UTouchEngineInfo::getDoubleOutput(const FString& identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return engine->getDoubleOutput(identifier);
}
void
UTouchEngineInfo::setDoubleInput(const FString& identifier, FTouchVar<TArray<double>>& op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	engine->setDoubleInput(identifier, op);
}
FTouchVar<int32_t>
UTouchEngineInfo::getIntegerOutput(const FString& identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return engine->getIntegerOutput(identifier);
}
void
UTouchEngineInfo::setIntegerInput(const FString& identifier, FTouchVar<int32_t>& op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	engine->setIntegerInput(identifier, op);
}
FTouchVar<TEString*>
UTouchEngineInfo::getStringOutput(const FString& identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return engine->getStringOutput(identifier);
}
void
UTouchEngineInfo::setStringInput(const FString& identifier, FTouchVar<char*>& op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	engine->setStringInput(identifier, op);
}

FTouchVar<TETable*> UTouchEngineInfo::getTableOutput(const FString& identifier)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarGet);
	return engine->getTableOutput(identifier);
}

void UTouchEngineInfo::setTableInput(const FString& identifier, FTouchVar<TETable*>& op)
{
	SCOPE_CYCLE_COUNTER(STAT_StatsVarSet);
	engine->setTableInput(identifier, op);
}



void
UTouchEngineInfo::cookFrame()
{
	if (engine)
	{
		return engine->cookFrame();
	}
}

bool UTouchEngineInfo::isLoaded()
{
	return engine->getDidLoad();
}

bool UTouchEngineInfo::isCookComplete()
{
	return !engine->myCooking;
}

bool UTouchEngineInfo::hasFailedLoad() { return engine->getFailedLoad(); }

void UTouchEngineInfo::logTouchEngineError(FString error)
{
	engine->outputError(error);
}

FTouchOnLoadComplete* UTouchEngineInfo::getOnLoadCompleteDelegate()
{
	return &engine->OnLoadComplete;
}

FTouchOnLoadFailed* UTouchEngineInfo::getOnLoadFailedDelegate()
{
	return &engine->OnLoadFailed;
}

FTouchOnParametersLoaded* UTouchEngineInfo::getOnParametersLoadedDelegate()
{
	return &engine->OnParametersLoaded;
}


UTouchOnLoadTask* UTouchOnLoadTask::WaitOnLoadTask(UObject* WorldContextObject, UTouchEngineInfo* EngineInfo, FString toxPath)
{
	UTouchOnLoadTask* LoadTask = NewObject<UTouchOnLoadTask>();

	LoadTask->engineInfo = EngineInfo;
	LoadTask->ToxPath = toxPath;
	LoadTask->RegisterWithGameInstance(WorldContextObject);

	return LoadTask;
}

void UTouchOnLoadTask::Activate()
{
	if (IsValid(engineInfo))
	{
		engineInfo->engine->OnLoadComplete.AddUObject(this, &UTouchOnLoadTask::OnLoadComplete);
		engineInfo->load(ToxPath);
	}
}

void UTouchOnLoadTask::OnLoadComplete()
{
	if (IsValid(engineInfo))
	{
		LoadComplete.Broadcast();
		SetReadyToDestroy();
	}
}