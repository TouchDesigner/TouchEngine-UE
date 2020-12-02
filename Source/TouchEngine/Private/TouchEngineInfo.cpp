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
	//engine = nullptr;
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

	/*
	*/
	if (engine->getToxPath() != toxPath)
	{
		engine->loadTox(toxPath);
	}

	/*
	if (!engine)
	{
		if (toxPath.IsEmpty())
			return false;

		UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		engine = TESubsystem->LoadTox(toxPath, this);
	}
	*/

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

FTouchOnLoadComplete* UTouchEngineInfo::getOnLoadCompleteDelegate()
{
	return &engine->OnLoadComplete;
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