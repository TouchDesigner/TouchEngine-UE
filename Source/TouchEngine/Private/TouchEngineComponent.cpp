/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "TouchEngineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TouchEngineSubsystem.h"
#include "TouchEngineInfo.h"
#include "Misc/CoreDelegates.h"
#include "Engine.h"

UTouchEngineComponentBase::UTouchEngineComponentBase() : Super()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

UTouchEngineComponentBase::~UTouchEngineComponentBase()
{
	// Remove delegates if they're bound
	if (BeginFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnBeginFrame.Remove(BeginFrameDelHandle);
	}
	if (EndFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnEndFrame.Remove(EndFrameDelHandle);
	}

	UnbindDelegates();
}

void UTouchEngineComponentBase::BeginPlay()
{
	Super::BeginPlay();

	if (LoadOnBeginPlay)
	{
		// Create engine instance
		LoadTox();
	}

	// Bind delegates based on cook mode
	switch (CookMode)
	{
	case ETouchEngineCookMode::COOKMODE_DELAYEDSYNCHRONIZED:
	case ETouchEngineCookMode::COOKMODE_SYNCHRONIZED:
		BeginFrameDelHandle = FCoreDelegates::OnBeginFrame.AddUObject(this, &UTouchEngineComponentBase::OnBeginFrame);
		EndFrameDelHandle = FCoreDelegates::OnEndFrame.AddUObject(this, &UTouchEngineComponentBase::OnEndFrame);
		break;
	case ETouchEngineCookMode::COOKMODE_INDEPENDENT:

		break;
	}

	// without this crash can happen if the details panel accidentally binds to a world object
	DynamicVariables.OnToxLoaded.Clear();
	DynamicVariables.OnToxFailedLoad.Clear();
}

void UTouchEngineComponentBase::OnBeginFrame()
{
	if (!EngineInfo || !EngineInfo->IsLoaded())
	{
		// TouchEngine has not been started
		return;
	}


	switch (CookMode)
	{
	case ETouchEngineCookMode::COOKMODE_INDEPENDENT:

		break;
	case ETouchEngineCookMode::COOKMODE_DELAYEDSYNCHRONIZED:
	{
	}
	break;
	case ETouchEngineCookMode::COOKMODE_SYNCHRONIZED:

		// set cook time variables since we don't have delta time
		CookTime = UGameplayStatics::GetRealTimeSeconds(this);

		if (LastCookTime == 0)
			LastCookTime = CookTime;

		// start cook as early as possible
		VarsSetInputs();
		EngineInfo->CookFrame((CookTime - LastCookTime) * 10000);

		LastCookTime = CookTime;

		break;
	}
}

void UTouchEngineComponentBase::OnEndFrame()
{
	if (!EngineInfo || !EngineInfo->IsLoaded())
	{
		// TouchEngine has not been started
		return;
	}

	switch (CookMode)
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
	// Ensure we tick as early as possible
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	Super::OnComponentCreated();
}

void UTouchEngineComponentBase::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// Remove delegates if they're bound
	if (BeginFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnBeginFrame.Remove(BeginFrameDelHandle);
	}
	if (EndFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnEndFrame.Remove(EndFrameDelHandle);
	}
	if (ParamsLoadedDelHandle.IsValid() && LoadFailedDelHandle.IsValid())
	{
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		teSubsystem->UnbindDelegates(ParamsLoadedDelHandle, LoadFailedDelHandle);
	}

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UTouchEngineComponentBase::OnRegister()
{
	// Ensure we tick as early as possible
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	LoadParameters();

	Super::OnRegister();
}

void UTouchEngineComponentBase::OnUnregister()
{
	// Remove delegates if they're bound
	if (BeginFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnBeginFrame.Remove(BeginFrameDelHandle);
		BeginFrameDelHandle.Reset();
	}
	if (EndFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnEndFrame.Remove(EndFrameDelHandle);
		EndFrameDelHandle.Reset();
	}
	if (ParamsLoadedDelHandle.IsValid() && LoadFailedDelHandle.IsValid())
	{
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		teSubsystem->UnbindDelegates(ParamsLoadedDelHandle, LoadFailedDelHandle);
		ParamsLoadedDelHandle.Reset();
		LoadFailedDelHandle.Reset();
	}

	Super::OnUnregister();
}

#if WITH_EDITORONLY_DATA
void UTouchEngineComponentBase::PostEditChangeProperty(FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);

	FName PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxFilePath))
	{
		// unbind delegates if they're already bound
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		if (ParamsLoadedDelHandle.IsValid() || LoadFailedDelHandle.IsValid())
			teSubsystem->UnbindDelegates(ParamsLoadedDelHandle, LoadFailedDelHandle);
		// Regrab parameters if the ToxFilePath variable has been changed
		LoadParameters();
		// Refresh details panel
		DynamicVariables.OnToxFailedLoad.Broadcast(ErrorMessage);
	}
}

void UTouchEngineComponentBase::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}
#endif

void UTouchEngineComponentBase::LoadParameters()
{
	// Make sure dynamic variables parent is set
	DynamicVariables.parent = this;

	if (!GEngine)
	{
		return;
	}

	UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();

	// Attempt to grab parameters list. Send delegates to TouchEngine engine subsystem that will be called when parameters are loaded or fail to load.
	teSubsystem->GetParamsFromTox(
		GetAbsoluteToxPath(), this,
		FTouchOnParametersLoaded::FDelegate::CreateRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded),
		FTouchOnFailedLoad::FDelegate::CreateRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad),
		ParamsLoadedDelHandle, LoadFailedDelHandle
	);
}

