// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineInfo.h"

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

	return engine->getDidLoad();
}

FTouchCHOPSingleSample
UTouchEngineInfo::getCHOPOutputSingleSample(const FString& identifier)
{
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
	if (engine)
	{
		return engine->setCHOPInputSingleSample(identifier, chop);
	}
}

FTouchTOP
UTouchEngineInfo::getTOPOutput(const FString& identifier)
{
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
	if (engine)
		engine->setTOPInput(identifier, texture);
}

FTouchOP<bool>				
UTouchEngineInfo::getBOPOutput(const FString& identifier) { return engine->getBOPOutput(identifier); }
void						
UTouchEngineInfo::setBOPInput(const FString& identifier, FTouchOP<bool>& op) { engine->setBOPInput(identifier, op); }
FTouchOP<double>			
UTouchEngineInfo::getDOPOutput(const FString& identifier) { return engine->getDOPOutput(identifier); }
void						
UTouchEngineInfo::setDOPInput(const FString& identifier, FTouchOP<TArray<double>>& op) { engine->setDOPInput(identifier, op); }
FTouchOP<int32_t>			
UTouchEngineInfo::getIOPOutput(const FString& identifier) { return engine->getIOPOutput(identifier); }
void						
UTouchEngineInfo::setIOPInput(const FString& identifier, FTouchOP<int32_t>& op) { engine->setIOPInput(identifier, op); }
FTouchOP<TEString*>			
UTouchEngineInfo::getSOPOutput(const FString& identifier) { return engine->getSOPOutput(identifier); }
void						
UTouchEngineInfo::setSOPInput(const FString& identifier, FTouchOP<char*>& op) { engine->setSOPInput(identifier, op); }

FTouchOP<TETable*> UTouchEngineInfo::getSTOPOutput(const FString& identifier)
{
	return engine->getSTOPOutput(identifier);
}

void UTouchEngineInfo::setSTOPInput(const FString& identifier, FTouchOP<TETable*>& op)
{
	engine->setSTOPInput(identifier, op);
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