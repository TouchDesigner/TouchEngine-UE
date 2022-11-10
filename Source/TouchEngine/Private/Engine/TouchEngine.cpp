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

#include "Async/Async.h"
#include "Engine/Util/CookFrameData.h"
#include "Util/TouchFrameCooker.h"

#define LOCTEXT_NAMESPACE "UTouchEngine"

void UTouchEngine::BeginDestroy()
{
	Clear_GameThread();
	Super::BeginDestroy();
}

void UTouchEngine::LoadTox(const FString& InToxPath)
{
	if (GIsCookerLoadingPackage)
	{
		return;
	}

	if (bIsDestroyingTouchEngine)
	{
		// TODO: Right now we do not provide any events so people can find out whether it's being destroyed
		UE_LOG(LogTouchEngine, Error, TEXT("The engine is still cleaning up old resources. Try again later."))
		return;
	}

	TouchResources.ErrorLog = MakeShared<UE::TouchEngine::FTouchErrorLog>();
	bDidLoad = false;

	if (InToxPath.IsEmpty())
	{
		const FString ErrMessage(FString::Printf(TEXT("%S: Tox file path is empty"), __FUNCTION__));
		TouchResources.ErrorLog->AddError(ErrMessage);
		bFailedLoad = true;
		OnLoadFailed.Broadcast(ErrMessage);
		return;
	}

	if (!InstantiateEngineWithToxFile(InToxPath))
	{
		return;
	}

	bLoadCalled = true;
	UE_LOG(LogTouchEngine, Log, TEXT("Loading %s"), *InToxPath);
	if (!OutputResultAndCheckForError(TEInstanceLoad(TouchResources.TouchEngineInstance), FString::Printf(TEXT("TouchEngine instance failed to load tox file '%s'"), *InToxPath)))
	{
		return;
	}

	OutputResultAndCheckForError(TEInstanceResume(TouchResources.TouchEngineInstance), TEXT("Unable to resume TouchEngine"));
}

void UTouchEngine::Unload()
{
	if (TouchResources.TouchEngineInstance)
	{
		bConfiguredWithTox = false;
		bDidLoad = false;
		bFailedLoad = false;
		ToxPath = "";
		bLoadCalled = false;
	}
}

TFuture<UE::TouchEngine::FCookFrameResult> UTouchEngine::CookFrame_GameThread(const UE::TouchEngine::FCookFrameRequest& CookFrameRequest)
{
	using namespace UE::TouchEngine;
	check(IsInGameThread());
	
	if (!TouchResources.FrameCooker || bIsDestroyingTouchEngine)
	{
		return MakeFulfilledPromise<FCookFrameResult>(FCookFrameResult{ ECookFrameErrorCode::BadRequest }).GetFuture();
	}
	
	TouchResources.ErrorLog->OutputMessages_GameThread();
	return TouchResources.FrameCooker->CookFrame_GameThread(CookFrameRequest)
		.Next([this](FCookFrameResult Value)
		{
			switch (Value.ErrorCode)
			{
				// These cases are expected and indicate no error
				case ECookFrameErrorCode::Success: break;
				case ECookFrameErrorCode::Replaced: break;
				case ECookFrameErrorCode::Cancelled: break;
					
				case ECookFrameErrorCode::BadRequest: TouchResources.ErrorLog->AddError(TEXT("You made a request to cook a frame while the engine was not fully initialized or shutting down.")); break;
				case ECookFrameErrorCode::FailedToStartCook: TouchResources.ErrorLog->AddError(TEXT("Failed to start cook.")); break;
				case ECookFrameErrorCode::InternalTouchEngineError: TouchResources.ErrorLog->AddError(TEXT("TouchEngine encountered an error cooking the frame.")); break;
			default:
				static_assert(static_cast<int32>(ECookFrameErrorCode::Count) == 6, "Update this switch");
				break;
			}

			return Value;
		});
}

