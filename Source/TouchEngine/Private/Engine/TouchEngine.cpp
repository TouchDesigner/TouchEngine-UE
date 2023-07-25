/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the Information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "Engine/TouchEngine.h"

#include "Engine/Util/TouchVariableManager.h"
#include "ITouchEngineModule.h"
#include "Logging.h"
#include "Rendering/TouchResourceProvider.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineParserUtils.h"

#include "Algo/Transform.h"
#include "Async/Async.h"
#include "Engine/TEDebug.h"
#include "Util/TouchFrameCooker.h"
#include "Util/TouchHelpers.h"

#define LOCTEXT_NAMESPACE "FTouchEngine"

namespace UE::TouchEngine
{
	void FTouchEngineHazardPointer::TouchEventCallback_AnyThread(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale, void* Info)
	{
		UE_LOG(LogTouchEngineCallbacks, Display, TEXT("TouchEventCallback:  Event: `%s`   Result: `%hs`  StartTime: %lld   TimeScale: %d    EndTime: %lld   TimeScale: %d [%s]"),
			*TEEventToString(Event),
			TEResultGetDescription(Result),
			StartTimeValue, StartTimeScale, EndTimeValue, EndTimeScale,
			*GetCurrentThreadStr() );

		FTouchEngineHazardPointer* HazardPointer = static_cast<FTouchEngineHazardPointer*>(Info);
		if (TSharedPtr<FTouchEngine> TouchEnginePin = HazardPointer->TouchEngine.Pin())
		{
			TouchEnginePin->TouchEventCallback_AnyThread(Instance, Event, Result, StartTimeValue, StartTimeScale, EndTimeValue, EndTimeScale);
		}
	}

