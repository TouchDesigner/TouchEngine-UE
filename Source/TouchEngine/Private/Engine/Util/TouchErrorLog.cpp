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

#include "Engine/Util/TouchErrorLog.h"
#include "Blueprint/TouchEngineComponent.h"

#include "Logging.h"
#include "TouchEngineModule.h"
#include "ToxAsset.h"
#include "Engine/TEDebug.h"
#include "Misc/UObjectToken.h"
#include "GameFramework/Actor.h"
#include "UObject/WeakObjectPtrTemplates.h"

#define LOCTEXT_NAMESPACE "UTouchEngine"

namespace UE::TouchEngine
{
	FTouchErrorLog::FTouchErrorLog(const TWeakObjectPtr<UTouchEngineComponentBase> InComponent)
		: Component(InComponent)
	{
	}

	void FTouchErrorLog::AddResult(const FString& ResultString, TEResult Result, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription)
	{
		const FString Message = ResultString + " " + TEResultGetDescription(Result);
		switch (TEResultGetSeverity(Result))
		{
		case TESeverityWarning: AddLog({EMessageSeverity::Warning, EErrorType::None, Result, FunctionName, VarName, Message, AdditionalDescription});	break;
		case TESeverityError: AddLog({EMessageSeverity::Error, EErrorType::None, Result, FunctionName, VarName, Message, AdditionalDescription}); break;
		case TESeverityNone:  UE_LOG(LogTouchEngine, Display, TEXT("TouchEngine Result: %s %s for '%s'"), *ResultString, *AdditionalDescription, *VarName); break;
		default: ;
		}
	}
	
	void FTouchErrorLog::AddWarning(const FString& Message, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription)
	{
		// Successful result as default because it doesn't matter
		AddLog({EMessageSeverity::Warning, EErrorType::None, TEResultSuccess, FunctionName, VarName, Message, AdditionalDescription});
	}
	
	void FTouchErrorLog::AddError(const FString& Message, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription)
	{
		// Successful result as default because it doesn't matter
		AddLog({EMessageSeverity::Error, EErrorType::None, TEResultSuccess, FunctionName, VarName, Message, AdditionalDescription});
	}

	void FTouchErrorLog::AddResult(EErrorType ErrorCode, TEResult Result, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription)
	{
		switch (TEResultGetSeverity(Result))
        {
		case TESeverityWarning: AddLog({EMessageSeverity::Warning, ErrorCode, Result, FunctionName, VarName, GetErrorCodeDescription(ErrorCode), AdditionalDescription});	break;
		case TESeverityError: AddLog({EMessageSeverity::Error, ErrorCode, Result, FunctionName, VarName, GetErrorCodeDescription(ErrorCode), AdditionalDescription}); break;
		case TESeverityNone:  UE_LOG(LogTouchEngine, Display, TEXT("TouchEngine Result: %s for '%s' in function `%s`"), *GetErrorCodeDescription(ErrorCode, Result), *VarName, *FunctionName.ToString()); break;
        default: break;
        }
	}

	void FTouchErrorLog::AddWarning(EErrorType ErrorCode, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription)
	{
		// Successful result as default because it doesn't matter
		AddLog({EMessageSeverity::Warning, ErrorCode, TEResultSuccess, FunctionName, VarName, GetErrorCodeDescription(ErrorCode), AdditionalDescription});
	}

	void FTouchErrorLog::AddError(EErrorType ErrorCode, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription)
	{
		AddLog({EMessageSeverity::Error, ErrorCode, TEResultSuccess, FunctionName, VarName, GetErrorCodeDescription(ErrorCode), AdditionalDescription});
	}

	void FTouchErrorLog::AddCountMismatchWarning(const TouchObject<TELinkInfo>& Link, int ExpectedCount, const FString& VarName, const FName& FunctionName)
	{
		AddWarning(EErrorType::VariableCountMismatch, VarName, FunctionName,
			FString::Printf(TEXT("The variable is expecting data of length '%d' but data passed is of length '%d'"),
				Link->count, ExpectedCount));
	}

	void FTouchErrorLog::AddTypeMismatchError(const TouchObject<TELinkInfo>& Link, TELinkType ExpectedType , const FString& VarName, const FName& FunctionName)
	{
		AddError(EErrorType::VariableTypeMismatch, VarName, FunctionName,
			FString::Printf(TEXT("The variable is of type '%s' but the expected type for the function is '%s'"),
				*TELinkTypeToString(Link->type), *TELinkTypeToString(ExpectedType)));
	}

