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

#if WITH_EDITOR
#include "Editor.h" // used to access GEditor and especially GEditor->IsSimulatingInEditor()
#endif

#include "ToxAsset.h"
#include "Engine/TouchEngineInfo.h"
#include "Engine/TouchEngineSubsystem.h"
#include "Engine/Util/CookFrameData.h"

#include "Engine/Engine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FeedbackContext.h"
#include "Misc/Paths.h"
#include "Tasks/Task.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/TouchHelpers.h"
#include "RenderingThread.h"
#include "Engine/TEDebug.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY(LogTouchEngineComponent)

void UTouchEngineComponentBase::BroadcastOnToxStartedLoading(bool bInSkipBlueprintEvent)
{
	OnToxStartedLoading_Native.Broadcast(); // the native event should always broadcast as it affects UI

#if WITH_EDITOR
	const bool bCanBroadcastEvents = !bInSkipBlueprintEvent && (HasBegunPlay() || bAllowRunningInEditor);
#else
	const bool bCanBroadcastEvents = !bInSkipBlueprintEvent && HasBegunPlay();
#endif

	if (bCanBroadcastEvents)
	{
#if WITH_EDITOR
		FEditorScriptExecutionGuard ScriptGuard;
#endif
		OnToxStartedLoading.Broadcast();
	}
}

void UTouchEngineComponentBase::BroadcastOnToxLoaded(bool bInSkipBlueprintEvent)
{
	OnToxLoaded_Native.Broadcast(); // the native event should always broadcast as it affects UI

#if WITH_EDITOR
	const bool bCanBroadcastEvents = !bInSkipBlueprintEvent && (HasBegunPlay() || bAllowRunningInEditor);
#else
	const bool bCanBroadcastEvents = !bInSkipBlueprintEvent && HasBegunPlay();
#endif

	if (bCanBroadcastEvents)
	{
#if WITH_EDITOR
		FEditorScriptExecutionGuard ScriptGuard;
#endif
		OnToxLoaded.Broadcast();
	}
}

void UTouchEngineComponentBase::BroadcastOnToxReset(bool bInSkipBlueprintEvent)
{
	OnToxReset_Native.Broadcast();  // the native event should always broadcast as it affects UI
	
#if WITH_EDITOR
	const bool bCanBroadcastEvents = !bInSkipBlueprintEvent && (HasBegunPlay() || bAllowRunningInEditor);
#else
	const bool bCanBroadcastEvents = !bInSkipBlueprintEvent && HasBegunPlay();
#endif

	if (bCanBroadcastEvents)
	{
#if WITH_EDITOR
		FEditorScriptExecutionGuard ScriptGuard;
#endif
		OnToxReset.Broadcast();
	}
}

void UTouchEngineComponentBase::BroadcastOnToxFailedLoad(const FString& Error, bool bInSkipBlueprintEvent)
{
	OnToxFailedLoad_Native.Broadcast(Error); // the native event should always broadcast as it affects UI
	
#if WITH_EDITOR
	const bool bCanBroadcastEvents = !bInSkipBlueprintEvent && (HasBegunPlay() || bAllowRunningInEditor);
#else
	const bool bCanBroadcastEvents = !bInSkipBlueprintEvent && HasBegunPlay();
#endif

	if (bCanBroadcastEvents)
	{
#if WITH_EDITOR
		FEditorScriptExecutionGuard ScriptGuard;
#endif
		OnToxFailedLoad.Broadcast(Error);
	}
}

void UTouchEngineComponentBase::BroadcastOnToxUnloaded(bool bInSkipBlueprintEvent)
{
	OnToxUnloaded_Native.Broadcast(); // the native event should always broadcast as it affects UI
	
	#if WITH_EDITOR
	const bool bCanBroadcastEvents = !bInSkipBlueprintEvent && (HasBegunPlay() || bAllowRunningInEditor);
#else
	const bool bCanBroadcastEvents = !bInSkipBlueprintEvent && HasBegunPlay();
#endif

	if (bCanBroadcastEvents)
	{
#if WITH_EDITOR
		FEditorScriptExecutionGuard ScriptGuard;
#endif
		OnToxUnloaded.Broadcast();
	}
}

void UTouchEngineComponentBase::BroadcastOnStartFrame(const FTouchEngineInputFrameData& FrameData) const
{
#if WITH_EDITOR
	const bool bCanBroadcastEvents = HasBegunPlay() || bAllowRunningInEditor;
#else
	const bool bCanBroadcastEvents = HasBegunPlay();
#endif

	if (bCanBroadcastEvents)
	{
#if WITH_EDITOR
		FEditorScriptExecutionGuard ScriptGuard;
#endif
		OnStartFrame.Broadcast(FrameData);
	}
}

void UTouchEngineComponentBase::BroadcastOnEndFrame(ECookFrameResult Result, const FTouchEngineOutputFrameData& FrameData) const
{
#if WITH_EDITOR
	const bool bCanBroadcastEvents = HasBegunPlay() || bAllowRunningInEditor;
#else
	const bool bCanBroadcastEvents = HasBegunPlay();
#endif

	if (bCanBroadcastEvents)
	{
#if WITH_EDITOR
		FEditorScriptExecutionGuard ScriptGuard;
#endif
		OnEndFrame.Broadcast(Result == ECookFrameResult::Success && !FrameData.bWasFrameDropped, Result, FrameData);
	}
}

void UTouchEngineComponentBase::BroadcastCustomBeginPlay() const
{
#if WITH_EDITOR
	const bool bCanBroadcastEvents = HasBegunPlay() || bAllowRunningInEditor;
#else
	const bool bCanBroadcastEvents = HasBegunPlay();
#endif

	if (bCanBroadcastEvents)
	{
#if WITH_EDITOR
		FEditorScriptExecutionGuard ScriptGuard;
#endif
		CustomBeginPlay.Broadcast();
	}
}

