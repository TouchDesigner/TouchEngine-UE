// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineComponent.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Editor.h"

// Sets default values for this component's properties
UTouchEngineComponentBase::UTouchEngineComponentBase()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UTouchEngineComponentBase::BeginPlay()
{
	Super::BeginPlay();

	LoadTox();


	switch (cookMode)
	{
	case ETouchEngineCookMode::COOKMODE_DELAYEDSYNCHRONIZED:
	case ETouchEngineCookMode::COOKMODE_SYNCHRONIZED:
		beginFrameDelHandle = FCoreDelegates::OnBeginFrame.AddUObject(this, &UTouchEngineComponentBase::OnBeginFrame);
		endFrameDelHandle = FCoreDelegates::OnEndFrame.AddUObject(this, &UTouchEngineComponentBase::OnEndFrame);
		break;
	case ETouchEngineCookMode::COOKMODE_INDEPENDENT:

		break;
	}
}

void UTouchEngineComponentBase::OnBeginFrame()
{
	if (!EngineInfo || !EngineInfo->isLoaded())
	{
		return;
	}


	switch (cookMode)
	{
	case ETouchEngineCookMode::COOKMODE_INDEPENDENT:

		break;
	case ETouchEngineCookMode::COOKMODE_DELAYEDSYNCHRONIZED:
	{
	}
		break;
	case ETouchEngineCookMode::COOKMODE_SYNCHRONIZED:

		// start cook as early as possible
		dynamicVariables.SendInputs(EngineInfo);
		EngineInfo->cookFrame();

		break;
	}
}

void UTouchEngineComponentBase::OnEndFrame()
{
	if (!EngineInfo || !EngineInfo->isLoaded())
	{
		return;
	}

	switch (cookMode)
	{
	case ETouchEngineCookMode::COOKMODE_INDEPENDENT:

		break;
	case ETouchEngineCookMode::COOKMODE_DELAYEDSYNCHRONIZED:

		break;
	case ETouchEngineCookMode::COOKMODE_SYNCHRONIZED:

		break;
	}
}

void UTouchEngineComponentBase::OnComponentCreated()
{
	LoadParameters();
	Super::OnComponentCreated();

	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTouchEngineComponentBase::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (EngineInfo)
	{
		EngineInfo->destroy();
		EngineInfo = nullptr;
	}

	if (beginFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnBeginFrame.Remove(beginFrameDelHandle);
	}
	if (endFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnEndFrame.Remove(endFrameDelHandle);
	}

	if (paramsLoadedDelHandle.IsValid() && loadFailedDelHandle.IsValid())
	{
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		teSubsystem->UnbindDelegates(GetRelativeToxPath(), paramsLoadedDelHandle, loadFailedDelHandle);
	}

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UTouchEngineComponentBase::PostEditChangeProperty(FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);

	FName PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxFilePath))
	{
		LoadParameters();
		dynamicVariables.OnToxLoadFailed.Broadcast();
	}
}

void UTouchEngineComponentBase::LoadParameters()
{
 	if (ToxFilePath.IsEmpty())
		return;

	UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();

	teSubsystem->GetParamsFromTox(
		GetRelativeToxPath(),
		FTouchOnParametersLoaded::FDelegate::CreateRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded),
		FSimpleDelegate::CreateRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad),
		paramsLoadedDelHandle, loadFailedDelHandle
	);

	//teSubsystem-
}

void UTouchEngineComponentBase::LoadTox()
{
	if (ToxFilePath.IsEmpty())
		return;

	dynamicVariables.parent = this;
	CreateEngineInfo();
}

FString UTouchEngineComponentBase::GetRelativeToxPath()
{
	if (ToxFilePath.IsEmpty())
		return FString();

	FString relativeToxPath;
	relativeToxPath = FPaths::ProjectDir();
	relativeToxPath.Append(ToxFilePath);
	return relativeToxPath;
}


