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

#include "Blueprint/TouchEngineComponent.h"

#include "Engine/TouchEngineInfo.h"
#include "Engine/TouchEngineSubsystem.h"

#include "Engine/Engine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"

UTouchEngineComponentBase::UTouchEngineComponentBase()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTouchEngineComponentBase::ReloadTox()
{
	if (ShouldUseLocalTouchEngine()) 
	{
		ReleaseResources();
		LoadTox();
	}
	else
	{
		// We're in a play world, tell TouchEngine engine subsystem to reload the tox file
		UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		TESubsystem->ReloadTox(
			GetAbsoluteToxPath(),
			this,
			FTouchOnParametersLoaded::FDelegate::CreateRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded),
			FTouchOnFailedLoad::FDelegate::CreateRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad),
			ParamsLoadedDelegateHandle,
			LoadFailedDelegateHandle
			);
	}
}

bool UTouchEngineComponentBase::IsLoaded() const
{
	if (ShouldUseLocalTouchEngine())
	{
		return EngineInfo && EngineInfo->IsLoaded();
	}
	else
	{
		// this object has no local touch engine instance, must check the subsystem to see if our tox file has already been loaded
		UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return TESubsystem->IsLoaded(GetAbsoluteToxPath());
	}
}

bool UTouchEngineComponentBase::HasFailedLoad() const
{
	if (EngineInfo)
	{
		// if this is a world object that has begun play and has a local touch engine instance
		return EngineInfo->HasFailedLoad();
	}
	else
	{
		// this object has no local touch engine instance, must check the subsystem to see if our tox file has failed to load
		UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return TESubsystem->HasFailedLoad(GetAbsoluteToxPath());
	}
}

void UTouchEngineComponentBase::StartTouchEngine()
{
	ReloadTox();
}

void UTouchEngineComponentBase::StopTouchEngine()
{
	ReleaseResources();
}

bool UTouchEngineComponentBase::IsRunning() const
{
	return EngineInfo->IsRunning();
}

void UTouchEngineComponentBase::UnbindDelegates()
{
	if (ParamsLoadedDelegateHandle.IsValid() && LoadFailedDelegateHandle.IsValid())
	{
		if (EngineInfo)
		{
			EngineInfo->GetOnLoadFailedDelegate()->Remove(LoadFailedDelegateHandle);
			EngineInfo->GetOnParametersLoadedDelegate()->Remove(ParamsLoadedDelegateHandle);
		}

		if (ParamsLoadedDelegateHandle.IsValid() || LoadFailedDelegateHandle.IsValid())
		{
			if (!GEngine)
			{
				return;
			}

			if (UTouchEngineSubsystem* TouchEngineSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>())
			{
				TouchEngineSubsystem->UnbindDelegates(ParamsLoadedDelegateHandle, LoadFailedDelegateHandle);
			}
		}
	}
}


void UTouchEngineComponentBase::BeginDestroy()
{
	ReleaseResources();
	Super::BeginDestroy();
}

#if WITH_EDITORONLY_DATA
void UTouchEngineComponentBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxFilePath))
	{
		// unbind delegates if they're already bound
		UnbindDelegates();
		// Regrab parameters if the ToxFilePath variable has been changed
		LoadParameters();
		
		// Refresh details panel
		DynamicVariables.OnToxFailedLoad.Broadcast(ErrorMessage);
	}
}
#endif

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
	case ETouchEngineCookMode::DelayedSynchronized:
	case ETouchEngineCookMode::Synchronized:
		BeginFrameDelegateHandle = FCoreDelegates::OnBeginFrame.AddUObject(this, &UTouchEngineComponentBase::OnBeginFrame);
		break;
	case ETouchEngineCookMode::Independent:
		break;
	default: ;
	}

	// without this crash can happen if the details panel accidentally binds to a world object
	DynamicVariables.OnToxLoaded.Clear();
	DynamicVariables.OnToxFailedLoad.Clear();
}

void UTouchEngineComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Do nothing if ...
	if (!EngineInfo // ... we're not supposed to load anything
		// ... tox file isn't loaded yet
		|| !EngineInfo->IsLoaded())
	{
		return;
	}

	switch (CookMode)
	{
	case ETouchEngineCookMode::Independent:
		{
			// Tell TouchEngine to run in Independent mode. Sets inputs arbitrarily, get outputs whenever they arrive
			VarsSetInputs();
			EngineInfo->CookFrame((int64)(10000 * DeltaTime));
			VarsGetOutputs();
			break;
		}
	case ETouchEngineCookMode::Synchronized:
		{
			// locked sync mode stalls until we can get that frame's output. Cook is started on begin frame,
			// outputs are read on tick

			// stall until cook is finished
			UTouchEngineInfo* SavedEngineInfo = EngineInfo;
			FGenericPlatformProcess::ConditionalSleep([SavedEngineInfo]()
				{
					return !SavedEngineInfo->IsRunning() || SavedEngineInfo->IsCookComplete();
				}
			, .0001f);

			// cook is finished
			VarsGetOutputs();

			break;
		}
	case ETouchEngineCookMode::DelayedSynchronized:
		{
			// get previous frame output, then set new frame inputs and trigger a new cook.

			// make sure previous frame is done cooking, if it's not stall until it is
			UTouchEngineInfo* SavedEngineInfo = EngineInfo;
			FGenericPlatformProcess::ConditionalSleep([SavedEngineInfo]() {return !SavedEngineInfo->IsRunning() || SavedEngineInfo->IsCookComplete(); }, .0001f);
			// cook is finished, get outputs
			VarsGetOutputs();
			// send inputs (cook from last frame has been finished and outputs have been grabbed)
			VarsSetInputs();
			EngineInfo->CookFrame((int64)(10000 * DeltaTime));
			break;
		}
	default: ;
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
	ReleaseResources();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UTouchEngineComponentBase::OnRegister()
{
	//LoadParameters();
	ValidateParameters();
	Super::OnRegister();
}

void UTouchEngineComponentBase::OnUnregister()
{
	ReleaseResources();
	Super::OnUnregister();
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
	case ETouchEngineCookMode::Independent:
	case ETouchEngineCookMode::DelayedSynchronized:
		break;
	case ETouchEngineCookMode::Synchronized:
		VarsSetInputs();
		EngineInfo->CookFrame(GetWorld()->DeltaTimeSeconds * 10000);
		break;
	default: ;
	}
}