void UTouchEngineComponentBase::BroadcastCustomEndPlay() const
{
#if WITH_EDITOR
	const bool bCanBroadcastEvents = HasBegunPlay() || bAllowRunningInEditor;
#else
	const bool bCanBroadcastEvents = HasBegunPlay();
#endif

	if (bCanBroadcastEvents)
	{
#if WITH_EDITOR
		FEditorScriptExecutionGuard ScriptGuard;
#endif
		CustomEndPlay.Broadcast();
	}
}

#if WITH_EDITOR
void UTouchEngineComponentBase::OnToxStartedLoadingThroughSubsystem(UToxAsset* ReloadedToxAsset)
{
	BroadcastOnToxStartedLoading(true);
}

void UTouchEngineComponentBase::OnToxLoadedThroughSubsystem(UToxAsset* ReloadedToxAsset, const UE::TouchEngine::FCachedToxFileInfo& LoadResult)
{
	HandleToxLoaded(LoadResult.LoadResult, false, true);
}
#endif

UTouchEngineComponentBase::UTouchEngineComponentBase()
	: EngineInfo(nullptr) // this was necessary because there were cases where this was pointing to a valid EngineInfo in PIE, even though we hadn't started it
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics; // earliest tick for all types of cooks
}

void UTouchEngineComponentBase::LoadTox(bool bForceReloadTox) //todo: why is this needed as there is also StartTouchEngine
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
		// this object has no local TouchEngine instance, must check the subsystem to see if our tox file has already been loaded
		const UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return TESubsystem->IsLoaded(ToxAsset);
	}
}

bool UTouchEngineComponentBase::IsLoading() const
{
	if (ShouldUseLocalTouchEngine())
	{
		return EngineInfo && EngineInfo->Engine->IsLoading();
	}
	else
	{
		// this object has no local TouchEngine instance, must check the subsystem to see if our tox file has already been loaded
		const UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return TESubsystem->IsLoading(ToxAsset);
	}
}

bool UTouchEngineComponentBase::HasFailedLoad() const
{
	if (ShouldUseLocalTouchEngine())
	{
		if (EngineInfo)
		{
			// if this is a world object that has begun play and has a local TouchEngine instance
			return EngineInfo->Engine->HasFailedToLoad();
		}
	}
	else
	{
		// this object has no local TouchEngine instance, must check the subsystem to see if our tox file has failed to load
		const UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return TESubsystem->HasFailedLoad(ToxAsset);
	}
	return true; // We consider it failed if we cannot access the engine
}

bool UTouchEngineComponentBase::IsReadyToLoad() const
{
	if (ShouldUseLocalTouchEngine())
	{
		return EngineInfo && EngineInfo->Engine->IsReadyToLoad();
	}
	else
	{
		return true; // todo: check
	}
}

FString UTouchEngineComponentBase::GetFilePath() const
{
	if (ToxAsset)
	{
		return ToxAsset->GetRelativeFilePath();
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
	// In the same cases where we use the local TouchEngine instance, we are allowed to be started by player/editor
	const bool bIsNotLoading = !EngineInfo || !EngineInfo->Engine->IsLoading();
	return !IsRunning()
		&& bIsNotLoading
		&& ShouldUseLocalTouchEngine();
}

bool UTouchEngineComponentBase::IsRunning() const
{
	return EngineInfo && EngineInfo->Engine->IsReadyToCookFrame();
}

bool UTouchEngineComponentBase::KeepFrameTexture(UTexture2D* FrameTexture, UTexture2D*& Texture)
{
	Texture = nullptr;
	if (EngineInfo)
	{
		EngineInfo->Engine->RemoveImportedUTextureFromPool(FrameTexture);
		Texture = FrameTexture;
	}
	return false;
}

void UTouchEngineComponentBase::BeginDestroy()
{
	ReleaseResources(EReleaseTouchResources::KillProcess);
	Super::BeginDestroy();
}

#if WITH_EDITOR
void UTouchEngineComponentBase::PreEditChange(FProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);
	const FName PropertyName = PropertyThatWillChange != nullptr ? PropertyThatWillChange->GetFName() : NAME_None;
	
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxAsset))
	{
		if (IsValid(ToxAsset))
		{
			ToxAsset->GetOnToxStartedLoadingThroughSubsystem().RemoveAll(this);
			ToxAsset->GetOnToxLoadedThroughSubsystem().RemoveAll(this);
		}
	}
}

void UTouchEngineComponentBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxAsset))
	{
		if (ToxAsset)
		{
			ToxAsset->GetOnToxStartedLoadingThroughSubsystem().AddUObject(this, &UTouchEngineComponentBase::OnToxStartedLoadingThroughSubsystem);
			ToxAsset->GetOnToxLoadedThroughSubsystem().AddUObject(this, &UTouchEngineComponentBase::OnToxLoadedThroughSubsystem);
		}
		DynamicVariables.Reset();
		BroadcastOnToxReset();

		if (IsValid(ToxAsset))
		{
			LoadTox();
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, bAllowRunningInEditor))
	{
		// Due to the order of events in the editor and the few registering and unregistering of the Component, trying to start the engine here will fail
		// instead, we check in tick if the engine is not loaded and load it there.
		HandleAllowRunningInEditorChanged();
	}
}

void UTouchEngineComponentBase::PreEditUndo()
{
	Super::PreEditUndo();
	PreUndoValues = { DynamicVariables, bAllowRunningInEditor, EngineInfo };
}

