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

#include "Logging.h"
#include "ToxAsset.h"
#include "Engine/TouchEngineInfo.h"
#include "Engine/TouchEngineSubsystem.h"
#include "Engine/Util/CookFrameData.h"

#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FeedbackContext.h"
#include "Misc/Paths.h"
#include "Rendering/Importing/TouchTextureImporter.h"
#include "Util/TouchHelpers.h"

DEFINE_LOG_CATEGORY(LogTouchEngineComponent)

void UTouchEngineComponentBase::BroadcastOnToxLoaded(bool bInSkipBlueprintEvent)
{
	bSkipBlueprintEvents = bInSkipBlueprintEvent;
	OnToxLoaded_Native.Broadcast();
	bSkipBlueprintEvents = false;
}

void UTouchEngineComponentBase::BroadcastOnToxReset(bool bInSkipUIEvent)
{
	bSkipBlueprintEvents = bInSkipUIEvent;
	OnToxReset_Native.Broadcast();
	bSkipBlueprintEvents = false;
}

void UTouchEngineComponentBase::BroadcastOnToxFailedLoad(const FString& Error, bool bInSkipUIEvent)
{
	bSkipBlueprintEvents = bInSkipUIEvent;
	OnToxFailedLoad_Native.Broadcast(Error);
	bSkipBlueprintEvents = false;
}

void UTouchEngineComponentBase::BroadcastOnToxUnloaded(bool bInSkipUIEvent)
{
	bSkipBlueprintEvents = bInSkipUIEvent;
	OnToxUnloaded_Native.Broadcast();
	bSkipBlueprintEvents = false;
}

void UTouchEngineComponentBase::BroadcastSetInputs(const FTouchEngineInputFrameData& FrameData)
{
#if WITH_EDITOR
	FEditorScriptExecutionGuard ScriptGuard;
#endif
	SetInputs.Broadcast(FrameData);
}

void UTouchEngineComponentBase::BroadcastGetOutputs(const FTouchEngineOutputFrameData& FrameData, bool bIsSuccessful) const
{
#if WITH_EDITOR
	FEditorScriptExecutionGuard ScriptGuard;
#endif
	GetOutputs.Broadcast(bIsSuccessful, FrameData);
}

void UTouchEngineComponentBase::BroadcastCustomBeginPlay()
{
	FEditorScriptExecutionGuard ScriptGuard;
	CustomBeginPlay.Broadcast();
}

void UTouchEngineComponentBase::BroadcastCustomEndPlay()
{
	FEditorScriptExecutionGuard ScriptGuard;
	CustomEndPlay.Broadcast();
}


void FActorComponentBeginFrameTickFunction::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FActorComponentTickFunction::ExecuteTick);
	FActorComponentTickFunction::ExecuteTickHelper(Target, Target->bTickInEditor, DeltaTime, TickType, [this, TickType](float DilatedTime)
	{
		Target->TickBeginFrameComponent(DilatedTime, TickType, this);
	});
}

FString FActorComponentBeginFrameTickFunction::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[TickBeginFrameComponent]");
}

FName FActorComponentBeginFrameTickFunction::DiagnosticContext(bool bDetailed)
{
	// from FActorComponentTickFunction::DiagnosticContext(bool bDetailed)
	if (bDetailed)
	{
		AActor* OwningActor = Target->GetOwner();
		FString OwnerClassName = OwningActor ? OwningActor->GetClass()->GetName() : TEXT("None");
		// Format is "ComponentClass/OwningActorClass/ComponentName"
		FString ContextString = FString::Printf(TEXT("%s/%s/%s"), *Target->GetClass()->GetName(), *OwnerClassName, *Target->GetName());
		return FName(*ContextString);
	}
	else
	{
		return Target->GetClass()->GetFName();
	}
}

