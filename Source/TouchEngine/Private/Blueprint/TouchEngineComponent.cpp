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

#include "ToxAsset.h"
#include "Engine/TouchEngineInfo.h"
#include "Engine/TouchEngineSubsystem.h"
#include "Engine/Util/CookFrameData.h"

#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogTouchEngineComponent)

void UTouchEngineComponentBase::BroadcastOnToxLoaded()
{
	OnToxLoaded_Native.Broadcast();
}

void UTouchEngineComponentBase::BroadcastOnToxReset()
{
	OnToxReset_Native.Broadcast();
}

void UTouchEngineComponentBase::BroadcastOnToxFailedLoad(const FString& Error)
{
	OnToxFailedLoad_Native.Broadcast(Error);
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
	PrimaryComponentTick.TickGroup = CookMode == ETouchEngineCookMode::Synchronized || CookMode == ETouchEngineCookMode::DelayedSynchronized
		? TG_LastDemotable
		: TG_PrePhysics;

	OnToxLoaded_Native.AddLambda([this]()
	{
#if WITH_EDITOR
	FEditorScriptExecutionGuard ScriptGuard;
#endif
		OnToxLoaded.Broadcast();
	});
	
	OnToxReset_Native.AddLambda([this]()
	{
#if WITH_EDITOR
	FEditorScriptExecutionGuard ScriptGuard;
#endif
		OnToxReset.Broadcast();
	});
	
	OnToxFailedLoad_Native.AddLambda([this](const FString& ErrorMessage)
	{
#if WITH_EDITOR
	FEditorScriptExecutionGuard ScriptGuard;
#endif
		OnToxFailedLoad.Broadcast(ErrorMessage);
	});
}

void UTouchEngineComponentBase::LoadTox(bool bForceReloadTox)
{
	LoadToxInternal(bForceReloadTox);
}

bool UTouchEngineComponentBase::IsLoaded() const
{
	if (ShouldUseLocalTouchEngine())
	{
		return EngineInfo && EngineInfo->Engine->IsReadyToCookFrame();
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
		return EngineInfo && EngineInfo->Engine && !EngineInfo->Engine->IsLoading();
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
	if (EngineInfo && EngineInfo->Engine)
	{
		// if this is a world object that has begun play and has a local touch engine instance
		return EngineInfo->Engine->HasFailedToLoad();
	}
	else
	{
		// this object has no local touch engine instance, must check the subsystem to see if our tox file has failed to load
		UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return TESubsystem->HasFailedLoad(GetAbsoluteToxPath());
	}
}

FString UTouchEngineComponentBase::GetFilePath() const
{
	if (ToxAsset)
	{
		return ToxAsset->FilePath;
	}

	if (!ToxFilePath_DEPRECATED.IsEmpty())
	{
		UE_LOG(LogTouchEngineComponent, Warning, TEXT("%s: Falling back to deprecated ToxFilePath. Please set a valid asset for ToxAsset"), *GetReadableName())
		return ToxFilePath_DEPRECATED;
	}

	return FString();
}

void UTouchEngineComponentBase::StartTouchEngine()
{
	LoadTox();
}

void UTouchEngineComponentBase::StopTouchEngine()
{
	ReleaseResources(EReleaseTouchResources::KillProcess);
}

bool UTouchEngineComponentBase::CanStart() const
{
	// In the same cases where we use the local touch engine instance, we are allowed to be started by player/editor
	const bool bIsNotLoading = !EngineInfo || !EngineInfo->Engine || !EngineInfo->Engine->IsLoading();
	return !IsRunning()
		&& bIsNotLoading
		&& ShouldUseLocalTouchEngine();
}

bool UTouchEngineComponentBase::IsRunning() const
{
	return EngineInfo && EngineInfo->Engine && EngineInfo->Engine->IsReadyToCookFrame();
}

void UTouchEngineComponentBase::BeginDestroy()
{
	ReleaseResources(EReleaseTouchResources::KillProcess);
	Super::BeginDestroy();
}

#if WITH_EDITORONLY_DATA
void UTouchEngineComponentBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, DynamicVariables))
	{
		if (IsValid(ToxAsset))
		{
			const UWorld* World = GetWorld();
			if (!IsValid(World) || !World->IsPlayInEditor())
			{
				LoadTox();
			}
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxAsset))
	{
		DynamicVariables.Reset();
		BroadcastOnToxReset();

		if (IsValid(ToxAsset))
		{
			LoadTox();
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, AllowRunningInEditor))
	{
		bTickInEditor = AllowRunningInEditor;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, CookMode))
	{
		const bool bLastDemotable = CookMode == ETouchEngineCookMode::Synchronized || CookMode == ETouchEngineCookMode::DelayedSynchronized;
		PrimaryComponentTick.TickGroup = bLastDemotable
			? TG_LastDemotable
			: TG_PrePhysics;
	}
}
#endif


