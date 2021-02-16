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
}

void UTouchEngineComponentBase::OnComponentCreated()
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("On Component Created %s"), *this->GetFName().ToString()));
	LoadParameters();
	Super::OnComponentCreated();
}

void UTouchEngineComponentBase::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("On Component Destroyed %s"), *this->GetFName().ToString()));

	if (EngineInfo)
	{
		EngineInfo->destroy();
		EngineInfo = nullptr;
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
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Orange, FString::Printf(TEXT("Post Edit Tox Path")));
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
		// locked sync mode stalls until we can get that frame's output. Ideally the cook is started right at the start of the unreal frame,
		// but then the outputs aren't read until later, letting unreal do some other work in the meantime
		dynamicVariables.SendInputs(EngineInfo);
		EngineInfo->cookFrame();
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
		// cook is finished, get outputs and start cook for next frame
		dynamicVariables.GetOutputs(EngineInfo);
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
		if (paramsLoadedDelHandle.IsValid() && loadFailedDelHandle.IsValid())
		{
			UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
			teSubsystem->UnbindDelegates(GetRelativeToxPath(), paramsLoadedDelHandle, loadFailedDelHandle);
		}

		LoadParameters();
	}
	dynamicVariables.OnToxLoadFailed.Broadcast();
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
