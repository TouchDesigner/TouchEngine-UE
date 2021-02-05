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
		EngineInfo->destroy();

	//EngineInfo->getOnLoadCompleteDelegate()->Remove(&testStruct, &FTouchEngineDynamicVariable::ToxLoaded);


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
		LoadTox();
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

	// tell dynamic variables to send inputs
	dynamicVariables.SendInputs(EngineInfo);
	// cook frame - use the inputs to create outputs
	EngineInfo->cookFrame();
	// tell dynamic variables to get outputs
	dynamicVariables.GetOutputs(EngineInfo);
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