void UTouchEngineComponentBase::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITORONLY_DATA
	// Sync bTickInEditor with UPROP
	bTickInEditor = AllowRunningInEditor;
#endif

	LoadToxInternal(false, true);
}

void UTouchEngineComponentBase::BeginPlay()
{
	Super::BeginPlay();

	if (LoadOnBeginPlay)
	{
		// Create engine instance
		LoadTox();
	}
}

void UTouchEngineComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	using namespace UE::TouchEngine;
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Do nothing if ...
	if (!EngineInfo // ... we're not supposed to load anything
		// ... tox file isn't loaded yet
		|| !EngineInfo->Engine
		|| !EngineInfo->Engine->IsReadyToCookFrame())
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
	// Ensure we tick as late as possible for Synchronized and DelayedSynchronized Cook Modes, whilst for others as early as possible
	PrimaryComponentTick.TickGroup = CookMode == ETouchEngineCookMode::Synchronized || CookMode == ETouchEngineCookMode::DelayedSynchronized
		? TG_LastDemotable
		: TG_PrePhysics;
	Super::OnComponentCreated();
}

void UTouchEngineComponentBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseResources(EReleaseTouchResources::KillProcess);
	Super::EndPlay(EndPlayReason);
}

void UTouchEngineComponentBase::OnUnregister()
{
	// EndPlay will occur before OnUnregister - this is mainly for when the component is unregistered by the editor
#if WITH_EDITOR
	const UWorld* World = GetWorld();
	if (!World->IsPlayInEditor())
	{
#endif
	ReleaseResources(EReleaseTouchResources::Unload);
#if WITH_EDITOR
	}
#endif
	Super::OnUnregister();
}

void UTouchEngineComponentBase::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// Should not have any resources at this point but if there are, release them
	ReleaseResources(EReleaseTouchResources::KillProcess);
	Super::OnComponentDestroyed(bDestroyingHierarchy);
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
	if (EngineInfo && EngineInfo->Engine && EngineInfo->Engine->IsReadyToCookFrame() && CookMode == ETouchEngineCookMode::Synchronized)
	{
		StartNewCook(GetWorld()->DeltaTimeSeconds);
	}
}

