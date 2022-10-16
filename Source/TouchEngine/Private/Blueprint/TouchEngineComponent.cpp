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
#include "Engine/FileParams.h"
#include "Engine/Util/CookFrameData.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"

void UTouchEngineComponentBase::BroadcastOnToxLoaded()
{
#if WITH_EDITOR
	FEditorScriptExecutionGuard ScriptGuard;
#endif
	OnToxLoaded.Broadcast();
}

void UTouchEngineComponentBase::BroadcastOnToxFailedLoad(const FString& Error)
{
#if WITH_EDITOR
	FEditorScriptExecutionGuard ScriptGuard;
#endif
	OnToxFailedLoad.Broadcast(Error);
}

void UTouchEngineComponentBase::BroadcastSetInputs()
{
#if WITH_EDITOR
	FEditorScriptExecutionGuard ScriptGuard;
#endif
	SetInputs.Broadcast();
}

void UTouchEngineComponentBase::BroadcastGetOutputs()
{
#if WITH_EDITOR
	FEditorScriptExecutionGuard ScriptGuard;
#endif
	GetOutputs.Broadcast();
}

UTouchEngineComponentBase::UTouchEngineComponentBase()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTouchEngineComponentBase::ReloadTox()
{
	if (ShouldUseLocalTouchEngine())
	{
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

bool UTouchEngineComponentBase::IsLoading() const
{
	if (ShouldUseLocalTouchEngine())
	{
		return EngineInfo && !EngineInfo->IsLoaded();
	}
	else
	{
		// this object has no local touch engine instance, must check the subsystem to see if our tox file has already been loaded
		UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return !TESubsystem->IsLoaded(GetAbsoluteToxPath());
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

bool UTouchEngineComponentBase::CanStart() const
{
	// In the same cases where we use the local touch engine instance, we are allowed to be started by player/editor
	return !IsRunning() && ShouldUseLocalTouchEngine();
}

bool UTouchEngineComponentBase::IsRunning() const
{
	return EngineInfo ? EngineInfo->IsRunning() : false;
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
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, AllowRunningInEditor))
	{
		bTickInEditor = AllowRunningInEditor;
	}
}

void UTouchEngineComponentBase::PostLoad()
{
	Super::PostLoad();

	// Sync bTickInEditor with UPROP
	bTickInEditor = AllowRunningInEditor;
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

	// without this crash can happen if the details panel accidentally binds to a world object
	DynamicVariables.OnToxLoaded.Clear();
	DynamicVariables.OnToxFailedLoad.Clear();
}

void UTouchEngineComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	using namespace UE::TouchEngine;
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
			StartNewCook(DeltaTime);
			break;
		}
	case ETouchEngineCookMode::Synchronized:
		{
			// Locked sync mode stalls until we can get that frame's output.
			// Cook is started on begin frame, outputs are read on tick
			if (PendingCookFrame) // BeginFrame may not have started any cook yet because TE was not ready
			{
				PendingCookFrame->Wait();
			}
			break;
		}
	case ETouchEngineCookMode::DelayedSynchronized:
		{
			// get previous frame output, then set new frame inputs and trigger a new cook.

			// make sure previous frame is done cooking, if it's not stall until it is
			if (LIKELY(PendingCookFrame)) // Will be invalid on first frame
			{
				PendingCookFrame->Wait();
			}

			StartNewCook(DeltaTime);
			break;
		}
	default:
		checkNoEntry();
	}
}

void UTouchEngineComponentBase::OnComponentCreated()
{
	// TODO DP: For Synchronized and DelayedSynchronized we want the tick group to be as late as possible to minimize frame stalling, i.e. use TG_LastDemotable
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
	ValidateParameters();
	Super::OnRegister();
}

void UTouchEngineComponentBase::OnUnregister()
{
	// Probably doesn't need to be called, and breaks a bunch of stuff if it is
	//ReleaseResources();
	Super::OnUnregister();
}

void UTouchEngineComponentBase::StartNewCook(float DeltaTime)
{
	using namespace UE::TouchEngine;
	VarsSetInputs();
	const int64 Time = static_cast<int64>(DeltaTime * TimeScale);
	PendingCookFrame = EngineInfo->CookFrame_GameThread(UE::TouchEngine::FCookFrameRequest{ Time, TimeScale });
	PendingCookFrame->Next([this](FCookFrameResult Result)
		{
			if (Result.ErrorCode != ECookFrameErrorCode::Success)
			{
				return;
			}

			if (IsInGameThread())
			{
				VarsGetOutputs();
			}
			else
			{
				AsyncTask(ENamedThreads::GameThread, [this]()
				{
					VarsGetOutputs();
				});
			}
		});
}

void UTouchEngineComponentBase::OnBeginFrame()
{
	if (EngineInfo && EngineInfo->IsLoaded() && CookMode == ETouchEngineCookMode::Synchronized)
	{
		StartNewCook(GetWorld()->DeltaTimeSeconds);
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
	TESubsystem->GetOrLoadParamsFromTox(
		GetAbsoluteToxPath(), this,
		FTouchOnParametersLoaded::FDelegate::CreateRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded),
		FTouchOnFailedLoad::FDelegate::CreateRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad),
		ParamsLoadedDelegateHandle, LoadFailedDelegateHandle
	);
}

void UTouchEngineComponentBase::ValidateParameters()
{
	UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
	UFileParams* Params = TESubsystem->GetParamsFromToxIfLoaded(GetAbsoluteToxPath());
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
	ReleaseResources();

	// set the parent of the dynamic variable container to this
	DynamicVariables.Parent = this;
	CreateEngineInfo();
}

void UTouchEngineComponentBase::CreateEngineInfo()
{
	if (!EngineInfo)
	{
		// Create TouchEngine instance if we don't have one already
		EngineInfo = NewObject<UTouchEngineInfo>(this);

		LoadFailedDelegateHandle = EngineInfo->GetOnLoadFailedDelegate()->AddRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad);
		ParamsLoadedDelegateHandle = EngineInfo->GetOnParametersLoadedDelegate()->AddRaw(&DynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded);


		if (CookMode == ETouchEngineCookMode::Synchronized)
		{
			BeginFrameDelegateHandle = FCoreDelegates::OnBeginFrame.AddUObject(this, &UTouchEngineComponentBase::OnBeginFrame);
		}
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
	BroadcastSetInputs();
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
	BroadcastGetOutputs();
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
#if WITH_EDITOR
	return HasBegunPlay() || AllowRunningInEditor;
#else
	return HasBegunPlay();
#endif
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
		EngineInfo->Clear_GameThread();
		EngineInfo = nullptr;
	}
}