	void FTouchErrorLog::AddCountMismatchError(const TouchObject<TELinkInfo>& Link, int ExpectedCount, const FString& VarName, const FName& FunctionName)
	{
		AddError(EErrorType::VariableCountMismatch, VarName, FunctionName,
			FString::Printf(TEXT("The variable is expecting data of length '%d' but data passed is of length '%d'"),
				Link->count, ExpectedCount));
	}

	void FTouchErrorLog::AddScopeMismatchError(const TouchObject<TELinkInfo>& Link, TEScope ExpectedScope, const FString& VarName, const FName& FunctionName)
	{
		AddError(EErrorType::VariableScopeMismatch, VarName, FunctionName,
			FString::Printf(TEXT("The variable is an '%s' but the function called is for '%s'"),
				*TEScopeToString(Link->scope), *TEScopeToString(ExpectedScope)));
	}

	FString FTouchErrorLog::GetErrorCodeDescription(EErrorType ErrorCode, TEResult Result)
	{
		FString Message;
		switch (ErrorCode)
		{
		case EErrorType::VariableNameNotFound: Message = TEXT("The given variable was not found in the tox file, the spelling might be wrong."); break;
		case EErrorType::TEInstanceLinkGetInfoError: Message = TEXT("Retrieving the variable information from TouchEngine was not successful, the variable might not exist."); break;
		case EErrorType::TEInstanceLinkGetValueError: Message = TEXT("Getting the variable value from TouchEngine was not successful."); break;
		case EErrorType::TEInstanceLinkSetValueError: Message = TEXT("Setting the variable value to TouchEngine was not successful."); break;
		case EErrorType::VariableTypeMismatch: Message = TEXT("Data passed to the variable is not of the right type."); break;
		case EErrorType::VariableCountMismatch: Message = TEXT("The variable is expecting a different Count."); break;
		case EErrorType::VariableScopeMismatch: Message = TEXT("The variable is not of the right Scope."); break;
		case EErrorType::TELoadToxTimeout: Message = TEXT("The loading of the Tox file in TouchEngine timed-out. You can try increasing the ToxLoadTimeout in the TouchEngine Component."); break;
		case EErrorType::TECookTimeout: Message = TEXT("The TouchEngine Cook timed-out. You can try increasing the CookTimeout in the TouchEngine Component."); break;
		default: Message = TEXT("UNKWOWN ERROR");
		}

		if (Result != TEResultSuccess)
		{
			Message = Message + " " + TEResultGetDescription(Result);
		}
		
		return Message;
	}

	FMessageLog FTouchErrorLog::CreateMessageLog()
	{
		return FMessageLog(FTouchEngineModule::MessageLogName);
	}

	void FTouchErrorLog::AddLog(const FLogData& LogData)
	{
		if (TriggeredErrors.Contains(LogData))
		{
			return;
		}
		TriggeredErrors.Add(LogData);

		// We do not want this to run right away as it might be called from one of the promise
		// and this call flushes the RenderThread which can create deadlocks
		AsyncTask(ENamedThreads::GameThread, [LogData, WeakThis = AsWeak()]()
		{
			if (const TSharedPtr<FTouchErrorLog> This = WeakThis.Pin())
			{
				This->OutputLogData_GameThread(LogData);
			}
		});
	}

