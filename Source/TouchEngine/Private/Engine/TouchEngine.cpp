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
	Clear();
	Super::BeginDestroy();
}

void UTouchEngine::LoadTox(const FString& InToxPath)
{
	if (GIsCookerLoadingPackage)
	{
		return;
	}

	bDidLoad = false;

	if (InToxPath.IsEmpty())
	{
		const FString ErrMessage(FString::Printf(TEXT("%S: Tox file path is empty"), __FUNCTION__));
		ErrorLog.OutputError(ErrMessage);
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
	if (!OutputResultAndCheckForError(TEInstanceLoad(TouchEngineInstance), FString::Printf(TEXT("TouchEngine instance failed to load tox file '%s'"), *InToxPath)))
	{
		return;
	}

	OutputResultAndCheckForError(TEInstanceResume(TouchEngineInstance), TEXT("Unable to resume TouchEngine"));
}

void UTouchEngine::Unload()
{
	if (TouchEngineInstance)
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
	
	ErrorLog.OutputMessages();
	if (!FrameCooker)
	{
		return MakeFulfilledPromise<FCookFrameResult>(FCookFrameResult{ ECookFrameErrorCode::BadRequest }).GetFuture();
	}
	
	return FrameCooker->CookFrame_GameThread(CookFrameRequest)
		.Next([this](FCookFrameResult Value)
		{
			switch (Value.ErrorCode)
			{
				// These cases are expected and indicate no error
				case ECookFrameErrorCode::Success: break;
				case ECookFrameErrorCode::Replaced: break;
				case ECookFrameErrorCode::Cancelled: break;
					
				case ECookFrameErrorCode::BadRequest: ErrorLog.OutputError(TEXT("You made a request to cook a frame while the engine was not fully initialized or shutting down.")); break;
				case ECookFrameErrorCode::FailedToStartCook: ErrorLog.OutputError(TEXT("Failed to start cook.")); break;
				case ECookFrameErrorCode::InternalTouchEngineError: ErrorLog.OutputError(TEXT("Touch Engine encountered an error cooking the frame.")); break;
			default:
				static_assert(static_cast<int32>(ECookFrameErrorCode::Count) == 6, "Update this switch");
				break;
			}

			return Value;
		});
}

void UTouchEngine::SetCookMode(bool bIsIndependent)
{
	if (ensureMsgf(!TouchEngineInstance, TEXT("TimeMode can only be set before the engine is started.")))
	{
		TimeMode = bIsIndependent
			? TETimeInternal
			: TETimeExternal;
	}
}

bool UTouchEngine::SetFrameRate(int64 FrameRate)
{
	if (!ensureMsgf(!TouchEngineInstance, TEXT("TargetFrameRate can only be set before the engine is started.")))
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
		ErrorLog.OutputError(FullMessage);
		OnLoadFailed.Broadcast(FullMessage);
		return false;
	}

	// Causes old MyResourceProvider to be destroyed thus releasing its render resources
	ResourceProvider = UE::TouchEngine::ITouchEngineModule::Get().CreateResourceProvider();
	
	if (!TouchEngineInstance)
	{
		const TEResult TouchEngineInstace = TEInstanceCreate(TouchEventCallback_AnyThread, LinkValueCallback_AnyThread, this, TouchEngineInstance.take());
		if (!OutputResultAndCheckForError(TouchEngineInstace, TEXT("Unable to create TouchEngine Instance")))
		{
			return false;
		}

		const TEResult SetFrameResult = TEInstanceSetFrameRate(TouchEngineInstance, TargetFrameRate, 1);
		if (!OutputResultAndCheckForError(SetFrameResult, TEXT("Unable to set frame rate")))
		{
			return false;
		}

		const TEResult GraphicsContextResult = TEInstanceAssociateGraphicsContext(TouchEngineInstance, ResourceProvider->GetContext());
		if (!OutputResultAndCheckForError(GraphicsContextResult, TEXT("Unable to associate graphics Context")))
		{
			return false;
		}
	}

	const bool bLoadTox = !InToxPath.IsEmpty();
	const char* Path = bLoadTox ? TCHAR_TO_UTF8(*InToxPath) : nullptr;
	// Set different error message depending on intent
	const FString ErrorMessage = bLoadTox 
		? FString::Printf(TEXT("Unable to configure TouchEngine with tox file '%s'"), *InToxPath)
		: TEXT("Unable to configure TouchEngine");
	const TEResult ConfigurationResult = TEInstanceConfigure(TouchEngineInstance, Path, TimeMode);
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
	if (!Engine || Engine->bIsTearingDown)
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
		Engine->FrameCooker->OnFrameFinishedCooking(Result);
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
			VariableManager = MakeShared<UE::TouchEngine::FTouchVariableManager>(TouchEngineInstance, ResourceProvider, ErrorLog);

			FrameCooker = MakeShared<UE::TouchEngine::FTouchFrameCooker>(TouchEngineInstance, *VariableManager);
			FrameCooker->SetTimeMode(TimeMode);
			
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
			ErrorLog.OutputError(FinalMessage);
			
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
	if (!ensure(Instance))
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
	const TEResult Result = TEInstanceLinkGetTextureValue(TouchEngineInstance, Identifier, TELinkValueCurrent, Texture.take());
	if (Result != TEResultSuccess)
	{
		return;
	}

	const FName ParamId(Identifier);
	VariableManager->AllocateLinkedTop(ParamId); // Avoid system querying this param from generating an output error
	ResourceProvider->LinkTexture({ TouchEngineInstance, ParamId, Texture })
		.Next([this, ParamId](const FTouchLinkResult& TouchLinkResult)
		{
			if (TouchLinkResult.ResultType != ELinkResultType::Success)
			{
				return;
			}

			UTexture2D* Texture = TouchLinkResult.ConvertedTextureObject.GetValue();
			if (IsInGameThread())
			{
				VariableManager->UpdateLinkedTOP(ParamId, Texture);
			}
			else
			{
				AsyncTask(ENamedThreads::GameThread, [WeakVariableManger = TWeakPtr<FTouchVariableManager>(VariableManager), ParamId, Texture]()
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

void UTouchEngine::Clear()
{
	TGuardValue<bool> Guard(bIsTearingDown, true);
	
	// FrameCooker depends on VariableManager so we must destroy FrameCooker first
	FrameCooker.Reset();
	VariableManager.Reset();
	// Releases the rendering resources
	ResourceProvider.Reset(); 
	TouchEngineInstance.reset();

	bDidLoad = false;
	bFailedLoad = false;
	ToxPath = "";
	bConfiguredWithTox = false;
	bLoadCalled = false;
}

bool UTouchEngine::OutputResultAndCheckForError(const TEResult Result, const FString& ErrMessage)
{
	if (Result != TEResultSuccess)
	{
		ErrorLog.OutputResult(FString::Printf(TEXT("%s: "), *ErrMessage), Result);
		if (TEResultGetSeverity(Result) == TESeverity::TESeverityError)
		{
			bFailedLoad = true;
			OnLoadFailed.Broadcast(ErrMessage);
			return false;
		}
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