void UTouchEngineComponentBase::PostEditUndo()
{
	Super::PostEditUndo();
	
	EngineInfo = PreUndoValues.EngineInfo; //not supposed to be directly affected by Undo/Redo
	
	if (IsValid(EngineInfo))
	{
		// For Inputs, we just ask to resend the value if the value before the Undo/Redo is not matching the value after.
		for (FTouchEngineDynamicVariableStruct& Input : DynamicVariables.DynVars_Input)
		{
			if(const FTouchEngineDynamicVariableStruct* PreviousInput = PreUndoValues.DynamicVariables.GetDynamicVariableByIdentifier(Input.VarIdentifier))
			{
				if (!Input.HasSameValue(PreviousInput))
				{
					Input.SetFrameLastUpdatedFromNextCookFrame(EngineInfo);
				}
				else if (PreviousInput->FrameLastUpdated > Input.FrameLastUpdated) // if the input was last updated later, we don't go back in time
				{
					Input.FrameLastUpdated = PreviousInput->FrameLastUpdated;
				}
			}
		}
		// For Outputs, we keep the latest one we received
		for (FTouchEngineDynamicVariableStruct& Output : DynamicVariables.DynVars_Output)
		{
			if(const FTouchEngineDynamicVariableStruct* PreviousOutput = PreUndoValues.DynamicVariables.GetDynamicVariableByIdentifier(Output.VarIdentifier))
			{
				if (PreviousOutput->FrameLastUpdated > Output.FrameLastUpdated)
				{
					Output.SetValue(PreviousOutput);
					Output.FrameLastUpdated = PreviousOutput->FrameLastUpdated;
				}
			}
		}
	}
	if (PreUndoValues.bAllowRunningInEditor != bAllowRunningInEditor)
	{
		HandleAllowRunningInEditorChanged();
	}
	
	PreUndoValues = {};
}

void UTouchEngineComponentBase::PostReinitProperties()
{
	if (IsRunningCookCommandlet()) // If we are packaging, we do not want to try loading the Tox
	{
		Super::PostReinitProperties();
		return;
	}

	// This gets called when the component has been reset, which happens when it is first loaded, by doing a Reset Instance Changes to Blueprint Default, etc.
	// When resetting an instance to default, only the UProperties are copied (see EditorUtilities::CopyActorProperties) which is a problem for us as we need to set the value also.
	// The only callback we get is Serialize, where we are only asked to copy the values for undo, and this PostReinitProperties when everything has been copied from another Actor, not knowing which one.
	// So on PostReinit, we check all the values and if one value is null, we try to set it from the CDO if the same one is there, and if not we reset to its default value if one is present.
	const AActor* Owner = GetOwner();
	// UBlueprint* Blueprint = IsValid(Owner) ? Cast<UBlueprint>(Owner->GetClass()->ClassGeneratedBy) : nullptr;
	const AActor* BlueprintCDO = IsValid(Owner) ? Owner->GetClass()->GetDefaultObject<AActor>() : nullptr;
	UTouchEngineComponentBase* TEComponentCDO = IsValid(BlueprintCDO) ? BlueprintCDO->FindComponentByClass<UTouchEngineComponentBase>() : nullptr;
	
	LoadToxInternal(false,true, true); // this ensures the defaults are set
	for(FTouchEngineDynamicVariableStruct& DynVarInput : DynamicVariables.DynVars_Input)
	{
		if (DynVarInput.Value != nullptr)
		{
			continue;
		}

		if (IsValid(TEComponentCDO) && this != TEComponentCDO)
		{
			if (const FTouchEngineDynamicVariableStruct* OtherDynVarInput = TEComponentCDO->DynamicVariables.GetDynamicVariableByIdentifier(DynVarInput.VarIdentifier))
			{
				DynVarInput.SetValue(OtherDynVarInput);
			}
		}

		if (DynVarInput.Value == nullptr)
		{
			DynVarInput.ResetToDefault();
		}
	}
	
	BroadcastOnToxReset(true); // let the UI know if ever
	
	Super::PostReinitProperties();
}

void UTouchEngineComponentBase::HandleAllowRunningInEditorChanged()
{
	bTickInEditor = bAllowRunningInEditor;
	// Due to the order of events in the editor and the few registering and unregistering of the Component, trying to start the engine here will fail
	// instead, we check in tick if the engine is not loaded and load it there.
	const UWorld* World = GetWorld();
	if (IsValid(World) && World->IsEditorWorld() && !World->IsGameWorld())
	{
		if (!bAllowRunningInEditor)
		{
			EndPlay(EEndPlayReason::Type::EndPlayInEditor);
		}
	}
}
#endif

void UTouchEngineComponentBase::OnRegister()
{
#if WITH_EDITOR
	// Make sure that bTickInEditor is aligning with bAllowRunningInEditor. It happens that the values get misaligned in the editor, when the BP is reconstructed after recompiling for example
	bTickInEditor = bAllowRunningInEditor;
#endif
	Super::OnRegister();
}

void UTouchEngineComponentBase::Serialize(FArchive& Ar)
{
	if (Ar.IsSaving() && !IsValid(ToxAsset))
	{
		DynamicVariables.Reset();
	}
	
	Super::Serialize(Ar);
	
	if (Ar.IsLoading() && !IsValid(ToxAsset))
	{
		DynamicVariables.Reset();
	}
}

void UTouchEngineComponentBase::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (IsValid(ToxAsset))
	{
		ToxAsset->GetOnToxStartedLoadingThroughSubsystem().RemoveAll(this);
		ToxAsset->GetOnToxLoadedThroughSubsystem().RemoveAll(this);
		ToxAsset->GetOnToxStartedLoadingThroughSubsystem().AddUObject(this, &UTouchEngineComponentBase::OnToxStartedLoadingThroughSubsystem);
		ToxAsset->GetOnToxLoadedThroughSubsystem().AddUObject(this, &UTouchEngineComponentBase::OnToxLoadedThroughSubsystem);
	}
	else
	{
		DynamicVariables.Reset();
	}
	// Sync bTickInEditor with UPROPERTY
	bTickInEditor = bAllowRunningInEditor;