UTouchEngineComponentBase::UTouchEngineComponentBase()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = CookMode == ETouchEngineCookMode::Synchronized || CookMode == ETouchEngineCookMode::DelayedSynchronized
		? TG_LastDemotable
		: TG_PrePhysics;

	BeginFrameComponentTick.bCanEverTick = true;
	BeginFrameComponentTick.bStartWithTickEnabled = false;
	BeginFrameComponentTick.TickGroup = TG_PrePhysics;
	
	OnToxLoaded_Native.AddLambda([this]()
	{
		if (!bSkipBlueprintEvents)
		{
#if WITH_EDITOR
			FEditorScriptExecutionGuard ScriptGuard;
#endif
			OnToxLoaded.Broadcast();
		}
	});
	
	OnToxReset_Native.AddLambda([this]()
	{
		if (!bSkipBlueprintEvents)
		{
#if WITH_EDITOR
			FEditorScriptExecutionGuard ScriptGuard;
#endif
			OnToxReset.Broadcast();
		}
	});
	
	OnToxFailedLoad_Native.AddLambda([this](const FString& ErrorMessage)
	{
		if (!bSkipBlueprintEvents)
		{
#if WITH_EDITOR
			FEditorScriptExecutionGuard ScriptGuard;
#endif
			OnToxFailedLoad.Broadcast(ErrorMessage);
		}
	});

	OnToxUnloaded_Native.AddLambda([this]()
	{
		if (!bSkipBlueprintEvents)
		{
#if WITH_EDITOR
			FEditorScriptExecutionGuard ScriptGuard;
#endif
			OnToxUnloaded.Broadcast();
		}
	});
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
		// this object has no local touch engine instance, must check the subsystem to see if our tox file has already been loaded
		const UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return TESubsystem->IsLoaded(GetAbsoluteToxPath());
	}
}

bool UTouchEngineComponentBase::IsLoading() const
{
	if (ShouldUseLocalTouchEngine())
	{
		return EngineInfo && EngineInfo->Engine && EngineInfo->Engine->IsLoading();
	}
	else
	{
		// this object has no local touch engine instance, must check the subsystem to see if our tox file has already been loaded
		const UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return TESubsystem->IsLoading(GetAbsoluteToxPath());
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
		const UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return TESubsystem->HasFailedLoad(GetAbsoluteToxPath());
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

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxAsset))
	{
		DynamicVariables.Reset();
		BroadcastOnToxReset();

		if (IsValid(ToxAsset))
		{
			LoadTox();
		}
	}
#if WITH_EDITOR
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, bAllowRunningInEditor))
	{
		bTickInEditor = bAllowRunningInEditor;
		// Due to the order of events in the editor and the few registering and unregistering of the Component, trying to start the engine here will fail
		// instead, we check in tick if the engine is not loaded and load it there.
		const UWorld* World = GetWorld();
		if (World && IsValid(World) && World->IsEditorWorld() && !World->IsGameWorld())
		{
			if (!bAllowRunningInEditor)
			{
				EndPlay(EEndPlayReason::Type::EndPlayInEditor);
			}
		}
	}
#endif
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, CookMode))
	{
		const bool bLastDemotable = CookMode == ETouchEngineCookMode::Synchronized || CookMode == ETouchEngineCookMode::DelayedSynchronized;
		PrimaryComponentTick.TickGroup = bLastDemotable
			? TG_LastDemotable
			: TG_PrePhysics;
		if (EngineInfo && EngineInfo->Engine)
		{
			BeginFrameComponentTick.SetTickFunctionEnable(CookMode == ETouchEngineCookMode::Synchronized || CookMode == ETouchEngineCookMode::DelayedSynchronized);
		}
	}
}

void UTouchEngineComponentBase::OnRegister()
{
#if WITH_EDITOR
	// Make sure that bTickInEditor is aligning with bAllowRunningInEditor. It happens that the values get misaligned in the editor, when the BP is reconstructed after recompiling for example
	bTickInEditor = bAllowRunningInEditor;
#endif
	Super::OnRegister();
}

