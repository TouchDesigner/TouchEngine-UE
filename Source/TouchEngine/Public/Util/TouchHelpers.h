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
#include "Async/Async.h"
#include "UObject/Object.h"

namespace UE::TouchEngine
{
	inline FString GetCurrentThreadStr()
	{
		const uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
		if (IsInGameThread())
		{
			return FString::Printf(TEXT("GameThread(%d)"), CurrentThreadId);
		}
		else if (IsInRenderingThread())
		{
			return FString::Printf(TEXT("RenderThread(%d)"), CurrentThreadId);
		}
		else if (IsInParallelGameThread())
		{
			return FString::Printf(TEXT("ParallelGameThread(%d)"), CurrentThreadId);
		}
		FString Str;
		static TArray<ETaskTag> AllFlags {ETaskTag::EStaticInit, ETaskTag::EGameThread, ETaskTag::ESlateThread, ETaskTag::ERenderingThread,
			ETaskTag::ERhiThread, ETaskTag::EAsyncLoadingThread, ETaskTag::EWorkerThread, ETaskTag::EParallelThread};
		static TArray<FString> AllFlagsStr {FString(TEXT("EStaticInit")), FString(TEXT("EGameThread")), FString(TEXT("ESlateThread")), FString(TEXT("ERenderingThread")),
			FString(TEXT("ERhiThread")), FString(TEXT("EAsyncLoadingThread")), FString(TEXT("EWorkerThread")), FString(TEXT("EParallelThread"))};
		for (int i = 0; i < AllFlags.Num(); ++i)
		{
			if (EnumHasAnyFlags(FTaskTagScope::GetCurrentTag(), AllFlags[i]))
			{
				if (!Str.IsEmpty())
				{
					Str += TEXT(" | ");
				}
				Str += AllFlagsStr[i];
			}
		}

		return FString::Printf(TEXT("%s(%d)"), *(Str.IsEmpty() ? FString(TEXT("ENone")) : Str),CurrentThreadId);
		
		// switch (ETaskTag TaskTag = FTaskTagScope::GetCurrentTag()) {
		// 	case ETaskTag::ENone: return FString(TEXT("ENone"));
		// 	case ETaskTag::EStaticInit: return FString(TEXT("EStaticInit"));
		// 	case ETaskTag::EGameThread: return FString(TEXT("EGameThread"));
		// 	case ETaskTag::ESlateThread: return FString(TEXT("ESlateThread"));
		// 	// case ETaskTag::EAudioThread: return FString(TEXT("EAudioThread"));
		// 	case ETaskTag::ERenderingThread: return FString(TEXT("ERenderingThread"));
		// 	case ETaskTag::ERhiThread: return FString(TEXT("ERhiThread"));
		// 	case ETaskTag::EAsyncLoadingThread: return FString(TEXT("EAsyncLoadingThread"));
		// 	case ETaskTag::ENamedThreadBits: return FString(TEXT("ENamedThreadBits"));
		// 	case ETaskTag::EParallelThread: return FString(TEXT("EParallelThread"));
		// 	case ETaskTag::EWorkerThread: return FString(TEXT("EWorkerThread"));
		// 	case ETaskTag::EParallelRenderingThread: return FString(TEXT("EParallelRenderingThread"));
		// 	case ETaskTag::EParallelGameThread: return FString(TEXT("EParallelGameThread"));
		// 	case ETaskTag::EParallelRhiThread: return FString(TEXT("EParallelRhiThread"));
		// 	default: return FString(TEXT("UnknownThread"));
		// };
		
	};

	static int32 GetObjReferenceCount(UObject* Obj, TArray<UObject*>* OutReferredToObjects = nullptr) //from https://unrealcommunity.wiki/garbage-collection-~-count-references-to-any-object-2afrgp8l
	{
		if(!Obj || !Obj->IsValidLowLevelFast())
		{
			return -1;
		}
    
		TArray<UObject*> ReferredToObjects;             //req outer, ignore archetype, recursive, ignore transient
		FReferenceFinder ObjectReferenceCollector(ReferredToObjects, Obj, false, true, true, false);
		ObjectReferenceCollector.FindReferences(Obj);

		if(OutReferredToObjects)
		{
			OutReferredToObjects->Append(ReferredToObjects);
		}
		return ReferredToObjects.Num();
	}

	//todo: these functions are not reliable, there should be a better way to assess if a variable is an input/output/parameter
	inline bool IsInputVariable(const FString& VarName) { return VarName.StartsWith("i/"); }
	inline bool IsOutputVariable(const FString& VarName) { return VarName.StartsWith("o/"); }
	inline bool IsParameterVariable(const FString& VarName) { return VarName.StartsWith("p/"); }
}

/** Calls the given lambda on GameThread. If we are already on GameThread, calls it directly, otherwise calls AsyncTask(ENamedThreads::GameThread) */
template<typename T>
inline TFuture<T> ExecuteOnGameThread(TUniqueFunction<T()> Func)
{
	if (IsInGameThread())
	{
		return MakeFulfilledPromise<T>(Func()).GetFuture();
	}

	TPromise<T> Promise;
	TFuture<T> Result = Promise.GetFuture();
	AsyncTask(ENamedThreads::GameThread, [Func = MoveTemp(Func), Promise = MoveTemp(Promise)]() mutable
	{
		Promise.SetValue(Func());
	});
	return Result;
}

/** Calls the given lambda on GameThread. If we are already on GameThread, calls it directly, otherwise calls AsyncTask(ENamedThreads::GameThread) */
template<>
inline TFuture<void> ExecuteOnGameThread<void>(TUniqueFunction<void()> Func)
{
	if (IsInGameThread())
	{
		Func();
		TPromise<void> Promise;
		Promise.EmplaceValue();
		return Promise.GetFuture();
	}

	TPromise<void> Promise;
	TFuture<void> Result = Promise.GetFuture();
	AsyncTask(ENamedThreads::GameThread, [Func = MoveTemp(Func), Promise = MoveTemp(Promise)]() mutable
	{
		Func();
		Promise.SetValue();
	});
	return Result;
}