#endif

	// Prevents false warnings during cooking which arise from the creation of async tasks that end up calling FTouchEngine::GetSupportedPixelFormat
	if (GIsCookerLoadingPackage)
	{
		return;
	}

#if WITH_EDITOR
	const UWorld* World = GetWorld();
	if (IsValid(World) && World->IsEditorWorld() && !World->IsGameWorld())
	{
		// only load in Editor and Editor Preview, not PIE
		LoadToxInternal(false, true, true);
	}
#endif
}

void UTouchEngineComponentBase::BeginPlay()
{
	Super::BeginPlay();

	BroadcastCustomBeginPlay();

	const UWorld* World = GetWorld();
	if (LoadOnBeginPlay && IsValid(World) && World->IsGameWorld())
	{
		LoadToxInternal(false);
	}
}

void UTouchEngineComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	using namespace UE::TouchEngine;
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UWorld* World = GetWorld();

#if WITH_EDITOR
	// First we ensure that we called a begin Play. With the component ticking in Editor, we might reach this point without having called BeginPlay
	if (bAllowRunningInEditor && !HasBegunPlay())
	{
		if (IsValid(World) && World->IsEditorWorld() && !World->IsGameWorld())
		{
			BeginPlay();
		}
	}
#endif
	
	// Do nothing if ...
	if (!EngineInfo // ... we're not supposed to load anything
		// ... tox file isn't loaded yet
		|| !EngineInfo->Engine->IsReadyToCookFrame())
	{
#if WITH_EDITOR
		if (bAllowRunningInEditor)
		{
			// we don't use the functions from the component because we want to be sure we are checking the local engine
			const bool bHasPreviouslyFailedLoad = EngineInfo && EngineInfo->Engine->HasFailedToLoad();
			const bool bIsReadyToLoad = EngineInfo == nullptr || EngineInfo->Engine->IsReadyToLoad();
			if (!bHasPreviouslyFailedLoad && bIsReadyToLoad)
			{
				if (World && World->IsEditorWorld() && (!World->IsGameWorld() || (GEditor && GEditor->IsSimulatingInEditor())))
				{
					LoadToxInternal(false); // Load the Tox if we are in editor and none are loaded
				}
			}
		}
#endif
		return;
	}

	// for every Cook Mode we do the same thing
	const double WorldTimeSeconds = IsValid(World) ? World->GetTimeSeconds() : 0.0;
	static double StartTime = GStartTime;
	const double Now = FPlatformTime::Seconds();
	UE_LOG(LogTouchEngineComponent, Log, TEXT("  ====== ====== ====== ====== ------ ------ ====== ====== TickComponent ====== ====== ------ ------ ====== ====== ====== ======  %f"), Now - StartTime)
	StartTime = Now;
	StartNewCook(WorldTimeSeconds);
}

void UTouchEngineComponentBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseResources(EReleaseTouchResources::KillProcess);
	BroadcastCustomEndPlay();
	if (HasBegunPlay()) // We need to be sure, it might not always be the case as we are sometimes indirectly calling this function.
	{
		Super::EndPlay(EndPlayReason);
	}
}

void UTouchEngineComponentBase::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// Should not have any resources at this point but if there are, release them
	ReleaseResources(EReleaseTouchResources::KillProcess);
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UTouchEngineComponentBase::ExportCustomProperties(FOutputDevice& Out, uint32 Indent)
{
	Super::ExportCustomProperties(Out, Indent);

	const FStrProperty* PropertyCDO = GetDefault<FStrProperty>();
	
	FString ValueStr;
	int NbExported = 0;
	ValueStr += GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Input).ToString() + TEXT("=(");
	for (int32 i = 0; i < DynamicVariables.DynVars_Input.Num(); i++)
	{
		const FString ExportedValue = DynamicVariables.DynVars_Input[i].ExportValue();
		if (!ExportedValue.IsEmpty())
		{
			if (NbExported > 0)
			{
				ValueStr += TEXT(',');
			}
			PropertyCDO->ExportTextItem_Direct(ValueStr, &DynamicVariables.DynVars_Input[i].VarIdentifier, nullptr, nullptr, PPF_Delimited, nullptr);
			ValueStr += TEXT("=") + ExportedValue;
			++NbExported;
		}
	}
	ValueStr += TEXT(") ");
	
	Out.Logf(TEXT("%sCustomProperties DynamicVariablesValues (%s)\r\n"), FCString::Spc(Indent), *ValueStr);
}

static void SkipWhitespace(const TCHAR*& Str)
{
	while (Str && FChar::IsWhitespace(*Str))
	{
		Str++;
	}
}
static const TCHAR* ParseUntilNextChar(const TCHAR* Str, const TCHAR& ExpectedChar, FFeedbackContext* Warn, const FString& Name, bool SkipChar)
{
	while (Str && *Str != ExpectedChar) //FChar::IsWhitespace(*Str))
	{
		Str++;
	}
	if (!Str)
	{
		Warn->Logf(ELogVerbosity::Warning, TEXT("%s: Unexpected end-of-stream."), *Name);
		return nullptr; // Parse error
	}
	if (*Str != ExpectedChar)
	{
		return nullptr; // Parse error
	}
	if (SkipChar)
	{
		Str++;
	}
	return Str;
}

