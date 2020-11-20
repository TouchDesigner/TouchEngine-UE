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

ATouchEngineInfo::ATouchEngineInfo() : Super()
{
	engine = NewObject<UTouchEngine>();
}

FString
ATouchEngineInfo::getToxPath() const
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
ATouchEngineInfo::load(FString toxPath)
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
ATouchEngineInfo::getCHOPOutputSingleSample(const FString& identifier)
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
ATouchEngineInfo::setCHOPInputSingleSample(const FString &identifier, const FTouchCHOPSingleSample &chop)
{
	if (engine)
	{
		return engine->setCHOPInputSingleSample(identifier, chop);
	}
}

FTouchTOP
ATouchEngineInfo::getTOPOutput(const FString& identifier)
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
ATouchEngineInfo::setTOPInput(const FString& identifier, UTexture* texture)
{
	if (engine)
		engine->setTOPInput(identifier, texture);
}

void
ATouchEngineInfo::cookFrame()
{
	if (engine)
	{
		return engine->cookFrame();
	}
}

bool ATouchEngineInfo::isLoaded()
{
	return engine->getDidLoad();
}


UTouchOnLoadTask* UTouchOnLoadTask::WaitOnLoadTask(UObject* WorldContextObject, ATouchEngineInfo* EngineInfo, FString toxPath)
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
		engineInfo->engine->OnLoadComplete.AddDynamic(this, &UTouchOnLoadTask::OnLoadComplete);
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