void UTouchEngineComponentBase::RegisterComponentTickFunctions(bool bRegister)
{
	Super::RegisterComponentTickFunctions(bRegister);
	if(bRegister)
	{
		if (SetupActorComponentTickFunction(&BeginFrameComponentTick))
		{
			BeginFrameComponentTick.Target = this;
		}
	}
	else
	{
		if(BeginFrameComponentTick.IsTickFunctionRegistered())
		{
			BeginFrameComponentTick.UnRegisterTickFunction();
		}
	}
}
#endif


void UTouchEngineComponentBase::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	// Sync bTickInEditor with UPROPERTY
	bTickInEditor = bAllowRunningInEditor;
#endif

	// Prevents false warnings during cooking which arise from the creation of async tasks that end up calling FTouchEngine::GetSupportedPixelFormat
	if (GIsCookerLoadingPackage)
	{
		return;
	}

	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}

#if WITH_EDITOR
	const UWorld* World = GetWorld();
	if (World && IsValid(World) && World->IsEditorWorld() && !World->IsGameWorld())
	{
		// only load in Editor and Editor Preview, not PIE
		LoadToxInternal(false, true);
	}
#endif
}

void UTouchEngineComponentBase::BeginPlay()
{
	BroadcastCustomBeginPlay();

	Super::BeginPlay();
	
	const UWorld* World = GetWorld();
	if (LoadOnBeginPlay && World && IsValid(World) && World->IsGameWorld())
	{
		LoadToxInternal(false);
	}
}

void UTouchEngineComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	using namespace UE::TouchEngine;
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if WITH_EDITOR
	// First we ensure that we called a begin Play. With the component ticking in Editor, we might reach this point without having called BeginPlay
	if (bAllowRunningInEditor && !HasBegunPlay())
	{
		const UWorld* World = GetWorld();
		if (World && IsValid(World) && World->IsEditorWorld() && !World->IsGameWorld())
		{
			BeginPlay();
		}
	}