void UTouchEngineComponentBase::ImportCustomProperties(const TCHAR* Buffer, FFeedbackContext* Warn)
{
	Super::ImportCustomProperties(Buffer, Warn);

	if (FParse::Command(&Buffer, TEXT("DynamicVariablesValues")))
	{
		Buffer = ParseUntilNextChar(Buffer, '(', Warn, GetName(), true);
		if (!Buffer)
		{
			Warn->Logf(ELogVerbosity::Warning, TEXT("%s: Missing opening \'(\' while importing property values."), *GetName());
			return; // Parse error
		}

		const FStrProperty* StrPropCDO = GetDefault<FStrProperty>();
		while (*Buffer != ')') // Variable buffer like `(DynVars_Input=("pn/Filepath"="D:\\folder","pn/Float"=0.500000))`
		{
			const TCHAR* StartBuffer = Buffer;
			Buffer = ParseUntilNextChar(Buffer, '=', Warn, GetName(), false);
			if (!Buffer)
			{
				return; // Parse error
			}
			const int32 NumCharsInToken = Buffer - StartBuffer;
			if (NumCharsInToken <= 0)
			{
				Warn->Logf(ELogVerbosity::Warning, TEXT("%s: Encountered a missing property token while importing properties."), *GetName());
				return; // Parse error
			}
			Buffer++; // Advance over the '='

			FString PropertyToken(NumCharsInToken, StartBuffer);
			PropertyToken.TrimStartAndEndInline();
			TArray<FTouchEngineDynamicVariableStruct>* DynVars = nullptr;
			if (PropertyToken == GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Input).ToString())
			{
				DynVars = &DynamicVariables.DynVars_Input;
			}
			else if (PropertyToken == GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Output).ToString())
			{
				// Decided not to export the Outputs. Many issues with shared textures for example, and also doesn't seem needed
				// DynVars = &DynamicVariables.DynVars_Output;
			}
			
			if (!DynVars)
			{
				Warn->Logf(ELogVerbosity::Warning, TEXT("%s: Parse error while importing property values (PropertyName = %s)."), *GetName(), *PropertyToken);
				return; // Parse error
			}
			
			Buffer = ParseUntilNextChar(Buffer, '(', Warn, GetName(), true);
			if (!Buffer)
			{
				Warn->Logf(ELogVerbosity::Warning, TEXT("%s: Missing opening \'(\' while importing property values of %s."), *GetName(), *PropertyToken);
				return; // Parse error
			}

			while (*Buffer != ')') // loops through a list of variable identifier and values like `("pn/Filepath"="D:\\folder","pn/Float"=0.500000)`
			{
				SkipWhitespace(Buffer);
				FString VarIdentifier;
				Buffer = StrPropCDO->ImportText_Direct(Buffer, &VarIdentifier, nullptr, PPF_Delimited, GWarn);
				if (!Buffer)
				{
					Warn->Logf(ELogVerbosity::Warning, TEXT("%s: Unable to parse VarIdentifier while importing property values of %s."), *GetName(), *PropertyToken);
					return; // Parse error
				}
				FTouchEngineDynamicVariableStruct* VarStruct = nullptr;
				for (FTouchEngineDynamicVariableStruct& Var: *DynVars)
				{
					if (Var.VarIdentifier == VarIdentifier)
					{
						VarStruct = &Var;
						break;
					}
				}
				if (!VarStruct)
				{
					Warn->Logf(ELogVerbosity::Warning, TEXT("%s: Unexpected VarIdentifier `%s` while importing property values of %s."), *GetName(), *VarIdentifier, *PropertyToken);
					return; // Parse error
				}

				Buffer = ParseUntilNextChar(Buffer, '=', Warn, GetName(), true);
				if (!Buffer)
				{
					Warn->Logf(ELogVerbosity::Warning, TEXT("%s: Unexpected end-of-stream while importing property values."), *GetName());
					return; // Parse error
				}
				SkipWhitespace(Buffer);
				Buffer = VarStruct->ImportValue(Buffer, PPF_Delimited, GWarn);
				if (!Buffer)
				{
					Warn->Logf(ELogVerbosity::Warning, TEXT("%s: Error while parsing the value of VarIdentifier `%s` while importing property values of %s."), *GetName(), *VarIdentifier, *PropertyToken);
					return; // Parse error
				}
				SkipWhitespace(Buffer);
				if (*Buffer == TEXT(','))
				{
					Buffer++; //move to the next value
				}
			} // while (*Buffer != ')') => Value loop
			Buffer++; //move past the `)`
			SkipWhitespace(Buffer);
		} // while (*Buffer != ')') => Variable loop
	}
}

void UTouchEngineComponentBase::PostEditImport()
{
	Super::PostEditImport();

	PostLoad(); // Call PostLoad after this object has been imported via paste/duplicate
}

