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

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "Logging/MessageLog.h"
#include "TouchEngine/TEResult.h"
#include "TouchEngine/TouchObject.h"

struct TELinkInfo;
class UTouchEngineComponentBase;

namespace UE::TouchEngine
{
	class TOUCHENGINE_API FTouchErrorLog
	{
	public:
		FTouchErrorLog(TWeakObjectPtr<UTouchEngineComponentBase> InComponent);
		
		void AddResult(const FString& ResultString, TEResult Result, const FString& VarName = FString(), const FName& FunctionName = FName(), const FString& AdditionalDescription = FString());
		void AddWarning(const FString& Message, const FString& VarName = FString(), const FName& FunctionName = FName(), const FString& AdditionalDescription = FString());
		void AddError(const FString& Message, const FString& VarName = FString(), const FName& FunctionName = FName(), const FString& AdditionalDescription = FString());

		enum class EErrorType
		{
			VariableNameNotFound,
			TEInstanceLinkGetInfoError,
			TEInstanceLinkGetValueError,
			TEInstanceLinkSetValueError,
			VariableTypeMismatch,
			VariableScopeMismatch,
			VariableCountMismatch,
		};

		void AddResult(EErrorType ErrorCode, TEResult Result, const FString& VarName = FString(), const FName& FunctionName = FName(), const FString& AdditionalDescription = FString());
		void AddWarning(EErrorType ErrorCode, const FString& VarName = FString(), const FName& FunctionName = FName(), const FString& AdditionalDescription = FString());
		void AddError(EErrorType ErrorCode, const FString& VarName = FString(), const FName& FunctionName = FName(), const FString& AdditionalDescription = FString());

		void AddCountMismatchWarning(const TouchObject<TELinkInfo>& Link, int ExpectedCount, const FString& VarName, const FName& FunctionName);

		void AddTypeMismatchError(const TouchObject<TELinkInfo>& Link, TELinkType ExpectedType, const FString& VarName, const FName& FunctionName);
		void AddCountMismatchError(const TouchObject<TELinkInfo>& Link, int ExpectedCount, const FString& VarName, const FName& FunctionName);
		void AddScopeMismatchError(const TouchObject<TELinkInfo>& Link, TEScope ExpectedScope, const FString& VarName, const FName& FunctionName);
		
		void OutputMessages_GameThread();

		static FString GetErrorCodeDescription(EErrorType ErrorCode, TEResult Result = TEResultSuccess);

		/** Parameters that make an Error unique */
		struct FTriggeredErrorData
		{
			EMessageSeverity::Type Severity;
			FString VarName;
			FName FunctionName;
			TEResult Result;
			bool operator==(const FTriggeredErrorData& Other) const
			{
				return Severity == Other.Severity && Result == Other.Result && FunctionName == Other.FunctionName && VarName == Other.VarName;
			}
		};

	private:
		TWeakObjectPtr<class UTouchEngineComponentBase> Component;

		static FMessageLog CreateMessageLog();
		bool bWasLogOpened = false;

		struct FLogData
		{
			EMessageSeverity::Type Severity;
			FString Message;
			FString VarName;
			FName FunctionName;
			FString AdditionalDescription;
		};
		TQueue<FLogData, EQueueMode::Mpsc> PendingMessages;

		/** List of errors already triggered to not trigger them again */
		TSet<FTriggeredErrorData> TriggeredErrors;

		void AddLog(const FLogData& LogData);
		void OutputLogData_GameThread(const FLogData& LogData);
	};
}

FORCEINLINE uint32 GetTypeHash(const UE::TouchEngine::FTouchErrorLog::FTriggeredErrorData& ErrorData)
{
	const uint32 Hash = FCrc::MemCrc32(&ErrorData, sizeof(UE::TouchEngine::FTouchErrorLog::FTriggeredErrorData));
	return Hash;
}