void UTouchEngineComponentBase::ValidateParameters()
{
	UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
	auto params = teSubsystem->GetParamsFromTox(GetAbsoluteToxPath());
	if (params)
	{
		if (params->isLoaded)
		{
			DynamicVariables.ValidateParameters(params->Inputs, params->Outputs);
		}
	}
}

void UTouchEngineComponentBase::LoadTox()
{
	// set the parent of the dynamic variable container to this
	DynamicVariables.parent = this;

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

void UTouchEngineComponentBase::VarsSetInputs()
{
	SetInputs.Broadcast();
	switch (SendMode)
	{
	case ETouchEngineSendMode::SENDMODE_EVERYFRAME:
	{
		DynamicVariables.SendInputs(EngineInfo);
		break;
	}
	case ETouchEngineSendMode::SENDMODE_ONACCESS:
	{

		break;
	}
	}
}

void UTouchEngineComponentBase::VarsGetOutputs()
{
	GetOutputs.Broadcast();
	switch (SendMode)
	{
	case ETouchEngineSendMode::SENDMODE_EVERYFRAME:
	{
		DynamicVariables.GetOutputs(EngineInfo);
		break;
	}
	case ETouchEngineSendMode::SENDMODE_ONACCESS:
	{

		break;
	}
	}
}


void UTouchEngineComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// do nothing if tox file isn't loaded yet
	if (!EngineInfo || !EngineInfo->IsLoaded())
	{
		return;
	}


	switch (CookMode)
	{
	case ETouchEngineCookMode::COOKMODE_INDEPENDENT:
	{
		// Tell TouchEngine to run in Independent mode. Sets inputs arbitrarily, get outputs whenever they arrive
		VarsSetInputs();
		EngineInfo->CookFrame((int64)(10000 * DeltaTime));
		VarsGetOutputs();
	}
	break;
	case ETouchEngineCookMode::COOKMODE_SYNCHRONIZED:
	{
		// locked sync mode stalls until we can get that frame's output. Cook is started on begin frame,
		// outputs are read on tick

		// stall until cook is finished
		UTouchEngineInfo* savedEngineInfo = EngineInfo;
		FGenericPlatformProcess::ConditionalSleep([savedEngineInfo]() {return !savedEngineInfo->IsRunning() || savedEngineInfo->IsCookComplete(); }, .0001f);
		// cook is finished
		VarsGetOutputs();
	}
	break;
	case ETouchEngineCookMode::COOKMODE_DELAYEDSYNCHRONIZED:
	{
		// get previous frame output, then set new frame inputs and trigger a new cook.

		// make sure previous frame is done cooking, if it's not stall until it is
		UTouchEngineInfo* savedEngineInfo = EngineInfo;
		FGenericPlatformProcess::ConditionalSleep([savedEngineInfo]() {return !savedEngineInfo->IsRunning() || savedEngineInfo->IsCookComplete(); }, .0001f);
		// cook is finished, get outputs
		VarsGetOutputs();
		// send inputs (cook from last frame has been finished and outputs have been grabbed)
		VarsSetInputs();
		EngineInfo->CookFrame((int64)(10000 * DeltaTime));
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

		//EngineInfo->getOnLoadCompleteDelegate()->AddRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxLoaded);
		LoadFailedDelHandle = EngineInfo->GetOnLoadFailedDelegate()->AddRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad);
		ParamsLoadedDelHandle = EngineInfo->GetOnParametersLoadedDelegate()->AddRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded);
	}

	// Set variables in the EngineInfo
	EngineInfo->SetCookMode(CookMode == ETouchEngineCookMode::COOKMODE_INDEPENDENT);
	EngineInfo->SetFrameRate(TEFrameRate);
	// Tell the TouchEngine instance to load the tox file
	EngineInfo->Load(GetAbsoluteToxPath());
}

void UTouchEngineComponentBase::ReloadTox()
{
	if (EngineInfo)
	{
		// We're in a world object

		// destroy TouchEngine instance
		EngineInfo->Clear();
		EngineInfo = nullptr;
		// Reload 
		LoadTox();
	}
	else
	{
		// We're in an editor object, tell TouchEngine engine subsystem to reload the tox file
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		teSubsystem->ReloadTox(
			GetAbsoluteToxPath(), this,
			FTouchOnParametersLoaded::FDelegate::CreateRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded),
			FTouchOnFailedLoad::FDelegate::CreateRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad),
			ParamsLoadedDelHandle, LoadFailedDelHandle);
	}
}

bool UTouchEngineComponentBase::IsLoaded()
{
	if (EngineInfo)
	{
		// if this is a world object that has begun play and has a local touch engine instance
		return EngineInfo->IsLoaded();
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
		return EngineInfo->HasFailedLoad();
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
		EngineInfo->Destroy();
		EngineInfo = nullptr;
	}
}

void UTouchEngineComponentBase::UnbindDelegates()
{
	if (ParamsLoadedDelHandle.IsValid() && LoadFailedDelHandle.IsValid())
	{
		if (EngineInfo)
		{
			EngineInfo->GetOnLoadFailedDelegate()->Remove(LoadFailedDelHandle);
			EngineInfo->GetOnParametersLoadedDelegate()->Remove(ParamsLoadedDelHandle);
		}

		if (ParamsLoadedDelHandle.IsValid() || LoadFailedDelHandle.IsValid())
		{
			if (!GEngine)
				return;

			UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();

			if (teSubsystem)
			{
				teSubsystem->UnbindDelegates(ParamsLoadedDelHandle, LoadFailedDelHandle);
			}
		}
	}
}

bool UTouchEngineComponentBase::IsRunning()
{
	return EngineInfo->IsRunning();
}