void UTouchEngineComponentBase::StartNewCook(double TimeInSeconds)
{
	using namespace UE::TouchEngine;
	check(EngineInfo);

	// 1. First, we get a new frame ID and we set the inputs
	FTouchEngineInputFrameData InputFrameData{EngineInfo->Engine->GetNextFrameID()};

	UE_LOG(LogTouchEngineComponent, Log, TEXT("[StartNewCook[%s]] ------ Starting new Cook [Frame No %lld] ------"), *GetCurrentThreadStr(), InputFrameData.FrameID)
	UE_LOG(LogTouchEngineComponent, Verbose, TEXT("[StartNewCook[%s]] Calling `VarsOnStartFrame` for frame %lld"), *GetCurrentThreadStr(), InputFrameData.FrameID)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("  I.A [GT] Set Inputs"), STAT_TE_I_A, STATGROUP_TouchEngine);
		// Here we are only gathering the input values but we are only sending them to TouchEngine when the cook is processed
		BroadcastOnStartFrame(InputFrameData);
	}

	// 2. We prepare the request
	InputFrameData.StartTime = FPlatformTime::Seconds() - GStartTime;
	const int64 TimeScale = EngineInfo ? EngineInfo->Engine->GetFrameRate() * 1000 : 1000; // The TimeScale should be a multiplier of the frame rate for best results. Decided on TDUE-189
	FCookFrameRequest CookFrameRequest{
		TimeInSeconds, TimeScale, InputFrameData,
		DynamicVariables.CopyInputsForCook(InputFrameData.FrameID)
	};

	// 2b. If the user put a breakpoint in OnStartFrame and decided to turn off AllowRunningInEditor, we could arrive here with an invalid engine.
	if (!EngineInfo || !EngineInfo->Engine->IsReadyToCookFrame())
	{
		UE_LOG(LogTouchEngineComponent, Warning, TEXT("The internal TouchEngine reference became invalid while starting a cook."))
		const FCookFrameResult CookFrameResult = FCookFrameResult::FromCookFrameRequest(CookFrameRequest, ECookFrameResult::FailedToStartCook, -1);
		OnCookFinished(CookFrameResult);
		return;
	}

	// 3. We actually send the cook to the frame cooker. It will be enqueued until it can be processed
	const TFuture<void> PendingCookFrame = EngineInfo->CookFrame_GameThread(MoveTemp(CookFrameRequest), InputBufferLimit)
         .Next([WeakTEComponent = MakeWeakObjectPtr(this)](FCookFrameResult CookFrameResult)
         {
             // When done, we will need to be on GameThread to call BroadcastOnEndFrame, so better going there right away
             ExecuteOnGameThread<void>([WeakTEComponent, CookFrameResult = MoveTemp(CookFrameResult)]()
             {
                 DECLARE_SCOPE_CYCLE_COUNTER(TEXT("IV. [GT] Post Cook"), STAT_TE_IV, STATGROUP_TouchEngine);

                 if (UTouchEngineComponentBase* ThisPinned = WeakTEComponent.Get())
                 {
                     ThisPinned->OnCookFinished(CookFrameResult);
                 }
                 else if (CookFrameResult.OnReadyToStartNextCook)  // If the component is not valid anymore, we set all the promises and we leave
                 {
                     CookFrameResult.OnReadyToStartNextCook->SetValue();
                 }
             }); // ExecuteOnGameThread<void>
         }); // PendingCookFrame->Next

	// 4. In Synchronised mode, we do stall the GameThread. This is the only difference between Synchronised and Independent/Delayed Synchronised modes (apart from the TETimeMode)
	if (CookMode == ETouchEngineCookMode::Synchronized)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("II. [GT] Synchronized Wait"), STAT_TE_II, STATGROUP_TouchEngine);
		UE_LOG(LogTouchEngineComponent, Log, TEXT("   [UTouchEngineComponentBase::StartNewCook[%s]] About to wait for PendingCookFrame for frame %lld"), *GetCurrentThreadStr(), InputFrameData.FrameID)
		FlushRenderingCommands(); //We need to ensure the RHI Thread starts the copies before we wait or we would end in a deadlock
		const bool bDidCookTimeout = !PendingCookFrame.WaitFor(FTimespan::FromSeconds(CookTimeout));
		UE_LOG(LogTouchEngineComponent, Log, TEXT("   [UTouchEngineComponentBase::StartNewCook[%s]] Done waiting for PendingCookFrame for frame %lld. Cook timeout? %s"), *GetCurrentThreadStr(), InputFrameData.FrameID, bDidCookTimeout ? TEXT("TRUE") : TEXT("false"))
	}
	
	// 6. We check if the cook timed out
	if (EngineInfo)
	{
		EngineInfo->CheckIfCookTimedOut_GameThread(CookTimeout);
	}
}

void UTouchEngineComponentBase::OnCookFinished(const UE::TouchEngine::FCookFrameResult& CookFrameResult)
{
	using namespace UE::TouchEngine;
	check(IsInGameThread());

	ELogVerbosity::Type Verbosity = ELogVerbosity::Log;
	if (CookFrameResult.Result == ECookFrameResult::TouchEngineCookTimeout)
	{
		Verbosity = ELogVerbosity::Log;
	}
	else if (CookFrameResult.Result != ECookFrameResult::Success && CookFrameResult.Result != ECookFrameResult::InputsDiscarded && CookFrameResult.Result != ECookFrameResult::Cancelled)
	{
		const TESeverity Severity = TEResultGetSeverity(CookFrameResult.TouchEngineInternalResult);
		Verbosity = Severity == TESeverityError ? ELogVerbosity::Error : (Severity == TESeverityWarning ? ELogVerbosity::Warning : Verbosity);
	}
	UE_CLOG(Verbosity == ELogVerbosity::Log, LogTouchEngineComponent, Log, TEXT("[StartNewCook->Next[%s]] PendingCookFrame [Frame No %lld] done with result `%s` and internal result `%s`"),
		   *GetCurrentThreadStr(), CookFrameResult.FrameData.FrameID, *UEnum::GetValueAsString(CookFrameResult.Result), *TEResultToString(CookFrameResult.TouchEngineInternalResult))
	UE_CLOG(Verbosity == ELogVerbosity::Warning, LogTouchEngineComponent, Warning, TEXT("[StartNewCook->Next[%s]] PendingCookFrame [Frame No %lld] done with result `%s` and internal result `%s`"),
		   *GetCurrentThreadStr(), CookFrameResult.FrameData.FrameID, *UEnum::GetValueAsString(CookFrameResult.Result), *TEResultToString(CookFrameResult.TouchEngineInternalResult))
	UE_CLOG(Verbosity == ELogVerbosity::Error, LogTouchEngineComponent, Error, TEXT("[StartNewCook->Next[%s]] PendingCookFrame [Frame No %lld] done with result `%s` and internal result `%s`"),
		   *GetCurrentThreadStr(), CookFrameResult.FrameData.FrameID, *UEnum::GetValueAsString(CookFrameResult.Result), *TEResultToString(CookFrameResult.TouchEngineInternalResult))
	
	// 1. We update the latency and call BroadcastOnEndFrame
	if (EngineInfo) // they could be null if we stopped play for example
	{
		FTouchEngineOutputFrameData OutputFrameData{CookFrameResult.FrameData.FrameID};
		OutputFrameData.Latency = (FPlatformTime::Seconds() - GStartTime) - CookFrameResult.FrameData.StartTime;
		const int64 CurrentFrame = EngineInfo->Engine->GetNextFrameID() - 1;
		OutputFrameData.TickLatency = CurrentFrame - CookFrameResult.FrameData.FrameID;
		OutputFrameData.bWasFrameDropped = CookFrameResult.bWasFrameDropped;
		OutputFrameData.FrameLastUpdated = CookFrameResult.FrameLastUpdated;
		OutputFrameData.CookStartTime = CookFrameResult.TECookStartTime;
		OutputFrameData.CookEndTime = CookFrameResult.TECookEndTime;

		UE_LOG(LogTouchEngineComponent, Log, TEXT("[PendingCookFrame.Next[%s]] Calling `BroadcastOnEndFrame` for frame %lld"), *GetCurrentThreadStr(), CookFrameResult.FrameData.FrameID)

		if (CookFrameResult.Result == ECookFrameResult::Success && !OutputFrameData.bWasFrameDropped) // if the cook was skipped by TE or not successful, we know that the outputs have not changed, so no need to update them 
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    IV.B.1 [GT] Post Cook - DynVar Get Outputs"), STAT_TE_IV_B_1, STATGROUP_TouchEngine);
			DynamicVariables.GetOutputs(EngineInfo);
		}

		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    IV.B.2 [GT] Post Cook - BroadcastOnEndFrame"), STAT_TE_IV_B_2, STATGROUP_TouchEngine);
			BroadcastOnEndFrame(CookFrameResult.Result, OutputFrameData);
		}
	}
	else
	{
		FTouchEngineOutputFrameData OutputFrameData{CookFrameResult.FrameData.FrameID};
		OutputFrameData.Latency = (FPlatformTime::Seconds() - GStartTime) - CookFrameResult.FrameData.StartTime;
		OutputFrameData.TickLatency = -1; // Cannot compute at this stage
		OutputFrameData.bWasFrameDropped = CookFrameResult.bWasFrameDropped;
		OutputFrameData.FrameLastUpdated = CookFrameResult.FrameLastUpdated;
		OutputFrameData.CookStartTime = CookFrameResult.TECookStartTime;
		OutputFrameData.CookEndTime = CookFrameResult.TECookEndTime;

		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    IV.B.2 [GT] Post Cook - BroadcastOnEndFrame"), STAT_TE_IV_B_2, STATGROUP_TouchEngine);
		BroadcastOnEndFrame(CookFrameResult.Result == ECookFrameResult::Success ? ECookFrameResult::Cancelled : CookFrameResult.Result, OutputFrameData);
	}

	// 3. We let the FrameCooker know that we can accept a next cook job. Does not actually start a new cook.
	if (CookFrameResult.OnReadyToStartNextCook)
	{
		CookFrameResult.OnReadyToStartNextCook->SetValue();
	}

