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
		case TESeverityWarning: AddWarning_AnyThread(Message); break;
		case TESeverityError: AddError_AnyThread(Message); break;
		case TESeverityNone:
		default: ;
		}
	}

	void FTouchErrorLog::AddWarning_AnyThread(const FString& Str)
	{
	#if WITH_EDITOR
		FScopeLock Lock(&MyMessageLock);
		MyWarnings.Add(Str);
	#endif
	}

	void FTouchErrorLog::AddError_AnyThread(const FString& Str)
	{
	#if WITH_EDITOR
		FScopeLock Lock(&MyMessageLock);
		MyErrors.Add(Str);
	#endif
	}

	void FTouchErrorLog::OutputMessages()
	{
	#if WITH_EDITOR
		FScopeLock Lock(&MyMessageLock);
		for (FString& Message : MyErrors)
		{
			OutputError(Message);
		}
		for (FString& Message : MyWarnings)
		{
			OutputWarning(Message);
		}
	#endif
		MyErrors.Empty();
		MyWarnings.Empty();
	}

	void FTouchErrorLog::OutputResult(const FString& ResultString, TEResult Result)
	{
	#if WITH_EDITOR
		FString Message = ResultString + TEResultGetDescription(Result);
		switch (TEResultGetSeverity(Result))
		{
		case TESeverityWarning: OutputError(Message); break;
		case TESeverityError: OutputWarning(Message); break;
		case TESeverityNone:
		default: ;
		}
	#endif
	}

	void FTouchErrorLog::OutputError(const FString& Str)
	{
		UE_LOG(LogTouchEngine, Error, TEXT("TouchEngine error - %s"), *Str);

	#if WITH_EDITOR
		MyMessageLog.Error(FText::Format(LOCTEXT("TEErrorString", "TouchEngine error - {0}"), FText::FromString(Str)));
		if (!MyLogOpened)
		{
			MyMessageLog.Open(EMessageSeverity::Error, false);
			MyLogOpened = true;
		}
		else
		{
			MyMessageLog.Notify(LOCTEXT("TEError", "TouchEngine Error"), EMessageSeverity::Error);
		}
	#endif
	}

	void FTouchErrorLog::OutputWarning(const FString& Str)
	{
	#if WITH_EDITOR
		MyMessageLog.Warning(FText::Format(LOCTEXT("TEWarningString", "TouchEngine warning - {0}"), FText::FromString(Str)));
		if (!MyLogOpened)
		{
			MyMessageLog.Open(EMessageSeverity::Warning, false);
			MyLogOpened = true;
		}
		else
		{
			MyMessageLog.Notify(LOCTEXT("TEWarning", "TouchEngine Warning"), EMessageSeverity::Warning);
		}
	#endif
	}
}

#undef LOCTEXT_NAMESPACE