#endif
	
	// Do nothing if ...
	if (!EngineInfo // ... we're not supposed to load anything
		// ... tox file isn't loaded yet
		|| !EngineInfo->Engine
		|| !EngineInfo->Engine->IsReadyToCookFrame())
	{
#if WITH_EDITOR
		const UWorld* World = GetWorld();
		if (bAllowRunningInEditor && World && World->IsEditorWorld() && (!World->IsGameWorld() || (GEditor && GEditor->IsSimulatingInEditor())))
		{
			LoadToxInternal(false); // Load the Tox if we are in editor and none are loaded
		}
#endif
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
			// if (PendingTexturesImported) // from previous frame if we needed to wait
			// {
			// 	UE_LOG(LogTouchEngineComponent, Error, TEXT(" is PendingTexturesImported ready? %s"), PendingTexturesImported->IsReady() ? TEXT("TRUE") : TEXT("FALSE"))
			// 	PendingTexturesImported->Wait();
			// 	UE_LOG(LogTouchEngineComponent, Display, TEXT(" is PendingTexturesImported ready? %s"), PendingTexturesImported->IsReady() ? TEXT("TRUE") : TEXT("FALSE"))
			// }
			if (PendingCookFrame && false) // BeginFrame may not have started any cook yet because TE was not ready //todo: PendingCookFrame can never be valid as we used Next which invalidates it
			{
				// UE_LOG(LogTouchEngineComponent, Error, TEXT("   ---------- [TickComponent] ----------"))
				// UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] waiting for PendingCookFrame..."), *GetCurrentThreadStr())
				// PendingCookFrame->Wait();
				// FCookFrameResult CookFrameResult = PendingCookFrame->Get();
				// UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] PendingCookFrame [Frame No %lld] done: %s"), *GetCurrentThreadStr(), CookFrameResult.FrameData.FrameID, *ECookFrameErrorCodeToString(CookFrameResult.ErrorCode) )
				//
				// if (CookFrameResult.UTexturesToBeCreatedOnGameThread)
				// {
				// 	UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] waiting for UTexturesToBeCreatedOnGameThread [Frame No %lld]..."), *GetCurrentThreadStr(), CookFrameResult.FrameData.FrameID)
				// 	CookFrameResult.UTexturesToBeCreatedOnGameThread->Wait();
				// 	TArray<FTextureCreationFormat> UTexturesToBeCreated = CookFrameResult.UTexturesToBeCreatedOnGameThread->Get();
				// 	UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] UTexturesToBeCreatedOnGameThread [Frame No %lld] done. Nb Textures to be created: %d"), *GetCurrentThreadStr(),CookFrameResult.FrameData.FrameID,						UTexturesToBeCreated.Num());
				// 	
				// 	for (FTextureCreationFormat& TextureFormat : UTexturesToBeCreated) //todo: think on how to handle this for non Synchronised plays
				// 	{
				// 		if (TextureFormat.OutUnrealTexture)
				// 		{
				// 			UE_LOG(LogTouchEngineComponent, Error, TEXT("Already have a Texture"))
				// 			TextureFormat.OnTextureCreated->SetValue(TextureFormat.OutUnrealTexture);
				// 		}
				// 		else
				// 		{
				// 			UTexture2D* Texture = UTexture2D::CreateTransient(TextureFormat.SizeX, TextureFormat.SizeY, TextureFormat.PixelFormat, TextureFormat.Identifier);
				// 			Texture->AddToRoot(); //todo: add the frame number to the name of the texture
				// 			Texture->UpdateResource(); // this needs to be on Game Thread
				// 			// ThisPin->LinkData[IntermediateResult.RequestParams.ParameterName].OutUnrealTexture = Texture;
				// 			UE_LOG(LogTouchEngineComponent, Display, TEXT("[GetOrAllocateUnrealTexture] Created UTexture `%s` (%dx%d `%s`) for identifier `%s` at `%p`"),
				// 				*Texture->GetName(), TextureFormat.SizeX, TextureFormat.SizeY, GPixelFormats[TextureFormat.PixelFormat].Name,
				// 				*TextureFormat.Identifier.ToString(), Texture)
				// 			TextureFormat.OnTextureCreated->SetValue(Texture);
				// 		}
				// 	}
				// }
				//
				// if (CookFrameResult.PendingSimpleTexturesImport) // todo: this whole promise should not be needed anymore
				// {
				// 	UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] waiting for PendingSimpleTexturesImport [Frame No %lld]..."), *GetCurrentThreadStr(),CookFrameResult.FrameData.FrameID)
				// 	CookFrameResult.PendingSimpleTexturesImport->Wait(); //todo: do we need to wait if not successful?
				// 	FTouchTexturesReady TexturesImported = CookFrameResult.PendingSimpleTexturesImport->Get();
				// 	UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] PendingSimpleTexturesImport [Frame No %lld] done: %s"), *GetCurrentThreadStr(),TexturesImported.FrameData.FrameID, *EImportResultTypeToString(TexturesImported.Result));
				// }
				// if (CookFrameResult.PendingTexturesImportNeedingUTexture)
				// {
				// 	UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] waiting for PendingTexturesImportNeedingUTexture [Frame No %lld]..."), *GetCurrentThreadStr(),CookFrameResult.FrameData.FrameID)
				// 	CookFrameResult.PendingTexturesImportNeedingUTexture->Wait();
				// 	UE::TouchEngine::FTouchTexturesReady TexturesImported = CookFrameResult.PendingTexturesImportNeedingUTexture->Get();
				// 	UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] PendingTexturesImportNeedingUTexture [Frame No %lld] done: %s"), *GetCurrentThreadStr(),TexturesImported.FrameData.FrameID, *EImportResultTypeToString(TexturesImported.Result));
				// 	FTouchEngineInputFrameData& FrameData = TexturesImported.FrameData;
				// 	// VarsGetOutputs(FrameData);
				// }
				// // if (CookFrameResult.PendingTexturesImportNeedingUTexture->IsReady())
				// // {
				// // 	PendingCookFrame.Reset(); // to start a new cook on Begin Frame
				// // }
				// FTouchEngineOutputFrameData OutputData {static_cast<FTouchEngineInputFrameData>(CookFrameResult.FrameData)};
				// OutputData.Latency = (FPlatformTime::Seconds() - GStartTime) - CookFrameResult.FrameData.StartTime;
				// OutputData.TickLatency = EngineInfo->Engine->GetLatestCookNumber() - CookFrameResult.FrameData.FrameID;
				// VarsGetOutputs(OutputData, true);
				//
				// if (CookMode == ETouchEngineCookMode::Synchronized && bPauseOnTick && GetWorld())
				// {
				// 	UE_LOG(LogTouchEngineComponent, Error, TEXT("   Requesting Pause in TickComponent"))
				// 	FlushRenderingCommands(); //todo: should not be needed
				// 	GetWorld()->bDebugPauseExecution = true;
				// }
			}
			break;
		}
	case ETouchEngineCookMode::DelayedSynchronized: // todo: basically same as Independent mode, should not stall!
		{
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
	BroadcastCustomEndPlay();
	ReleaseResources(EReleaseTouchResources::KillProcess);
	Super::EndPlay(EndPlayReason);
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

void UTouchEngineComponentBase::StartNewCook(float DeltaTime)
{
	using namespace UE::TouchEngine;
	check(EngineInfo);
	check(EngineInfo->Engine);

	const FTouchEngineInputFrameData FrameData{EngineInfo->Engine->IncrementCookNumber()};
	
	// if (GetWorld()->bDebugPauseExecution || EngineInfo->IsCookingFrame()) return; //todo: just used for checking texture allocation, to be removed
	
	UE_LOG(LogTouchEngineComponent, Error, TEXT("[StartNewCook[%s]] ------ Starting new Cook [Frame No %lld] ------"), *GetCurrentThreadStr(), FrameData.FrameID)
	UE_LOG(LogTouchEngine, Display, TEXT("[StartNewCook[%s]] Calling `VarsSetInputs` for frame %lld"),
			*GetCurrentThreadStr(), FrameData.FrameID)
	VarsSetInputs(FrameData);

	FTouchEngineInputFrameData InputFrameData{FrameData};
	InputFrameData.StartTime = FPlatformTime::Seconds() - GStartTime;
	const int64 Time = static_cast<int64>(DeltaTime * TimeScale);
	FCookFrameRequest CookFrameRequest{Time, TimeScale, InputFrameData,
		SendMode == ETouchEngineSendMode::EveryFrame ? DynamicVariables : FTouchEngineDynamicVariableContainer()};

	PendingCookFrame = EngineInfo->CookFrame_GameThread(MoveTemp(CookFrameRequest));

	if (CookMode == ETouchEngineCookMode::Synchronized) // this is the only difference between Synchronised and Independent modes
	{
		UE_LOG(LogTouchEngineComponent, Warning, TEXT("   About to wait"))
		{
			// first we ensure the RHI and GPU commands are executed
			FRenderCommandFence Fence;
			Fence.BeginFence(true);
			Fence.Wait();
		}
		PendingCookFrame->Wait();
		UE_LOG(LogTouchEngineComponent, Warning, TEXT("   Waited"))
	}

	
	PendingCookFrame->Next([this](FCookFrameResult CookFrameResult)
	{
		// we will need to be on GameThread to call VarsGetOutputs, so better going there right away
		ExecuteOnGameThread<void>([this, CookFrameResult]()
		{
			UE_LOG(LogTouchEngineComponent, Error, TEXT("[StartNewCook->Next[%s]] PendingCookFrame [Frame No %lld] done: %s"), *GetCurrentThreadStr(), CookFrameResult.FrameData.FrameID, *ECookFrameErrorCodeToString(CookFrameResult.ErrorCode) )
			// if (CookFrameResult.UTexturesToBeCreatedOnGameThread)
			// {
				// UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] waiting for UTexturesToBeCreatedOnGameThread [Frame No %lld]..."), *GetCurrentThreadStr(), CookFrameResult.FrameData.FrameID)
				// CookFrameResult.UTexturesToBeCreatedOnGameThread->Wait();
				TArray<FTextureCreationFormat> UTexturesToBeCreated = CookFrameResult.UTexturesToBeCreatedOnGameThread;
				// UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] UTexturesToBeCreatedOnGameThread [Frame No %lld] done. Nb Textures to be created: %d"), *GetCurrentThreadStr(), CookFrameResult.FrameData.FrameID, UTexturesToBeCreated.Num());

				for (FTextureCreationFormat& TextureFormat : UTexturesToBeCreated) //todo: think on how to handle this for non Synchronised plays
				{
					FString Name = TextureFormat.Identifier.ToString() + FString::Printf(TEXT("_%lld"), CookFrameResult.FrameData.FrameID);
					UTexture2D* Texture = UTexture2D::CreateTransient(TextureFormat.SizeX, TextureFormat.SizeY, TextureFormat.PixelFormat, FName(Name));
					Texture->AddToRoot();
					Texture->UpdateResource(); // this needs to be on Game Thread

					UE_LOG(LogTouchEngineComponent, Display, TEXT("[PendingCookFrame->Next[%s]] Created UTexture `%s` (%dx%d `%s`) for identifier `%s` at `%p`"),
							*GetCurrentThreadStr(),
					       *Texture->GetName(), TextureFormat.SizeX, TextureFormat.SizeY, GPixelFormats[TextureFormat.PixelFormat].Name,
					       *TextureFormat.Identifier.ToString(), Texture)
					TextureFormat.OnTextureCreated->SetValue(Texture);
				}
			// }
			// if (CookFrameResult.PendingSimpleTexturesImport) // todo: this whole promise should not be needed anymore
			// {
			// 	// UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] waiting for PendingSimpleTexturesImport [Frame No %lld]..."), *GetCurrentThreadStr(), CookFrameResult.FrameData.FrameID)
			// 	CookFrameResult.PendingSimpleTexturesImport->Wait(); //todo: do we need to wait if not successful?
			// 	const FTouchTexturesReady TexturesImported = CookFrameResult.PendingSimpleTexturesImport->Get();
			// 	// UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] PendingSimpleTexturesImport [Frame No %lld] done: %s"), *GetCurrentThreadStr(), TexturesImported.FrameData.FrameID, *EImportResultTypeToString(TexturesImported.Result));
			// }
			// if (CookFrameResult.PendingTexturesImportNeedingUTexture)
			// {
			// 	// UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] waiting for PendingTexturesImportNeedingUTexture [Frame No %lld]..."), *GetCurrentThreadStr(), CookFrameResult.FrameData.FrameID)
			// 	CookFrameResult.PendingTexturesImportNeedingUTexture->Wait();
			// 	UE::TouchEngine::FTouchTexturesReady TexturesImported = CookFrameResult.PendingTexturesImportNeedingUTexture->Get();
			// 	// UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickComponent[%s]] PendingTexturesImportNeedingUTexture [Frame No %lld] done: %s"), *GetCurrentThreadStr(), TexturesImported.FrameData.FrameID, *EImportResultTypeToString(TexturesImported.Result));
			// 	FTouchEngineInputFrameData& FrameData = TexturesImported.FrameData;
			// 	// VarsGetOutputs(FrameData);
			// }
			if (EngineInfo && EngineInfo->Engine) // they could be null if we stopped play for example
			{
				FTouchEngineOutputFrameData OutputData{static_cast<FTouchEngineInputFrameData>(CookFrameResult.FrameData)};
				// OutputData.bIsSuccessful = CookFrameResult.ErrorCode == ECookFrameErrorCode::Success;
				OutputData.Latency = (FPlatformTime::Seconds() - GStartTime) - CookFrameResult.FrameData.StartTime;
				OutputData.TickLatency = EngineInfo->Engine->GetLatestCookNumber() - CookFrameResult.FrameData.FrameID;
				UE_LOG(LogTouchEngine, Display, TEXT("[PendingCookFrame.Next[%s]] Calling `VarsGetOutputs` for frame %lld"),
						*GetCurrentThreadStr(), CookFrameResult.FrameData.FrameID)
				VarsGetOutputs(OutputData, CookFrameResult.ErrorCode == ECookFrameErrorCode::Success);
			}

			if (bPauseOnTick && GetWorld())
			{
				UE_LOG(LogTouchEngineComponent, Error, TEXT("   Requesting Pause in TickComponent"))
				// FlushRenderingCommands(); //todo: should not be needed
				GetWorld()->bDebugPauseExecution = true;
			}
			// PendingCookFrame.Reset();
		});
	});
}

void UTouchEngineComponentBase::TickBeginFrameComponent(float DeltaTime, ELevelTick TickType, FActorComponentBeginFrameTickFunction* ThisTickFunction)
{
	if (EngineInfo && EngineInfo->Engine && (CookMode == ETouchEngineCookMode::Synchronized || CookMode == ETouchEngineCookMode::DelayedSynchronized) && EngineInfo->Engine->IsReadyToCookFrame())
	{
		// UE_LOG(LogTouchEngineComponent, Error, TEXT("   ---------- [TickBeginFrameComponent] ----------"))
		// if (PendingCookFrame && PendingCookFrame->IsReady()) // from previous frame if we needed to wait
		// {
		// 	UE::TouchEngine::FCookFrameResult CookFrameResult = PendingCookFrame.GetPtrOrNull()->Get();
		// 	if (CookFrameResult.PendingTexturesImportNeedingUTexture)
		// 	{
		// 		UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickBeginFrameComponent] waiting for PendingTexturesImportNeedingUTexture [Frame No %lld]..."),CookFrameResult.FrameData.FrameID)
		// 		CookFrameResult.PendingTexturesImportNeedingUTexture->Wait();
		// 		UE::TouchEngine::FTouchTexturesReady TexturesImported = CookFrameResult.PendingTexturesImportNeedingUTexture->Get();
		// 		UE_LOG(LogTouchEngineComponent, Error, TEXT("[TickBeginFrameComponent] PendingTexturesImportNeedingUTexture [Frame No %lld] done: %s"),TexturesImported.FrameData.FrameID, *EImportResultTypeToString(TexturesImported.Result));
		// 		FTouchEngineInputFrameData& FrameData = TexturesImported.FrameData;
		// 		// VarsGetOutputs(FrameData);
		// 	}
		// 	// PendingCookFrame.Reset();
		// }
		// else
		// {
			StartNewCook(GetWorld()->DeltaTimeSeconds);
		// }
	}
}

void UTouchEngineComponentBase::LoadToxInternal(bool bForceReloadTox, bool bInSkipBlueprintEvents)
{
	const bool bLoading = IsLoading();
	const bool bLoaded = IsLoaded();
	const bool bLoadingOrLoaded = bLoading || bLoaded;
	if (!bForceReloadTox && bLoadingOrLoaded && EngineInfo) // If not force reloading and the engine is already loaded, then exit here
	{
		return;
	}
	
	const bool bLoadLocalTouchEngine = ShouldUseLocalTouchEngine();
	TFuture<UE::TouchEngine::FTouchLoadResult> LoadResult = bLoadLocalTouchEngine
		? LoadToxThroughComponentInstance()
		: LoadToxThroughCache(bForceReloadTox);
	
	LoadResult.Next([WeakThis = TWeakObjectPtr<UTouchEngineComponentBase>(this), bLoadLocalTouchEngine, bInSkipBlueprintEvents](UE::TouchEngine::FTouchLoadResult LoadResult)
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
			if (WeakThis->EngineInfo) // bLoadLocalTouchEngine && 
			{
				// DynamicVariables.SendInputs(WeakThis->EngineInfo); //todo: why would we send inputs now? might be to ensure the parameters are well loaded for detail panel?
				// DynamicVariables.GetOutputs(WeakThis->EngineInfo); //todo: why would we get outputs now?
			}
			
			if (bLoadLocalTouchEngine) // we only cache data if it was not loaded from the subsystem
			{
				UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
				TESubsystem->CacheLoadedDataFromComponent(WeakThis->GetAbsoluteToxPath(), LoadResult);
				TESubsystem->LoadPixelFormats(WeakThis->EngineInfo);
			}

			WeakThis->BroadcastOnToxLoaded(bInSkipBlueprintEvents); 
		}
		else
		{
			const FString& ErrorMessage = LoadResult.FailureResult->ErrorMessage;
			WeakThis->ErrorMessage = ErrorMessage;
			
			if (bLoadLocalTouchEngine) // we only cache data if it was not loaded from the subsystem
			{
				UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
				TESubsystem->LoadPixelFormats(WeakThis->EngineInfo);
			}

			WeakThis->BroadcastOnToxFailedLoad(ErrorMessage, bInSkipBlueprintEvents);
			WeakThis->ReleaseResources(EReleaseTouchResources::KillProcess);
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
	CreateEngineInfo();
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
		// if (CookMode == ETouchEngineCookMode::Synchronized)
		// {
		// 	BeginFrameDelegateHandle = FCoreDelegates::OnBeginFrame.AddUObject(this, &UTouchEngineComponentBase::OnBeginFrame);
		// }
		BeginFrameComponentTick.SetTickFunctionEnable(CookMode == ETouchEngineCookMode::Synchronized || CookMode == ETouchEngineCookMode::DelayedSynchronized);
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

void UTouchEngineComponentBase::VarsSetInputs(const FTouchEngineInputFrameData& FrameData)
{
	// here we are only retrieving the inputs from the user, but we will send their values during cooking to handle a cooking queue
	BroadcastSetInputs(FrameData);
	// switch (SendMode)
	// {
	// case ETouchEngineSendMode::EveryFrame:
	// {
	// 	DynamicVariables.SendInputs(EngineInfo);
	// 	break;
	// }
	// case ETouchEngineSendMode::OnAccess:
	// {
	//
	// 	break;
	// }
	// default: ;
	// }
}

void UTouchEngineComponentBase::VarsGetOutputs(const FTouchEngineOutputFrameData& FrameData, bool bIsSuccessful)
{
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
	BroadcastGetOutputs(FrameData, bIsSuccessful); // todo: this gets broadcasted even when the cook was cancelled!!
}

bool UTouchEngineComponentBase::ShouldUseLocalTouchEngine() const
{
	// if this is a world object that has begun play we should use the local touch engine instance
#if WITH_EDITOR
	return HasBegunPlay() || bAllowRunningInEditor;
#else
	return HasBegunPlay();
#endif
}

void UTouchEngineComponentBase::ReleaseResources(EReleaseTouchResources ReleaseMode)
{
	// if (BeginFrameDelegateHandle.IsValid())
	// {
	// 	FCoreDelegates::OnBeginFrame.Remove(BeginFrameDelegateHandle);
	// }
	BeginFrameComponentTick.SetTickFunctionEnable(false);

	// if (PendingCookFrame)
	// {
	// 	PendingCookFrame->Reset();
	// }
	// PendingCookFrame.Reset();
	
	if (EngineInfo)
	{
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

	BroadcastOnToxUnloaded();
}