void UTouchEngine::SetCookMode(bool bIsIndependent)
{
	if (ensureMsgf(!TouchResources.TouchEngineInstance, TEXT("TimeMode can only be set before the engine is started.")))
	{
		TimeMode = bIsIndependent
			? TETimeInternal
			: TETimeExternal;
	}
}

bool UTouchEngine::SetFrameRate(int64 FrameRate)
{
	if (!ensureMsgf(!TouchResources.TouchEngineInstance, TEXT("TargetFrameRate can only be set before the engine is started.")))
	{
		TargetFrameRate = FrameRate;
		return true;
	}

	return false;
}

bool UTouchEngine::IsLoading() const
{
	return bLoadCalled && !bDidLoad && !bFailedLoad;
}

bool UTouchEngine::InstantiateEngineWithToxFile(const FString& InToxPath)
{
	if (!InToxPath.IsEmpty() && !InToxPath.EndsWith(".tox"))
	{
		bFailedLoad = true;
		const FString FullMessage = FString::Printf(TEXT("Invalid file path - %s"), *InToxPath);
		TouchResources.ErrorLog->AddError(FullMessage);
		OnLoadFailed.Broadcast(FullMessage);
		return false;
	}
	
	if (!TouchResources.TouchEngineInstance)
	{
		checkf(!TouchResources.ResourceProvider, TEXT("ResourceProvider was expected to be null if there is no running instance!"));
		TouchResources.ResourceProvider = UE::TouchEngine::ITouchEngineModule::Get().CreateResourceProvider();

		const TEResult TouchEngineInstace = TEInstanceCreate(TouchEventCallback_AnyThread, LinkValueCallback_AnyThread, this, TouchResources.TouchEngineInstance.take());
		if (!OutputResultAndCheckForError(TouchEngineInstace, TEXT("Unable to create TouchEngine Instance")))
		{
			return false;
		}

		const TEResult SetFrameResult = TEInstanceSetFrameRate(TouchResources.TouchEngineInstance, TargetFrameRate, 1);
		if (!OutputResultAndCheckForError(SetFrameResult, TEXT("Unable to set frame rate")))
		{
			return false;
		}
		
		const TEResult GraphicsContextResult = TEInstanceAssociateGraphicsContext(TouchResources.TouchEngineInstance, TouchResources.ResourceProvider->GetContext());
		if (!OutputResultAndCheckForError(GraphicsContextResult, TEXT("Unable to associate graphics Context")))
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
	if (!OutputResultAndCheckForError(ConfigurationResult, ErrorMessage))
	{
		bConfiguredWithTox = false;
		ToxPath = "";
		return false;
	}
	
	bConfiguredWithTox = bLoadTox;
	ToxPath = InToxPath;
	return true;
}

void UTouchEngine::TouchEventCallback_AnyThread(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale, void* Info)
{
	UTouchEngine* Engine = static_cast<UTouchEngine*>(Info);
	if (!Engine || Engine->bIsDestroyingTouchEngine)
	{
		return;
	}

	switch (Event)
	{
	case TEEventInstanceDidLoad:
	{
		Engine->OnInstancedLoaded_AnyThread(Instance, Result);
		break;
	}
	case TEEventFrameDidFinish:
	{
		Engine->TouchResources.FrameCooker->OnFrameFinishedCooking(Result);
		break;
	}
	case TEEventGeneral:
	default:
		break;
	}
}

void UTouchEngine::OnInstancedLoaded_AnyThread(TEInstance* Instance, TEResult Result)
{
	switch (Result)
	{
	case TEResultFileError:
		OnLoadError_AnyThread(Result, FString::Printf(TEXT("load() failed to load .tox \"%s\""), *ToxPath));
		break;

	case TEResultIncompatibleEngineVersion:
		OnLoadError_AnyThread(Result);
		break;
		
	case TEResultSuccess:
		FinishLoadInstance_AnyThread(Instance);
		break;
	default:
		TEResultGetSeverity(Result) == TESeverityError
			? OnLoadError_AnyThread(Result, TEXT("load() severe tox file error:"))
			: FinishLoadInstance_AnyThread(Instance);
	}
}

void UTouchEngine::FinishLoadInstance_AnyThread(TEInstance* Instance)
{
	const TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> VariablesIn = ProcessTouchVariables(Instance, TEScopeInput);
	const TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> VariablesOut = ProcessTouchVariables(Instance, TEScopeOutput);

	const TEResult VarInResult = VariablesIn.Key;
	if (VarInResult != TEResultSuccess)
	{
		OnLoadError_AnyThread(VarInResult, TEXT("Failed to load input variables."));
		return;
	}

	const TEResult VarOutResult = VariablesIn.Key;
	if (VarOutResult != TEResultSuccess)
	{
		OnLoadError_AnyThread(VarOutResult, TEXT("Failed to load ouput variables."));
		return;
	}
	
	AsyncTask(ENamedThreads::GameThread,
		[this, VariablesIn, VariablesOut]()
		{
			TouchResources.VariableManager = MakeShared<UE::TouchEngine::FTouchVariableManager>(TouchResources.TouchEngineInstance, TouchResources.ResourceProvider, TouchResources.ErrorLog);

			TouchResources.FrameCooker = MakeShared<UE::TouchEngine::FTouchFrameCooker>(TouchResources.TouchEngineInstance, *TouchResources.VariableManager);
			TouchResources.FrameCooker->SetTimeMode(TimeMode);
			
			SetDidLoad();
			UE_LOG(LogTouchEngine, Log, TEXT("Loaded %s"), *GetToxPath());
			OnParametersLoaded.Broadcast(VariablesIn.Value, VariablesOut.Value);
		}
	);
}

void UTouchEngine::OnLoadError_AnyThread(TEResult Result, const FString& BaseErrorMessage)
{
	AsyncTask(ENamedThreads::GameThread,
		[this, BaseErrorMessage, Result]()
		{
			const FString FinalMessage = BaseErrorMessage.IsEmpty()
				? FString::Printf(TEXT("%s %hs"), *BaseErrorMessage, TEResultGetDescription(Result))
				: TEResultGetDescription(Result);
			TouchResources.ErrorLog->AddError(FinalMessage);
			
			bFailedLoad = true;
			OnLoadFailed.Broadcast(TEResultGetDescription(Result));
		}
	);
}

TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> UTouchEngine::ProcessTouchVariables(TEInstance* Instance, TEScope Scope)
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

void UTouchEngine::LinkValueCallback_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier, void* Info)
{
	UTouchEngine* Doc = static_cast<UTouchEngine*>(Info);
	Doc->LinkValue_AnyThread(Instance, Event, Identifier);
}