	void FTouchErrorLog::OutputLogData_GameThread(const FLogData& LogData)
	{
		check(IsInGameThread());

		FText SeverityStr;
		switch (LogData.Severity) {
		case EMessageSeverity::Error:
			SeverityStr = FText::FromString(" Error");
			break;
		case EMessageSeverity::PerformanceWarning:
			SeverityStr = FText::FromString(" PerformanceWarning");
			break;
		case EMessageSeverity::Warning:
			SeverityStr = FText::FromString(" Warning");
			break;
		case EMessageSeverity::Info:
			SeverityStr = FText::FromString(" Info");
			break;
		default:
			break;
		}

#if WITH_EDITOR
		
		
		FMessageLog MessageLog = CreateMessageLog();
		if (!bWasLogOpened) // Create a new page for the component the first time only
		{
			FText PageName;
			if (Component.IsValid() && IsValid(Component->GetOwner()))
			{
				PageName = FText::FromString(Component->GetOwner()->GetActorLabel());
			}
			MessageLog.SetCurrentPage(PageName);
		}
		
		const TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(LogData.Severity);
		if (Component.IsValid() && IsValid(Component->GetOwner()))
		{
			Message->AddToken(FActorToken::Create(Component->GetOwner()->GetPathName(), Component->GetOwner()->GetActorGuid(), FText::FromString(Component->GetOwner()->GetActorLabel())));
		}
		Message->AddToken(FTextToken::Create(FText::Format(LOCTEXT("TEMessageStringBase", " {1}"), SeverityStr, FText::FromString(LogData.Message))));
		if (!LogData.AdditionalDescription.IsEmpty())
		{
			Message->AddToken(FTextToken::Create(FText::Format(INVTEXT(" {0}"), FText::FromString(LogData.AdditionalDescription))));
		}
		if (!LogData.VarName.IsEmpty()) //todo: there should be a better way to determine if the variable was supposed to be an input/output/parameter
		{
			FText VariableTypeStr;
			FString CleanVarName = LogData.VarName;
			if (UE::TouchEngine::IsInputVariable(LogData.VarName))
			{
				VariableTypeStr = INVTEXT("Input");
				CleanVarName.RemoveFromStart("i/");
			}
			else if (UE::TouchEngine::IsOutputVariable(LogData.VarName))
			{
				VariableTypeStr = INVTEXT("Output");
				CleanVarName.RemoveFromStart("o/");
			}
			else if (UE::TouchEngine::IsParameterVariable(LogData.VarName))
			{
				VariableTypeStr = INVTEXT("Parameter");
				CleanVarName.RemoveFromStart("p/");
			}
			else
			{
				VariableTypeStr = INVTEXT("Variable");
			}

			Message->AddToken(FTextToken::Create(FText::Format(LOCTEXT("TEMessageStringVar", " [{0} '{1}']"), VariableTypeStr, FText::FromString(CleanVarName))));
		}
		if (Component.IsValid() && IsValid(Component->ToxAsset))
		{
			Message->AddToken(FTextToken::Create(LOCTEXT("TEMessageStringTox", " (Tox file: ")));
			Message->AddToken(FUObjectToken::Create(Component->ToxAsset, FText::FromString(Component->ToxAsset->GetRelativeFilePath())));
			Message->AddToken(FTextToken::Create(INVTEXT(")")));
		}
		MessageLog.AddMessage(Message);
		
		if (LogData.Severity == EMessageSeverity::Error && !bWasLogOpened)
		{
			MessageLog.Open();
			bWasLogOpened = true;
		}
		else
		{
			MessageLog.Notify(FText::Format(LOCTEXT("TENotify", "TouchEngine{0}"), SeverityStr), EMessageSeverity::Info);
		}
#else
		FString Str;

		if (Component.IsValid() && IsValid(Component->GetOwner()))
		{
			Str += GetNameSafe(Component->GetOwner()) + TEXT(" ");
		}
		Str += FText::Format(LOCTEXT("TEMessageStringBase", " {0}: {1}"), SeverityStr, FText::FromString(LogData.Message)).ToString();
		if (!LogData.AdditionalDescription.IsEmpty())
		{
			Str += FText::Format(INVTEXT(" {0}"), FText::FromString(LogData.AdditionalDescription)).ToString();
		}
		if (!LogData.VarName.IsEmpty()) //todo: there should be a better way to determine if the variable was supposed to be an input/output/parameter
		{
			FText VariableTypeStr;
			FString CleanVarName = LogData.VarName;
			if (UE::TouchEngine::IsInputVariable(LogData.VarName))
			{
				VariableTypeStr = INVTEXT("Input");
				CleanVarName.RemoveFromStart("i/");
			}
			else if (UE::TouchEngine::IsOutputVariable(LogData.VarName))
			{
				VariableTypeStr = INVTEXT("Output");
				CleanVarName.RemoveFromStart("o/");
			}
			else if (UE::TouchEngine::IsParameterVariable(LogData.VarName))
			{
				VariableTypeStr = INVTEXT("Parameter");
				CleanVarName.RemoveFromStart("p/");
			}
			else
			{
				VariableTypeStr = INVTEXT("Variable");
			}

			Str += FText::Format(LOCTEXT("TEMessageStringVar", " [{0} '{1}']"), VariableTypeStr, FText::FromString(CleanVarName)).ToString();
		}
		if (Component.IsValid() && IsValid(Component->ToxAsset))
		{
			Str += LOCTEXT("TEMessageStringTox", " (Tox file: ").ToString() + Component->ToxAsset->GetRelativeFilePath() + TEXT(")");
		}

		UE_LOG(LogTouchEngine, Error, TEXT("TouchEngine Error: %s"), *Str);
#endif
	}
}

#undef LOCTEXT_NAMESPACE
