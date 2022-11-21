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

#include "Logging.h"

#define LOCTEXT_NAMESPACE "UTouchEngine"

namespace UE::TouchEngine
{
	void FTouchErrorLog::AddResult(const FString& ResultString, TEResult Result)
	{
		FString Message = ResultString + TEResultGetDescription(Result);
		switch (TEResultGetSeverity(Result))
		{
		case TESeverityWarning: AddWarning(Message); break;
		case TESeverityError: AddError(Message); break;
		case TESeverityNone:  UE_LOG(LogTouchEngine, Log, TEXT("TouchEngine Result - %s"), *ResultString); break;
		default: ;
		}
	}

	void FTouchErrorLog::AddWarning(const FString& Str)
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("TouchEngine Warning - %s"), *Str);
		if (IsInGameThread())
		{
			OutputWarning_GameThread(Str);
		}
		else
		{
#if WITH_EDITOR
			PendingWarnings.Enqueue(Str);
#endif
		}
	}

	void FTouchErrorLog::AddError(const FString& Str)
	{
		UE_LOG(LogTouchEngine, Error, TEXT("TouchEngine error - %s"), *Str);
		if (IsInGameThread())
		{
			OutputError_GameThread(Str);
		}
		else
		{
#if WITH_EDITOR
			PendingErrors.Enqueue(Str);
#endif
		}
	}

	void FTouchErrorLog::OutputMessages_GameThread()
	{
	#if WITH_EDITOR
		FString Message;
		while (PendingErrors.Dequeue(Message))
		{
			OutputError_GameThread(Message);
		}
		while (PendingWarnings.Dequeue(Message))
		{
			OutputWarning_GameThread(Message);
		}
	#endif
	}

	void FTouchErrorLog::OutputResult_GameThread(const FString& ResultString, TEResult Result)
	{
		check(IsInGameThread());
	#if WITH_EDITOR
		const FString Message = ResultString + TEResultGetDescription(Result);
		switch (TEResultGetSeverity(Result))
		{
		case TESeverityWarning: OutputError_GameThread(Message); break;
		case TESeverityError: OutputWarning_GameThread(Message); break;
		case TESeverityNone:
		default: ;
		}
	#endif
	}

	void FTouchErrorLog::OutputError_GameThread(const FString& Str)
	{
		check(IsInGameThread());

	#if WITH_EDITOR
		MessageLog.Error(FText::Format(LOCTEXT("TEErrorString", "TouchEngine error - {0}"), FText::FromString(Str)));
		if (!bWasLogOpened)
		{
			MessageLog.Open(EMessageSeverity::Error, false);
			bWasLogOpened = true;
		}
		else
		{
			MessageLog.Notify(LOCTEXT("TEError", "TouchEngine Error"), EMessageSeverity::Error);
		}
	#endif
	}

	void FTouchErrorLog::OutputWarning_GameThread(const FString& Str)
	{
		check(IsInGameThread());
	#if WITH_EDITOR
		MessageLog.Warning(FText::Format(LOCTEXT("TEWarningString", "TouchEngine warning - {0}"), FText::FromString(Str)));
		if (!bWasLogOpened)
		{
			MessageLog.Open(EMessageSeverity::Warning, false);
			bWasLogOpened = true;
		}
		else
		{
			MessageLog.Notify(LOCTEXT("TEWarning", "TouchEngine Warning"), EMessageSeverity::Warning);
		}
	#endif
	}
}

#undef LOCTEXT_NAMESPACE