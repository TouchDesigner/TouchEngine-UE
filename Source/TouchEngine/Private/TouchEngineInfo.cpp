// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineInfo.h"
#include "UTouchEngine.h"

DECLARE_STATS_GROUP(TEXT("TouchEngine"), STATGROUP_TouchEngine, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("VarSet"), STAT_StatsVarSet, STATGROUP_TouchEngine);
DECLARE_CYCLE_STAT(TEXT("VarGet"), STAT_StatsVarGet, STATGROUP_TouchEngine);


UTouchEngineInfo::UTouchEngineInfo() : Super()
{
	engine = NewObject<UTouchEngine>();
}

FString
UTouchEngineInfo::getToxPath() const
{
	if (engine)
	{
		// engine has been created
		return engine->getToxPath();
	}
	else
	{
		// engine has not been loaded, return empty
		return FString();
	}

}

bool 
UTouchEngineInfo::setCookMode(bool isIndependent)
{
	return engine->setCookMode(isIndependent);
}

bool
UTouchEngineInfo::setFrameRate(int64 FrameRate)
{
	return engine->setFrameRate(FrameRate);
}

bool
UTouchEngineInfo::load(FString toxPath)
{
	// ensure file exists
	if (!FPaths::FileExists(toxPath))
	{
		// try scoping to content directory if we can't find it
		toxPath = FPaths::ProjectContentDir() + toxPath;

		if (!FPaths::FileExists(toxPath))
		{
			// file does not exist
			UE_LOG(LogTemp, Error, TEXT("Invalid file path"));
			engine->OnLoadFailed.Broadcast();
			return false;
		}
	}

	if (engine->getToxPath() != toxPath)
	{
		engine->loadTox(toxPath);
	}

	// sometimes we destroy engine on failure notifications, make sure it's still valid
	if (engine)
	{
		return engine->getDidLoad();
	}
	else
	{
		// engine->loadTox failed and engine was destroyed
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
UTouchEngineInfo::setIntegerInput(const FString& identifier, FTouchVar<TArray<int32_t>>& op)
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
UTouchEngineInfo::cookFrame(int64 FrameTime_Mill)
{
	if (engine)
	{
		return engine->cookFrame(FrameTime_Mill);
	}
}

bool 
UTouchEngineInfo::isLoaded()
{
	return engine->getDidLoad();
}

bool 
UTouchEngineInfo::isCookComplete()
{
	return !engine->myCooking;
}

bool 
UTouchEngineInfo::hasFailedLoad() { return engine->getFailedLoad(); }

void 
UTouchEngineInfo::logTouchEngineError(FString error)
{
	engine->outputError(error);
}

FTouchOnLoadComplete* 
UTouchEngineInfo::getOnLoadCompleteDelegate()
{
	return &engine->OnLoadComplete;
}

FTouchOnLoadFailed* 
UTouchEngineInfo::getOnLoadFailedDelegate()
{
	return &engine->OnLoadFailed;
}

FTouchOnParametersLoaded* 
UTouchEngineInfo::getOnParametersLoadedDelegate()
{
	return &engine->OnParametersLoaded;
}