void UTouchEngine::LinkValue_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier)
{
	if (!ensure(Instance) || bIsDestroyingTouchEngine)
	{
		return;
	}

	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(Instance, Identifier, &Param);
	ON_SCOPE_EXIT
	{
		TERelease(&Param);
	};

	const bool bIsOutputValue = Result == TEResultSuccess && Param && Param->scope == TEScopeOutput;
	const bool bHasValueChanged = Event == TELinkEventValueChange;
	const bool bIsTextureValue = Param && Param->type == TELinkTypeTexture;
	if (bIsOutputValue && bHasValueChanged && bIsTextureValue)
	{
		ProcessLinkTextureValueChanged_AnyThread(Identifier);
	}
}

void UTouchEngine::ProcessLinkTextureValueChanged_AnyThread(const char* Identifier)
{
	using namespace UE::TouchEngine;
	
	// Stash the state, we don't do any actual renderer work from this thread
	TouchObject<TETexture> Texture = nullptr;
	const TEResult Result = TEInstanceLinkGetTextureValue(TouchResources.TouchEngineInstance, Identifier, TELinkValueCurrent, Texture.take());
	if (Result != TEResultSuccess)
	{
		return;
	}

	// Do not create any more values until we've processed this one (better performance)
	TEInstanceLinkSetInterest(TouchResources.TouchEngineInstance, Identifier, TELinkInterestSubsequentValues);

	const FName ParamId(Identifier);
	TouchResources.VariableManager->AllocateLinkedTop(ParamId); // Avoid system querying this param from generating an output error
	TouchResources.ResourceProvider->ImportTextureToUnrealEngine({ TouchResources.TouchEngineInstance, ParamId, Texture })
		.Next([this, ParamId](const FTouchImportResult& TouchLinkResult)
		{
			if (TouchLinkResult.ResultType != EImportResultType::Success)
			{
				return;
			}

			UTexture2D* Texture = TouchLinkResult.ConvertedTextureObject.GetValue();
			if (IsInGameThread())
			{
				TouchResources.VariableManager->UpdateLinkedTOP(ParamId, Texture);
			}
			else
			{
				AsyncTask(ENamedThreads::GameThread, [WeakVariableManger = TWeakPtr<FTouchVariableManager>(TouchResources.VariableManager), ParamId, Texture]()
				{
					// Scenario: end PIE session > causes FlushRenderCommands > finishes the link texture task > enqueues a command on game thread > will execute when we've already been destroyed
					if (TSharedPtr<FTouchVariableManager> PinnedVariableManager = WeakVariableManger.Pin())
					{
						PinnedVariableManager->UpdateLinkedTOP(ParamId, Texture);
					}
				});
			}
		});
}

