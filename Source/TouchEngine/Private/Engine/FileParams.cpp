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

#include "Engine/FileParams.h"

#include "Engine/Engine.h"
#include "Engine/TouchEngineInfo.h"
#include "Engine/TouchEngineSubsystem.h"

void UFileParams::BindOrCallDelegates(UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate FailedLoadDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle)
{
	if (ParamsLoadedDelegate.IsBoundToObject(Owner) || FailedLoadDelegate.IsBoundToObject(Owner))
	{
		return;
	}

	ParamsLoadedDelHandle = ParamsLoadedDelegate.Add(ParamsLoadedDel);
	LoadFailedDelHandle = FailedLoadDelegate.Add(FailedLoadDel);

	if (bIsLoaded)
	{
		ParamsLoadedDel.Execute(Inputs, Outputs);
	}

	if (bHasFailedLoad)
	{
		FailedLoadDel.Execute(ErrorString);
	}
}

void UFileParams::OnParamsLoaded(const TArray<FTouchEngineDynamicVariableStruct>& InInputs, const TArray<FTouchEngineDynamicVariableStruct>& InOutputs)
{
	// set dynamic variable arrays
	Inputs = InInputs;
	Outputs = InOutputs;
	// set variables
	bIsLoaded = true;
	bHasFailedLoad = false;
	// call delegate for parameters being loaded
	if (ParamsLoadedDelegate.IsBound())
	{
		ParamsLoadedDelegate.Broadcast(Inputs, Outputs);
	}
	
	// load next
	UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
	TESubsystem->LoadNext();
}

void UFileParams::OnFailedLoad(const FString& Error)
{
	bIsLoaded = false;
	bHasFailedLoad = true;

	FailedLoadDelegate.Broadcast(Error);
	ErrorString = Error;

	UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
	TESubsystem->LoadNext();
}

void UFileParams::ResetEngine()
{
	bIsLoaded = false;
	bHasFailedLoad = false;
	Inputs.Empty();
	Outputs.Empty();
}