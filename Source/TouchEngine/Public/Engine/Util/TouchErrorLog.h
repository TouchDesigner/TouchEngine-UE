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
#include "TouchEngine/TEResult.h"
#include "Containers/Queue.h"
#include "Logging/MessageLog.h"

namespace UE::TouchEngine
{
	class TOUCHENGINE_API FTouchErrorLog
	{
	public:
	
		void AddResult(const FString& ResultString, TEResult Result);
		void AddError(const FString& Str);
		void AddWarning(const FString& Str);

		void OutputMessages_GameThread();
	
	private:
		
		FMessageLog MessageLog = FMessageLog(TEXT("TouchEngine"));
		bool bWasLogOpened = false;
		TQueue<FString, EQueueMode::Mpsc> PendingErrors;
		TQueue<FString, EQueueMode::Mpsc> PendingWarnings;

		void OutputResult_GameThread(const FString& Str, TEResult Result);
		void OutputError_GameThread(const FString& Str);
		void OutputWarning_GameThread(const FString& Str);
	};
}

