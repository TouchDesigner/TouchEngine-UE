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
#include "ToxDelegateInfo.generated.h"

struct FTouchEngineDynamicVariableStruct;

DECLARE_MULTICAST_DELEGATE_TwoParams(FTouchOnParametersLoaded, const TArray<FTouchEngineDynamicVariableStruct>&, const TArray<FTouchEngineDynamicVariableStruct>&);
DECLARE_MULTICAST_DELEGATE_OneParam(FTouchOnFailedLoad, const FString&);

USTRUCT()
struct FToxDelegateInfo
{
	GENERATED_BODY()

	FToxDelegateInfo() = default;
	FToxDelegateInfo(
		UObject* Owner,
		const FTouchOnParametersLoaded::FDelegate& ParamsLoadedDelegate,
		const FTouchOnFailedLoad::FDelegate& FailedLoadDelegate,
		FDelegateHandle ParamsLoadedDelegateHandle,
		FDelegateHandle LoadFailedDelegateHandle)
		: Owner(Owner)
		, ParamsLoadedDelegate(ParamsLoadedDelegate)
		, FailedLoadDelegate(FailedLoadDelegate)
		, ParamsLoadedDelegateHandle(ParamsLoadedDelegateHandle)
		, LoadFailedDelegateHandle(LoadFailedDelegateHandle)
	{}

	UPROPERTY()
	TObjectPtr<UObject> Owner = nullptr;
	FTouchOnParametersLoaded::FDelegate ParamsLoadedDelegate;
	FTouchOnFailedLoad::FDelegate FailedLoadDelegate;
	FDelegateHandle ParamsLoadedDelegateHandle;
	FDelegateHandle LoadFailedDelegateHandle;
};