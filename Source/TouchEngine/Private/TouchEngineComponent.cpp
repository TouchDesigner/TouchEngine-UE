// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TouchEngineSubsystem.h"
#include "TouchEngineInfo.h"

// Sets default values for this component's properties
UTouchEngineComponentBase::UTouchEngineComponentBase() : Super()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

UTouchEngineComponentBase::~UTouchEngineComponentBase()
{
}

// Called when the game starts
void UTouchEngineComponentBase::BeginPlay()
{
	Super::BeginPlay();

	if (LoadOnBeginPlay)
	{
		// Create engine instance
		LoadTox();
	}

	// Bind delegates based on cook mode
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

	// without this crash can happen if the details panel accidentally binds to a world object
	dynamicVariables.OnToxLoaded.Clear();
	dynamicVariables.OnToxFailedLoad.Clear();
}

void UTouchEngineComponentBase::OnBeginFrame()
{
	if (!EngineInfo || !EngineInfo->isLoaded())
	{
		// TouchEngine has not been started
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

		// set cook time variables since we don't have delta time
		cookTime = UGameplayStatics::GetRealTimeSeconds(this);

		if (lastCookTime == 0)
			lastCookTime = cookTime;

		// start cook as early as possible
		dynamicVariables.SendInputs(EngineInfo);
		EngineInfo->cookFrame((cookTime - lastCookTime) * 10000);

		lastCookTime = cookTime;

		break;
	}
}

void UTouchEngineComponentBase::OnEndFrame()
{
	if (!EngineInfo || !EngineInfo->isLoaded())
	{
		// TouchEngine has not been started
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
	// Attempt to grab parameters from the TouchEngine engine subsystem
	LoadParameters();
	// Ensure we tick as early as possible
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	Super::OnComponentCreated();
}

void UTouchEngineComponentBase::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// Remove delegates if they're bound
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
		teSubsystem->UnbindDelegates(GetAbsoluteToxPath(), paramsLoadedDelHandle, loadFailedDelHandle);
	}

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UTouchEngineComponentBase::PostEditChangeProperty(FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);

	FName PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxFilePath))
	{
		// Regrab parameters if the ToxFilePath variable has been changed
		LoadParameters();
		// Refresh details panel
		dynamicVariables.OnToxFailedLoad.Broadcast();
	}
}

void UTouchEngineComponentBase::LoadParameters()
{
	// Make sure dynamic variables parent is set
	dynamicVariables.parent = this;

	if (ToxFilePath.IsEmpty())
	{
		// No tox path set
		return;
	}

	// Attempt to grab parameters list. Send delegates to TouchEngine engine subsystem that will be called when parameters are loaded or fail to load.
	UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
	teSubsystem->GetParamsFromTox(
		GetAbsoluteToxPath(),
		FTouchOnParametersLoaded::FDelegate::CreateRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded),
		FSimpleDelegate::CreateRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad),
		paramsLoadedDelHandle, loadFailedDelHandle
	);
}

void UTouchEngineComponentBase::LoadTox()
{
	if (ToxFilePath.IsEmpty())
	{
		// No tox path set
		return;
	}

	// set the parent of the dynamic variable container to this
	dynamicVariables.parent = this;

	if (!EngineInfo)
	{
		// Create TouchEngine instance if we haven't loaded one already
		CreateEngineInfo();
	}
}

FString UTouchEngineComponentBase::GetAbsoluteToxPath()
{
	if (ToxFilePath.IsEmpty())
	{
		// No tox path set
		return FString();
	}

	FString absoluteToxPath;
	absoluteToxPath = FPaths::ProjectContentDir();
	absoluteToxPath.Append(ToxFilePath);
	return absoluteToxPath;
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
		EngineInfo->cookFrame((int64)(10000 * DeltaTime));
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
		EngineInfo->cookFrame((int64)(10000 * DeltaTime));
	}
	break;
	}
}

void UTouchEngineComponentBase::CreateEngineInfo()
{
	if (!EngineInfo)
	{
		// Create TouchEngine instance if we don't have one already
		EngineInfo = NewObject< UTouchEngineInfo>();

		paramsLoadedDelHandle = EngineInfo->getOnLoadCompleteDelegate()->AddRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxLoaded);
		loadFailedDelHandle = EngineInfo->getOnLoadFailedDelegate()->AddRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad);
		EngineInfo->getOnParametersLoadedDelegate()->AddRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded);
	}
	
	// Set variables in the EngineInfo
	EngineInfo->setCookMode(cookMode == ETouchEngineCookMode::COOKMODE_INDEPENDENT);
	EngineInfo->setFrameRate(TEFrameRate);
	// Tell the TouchEngine instance to load the tox file
	EngineInfo->load(GetAbsoluteToxPath());
}

void UTouchEngineComponentBase::ReloadTox()
{
	if (ToxFilePath.IsEmpty())
	{
		// No tox path set
		return;
	}

	if (EngineInfo)
	{
		// We're in a world object

		// destroy TouchEngine instance
		EngineInfo->clear();
		// Reload 
		LoadTox();
	}
	else
	{
		// We're in an editor object, tell TouchEngine engine subsystem to reload the tox file
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		teSubsystem->ReloadTox(
			GetAbsoluteToxPath(),
			FTouchOnParametersLoaded::FDelegate::CreateRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded),
			FSimpleDelegate::CreateRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad),
			paramsLoadedDelHandle, loadFailedDelHandle);
	}
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
		return teSubsystem->IsLoaded(GetAbsoluteToxPath());
	}
}

bool UTouchEngineComponentBase::HasFailedLoad()
{
	if (EngineInfo)
	{
		// if this is a world object that has begun play and has a local touch engine instance
		return EngineInfo->hasFailedLoad();
	}
	else
	{
		// this object has no local touch engine instance, must check the subsystem to see if our tox file has failed to load
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return teSubsystem->HasFailedLoad(GetAbsoluteToxPath());
	}
}

void UTouchEngineComponentBase::StartTouchEngine()
{
	LoadTox();
}

void UTouchEngineComponentBase::StopTouchEngine()
{
	if (EngineInfo)
	{
		EngineInfo->destroy();
		EngineInfo = nullptr;
	}
}