#if WITH_EDITOR
	// 4. For debugging purpose, we might want to pause after that tick to see the outputs. This code should not run in shipping
	if (bPauseOnEndFrame && GetWorld() && CookFrameResult.Result != ECookFrameResult::InputsDiscarded)
	{
		UE_CLOG(GetWorld()->IsGameWorld(), LogTouchEngineComponent, Warning, TEXT("   Requesting Pause in TickComponent after frame %lld"), CookFrameResult.FrameData.FrameID)
		GetWorld()->bDebugPauseExecution = true;
	}
#endif

	// 5. We start a task on a background thread that will execute the next pending cook frame
	if (EngineInfo)
	{
		UE::Tasks::Launch(*(FString("ExecuteNextPendingCookFrame_") + UE_SOURCE_LOCATION), [WeakTEComponent = MakeWeakObjectPtr(this), FrameID = CookFrameResult.FrameData.FrameID]() mutable
		{
			FPlatformProcess::SleepNoStats(1.f / 1000); // to let the engine run
			AsyncTask(ENamedThreads::GameThread, [WeakTEComponent, FrameID]()
			{
				if (WeakTEComponent.IsValid() && WeakTEComponent->EngineInfo)
				{
					UE_LOG(LogTouchEngineComponent, Verbose, TEXT("[UTouchEngineComponentBase::StartNewCook[%s]] Calling ExecuteNextPendingCookFrame_GameThread after frame %lld"),
					       *GetCurrentThreadStr(), FrameID)
					const bool Started = WeakTEComponent->EngineInfo->ExecuteNextPendingCookFrame_GameThread();
					UE_LOG(LogTouchEngineComponent, Verbose, TEXT("[UTouchEngineComponentBase::StartNewCook[%s]] Called ExecuteNextPendingCookFrame_GameThread after frame %lld which returned `%s`"),
					       *GetCurrentThreadStr(), FrameID, Started ? TEXT("TRUE") : TEXT("FALSE"))
				}
			});
		}, LowLevelTasks::ETaskPriority::BackgroundNormal);
	}
}