void UTouchEngine::Clear_GameThread()
{
	check(IsInGameThread());
	UE_LOG(LogTouchEngine, Verbose, TEXT("Shutting down TouchEngine instance (%s)"), *GetToxPath());

	// Instantiated first - if not set there is nothing to clean up
	if (!TouchResources.ResourceProvider)
	{
		return;
	}
	
	if (!ensureMsgf(!bIsDestroyingTouchEngine, TEXT("Previous destruction never finished. Investigate.")))
	{
		return;	
	}
	
	bIsDestroyingTouchEngine = true;
	// Invalid if cancelled while loading
	if (TouchResources.FrameCooker)
	{
		TouchResources.FrameCooker->CancelCurrentAndNextCook();
	}

	FTouchResources KeepAlive = TouchResources;

	// Maybe not needed - not sure whether Unreal API will call destructor after BeginDestroy
	TouchResources.TouchEngineInstance.reset();
	TouchResources.FrameCooker.Reset();
	TouchResources.VariableManager.Reset();
	TouchResources.ResourceProvider.Reset();
	TouchResources.ErrorLog.Reset();

	KeepAlive.ResourceProvider->SuspendAsyncTasks()
		.Next([KeepAlive](auto) mutable
		{
			// Important to destroy the instance first so it triggers its callbacks
			KeepAlive.TouchEngineInstance.reset();
			KeepAlive.FrameCooker.Reset();
			KeepAlive.VariableManager.Reset();
			// Latent tasks are done - the below pointers are the only ones keeping the resources alive.
			KeepAlive.ResourceProvider.Reset();
			KeepAlive.ErrorLog.Reset();
		});
}

bool UTouchEngine::OutputResultAndCheckForError(const TEResult Result, const FString& ErrMessage)
{
	if (Result != TEResultSuccess)
	{
		TouchResources.ErrorLog->AddResult(FString::Printf(TEXT("%s: "), *ErrMessage), Result);
		if (TEResultGetSeverity(Result) == TESeverity::TESeverityError)
		{
			bFailedLoad = true;
			OnLoadFailed.Broadcast(ErrMessage);
			return false;
		}
	}
	return true;
}

TSet<TEnumAsByte<EPixelFormat>> UTouchEngine::GetSupportedPixelFormat() const
{
	TSet<TEnumAsByte<EPixelFormat>> OutPixelFormat;
	if (TouchResources.ResourceProvider && TouchResources.TouchEngineInstance)
	{
		Algo::Transform(TouchResources.ResourceProvider->GetExportablePixelTypes(*TouchResources.TouchEngineInstance.get()), OutPixelFormat, [](EPixelFormat PixelFormat){ return PixelFormat; });
	}
	return OutPixelFormat;
}


#undef LOCTEXT_NAMESPACE