// Called every frame
void UTouchEngineComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// do nothing if tox file isn't loaded yet
	if (!EngineInfo || !EngineInfo->isLoaded())
	{
		return;
	}


	switch (cookMode)
	{
	case ETouchEngineCookMode::COOKMODE_INDEPENDENT:
	{
		// Tell TouchEngine to run in Independent mode. Sets inputs arbitrarily, get outputs whenever they arrive
		dynamicVariables.SendInputs(EngineInfo);
		EngineInfo->cookFrame();
		dynamicVariables.GetOutputs(EngineInfo);
	}
	break;
	case ETouchEngineCookMode::COOKMODE_SYNCHRONIZED:
	{
		// locked sync mode stalls until we can get that frame's output. Cook is started on begin frame,
		// outputs are read on tick

		// stall until cook is finished
		UTouchEngineInfo* savedEngineInfo = EngineInfo;
		FGenericPlatformProcess::ConditionalSleep([savedEngineInfo]() {return savedEngineInfo->isCookComplete(); }, .0001f);
		// cook is finished
		dynamicVariables.GetOutputs(EngineInfo);
	}
	break;
	case ETouchEngineCookMode::COOKMODE_DELAYEDSYNCHRONIZED:
	{
		// get previous frame output, then set new frame inputs and trigger a new cook.

		// make sure previous frame is done cooking, if it's not stall until it is
		UTouchEngineInfo* savedEngineInfo = EngineInfo;
		FGenericPlatformProcess::ConditionalSleep([savedEngineInfo]() {return savedEngineInfo->isCookComplete(); }, .0001f);
		// cook is finished, get outputs
		dynamicVariables.GetOutputs(EngineInfo);
		// send inputs (cook from last frame has been finished and outputs have been grabbed)
		dynamicVariables.SendInputs(EngineInfo);
		EngineInfo->cookFrame();
	}
	break;
	}
}


void UTouchEngineComponentBase::CreateEngineInfo()
{
	if (!EngineInfo)
	{
		EngineInfo = NewObject< UTouchEngineInfo>();

		EngineInfo->getOnLoadCompleteDelegate()->AddRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxLoaded);
		EngineInfo->getOnLoadFailedDelegate()->AddRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad);
		EngineInfo->getOnParametersLoadedDelegate()->AddRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded);
	}

	EngineInfo->setCookMode(cookMode == ETouchEngineCookMode::COOKMODE_INDEPENDENT);
	EngineInfo->load(GetRelativeToxPath());
}

void UTouchEngineComponentBase::ReloadTox()
{
	if (ToxFilePath.IsEmpty())
		return;

	if (EngineInfo)
	{
		EngineInfo->clear();
		LoadTox();
	}
	else
	{
		//if (paramsLoadedDelHandle.IsValid() && loadFailedDelHandle.IsValid())
		//{
			UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
			//teSubsystem->UnbindDelegates(GetRelativeToxPath(), paramsLoadedDelHandle, loadFailedDelHandle);
			teSubsystem->ReloadTox(
				GetRelativeToxPath(),
				FTouchOnParametersLoaded::FDelegate::CreateRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded),
		FSimpleDelegate::CreateRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad),
		paramsLoadedDelHandle, loadFailedDelHandle);
		//}

		//LoadParameters();
	}
	//dynamicVariables.OnToxLoadFailed.Broadcast();
}

bool UTouchEngineComponentBase::IsLoaded()
{
	if (EngineInfo)
	{
		// if this is a world object that has begun play and has a local touch engine instance
		return EngineInfo->isLoaded();
	}
	else
	{
		// this object has no local touch engine instance, must check the subsystem to see if our tox file has already been loaded
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return teSubsystem->IsLoaded(GetRelativeToxPath());
	}
}

bool UTouchEngineComponentBase::HasFailedLoad()
{
	if (EngineInfo)
	{
		return EngineInfo->hasFailedLoad();
	}
	else
	{
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return teSubsystem->HasFailedLoad(GetRelativeToxPath());
	}
}