void UTouchEngineComponentBase::LoadParameters()
{
	// Make sure dynamic variables parent is set
	DynamicVariables.Parent = this;

	if (!IsValid(GEngine))
	{
		return;
	}

	UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();

	// Attempt to grab parameters list. Send delegates to TouchEngine engine subsystem that will be called when parameters are loaded or fail to load.
	TESubsystem->GetParamsFromTox(
		GetAbsoluteToxPath(), this,
		FTouchOnParametersLoaded::FDelegate::CreateRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded),
		FTouchOnFailedLoad::FDelegate::CreateRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad),
		ParamsLoadedDelegateHandle, LoadFailedDelegateHandle
	);
}

void UTouchEngineComponentBase::ValidateParameters()
{
	UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
	UFileParams* Params = TESubsystem->GetParamsFromTox(GetAbsoluteToxPath());
	if (Params)
	{
		if (Params->bIsLoaded)
		{
			// these params have loaded from another object
			DynamicVariables.ValidateParameters(Params->Inputs, Params->Outputs);
		}
		else
		{
			if (ParamsLoadedDelegateHandle.IsValid())
			{
				UnbindDelegates();
			}

			if (!Params->bHasFailedLoad)
			{
				Params->BindOrCallDelegates(
					this,
					FTouchOnParametersLoaded::FDelegate::CreateRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded),
					FTouchOnFailedLoad::FDelegate::CreateRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad),
					ParamsLoadedDelegateHandle,
					LoadFailedDelegateHandle
					);
			}
		}
	}
	else
	{
		LoadParameters();
	}
}

void UTouchEngineComponentBase::LoadTox()
{
	// set the parent of the dynamic variable container to this
	DynamicVariables.Parent = this;

	if (!EngineInfo)
	{
		// Create TouchEngine instance if we haven't loaded one already
		CreateEngineInfo();
	}
}

void UTouchEngineComponentBase::CreateEngineInfo()
{
	if (!EngineInfo)
	{
		// Create TouchEngine instance if we don't have one already
		EngineInfo = NewObject<UTouchEngineInfo>(this);

		LoadFailedDelegateHandle = EngineInfo->GetOnLoadFailedDelegate()->AddRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad);
		ParamsLoadedDelegateHandle = EngineInfo->GetOnParametersLoadedDelegate()->AddRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded);
	}

	// Set variables in the EngineInfo
	EngineInfo->SetCookMode(CookMode == ETouchEngineCookMode::Independent);
	EngineInfo->SetFrameRate(TEFrameRate);
	// Tell the TouchEngine instance to load the tox file
	EngineInfo->Load(GetAbsoluteToxPath());
}

FString UTouchEngineComponentBase::GetAbsoluteToxPath() const
{
	return ToxFilePath.IsEmpty()
		? FString()
		: FPaths::ProjectContentDir() / ToxFilePath;
}

void UTouchEngineComponentBase::VarsSetInputs()
{
	SetInputs.Broadcast();
	switch (SendMode)
	{
	case ETouchEngineSendMode::EveryFrame:
	{
		DynamicVariables.SendInputs(EngineInfo);
		break;
	}
	case ETouchEngineSendMode::OnAccess:
	{

		break;
	}
	default: ;
	}
}

void UTouchEngineComponentBase::VarsGetOutputs()
{
	GetOutputs.Broadcast();
	switch (SendMode)
	{
	case ETouchEngineSendMode::EveryFrame:
	{
		DynamicVariables.GetOutputs(EngineInfo);
		break;
	}
	case ETouchEngineSendMode::OnAccess:
	{

		break;
	}
	default: ;
	}
}

bool UTouchEngineComponentBase::ShouldUseLocalTouchEngine() const
{
	// if this is a world object that has begun play we should use the local touch engine instance
	return HasBegunPlay();
}

void UTouchEngineComponentBase::ReleaseResources()
{
	if (BeginFrameDelegateHandle.IsValid())
	{
		FCoreDelegates::OnBeginFrame.Remove(BeginFrameDelegateHandle);
	}

	UnbindDelegates();

	if (EngineInfo)
	{
		EngineInfo->Clear();
		EngineInfo = nullptr;
	}
}
