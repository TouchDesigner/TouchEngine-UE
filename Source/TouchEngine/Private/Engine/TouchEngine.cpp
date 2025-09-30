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
#include "Blueprint/TouchEngineComponent.h"

#include "Algo/Transform.h"
#include "Async/Async.h"
#include "Engine/TEDebug.h"
#include "Util/TouchFrameCooker.h"
#include "Util/TouchHelpers.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FTouchEngine"

namespace UE::TouchEngine
{
	void FTouchEngineHazardPointer::TouchEventCallback_AnyThread(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale, void* Info)
	{
		UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceEventCallback(TEInstance: '%p', event: '%s', result: '%hs', start_time_value: '%lld', start_time_scale: '%d', end_time_value: '%lld', end_time_scale: '%d', info: '%p') [Thread: '%s']"),
			Instance,
			*TEEventToString(Event),
			TEResultGetDescription(Result),
			StartTimeValue,
			StartTimeScale,
			EndTimeValue,
			EndTimeScale,
			Info,
			*GetCurrentThreadStr()
		);

		const FTouchEngineHazardPointer* HazardPointer = static_cast<FTouchEngineHazardPointer*>(Info);
		if (HazardPointer && HazardPointer->TouchEngine.IsValid())
		{
			if (const TSharedPtr<FTouchEngine> TouchEnginePin = HazardPointer->TouchEngine.Pin())
			{
				TouchEnginePin->TouchEventCallback_AnyThread(Instance, Event, Result, StartTimeValue, StartTimeScale, EndTimeValue, EndTimeScale);
			}
		}
	}

	void FTouchEngineHazardPointer::LinkValueCallback_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier, void* Info)
	{
		UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkCallback(TEInstance: '%p', event: '%s', identifier '%hs', info: '%p') [Thread: '%s']"),
			Instance,
			*TELinkEventToString(Event),
			Identifier,
			Info,
			*GetCurrentThreadStr()
		);

		const FTouchEngineHazardPointer* HazardPointer = static_cast<FTouchEngineHazardPointer*>(Info);
		if (HazardPointer && HazardPointer->TouchEngine.IsValid())
		{
			if (const TSharedPtr<FTouchEngine> TouchEnginePin = HazardPointer->TouchEngine.Pin())
			{
				TouchEnginePin->LinkValue_AnyThread(Instance, Event, Identifier);
			}
		}
	}

	FTouchEngine::~FTouchEngine()
	{
		check(IsInGameThread());
		DestroyTouchEngine_GameThread();
	}

	TFuture<FTouchLoadResult> FTouchEngine::LoadTox_GameThread(const FString& InToxPath, UTouchEngineComponentBase* Component, double TimeoutInSeconds)
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
			TouchResources.ErrorLog = MakeShared<FTouchErrorLog>(TWeakObjectPtr<UTouchEngineComponentBase>(Component));
		}
		if (InToxPath.IsEmpty())
		{
			const FString ErrMessage(TEXT("Invalid .tox file path. The path is empty"));
			TouchResources.ErrorLog->AddError(ErrMessage);
			LoadState_GameThread = ELoadState::FailedToLoad;
			return MakeFulfilledPromise<FTouchLoadResult>(FTouchLoadResult::MakeFailure(ErrMessage)).GetFuture();
		}
		if (!FPaths::FileExists(InToxPath))
		{
			const FString ErrMessage(FString::Printf(TEXT("Invalid .tox file path. The file is not found at location '%s'"), *InToxPath));
			TouchResources.ErrorLog->AddError(ErrMessage);
			LoadState_GameThread = ELoadState::FailedToLoad;
			return MakeFulfilledPromise<FTouchLoadResult>(FTouchLoadResult::MakeFailure(ErrMessage)).GetFuture();
		}

		// Set the path now so it may be used later in case we're in an unload
		LastToxPathAttemptedToLoad = InToxPath;
		LastLoadTimeoutInSeconds = IsValid(Component) && TimeoutInSeconds <= 0 ? Component->ToxLoadTimeout : TimeoutInSeconds;
		if (!ensureMsgf(LastLoadTimeoutInSeconds > 0, TEXT("Given an invalid time-out to load the tox file")))
		{
			LastLoadTimeoutInSeconds = 30.0;
		}
		
		// Defer the load until Unloading is done. OnInstancedUnloaded_AnyThread will schedule a task on the game thread once unloading is done.
		if (LoadState_GameThread == ELoadState::Unloading)
		{
			LoadPromise = TPromise<FTouchLoadResult>();
			return LoadPromise->GetFuture();
		}

		return LoadTouchEngine(InToxPath, LastLoadTimeoutInSeconds);
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
			DestroyResources_GameThread(MoveTemp(OldToxPath));
		}
	}
	
	int64 FTouchEngine::GetNextFrameID() const
	{
		return LoadState_GameThread == ELoadState::Ready && TouchResources.FrameCooker ? TouchResources.FrameCooker->GetNextFrameID() : -1;
	}

	TFuture<FCookFrameResult> FTouchEngine::CookFrame_GameThread(FCookFrameRequest&& CookFrameRequest, int32 InputBufferLimit)
	{
		check(IsInGameThread());

		const bool bIsDestroyingTouchEngine = !TouchResources.FrameCooker.IsValid() || !TouchResources.VariableManager.IsValid();
		if (bIsDestroyingTouchEngine || !IsReadyToCookFrame())
		{
			const int64 FrameLastUpdated = TouchResources.FrameCooker.IsValid() ? TouchResources.FrameCooker->GetFrameLastUpdated() : -1;
			return MakeFulfilledPromise<FCookFrameResult>(FCookFrameResult::FromCookFrameRequest(CookFrameRequest, ECookFrameResult::BadRequest, FrameLastUpdated)).GetFuture();
		}
		
		TFuture<FCookFrameResult> CookFrame = TouchResources.FrameCooker->CookFrame_GameThread(MoveTemp(CookFrameRequest), InputBufferLimit)
           .Next([this](FCookFrameResult Value)
           {
               UE_LOG(LogTouchEngine, Verbose, TEXT("[CookFrame_GameThread->Next[%s]] Finished cooking frame (code: %d)"), *GetCurrentThreadStr(), static_cast<int32>(Value.Result));

               switch (Value.Result)
               {
               // These cases are expected and indicate no error
               case ECookFrameResult::Success: break;
               case ECookFrameResult::Cancelled: break;
               case ECookFrameResult::InputsDiscarded: break;

               case ECookFrameResult::BadRequest: TouchResources.ErrorLog->AddError(TEXT("A request to cook a frame was made while the engine was not fully initialized or shutting down."));
                   break;
               case ECookFrameResult::FailedToStartCook: TouchResources.ErrorLog->AddError(TEXT("Failed to start cook."));
                   break;
               case ECookFrameResult::InternalTouchEngineError:
                   HandleTouchEngineInternalError(Value.TouchEngineInternalResult);
                   break;
               case ECookFrameResult::TouchEngineCookTimeout: TouchResources.ErrorLog->AddWarning(FTouchErrorLog::EErrorType::TECookTimeout);
				   break;

               default:
                   static_assert(static_cast<int32>(ECookFrameResult::Count) == 7, "Update this switch");
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


	void FTouchEngine::CancelCurrentAndNextCooks_GameThread(ECookFrameResult CookFrameResult)
	{
		if (LoadState_GameThread == ELoadState::Ready && TouchResources.FrameCooker)
		{
			TouchResources.FrameCooker->CancelCurrentAndNextCooks(CookFrameResult);
		}
	}

	bool FTouchEngine::CancelCurrentFrame_GameThread(int64 FrameID, ECookFrameResult CookFrameResult)
	{
		if (LoadState_GameThread == ELoadState::Ready && TouchResources.FrameCooker)
		{
			return TouchResources.FrameCooker->CancelCurrentFrame_GameThread(FrameID, CookFrameResult);
		}
		return false;
	}

	bool FTouchEngine::CheckIfCookTimedOut_GameThread(double CookTimeoutInSeconds)
	{
		if (LoadState_GameThread == ELoadState::Ready && TouchResources.FrameCooker)
		{
			return TouchResources.FrameCooker->CheckIfCookTimedOut_GameThread(CookTimeoutInSeconds);
		}
		return false;
	}

	void FTouchEngine::HandleTouchEngineInternalError(const TEResult CookResult)
	{
		const FString Message = TEResultGetDescription(CookResult);

		if (TEResultGetSeverity(CookResult) == TESeverityError)
		{
			TouchResources.ErrorLog->AddError(Message);
		}
		else
		{
			TouchResources.ErrorLog->AddWarning(Message);
		}
	}

	void FTouchEngine::FTouchResources::ForceCloseTEInstance()
	{
		TouchEngineInstance.reset();
		if (FrameCooker)
		{
			FrameCooker->ResetTouchEngineInstance();
		}
		if (VariableManager)
		{
			VariableManager->ResetTouchEngineInstance();
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
	
	bool FTouchEngine::SetExportedTexturePoolSize(int ExportedTexturePoolSize)
	{
		if (ensureMsgf(TouchResources.ResourceProvider, TEXT("ExportedTexturePoolSize can only be set after the engine is started.")))
		{
			TouchResources.ResourceProvider->SetExportedTexturePoolSize(ExportedTexturePoolSize);
			return true;
		}
		return false;
	}
	bool FTouchEngine::SetImportedTexturePoolSize(int ImportedTexturePoolSize)
	{
		if (ensureMsgf(TouchResources.ResourceProvider, TEXT("ImportedTexturePoolSize can only be set after the engine is started.")))
		{
			TouchResources.ResourceProvider->SetExportedTexturePoolSize(ImportedTexturePoolSize);
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

	TFuture<FTouchLoadResult> FTouchEngine::LoadTouchEngine(const FString& InToxPath, double TimeoutInSeconds)
	{
		if (!InstantiateEngineWithToxFile(InToxPath))
		{
			LoadState_GameThread = ELoadState::FailedToLoad;
			return MakeFulfilledPromise<FTouchLoadResult>(FTouchLoadResult::MakeFailure(TEXT("Failed to init TE instance with .tox file."))).GetFuture();
		}
		check(LoadPromise);
		LoadState_GameThread = ELoadState::Loading;
		
		UE_LOG(LogTouchEngine, Display, TEXT("Started loading of Tox file '%s'"), *InToxPath);
		if (!OutputResultAndCheckForError_GameThread(TEInstanceLoad(TouchResources.TouchEngineInstance), FString::Printf(TEXT("TouchEngine instance failed to load tox file '%s'"), *InToxPath)))
		{
			return EmplaceFailedPromiseAndReturnFuture_GameThread(TEXT("TEInstanceLoad failed."));
		}

		if (!OutputResultAndCheckForError_GameThread(TEInstanceResume(TouchResources.TouchEngineInstance), TEXT("Unable to resume TouchEngine")))
		{
			return EmplaceFailedPromiseAndReturnFuture_GameThread(TEXT("TEInstanceResume failed."));
		}


		{
			FScopeLock Lock(&LoadTimeoutTaskLock);
			// Here we will check if the task passed its timeout, and if yes we unload it. Using the OnBeginFrame delegate to have a consistent implementation for Editor and Game
			LoadTimeoutTaskHandle = FCoreDelegates::OnBeginFrame.AddLambda([EndTime = FPlatformTime::Seconds() + TimeoutInSeconds, TimeoutInSeconds = TimeoutInSeconds, InToxPath = InToxPath, WeakThis = SharedThis(this)->AsWeak()]()
			{
				if (FPlatformTime::Seconds() < EndTime)
				{
					return;
				}
				
				if (const TSharedPtr<FTouchEngine> SharedThis = WeakThis.Pin())
				{
					FScopeLock Lock(&SharedThis->LoadTimeoutTaskLock);
					if (SharedThis->LoadTimeoutTaskHandle)
					{
						if (SharedThis->TouchResources.ErrorLog)
						{
							SharedThis->TouchResources.ErrorLog->AddError(FTouchErrorLog::EErrorType::TELoadToxTimeout,FString(),
								GET_FUNCTION_NAME_CHECKED(FTouchEngine, LoadTouchEngine), FString::Printf(TEXT("Tox file '%s' timed-out after %f s"), *InToxPath, TimeoutInSeconds));
						}
						else
						{
							UE_LOG(LogTouchEngine, Error, TEXT("Loading of the Tox '%s' timed-out after %f s"), *InToxPath, TimeoutInSeconds);
						}
						FCoreDelegates::OnBeginFrame.Remove(SharedThis->LoadTimeoutTaskHandle.GetValue());
						SharedThis->LoadTimeoutTaskHandle.Reset();
						
						Lock.Unlock();
						// if TEInstanceUnload is successful, TouchEventCallback_AnyThread will end up being called with event TEEventInstanceDidLoad and result TEResultCancelled
						const TEResult UnloadResult = TEInstanceUnload(SharedThis->TouchResources.TouchEngineInstance);
					}
				}
			});
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
			if (!OutputResultAndCheckForError_GameThread(TouchResources.ResourceProvider ? TEResultSuccess : TEResultFeatureNotSupportedBySystem,
				FString::Printf(TEXT("Impossible to create a ressource provider for the current RHI `%s` which is not supported."), GDynamicRHI->GetName())))
			{
				return false;
			}
			
			// The TE instance may get destroyed latently after the owning FTouchEngine is!
			// HazardPointer's job is to avoid TE from keep on to garbage memory; the HazardPointer is destroyed after the TE instance is destroyed.
			TouchResources.HazardPointer = MakeShared<FTouchEngineHazardPointer>(SharedThis(this));
			const TEResult TouchEngineInstanceResult = TEInstanceCreate(FTouchEngineHazardPointer::TouchEventCallback_AnyThread, FTouchEngineHazardPointer::LinkValueCallback_AnyThread, TouchResources.HazardPointer.Get(), TouchResources.TouchEngineInstance.take());
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceCreate(event_callback: '%p', link_callback: '%p', callback_info: '%p', instance: '%p') [Thread: '%s'] => Returned:  '%s'"),
				FTouchEngineHazardPointer::TouchEventCallback_AnyThread,
				FTouchEngineHazardPointer::LinkValueCallback_AnyThread,
				TouchResources.HazardPointer.Get(),
				TouchResources.TouchEngineInstance.get(),
				*UE::TouchEngine::GetCurrentThreadStr(),
				*TEResultToString(TouchEngineInstanceResult)
			);
			
			if (!OutputResultAndCheckForError_GameThread(TouchEngineInstanceResult, TEXT("Unable to create TouchEngine Instance")))
			{
				return false;
			}

			const TEResult SetFrameResult = TEInstanceSetFrameRate(TouchResources.TouchEngineInstance, TargetFrameRate, 1);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceSetFrameRate(TEInstance: '%p', numerator: '%lld', denominator: '1') [Thread: '%s'] => Returned:  '%s'"),
				TouchResources.TouchEngineInstance.get(),
				FMath::RoundToInt64(TargetFrameRate),
				*UE::TouchEngine::GetCurrentThreadStr(),
				*TEResultToString(SetFrameResult)
			);

			if (!OutputResultAndCheckForError_GameThread(SetFrameResult, TEXT("Unable to set frame rate")))
			{
				return false;
			}
			
			const TEResult GraphicsContextResult = TEInstanceAssociateGraphicsContext(TouchResources.TouchEngineInstance, TouchResources.ResourceProvider->GetContext());
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceAssociateGraphicsContext(TEInstance: '%p', context: '%p') [Thread: '%s'] => Returned:  '%s'"),
				TouchResources.TouchEngineInstance.get(),
				TouchResources.ResourceProvider->GetContext(),
				*UE::TouchEngine::GetCurrentThreadStr(),
				*TEResultToString(GraphicsContextResult)
			);

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
		UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceConfigure(TEInstance: '%p', path: '%hs', mode: '%s') [Thread: '%s'] => Returned:  '%s'"),
			TouchResources.TouchEngineInstance.get(),
			Path,
			TimeMode == TETimeInternal ? TEXT("TimeInternal") : TEXT("TimeExternal"),
			*UE::TouchEngine::GetCurrentThreadStr(),
			*TEResultToString(ConfigurationResult)
		);

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
				//todo: when cancelled, TouchResources.FrameCooker->GetCookingFrameID() returns -1, we should be able to return the last one that was set prior
				const int64 CookingFrameID = TouchResources.FrameCooker ? TouchResources.FrameCooker->GetCookingFrameID() : -1;
				UE_CLOG(Result == TEResultSuccess, LogTouchEngine, Log, TEXT("TEEventFrameDidFinish[%s] for frame `%lld`:  StartTime: %lld  TimeScale: %d    EndTime: %lld  TimeScale: %d => %s"), *GetCurrentThreadStr(), CookingFrameID, StartTimeValue, StartTimeScale, EndTimeValue, EndTimeScale, *TEResultToString(Result));
				if (Result != TEResultSuccess)
				{
					UE_CLOG(Result == TEResultCancelled, LogTouchEngine, Log, TEXT("TEEventFrameDidFinish[%s] for frame `%lld`:  StartTime: %lld  TimeScale: %d    EndTime: %lld  TimeScale: %d => %s (`%hs`)"), *GetCurrentThreadStr(), CookingFrameID, StartTimeValue, StartTimeScale, EndTimeValue, EndTimeScale, *TEResultToString(Result), TEResultGetDescription(Result));
					if (Result != TEResultCancelled && TouchResources.ErrorLog)
					{
						TouchResources.ErrorLog->AddResult(TEXT("The Cook was not successful."), Result, FString(), GET_FUNCTION_NAME_CHECKED(FTouchEngine, TouchEventCallback_AnyThread));
					}
				}
				
				// We know the cook was not processed if we receive a TEEventFrameDidFinish event with the same time as the previous one.
				const bool bFrameDropped = LastFrameStartTimeValue.IsSet() && LastFrameStartTimeValue.GetValue() == StartTimeValue;
				UE_LOG(LogTouchEngine, Log, TEXT(" -- TouchEventCallback_AnyThread with event 'TEEventFrameDidFinish' and start_time_value '%lld' [time_scale: '%d'], end_time_value '%lld' [time_scale: '%d'], for CookingFrame `%lld`. FrameDropped? `%s"),
					StartTimeValue, StartTimeScale, EndTimeValue, EndTimeScale, CookingFrameID, bFrameDropped ? TEXT("TRUE") : TEXT("FALSE"))

				if (TouchResources.FrameCooker.IsValid())
				{
					TouchResources.FrameCooker->OnFrameFinishedCooking_AnyThread(Result, bFrameDropped, static_cast<double>(StartTimeValue) / StartTimeScale, static_cast<double>(EndTimeValue) / EndTimeScale);
				}
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
		{
			FScopeLock Lock(&LoadTimeoutTaskLock);
			if (LoadTimeoutTaskHandle) // If we reached that point and we still have a Timeout task handle, we destroy it as we got a result before the timeout
			{
				FCoreDelegates::OnBeginFrame.Remove(LoadTimeoutTaskHandle.GetValue());
				LoadTimeoutTaskHandle.Reset();
			}
		}

		if (Result == TEResultSuccess)
		{
			FinishLoadInstance_AnyThread(Instance);
			return;
		}

		const FString ErrorMessage = FString::Printf(TEXT("Loading the Tox file failed '%s'"), *LastToxPathAttemptedToLoad);
		switch (Result)
		{
		case TEResultFileError:
			OnLoadError_AnyThread(ErrorMessage, Result);
			break;

		case TEResultIncompatibleEngineVersion:
			OnLoadError_AnyThread(ErrorMessage, Result);
			break;
			
		case TEResultCancelled:
			OnLoadError_AnyThread(ErrorMessage, Result);
			break;
		
		default:
			const TESeverity Severity = TEResultGetSeverity(Result);
			if (Severity == TESeverityError)
			{
				OnLoadError_AnyThread(ErrorMessage, Result);
				return;
			}
			if (Severity == TESeverityWarning && TouchResources.ErrorLog)
			{
				TouchResources.ErrorLog->AddResult( FString::Printf(TEXT("Loading the Tox file '%s' issued a warning."), *LastToxPathAttemptedToLoad),
					Result,FString(),GET_FUNCTION_NAME_CHECKED(FTouchEngine, OnInstancedLoaded_AnyThread));
			}
			FinishLoadInstance_AnyThread(Instance);
		}
	}

	void FTouchEngine::FinishLoadInstance_AnyThread(TEInstance* Instance)
	{
		// We must check whether the loaded instance is compatible with Unreal Engine
		const FTouchLoadInstanceResult ValidationResult = TouchResources.ResourceProvider->ValidateLoadedTouchEngine();
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
		
		// We want to call Async and not ExecuteOnGameThread to be sure any TE callback has had the chance to finish before we raise BP events that might end up firing other TE Callbacks
		AsyncTask(ENamedThreads::GameThread, [WeakThis = SharedThis(this)->AsWeak(), VariablesIn = MoveTemp(VariablesIn), VariablesOut = MoveTemp(VariablesOut)]() mutable
		{
			if (const TSharedPtr<FTouchEngine> SharedThis = WeakThis.Pin())
			{
				if (SharedThis->LoadState_GameThread != ELoadState::Loading)
				{
					return;
				}
				SharedThis->TouchResources.VariableManager = MakeShared<FTouchVariableManager>(SharedThis->TouchResources.TouchEngineInstance, SharedThis->TouchResources.ResourceProvider, SharedThis->TouchResources.ErrorLog);

				check(SharedThis->TouchResources.ResourceProvider); //TouchResources.ResourceProvider is supposed to be valid at this point as it has been created in InstantiateEngineWithToxFile
				SharedThis->TouchResources.FrameCooker = MakeShared<FTouchFrameCooker>(SharedThis->TouchResources.TouchEngineInstance, *SharedThis->TouchResources.VariableManager, *SharedThis->TouchResources.ResourceProvider);
				SharedThis->TouchResources.FrameCooker->SetTimeMode(SharedThis->TimeMode);
			
				SharedThis->LoadState_GameThread = ELoadState::Ready;
				SharedThis->EmplaceLoadPromiseIfSet_GameThread(FTouchLoadResult::MakeSuccess(MoveTemp(VariablesIn.Value), MoveTemp(VariablesOut.Value)));
			}
		});
	}

	void FTouchEngine::OnLoadError_AnyThread(const FString& BaseErrorMessage, TOptional<TEResult> Result)
	{
		// We want to call Async and not ExecuteOnGameThread to be sure any TE callback has had the chance to finish before we raise BP events that might end up firing other TE Callbacks
		AsyncTask(ENamedThreads::GameThread, [WeakThis = SharedThis(this)->AsWeak(), BaseErrorMessage, Result]()
			{
				if (const TSharedPtr<FTouchEngine> ThisPin = WeakThis.Pin())
				{
					const FString ResultDescription = Result ? TEResultGetDescription(Result.GetValue()) : FString();
					const FString FinalMessage = !BaseErrorMessage.IsEmpty() ? FString::Printf(TEXT("%s %s"), *BaseErrorMessage, *ResultDescription) : ResultDescription;
					if (ThisPin->TouchResources.ErrorLog)
					{
						if (Result)
						{
							ThisPin->TouchResources.ErrorLog->AddResult(BaseErrorMessage, Result.GetValue(), {},GET_FUNCTION_NAME_CHECKED(FTouchEngine, OnLoadError_AnyThread));
						}
						else
						{
							ThisPin->TouchResources.ErrorLog->AddError(FinalMessage, {},GET_FUNCTION_NAME_CHECKED(FTouchEngine, OnLoadError_AnyThread));
						}
					}

					ThisPin->EmplaceLoadPromiseIfSet_GameThread(FTouchLoadResult::MakeFailure(FinalMessage));
				}
			}
		);
	}

	TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> FTouchEngine::ProcessTouchVariables(TEInstance* Instance, TEScope Scope)
	{
		TArray<FTouchEngineDynamicVariableStruct> Variables;
		
		TouchObject<TEStringArray> Groups;
		TEResult Result = TEInstanceGetLinkGroups(Instance, Scope, Groups.take());
		if (Result != TEResultSuccess)
		{
			return { Result, Variables };
		}

		for (int32_t i = 0; i < Groups->count; i++)
		{
			Result = FTouchEngineParserUtils::Parse(Instance, Groups->strings[i], Variables);
			if (Result != TEResultSuccess)
			{
				return { Result, Variables };
			}
		}

		return { Result, Variables };
	}

	void FTouchEngine::OnInstancedUnloaded_AnyThread()
	{
		// We want to call Async and not ExecuteOnGameThread to be sure any TE callback has had the chance to finish before we raise BP events that might end up firing other TE Callbacks
		AsyncTask(ENamedThreads::GameThread, [WeakThis = SharedThis(this)->AsWeak()]()
		{
			if (const TSharedPtr<FTouchEngine> ThisPin = WeakThis.Pin())
			{
				// We want to keep the failed to load state to avoid automatically reloading the Tox file
				ThisPin->LoadState_GameThread = ThisPin->LoadState_GameThread == ELoadState::FailedToLoad ? ThisPin->LoadState_GameThread : ELoadState::Unloaded;
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
			
			LoadTouchEngine(GetToxPath(), LastLoadTimeoutInSeconds)
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

		{
			FScopeLock Lock(&LoadTimeoutTaskLock);
			if (LoadTimeoutTaskHandle) // We might be loading a tox file while we are trying to destroy
			{
				FCoreDelegates::OnBeginFrame.Remove(LoadTimeoutTaskHandle.GetValue());
				LoadTimeoutTaskHandle.Reset();
			}
		}
		
		EmplaceLoadPromiseIfSet_GameThread(FTouchLoadResult::MakeFailure(TEXT("TouchEngine being reset.")));
		LastToxPathAttemptedToLoad.Empty();
		if (TouchResources.FrameCooker)
		{
			TouchResources.FrameCooker->CancelCurrentAndNextCooks();
		}
		if (TouchResources.ResourceProvider)
		{
			TouchResources.ResourceProvider->ClearSavedInstance();
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

		UE_CLOG(LoadResult.IsSuccess(), LogTouchEngine, Display, TEXT("Successfully finished loading TouchEngine instance with tox file `%s`"), *GetToxPath());
		UE_CLOG(LoadResult.IsFailure(), LogTouchEngine, Warning, TEXT("Loading TouchEngine instance with tox file '%s' raised the error: '%s'"), *GetToxPath(), *LoadResult.FailureResult->ErrorMessage);
		LoadState_GameThread = LoadResult.IsSuccess()
			? ELoadState::Ready
			: ELoadState::FailedToLoad;
		LastToxPathAttemptedToLoad = LoadResult.IsSuccess() ? LastToxPathAttemptedToLoad : FString();
		Promise.EmplaceValue(MoveTemp(LoadResult));
	}

	void FTouchEngine::DestroyResources_GameThread(FString OldToxPath)
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
		
		FTouchResources KeepAlive = MoveTemp(TouchResources);
		TouchResources.Reset(); // We need the TouchResources variable of this FTouchEngine to be cleared before calling the functions below.

		KeepAlive.ForceCloseTEInstance(); // We want to ensure that the TEInstance has been released, so we start destroying some of the resources

		KeepAlive.ResourceProvider->SuspendAsyncTasks_GameThread()
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