void UTouchEngineComponentBase::LoadToxInternal(bool bForceReloadTox, bool bSkipEvents)
{
	const bool bLoadLocalTouchEngine = ShouldUseLocalTouchEngine();
	TFuture<UE::TouchEngine::FTouchLoadResult> LoadResult = bLoadLocalTouchEngine
		? LoadToxThroughComponentInstance()
		: LoadToxThroughCache(bForceReloadTox);
	
	LoadResult.Next([WeakThis = TWeakObjectPtr<UTouchEngineComponentBase>(this), bLoadLocalTouchEngine, bSkipEvents](UE::TouchEngine::FTouchLoadResult LoadResult)
	{
		check(IsInGameThread());

		if (!WeakThis.IsValid())
		{
			return;
		}

		FTouchEngineDynamicVariableContainer& DynamicVariables = WeakThis->DynamicVariables;
		if (LoadResult.IsSuccess())
		{
			DynamicVariables.ToxParametersLoaded(LoadResult.SuccessResult->Inputs, LoadResult.SuccessResult->Outputs);
			if (bLoadLocalTouchEngine && WeakThis->EngineInfo)
			{
				DynamicVariables.SendInputs(WeakThis->EngineInfo);
				DynamicVariables.GetOutputs(WeakThis->EngineInfo);
			}
			if (!bSkipEvents)
			{
				WeakThis->BroadcastOnToxLoaded();
			}
		}
		else
		{
			const FString& ErrorMessage = LoadResult.FailureResult->ErrorMessage;
			DynamicVariables.Reset();
			WeakThis->ErrorMessage = ErrorMessage;
			if (!bSkipEvents)
			{
				WeakThis->BroadcastOnToxFailedLoad(ErrorMessage);
			}
		}
	});
}

TFuture<UE::TouchEngine::FTouchLoadResult> UTouchEngineComponentBase::LoadToxThroughComponentInstance()
{
	ReleaseResources(EReleaseTouchResources::Unload);
	CreateEngineInfo();
	return EngineInfo->LoadTox(GetAbsoluteToxPath());
}

TFuture<UE::TouchEngine::FTouchLoadResult> UTouchEngineComponentBase::LoadToxThroughCache(bool bForceReloadTox)
{
	UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
	return TESubsystem->GetOrLoadParamsFromTox(GetAbsoluteToxPath(), bForceReloadTox)
		.Next([](UE::TouchEngine::FCachedToxFileInfo Result)
		{
			return Result.LoadResult;
		});
}

void UTouchEngineComponentBase::CreateEngineInfo()
{
	if (!EngineInfo || !EngineInfo->Engine)
	{
		// Create TouchEngine instance if we don't have one already
		EngineInfo = NewObject<UTouchEngineInfo>(this);
		if (CookMode == ETouchEngineCookMode::Synchronized)
		{
			BeginFrameDelegateHandle = FCoreDelegates::OnBeginFrame.AddUObject(this, &UTouchEngineComponentBase::OnBeginFrame);
		}
	}

	TSharedPtr<UE::TouchEngine::FTouchEngine> Engine = EngineInfo->Engine;
	
	// We may have already started the engine earlier and just suspended it - these properties can only be set before an instance is spun up
	if (!Engine->HasCreatedTouchInstance())
	{
		Engine->SetCookMode(CookMode == ETouchEngineCookMode::Independent);
		Engine->SetFrameRate(TEFrameRate);
	}
}

FString UTouchEngineComponentBase::GetAbsoluteToxPath() const
{
	if (IsValid(ToxAsset))
	{
		return FPaths::ProjectContentDir() / ToxAsset->FilePath;
	}

	if (!ToxFilePath_DEPRECATED.IsEmpty())
	{
		UE_LOG(LogTouchEngineComponent, Warning, TEXT("%s: Falling back to deprecated ToxFilePath. Please set a valid asset for ToxAsset"), *GetReadableName())
		return FPaths::ProjectContentDir() / ToxFilePath_DEPRECATED;
	}

	return FString();
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

void UTouchEngineComponentBase::ReleaseResources(EReleaseTouchResources ReleaseMode)
{
	if (BeginFrameDelegateHandle.IsValid())
	{
		FCoreDelegates::OnBeginFrame.Remove(BeginFrameDelegateHandle);
	}

	if (!EngineInfo)
	{
		return;
	}

	switch (ReleaseMode)
	{
	case EReleaseTouchResources::KillProcess:
		EngineInfo->Destroy();
		EngineInfo = nullptr;
		break;
	case EReleaseTouchResources::Unload:
		EngineInfo->Unload();
		break;
	default: ;
	}
}