	void FTouchEngineHazardPointer::LinkValueCallback_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier, void* Info)
	{
		UE_LOG(LogTouchEngineCallbacks, Display, TEXT("LinkValueCallback:  Event: `%s`   Identifier `%hs` [%s]"),
			*TELinkEventToString(Event),
			Identifier,
			*GetCurrentThreadStr());

		FTouchEngineHazardPointer* HazardPointer = static_cast<FTouchEngineHazardPointer*>(Info);
		if (TSharedPtr<FTouchEngine> TouchEnginePin = HazardPointer->TouchEngine.Pin())
		{
			TouchEnginePin->LinkValue_AnyThread(Instance, Event, Identifier);
		}
	}

	FTouchEngine::~FTouchEngine()
	{
		check(IsInGameThread());
		DestroyTouchEngine_GameThread();
	}

	TFuture<FTouchLoadResult> FTouchEngine::LoadTox_GameThread(const FString& InToxPath)
	{
		check(IsInGameThread());
		EmplaceLoadPromiseIfSet_GameThread(FTouchLoadResult::MakeFailure(TEXT("New load started")));
		
		// Legacy? Not sure why this is needed.
		if (GIsCookerLoadingPackage)
		{
			return MakeFulfilledPromise<FTouchLoadResult>(FTouchLoadResult::MakeFailure(TEXT("Loading .tox files is not allowed while GIsCookerLoadingPackage is set."))).GetFuture();
		}

		if (!TouchResources.ErrorLog)
		{
			TouchResources.ErrorLog = MakeShared<FTouchErrorLog>();
		}
		if (InToxPath.IsEmpty())
		{
			const FString ErrMessage(FString::Printf(TEXT("%S: Tox file path is empty"), __FUNCTION__));
			TouchResources.ErrorLog->AddError(ErrMessage);
			LoadState_GameThread = ELoadState::FailedToLoad;
			return MakeFulfilledPromise<FTouchLoadResult>(FTouchLoadResult::MakeFailure(TEXT("Invalid .tox file path (empty)."))).GetFuture();
		}

		// Set the path now so it may be used later in case we're in an unload
		LastToxPathAttemptedToLoad = InToxPath;
		
		// Defer the load until Unloading is done. OnInstancedUnloaded_AnyThread will schedule a task on the game thread once unloading is done.
		if (LoadState_GameThread == ELoadState::Unloading)
		{
			LoadPromise = TPromise<FTouchLoadResult>();
			return LoadPromise->GetFuture();
		}

		return LoadTouchEngine(InToxPath);
	}

	void FTouchEngine::Unload_GameThread()
	{
		SharedCleanUp();
		
		if (TouchResources.TouchEngineInstance)
		{
			LoadState_GameThread = ELoadState::Unloading;
			TEInstanceUnload(TouchResources.TouchEngineInstance);
		}
		else
		{
			LoadState_GameThread = ELoadState::NoTouchInstance;
		}
	}

	void FTouchEngine::DestroyTouchEngine_GameThread()
	{
		// SharedCleanUp resets the path - we want to print the path in UE_LOGs later.
		FString OldToxPath = GetToxPath();
		SharedCleanUp();
		
		LoadState_GameThread = ELoadState::NoTouchInstance;
		if (TouchResources.TouchEngineInstance)
		{
			DestroyResources(MoveTemp(OldToxPath));
		}
	}
	
	int64 FTouchEngine::GetLatestCookNumber() const
	{
		return LoadState_GameThread == ELoadState::Ready && TouchResources.FrameCooker ? TouchResources.FrameCooker->GetLatestCookNumber() : -1;
	}

	int64 FTouchEngine::IncrementCookNumber()
	{
		return LoadState_GameThread == ELoadState::Ready && TouchResources.FrameCooker ? TouchResources.FrameCooker->IncrementCookNumber() : -1;
	}

	TFuture<FCookFrameResult> FTouchEngine::CookFrame_GameThread(FCookFrameRequest&& CookFrameRequest, int32 InputBufferLimit)
	{
		check(IsInGameThread());

		const bool bIsDestroyingTouchEngine = !TouchResources.FrameCooker.IsValid() || !TouchResources.VariableManager.IsValid();
		if (bIsDestroyingTouchEngine || !IsReadyToCookFrame())
		{
			return MakeFulfilledPromise<FCookFrameResult>(FCookFrameResult::FromCookFrameRequest(CookFrameRequest, ECookFrameErrorCode::BadRequest)).GetFuture();
		}
		
		TouchResources.ErrorLog->OutputMessages_GameThread();
		TFuture<FCookFrameResult> CookFrame = TouchResources.FrameCooker->CookFrame_GameThread(MoveTemp(CookFrameRequest), InputBufferLimit)
           .Next([this](FCookFrameResult Value)
           {
               UE_LOG(LogTouchEngine, Verbose, TEXT("[CookFrame_GameThread->Next[%s]] Finished cooking frame (code: %d)"), *GetCurrentThreadStr(), static_cast<int32>(Value.ErrorCode));

               switch (Value.ErrorCode)
               {
               // These cases are expected and indicate no error
               case ECookFrameErrorCode::Success: break;
               // case ECookFrameErrorCode::Replaced: break;
               case ECookFrameErrorCode::Cancelled: break;
               case ECookFrameErrorCode::InputDropped: break;

               case ECookFrameErrorCode::BadRequest: TouchResources.ErrorLog->AddError(TEXT("A request to cook a frame was made while the engine was not fully initialized or shutting down."));
                   break;
               case ECookFrameErrorCode::FailedToStartCook: TouchResources.ErrorLog->AddError(TEXT("Failed to start cook."));
                   break;
               // case ECookFrameErrorCode::Cancelled: UE_LOG(LogTouchEngine, Display, TEXT("Cook was cancelled"));
               //     break;
               case ECookFrameErrorCode::InternalTouchEngineError:
                   HandleTouchEngineInternalError(Value.TouchEngineInternalResult);
                   break;

               default:
                   static_assert(static_cast<int32>(ECookFrameErrorCode::Count) == 6, "Update this switch");
                   break;
               }

               return Value;
           });
		
		return CookFrame;
	}

	bool FTouchEngine::ExecuteNextPendingCookFrame_GameThread() const
	{
		const bool bIsDestroyingTouchEngine = !TouchResources.FrameCooker.IsValid() || !TouchResources.VariableManager.IsValid();
		if (bIsDestroyingTouchEngine || !IsReadyToCookFrame())
		{
			return false;
		}
		
		return TouchResources.FrameCooker->ExecuteNextPendingCookFrame_GameThread();
	}


	void FTouchEngine::HandleTouchEngineInternalError(const TEResult CookResult)
	{
		FString Message = TEResultGetDescription(CookResult);

		if (TEResultGetSeverity(CookResult) == TESeverityError)
		{
			TouchResources.ErrorLog->AddError(Message);
		}
		else
		{
			TouchResources.ErrorLog->AddWarning(Message);
		}
	}

	void FTouchEngine::SetCookMode(bool bIsIndependent)
	{
		if (ensureMsgf(!TouchResources.TouchEngineInstance, TEXT("TimeMode can only be set before the engine is started.")))
		{
			TimeMode = bIsIndependent
				? TETimeInternal
				: TETimeExternal;
		}
	}

	bool FTouchEngine::SetFrameRate(int64 FrameRate)
	{
		if (ensureMsgf(!TouchResources.TouchEngineInstance, TEXT("TargetFrameRate can only be set before the engine is started.")))
		{
			TargetFrameRate = FrameRate;
			return true;
		}

		return false;
	}
	
	bool FTouchEngine::GetSupportedPixelFormat(TSet<TEnumAsByte<EPixelFormat>>& SupportedPixelFormat) const
	{
		check(IsInGameThread());
		SupportedPixelFormat.Empty();
		
		if (TouchResources.ResourceProvider && TouchResources.TouchEngineInstance && LoadState_GameThread == ELoadState::Ready)
		{
			Algo::Transform(TouchResources.ResourceProvider->GetExportablePixelTypes(*TouchResources.TouchEngineInstance.get()), SupportedPixelFormat, [](EPixelFormat PixelFormat){ return PixelFormat; });
			return true;
		}
		
		UE_LOG(LogTouchEngine, Warning, TEXT("FTouchEngine::GetSupportedPixelFormat: Called when TouchEngine was not yet loaded. There are no meaningful results, yet."));
		return false;
	}

	TFuture<FTouchLoadResult> FTouchEngine::LoadTouchEngine(const FString& InToxPath)
	{
		if (!InstantiateEngineWithToxFile(InToxPath))
		{
			LoadState_GameThread = ELoadState::FailedToLoad;
			return MakeFulfilledPromise<FTouchLoadResult>(FTouchLoadResult::MakeFailure(TEXT("Failed to init TE instance with .tox file."))).GetFuture();
		}
		check(LoadPromise);
		LoadState_GameThread = ELoadState::Loading;
		
		UE_LOG(LogTouchEngine, Display, TEXT("Started load for %s"), *InToxPath);
		if (!OutputResultAndCheckForError_GameThread(TEInstanceLoad(TouchResources.TouchEngineInstance), FString::Printf(TEXT("TouchEngine instance failed to load tox file '%s'"), *InToxPath)))
		{
			return EmplaceFailedPromiseAndReturnFuture_GameThread(TEXT("TEInstanceLoad failed."));
		}

		if (!OutputResultAndCheckForError_GameThread(TEInstanceResume(TouchResources.TouchEngineInstance), TEXT("Unable to resume TouchEngine")))
		{
			return EmplaceFailedPromiseAndReturnFuture_GameThread(TEXT("TEInstanceResume failed."));
		}

		return LoadPromise->GetFuture();
	}
	
	bool FTouchEngine::InstantiateEngineWithToxFile(const FString& InToxPath)
	{
		if (!InToxPath.IsEmpty() && !InToxPath.EndsWith(".tox"))
		{
			const FString FullMessage = FString::Printf(TEXT("Invalid file path - %s"), *InToxPath);
			TouchResources.ErrorLog->AddError(FullMessage);
			return false;
		}
		
		if (!TouchResources.TouchEngineInstance)
		{
			checkf(!TouchResources.ResourceProvider, TEXT("ResourceProvider was expected to be null if there is no running instance!"));
			TouchResources.ResourceProvider = ITouchEngineModule::Get().CreateResourceProvider();
			
			// The TE instance may get destroyed latently after the owning FTouchEngine is!
			// HazardPointer's job is to avoid TE from keep on to garbage memory; the HazardPointer is destroyed after the TE instance is destroyed.
			TouchResources.HazardPointer = MakeShared<FTouchEngineHazardPointer>(SharedThis(this));
			const TEResult TouchEngineInstance = TEInstanceCreate(FTouchEngineHazardPointer::TouchEventCallback_AnyThread, FTouchEngineHazardPointer::LinkValueCallback_AnyThread, TouchResources.HazardPointer.Get(), TouchResources.TouchEngineInstance.take());
			if (!OutputResultAndCheckForError_GameThread(TouchEngineInstance, TEXT("Unable to create TouchEngine Instance")))
			{
				return false;
			}

			const TEResult SetFrameResult = TEInstanceSetFrameRate(TouchResources.TouchEngineInstance, TargetFrameRate, 1);
			if (!OutputResultAndCheckForError_GameThread(SetFrameResult, TEXT("Unable to set frame rate")))
			{
				return false;
			}
			
			const TEResult GraphicsContextResult = TEInstanceAssociateGraphicsContext(TouchResources.TouchEngineInstance, TouchResources.ResourceProvider->GetContext());
			if (!OutputResultAndCheckForError_GameThread(GraphicsContextResult, TEXT("Unable to associate graphics Context")))
			{
				return false;
			}

			TouchResources.ResourceProvider->ConfigureInstance(TouchResources.TouchEngineInstance);
		}

		const bool bLoadTox = !InToxPath.IsEmpty();
		const auto PathAnsiString = StringCast<ANSICHAR>(*InToxPath);
		const char* Path = bLoadTox ? PathAnsiString.Get() : nullptr;
		// Set different error message depending on intent
		const FString ErrorMessage = bLoadTox 
			? FString::Printf(TEXT("Unable to configure TouchEngine with tox file '%s'"), *InToxPath)
			: TEXT("Unable to configure TouchEngine");
		const TEResult ConfigurationResult = TEInstanceConfigure(TouchResources.TouchEngineInstance, Path, TimeMode);
		if (!OutputResultAndCheckForError_GameThread(ConfigurationResult, ErrorMessage))
		{
			return false;
		}
		
		CreateNewLoadPromise();
		return true;
	}

	void FTouchEngine::TouchEventCallback_AnyThread(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale)
	{
		const bool bIsDestroyingTouchEngine = !TouchResources.ResourceProvider.IsValid();
		if (bIsDestroyingTouchEngine)
		{
			return;
		}
		UE_LOG(LogTouchEngine, Log, TEXT(" [FTouchEngine::TouchEventCallback_AnyThread[%s]] Received TouchEvent `%s` with result `%hs` for frame `%lld`   (StartTime: %lld  TimeScale: %d    EndTime: %lld  TimeScale: %d)"),
			*GetCurrentThreadStr(),
			*TEEventToString(Event),
			TEResultGetDescription(Result), 
			TouchResources.FrameCooker ? TouchResources.FrameCooker->GetCookingFrameID() : -1,
			StartTimeValue, StartTimeScale, EndTimeValue, EndTimeScale );

		switch (Event)
		{
		case TEEventInstanceDidLoad:
			OnInstancedLoaded_AnyThread(Instance, Result);
			LastFrameStartTimeValue.Reset();
			break;
		case TEEventFrameDidFinish:
			{
				UE_CLOG(Result == TEResultSuccess, LogTouchEngine, Log, TEXT("TEEventFrameDidFinish[%s] for frame `%lld`:  StartTime: %lld  TimeScale: %d    EndTime: %lld  TimeScale: %d => %s"), *GetCurrentThreadStr(), TouchResources.FrameCooker ? TouchResources.FrameCooker->GetCookingFrameID() : -1, StartTimeValue, StartTimeScale, EndTimeValue, EndTimeScale, *TEResultToString(Result));
				UE_CLOG(Result != TEResultSuccess, LogTouchEngine, Error, TEXT("TEEventFrameDidFinish[%s] for frame `%lld`:  StartTime: %lld  TimeScale: %d    EndTime: %lld  TimeScale: %d => %s (`%hs`)"), *GetCurrentThreadStr(), TouchResources.FrameCooker ? TouchResources.FrameCooker->GetCookingFrameID() : -1, StartTimeValue, StartTimeScale, EndTimeValue, EndTimeScale, *TEResultToString(Result), TEResultGetDescription(Result));
				
				// We know the cook was not processed if we receive a TEEventFrameDidFinish event with the same time as the previous one.
				const bool bFrameDropped = LastFrameStartTimeValue.IsSet() && LastFrameStartTimeValue.GetValue() == StartTimeValue;
				UE_LOG(LogTemp, Error, TEXT(" -- TouchEventCallback_AnyThread with event `TEEventFrameDidFinish` for StartTimeValue `%lld` for CookingFrame `%lld`. FrameDropped? `%s"),
					StartTimeValue, TouchResources.FrameCooker->GetCookingFrameID(), bFrameDropped ? TEXT("TRUE") : TEXT("FALSE"))

				TouchResources.FrameCooker->OnFrameFinishedCooking(Result, bFrameDropped);
				LastFrameStartTimeValue = StartTimeValue;
				break;
			}
		case TEEventInstanceDidUnload:
			OnInstancedUnloaded_AnyThread();
			LastFrameStartTimeValue.Reset();
			break;
		case TEEventGeneral:
		default:
			break;
		}
	}

	void FTouchEngine::OnInstancedLoaded_AnyThread(TEInstance* Instance, TEResult Result)
	{
		switch (Result)
		{
		case TEResultFileError:
			OnLoadError_AnyThread(FString::Printf(TEXT("load() failed to load .tox \"%s\""), *LastToxPathAttemptedToLoad), Result);
			break;

		case TEResultIncompatibleEngineVersion:
			OnLoadError_AnyThread(TEXT(""), Result);
			break;
			
		case TEResultCancelled: //todo: is there anything else to do?
			OnLoadError_AnyThread(TEXT(""), Result);
			break;
			
		case TEResultSuccess:
			FinishLoadInstance_AnyThread(Instance);
			break;
		
		default:
			TEResultGetSeverity(Result) == TESeverityError
				? OnLoadError_AnyThread(TEXT("load() severe tox file error:"), Result)
				: FinishLoadInstance_AnyThread(Instance);
		}
	}

	void FTouchEngine::FinishLoadInstance_AnyThread(TEInstance* Instance)
	{
		// We must check whether the loaded instance is compatible with Unreal Engine
		const FTouchLoadInstanceResult ValidationResult = TouchResources.ResourceProvider->ValidateLoadedTouchEngine(*Instance);
		if (ValidationResult.IsFailure())
		{
			OnLoadError_AnyThread(ValidationResult.Error.GetValue());
			return;
		}
		
		TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> VariablesIn = ProcessTouchVariables(Instance, TEScopeInput);
		TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> VariablesOut = ProcessTouchVariables(Instance, TEScopeOutput);

		const TEResult VarInResult = VariablesIn.Key;
		if (VarInResult != TEResultSuccess)
		{
			OnLoadError_AnyThread(TEXT("Failed to load input variables."), VarInResult);
			return;
		}

		const TEResult VarOutResult = VariablesOut.Key;
		if (VarOutResult != TEResultSuccess)
		{
			OnLoadError_AnyThread(TEXT("Failed to load ouput variables."), VarOutResult);
			return;
		}
		
		AsyncTask(ENamedThreads::GameThread,
			[this, VariablesIn, VariablesOut]() mutable
			{
				TouchResources.VariableManager = MakeShared<FTouchVariableManager>(TouchResources.TouchEngineInstance, TouchResources.ResourceProvider, TouchResources.ErrorLog);

				check(TouchResources.ResourceProvider); //TouchResources.ResourceProvider is supposed to be valid at this point as it has been created in InstantiateEngineWithToxFile
				TouchResources.FrameCooker = MakeShared<FTouchFrameCooker>(TouchResources.TouchEngineInstance, *TouchResources.VariableManager, *TouchResources.ResourceProvider);
				TouchResources.FrameCooker->SetTimeMode(TimeMode);
				
				LoadState_GameThread = ELoadState::Ready;
				EmplaceLoadPromiseIfSet_GameThread(FTouchLoadResult::MakeSuccess(MoveTemp(VariablesIn.Value), MoveTemp(VariablesOut.Value)));
			}
		);
	}

	void FTouchEngine::OnLoadError_AnyThread(const FString& BaseErrorMessage, TOptional<TEResult> Result)
	{
		AsyncTask(ENamedThreads::GameThread,
			[this, BaseErrorMessage, Result]()
			{
				const FString ResultDescription = Result
					? TEResultGetDescription(Result.GetValue())
					: FString();
				const FString FinalMessage = !BaseErrorMessage.IsEmpty()
					? FString::Printf(TEXT("%s %s"), *BaseErrorMessage, *ResultDescription)
					: ResultDescription;
				TouchResources.ErrorLog->AddError(FinalMessage);
				
				EmplaceLoadPromiseIfSet_GameThread(FTouchLoadResult::MakeFailure(FinalMessage));
			}
		);
	}

	TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> FTouchEngine::ProcessTouchVariables(TEInstance* Instance, TEScope Scope)
	{
		TArray<FTouchEngineDynamicVariableStruct> Variables;
		
		TEStringArray* Groups;
		const TEResult LinkResult = TEInstanceGetLinkGroups(Instance, Scope, &Groups);
		if (LinkResult != TEResultSuccess)
		{
			return { LinkResult, Variables };
		}

		for (int32_t i = 0; i < Groups->count; i++)
		{
			FTouchEngineParserUtils::ParseGroup(Instance, Groups->strings[i], Variables);
		}

		return { LinkResult, Variables };
	}

	void FTouchEngine::OnInstancedUnloaded_AnyThread()
	{
		AsyncTask(ENamedThreads::GameThread, [WeakThis = TWeakPtr<FTouchEngine>(SharedThis(this))]()
		{
			if (TSharedPtr<FTouchEngine> ThisPin = WeakThis.Pin())
			{
				ThisPin->LoadState_GameThread = ELoadState::Unloaded;
				ThisPin->ResumeLoadAfterUnload_GameThread();
			}
		});
	}

	void FTouchEngine::ResumeLoadAfterUnload_GameThread()
	{
		check(IsInGameThread());

		if (LoadPromise
			&& ensureMsgf(!GetToxPath().IsEmpty(), TEXT("LoadPromise is reset when we start a load. So there must have been LoadTox call which set the promise and tox path. However, the tox path is lost. Investigate")))
		{
			TPromise<FTouchLoadResult> PendingPromise = MoveTemp(*LoadPromise);
			LoadPromise.Reset(); // This step is important for EmplaceLoadPromiseIfSet_GameThread
			
			LoadTouchEngine(GetToxPath())
				.Next([OldPromise = MoveTemp(PendingPromise)](FTouchLoadResult LoadResult) mutable
				{
					OldPromise.EmplaceValue(LoadResult);
				});
		}
	}

	void FTouchEngine::LinkValue_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier)
	{
		const bool bIsDestroyingTouchEngine = !TouchResources.ResourceProvider.IsValid();
		if (bIsDestroyingTouchEngine)
		{
			return;
		}

		TouchObject<TELinkInfo> Info;
		const TEResult Result = TEInstanceLinkGetInfo(Instance, Identifier, Info.take());

		// UE_LOG(LogTemp, Log, TEXT("  LinkValue_AnyThread for `%hs` with event `%s` for CookingFrame `%lld`"), Identifier, *TELinkEventToString(Event), TouchResources.FrameCooker ? TouchResources.FrameCooker->GetCookingFrameID() : -1)
		const bool bIsOutputValue = Result == TEResultSuccess && Info && Info->scope == TEScopeOutput;
		const bool bHasValueChanged = Event == TELinkEventValueChange;
		const bool bIsTextureValue = Info && Info->type == TELinkTypeTexture;
		if (bIsOutputValue && bHasValueChanged && TouchResources.FrameCooker)
		{
			if (bIsTextureValue)
			{
				TouchResources.FrameCooker->ProcessLinkTextureValueChanged_AnyThread(Identifier);
				UE_LOG(LogTemp, Warning, TEXT("  LinkValue_AnyThread for `%hs` with event TELinkEventValueChange for CookingFrame `%lld`"), Identifier, TouchResources.FrameCooker->GetCookingFrameID())
			}
			if (TouchResources.VariableManager && TouchResources.FrameCooker->GetCookingFrameID() >= 0)
			{
				TouchResources.VariableManager->SetFrameLastUpdatedForParameter(Identifier, TouchResources.FrameCooker->GetCookingFrameID());
			}
		}
	}

	void FTouchEngine::SharedCleanUp()
	{
		check(IsInGameThread());
		
		LastToxPathAttemptedToLoad.Empty();
		EmplaceLoadPromiseIfSet_GameThread(FTouchLoadResult::MakeFailure(TEXT("TouchEngine being reset.")));
		if (TouchResources.FrameCooker)
		{
			TouchResources.FrameCooker->CancelCurrentAndNextCooks();
		}
	}

	void FTouchEngine::CreateNewLoadPromise()
	{
		EmplaceLoadPromiseIfSet_GameThread(FTouchLoadResult::MakeFailure(TEXT("New load started")));
		LoadPromise = TPromise<FTouchLoadResult>();
	}

	TFuture<FTouchLoadResult> FTouchEngine::EmplaceFailedPromiseAndReturnFuture_GameThread(FString ErrorMessage)
	{
		check(LoadPromise);
		TFuture<FTouchLoadResult> Future = LoadPromise->GetFuture();
		EmplaceLoadPromiseIfSet_GameThread(FTouchLoadResult::MakeFailure(MoveTemp(ErrorMessage)));
		return Future;
	}

	void FTouchEngine::EmplaceLoadPromiseIfSet_GameThread(FTouchLoadResult LoadResult)
	{
		check(IsInGameThread());
		
		if (!LoadPromise.IsSet())
		{
			return;
		}
		TPromise<FTouchLoadResult> Promise = MoveTemp(*LoadPromise);
		LoadPromise.Reset();

		UE_CLOG(LoadResult.IsSuccess(), LogTouchEngine, Display, TEXT("Finished loading TouchEngine instance with %s successfully"), *GetToxPath());
		UE_CLOG(LoadResult.IsFailure(), LogTouchEngine, Warning, TEXT("Finished loading TouchEngine instance with %s with error: %s"), *GetToxPath(), *LoadResult.FailureResult->ErrorMessage);
		LoadState_GameThread = LoadResult.IsSuccess()
			? ELoadState::Ready
			: ELoadState::FailedToLoad;
		LastToxPathAttemptedToLoad = LoadResult.IsSuccess() ? LastToxPathAttemptedToLoad : FString();
		Promise.EmplaceValue(MoveTemp(LoadResult));
	}

	void FTouchEngine::DestroyResources(FString OldToxPath)
	{
		// Instantiated first - if not set there is nothing to clean up
		if (!TouchResources.ResourceProvider)
		{
			return;
		}
		
		UE_LOG(LogTouchEngine, Verbose, TEXT("Shutting down TouchEngine instance (%s)..."), *OldToxPath);
		// Invalid if cancelled while loading
		if (TouchResources.FrameCooker)
		{
			TouchResources.FrameCooker->CancelCurrentAndNextCooks();
		}

		if (TouchResources.VariableManager)
		{
			// We need to clear the inputs and outputs as they might hold references which will stop them from being destroyed
			TouchResources.VariableManager->ClearSavedData(); //it is safe to call it here as all calls to SetTOPInput would have happened by now
		}
		
		FTouchResources KeepAlive = TouchResources;
		TouchResources.Reset();
		KeepAlive.ResourceProvider->SuspendAsyncTasks()
			.Next([KeepAlive = MoveTemp(KeepAlive), OldToxPath](auto) mutable
			{
				// Important to destroy the instance first so it triggers its callbacks
				KeepAlive.Reset();
				UE_LOG(LogTouchEngine, Verbose, TEXT("Finished destroying TouchEngine instance (%s)..."), *OldToxPath);
				// Now the hazard pointer is destroyed - it is safe to destroy now since TE triggered all callbacks at this stage
			});
	}

	bool FTouchEngine::OutputResultAndCheckForError_GameThread(const TEResult Result, const FString& ErrMessage) const
	{
		if (Result != TEResultSuccess)
		{
			TouchResources.ErrorLog->AddResult(FString::Printf(TEXT("%s: "), *ErrMessage), Result);
			if (TEResultGetSeverity(Result) == TESeverity::TESeverityError)
			{
				return false;
			}
		}
		return true;
	}
}

#undef LOCTEXT_NAMESPACE