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

FString
ATouchEngineInfo::getToxPath() const
{
	if (myEngine)
	{
		return myEngine->getToxPath();
	}
	else
	{
		return FString();
	}

}

void
ATouchEngineInfo::load(FString toxPath)
{
	if (!myEngine || myEngine->getToxPath() != toxPath)
	{
		myEngine = nullptr;
		myEngine = UTouchEngineSubsystem::createEngine(toxPath);
	}
}

FTouchCHOPSingleSample
ATouchEngineInfo::getCHOPOutputSingleSample(const FString& identifier)
{
	if (myEngine)
	{
		return myEngine->getCHOPOutputSingleSample(identifier);
	}
	else
	{
		return FTouchCHOPSingleSample();
	}
}

void
ATouchEngineInfo::setCHOPInputSingleSample(const FString &identifier, const FTouchCHOPSingleSample &chop)
{
	if (myEngine)
	{
		return myEngine->setCHOPInputSingleSample(identifier, chop);
	}
}

FTouchTOP
ATouchEngineInfo::getTOPOutput(const FString& identifier)
{
	if (myEngine)
	{
		return myEngine->getTOPOutput(identifier);
	}
	else
	{
		return FTouchTOP();
	}
}

void
ATouchEngineInfo::cookFrame()
{
	if (myEngine)
	{
		return myEngine->cookFrame();
	}
}
