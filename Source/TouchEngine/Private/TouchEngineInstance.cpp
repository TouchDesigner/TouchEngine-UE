// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineInstance.h"

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
ATouchEngineInstance::getToxPath() const
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
ATouchEngineInstance::load(FString toxPath)
{
	if (!myEngine || myEngine->getToxPath() != toxPath)
	{
		myEngine = nullptr;
		myEngine = UTouchEngineSubsystem::createEngine(toxPath);
	}
}

FTouchCHOPSingleSample
ATouchEngineInstance::getCHOPOutputSingleSample(const FString& identifier)
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
ATouchEngineInstance::setCHOPInputSingleSample(const FString &identifier, const FTouchCHOPSingleSample &chop)
{
	if (myEngine)
	{
		return myEngine->setCHOPInputSingleSample(identifier, chop);
	}
}

FTouchTOP
ATouchEngineInstance::getTOPOutput(const FString& identifier)
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
ATouchEngineInstance::cookFrame()
{
	if (myEngine)
	{
		return myEngine->cookFrame();
	}
}