void UTouchEngineComponentBase::LoadToxInternal(bool bForceReloadTox, bool bInSkipBlueprintEvents, bool bForceReloadFromCache)
{
	if (!IsValid(ToxAsset))
	{
		return;
	}
	const bool bLoading = IsLoading();
	const bool bLoaded = IsLoaded();
	const bool bLoadingOrLoaded = bLoading || bLoaded;
	if (!bForceReloadTox && bLoadingOrLoaded && EngineInfo && !bForceReloadFromCache) // If not force reloading and the engine is already loaded, then exit here
	{
		return;
	}

	
	const bool bLoadLocalTouchEngine = ShouldUseLocalTouchEngine();
	if (bLoadLocalTouchEngine)
	{
		TFuture<UE::TouchEngine::FTouchLoadResult> Future = LoadToxThroughComponentInstance();
		BroadcastOnToxStartedLoading(bInSkipBlueprintEvents);
		Future.Next([WeakThis = TWeakObjectPtr<UTouchEngineComponentBase>(this), bLoadLocalTouchEngine, bInSkipBlueprintEvents](const UE::TouchEngine::FTouchLoadResult& LoadResult)
		{
			check(IsInGameThread());

			if (WeakThis.IsValid())
			{
				WeakThis->HandleToxLoaded(LoadResult, bLoadLocalTouchEngine, bInSkipBlueprintEvents);
			}
		});
	}
	else
	{
		LoadToxThroughCache(bForceReloadTox)
			.Next([WeakThis = TWeakObjectPtr<UTouchEngineComponentBase>(this), bLoadLocalTouchEngine](const UE::TouchEngine::FCachedToxFileInfo& FileInfo)
			{
				// If we load through the subsystem, the function HandleToxLoaded will end up being called through the
				// broadcasted event UTouchEngineComponentBase::GetOnToxLoadedThroughSubsystem, unless it was previously cached.
				if (UTouchEngineComponentBase* StrongThis = WeakThis.Get())
				{
					if (FileInfo.bWasCached)
					{
						StrongThis->EnsureToxMetadataIsSet(FileInfo.LoadResult);
					}
					else
					{
						StrongThis->HandleToxLoaded(FileInfo.LoadResult, bLoadLocalTouchEngine, false);
					}
				}
			});
	}
}

void UTouchEngineComponentBase::HandleToxLoaded(const UE::TouchEngine::FTouchLoadResult& LoadResult, bool bLoadedLocalTouchEngine, bool bInSkipBlueprintEvents)
{
	if (LoadResult.IsSuccess())
	{
		if (!EngineInfo)
		{
			ErrorMessage = TEXT("Error loading the Tox file: The Engine has been invalidated.");
			BroadcastOnToxFailedLoad(ErrorMessage, bInSkipBlueprintEvents);
			return;
		}
		
		DynamicVariables.ToxParametersLoaded(LoadResult.SuccessResult->Inputs, LoadResult.SuccessResult->Outputs);
		DynamicVariables.SetupForFirstCook(EngineInfo->Engine->GetResourceProvider());
			
		if (bLoadedLocalTouchEngine) // we only cache data if it was not loaded from the subsystem
		{
			UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
			TESubsystem->CacheLoadedDataFromComponent(ToxAsset, LoadResult);
			TESubsystem->LoadPixelFormats(EngineInfo);
				
			EngineInfo->Engine->SetExportedTexturePoolSize(ExportedTexturePoolSize);
			EngineInfo->Engine->SetImportedTexturePoolSize(ImportedTexturePoolSize);
		}
			
		BroadcastOnToxLoaded(bInSkipBlueprintEvents); 
	}
	else
	{
		const FString& LoadErrorMessage = LoadResult.FailureResult->ErrorMessage;
		ErrorMessage = LoadErrorMessage;

		BroadcastOnToxFailedLoad(ErrorMessage, bInSkipBlueprintEvents);
	}
}

void UTouchEngineComponentBase::EnsureToxMetadataIsSet(const UE::TouchEngine::FTouchLoadResult& LoadResult)
{
	if (LoadResult.IsSuccess() && ensure(EngineInfo))
	{
		DynamicVariables.EnsureMetadataIsSet(LoadResult.SuccessResult->Inputs);
		DynamicVariables.SetupForFirstCook(EngineInfo->Engine->GetResourceProvider());
	}
	else
	{
		ErrorMessage = LoadResult.FailureResult->ErrorMessage;
	}
}

TFuture<UE::TouchEngine::FTouchLoadResult> UTouchEngineComponentBase::LoadToxThroughComponentInstance()
{
	ReleaseResources(EReleaseTouchResources::Unload);
	CreateEngineInfo();
	return EngineInfo->LoadTox(GetAbsoluteToxPath(), this);
}

TFuture<UE::TouchEngine::FCachedToxFileInfo> UTouchEngineComponentBase::LoadToxThroughCache(bool bForceReloadTox)
{
	UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
	CreateEngineInfo();
	return TESubsystem->GetOrLoadParamsFromTox(ToxAsset, ToxLoadTimeout, bForceReloadTox);
}

void UTouchEngineComponentBase::CreateEngineInfo()
{
	if (!EngineInfo)
	{
		// Create TouchEngine instance if we don't have one already
		EngineInfo = NewObject<UTouchEngineInfo>(this);
	}

	const TSharedPtr<UE::TouchEngine::FTouchEngine> Engine = EngineInfo->Engine;
	
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
		return ToxAsset->GetAbsoluteFilePath();
	}

	if (!ToxFilePath_DEPRECATED.IsEmpty())
	{
		UE_LOG(LogTouchEngineComponent, Warning, TEXT("%s: Falling back to deprecated ToxFilePath. Please set a valid asset for ToxAsset"), *GetReadableName())
		return FPaths::ProjectContentDir() / ToxFilePath_DEPRECATED;
	}

	return FString();
}

bool UTouchEngineComponentBase::ShouldUseLocalTouchEngine() const
{
	// if this is a world object that has begun play we should use the local TouchEngine instance
#if WITH_EDITOR
	return HasBegunPlay() || bAllowRunningInEditor;
#else
	return HasBegunPlay();
#endif
}

void UTouchEngineComponentBase::ReleaseResources(EReleaseTouchResources ReleaseMode)
{
	UE_LOG(LogTouchEngineComponent, Log, TEXT("[UTouchEngineComponentBase::ReleaseResources] Requesting the %s of TouchEngine..."), ReleaseMode == EReleaseTouchResources::KillProcess ? TEXT("CLOSING") : TEXT("UNLOADING"))
	if (EngineInfo)
	{
		const bool bHadValidEngine = EngineInfo->Engine->IsLoading() || EngineInfo->Engine->IsReadyToCookFrame();
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
		
		if (bHadValidEngine) // we don't want to broadcast this event if there is no Engine that was loaded
		{
			BroadcastOnToxUnloaded();
		}
	}
}