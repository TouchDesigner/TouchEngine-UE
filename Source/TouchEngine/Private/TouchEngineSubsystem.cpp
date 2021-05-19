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

#include "TouchEngineSubsystem.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "Interfaces/IPluginManager.h"
#include "TouchEngineInfo.h"

void
UTouchEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	FString dllPath = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("TouchEngine"))->GetBaseDir(), TEXT("/Binaries/ThirdParty/Win64"));
	FPlatformProcess::PushDllDirectory(*dllPath);
	FString dll = FPaths::Combine(dllPath, TEXT("libTDP.dll"));
	if (!FPaths::FileExists(dll))
	{
		//		UE_LOG(LogAjaMedia, Error, TEXT("Failed to find the binary folder for the AJA dll. Plug-in will not be functional."));
		//		return false;
	}
	myLibHandle = FPlatformProcess::GetDllHandle(*dll);

	FPlatformProcess::PopDllDirectory(*dllPath);

	if (!myLibHandle)
	{

	}
}

void
UTouchEngineSubsystem::Deinitialize()
{
	if (myLibHandle)
	{
		FPlatformProcess::FreeDllHandle(myLibHandle);
		myLibHandle = nullptr;
	}
}

void 
UTouchEngineSubsystem::GetParamsFromTox(FString toxPath, FTouchOnParametersLoaded::FDelegate paramsLoadedDel, FSimpleDelegate loadFailedDel,
														 FDelegateHandle& paramsLoadedDelHandle, FDelegateHandle& loadFailedDelHandle)
{
	if (loadedParams.Contains(toxPath))
	{
		// tox file has at least started loading

		UFileParams* params = loadedParams[toxPath];

		if (params->isLoaded)
		{
			// tox file has already been loaded
			params->BindOrCallDelegates(paramsLoadedDel, loadFailedDel, paramsLoadedDelHandle, loadFailedDelHandle);
		}
		else
		{
			if (params->failedLoad)
			{
				// tox file has failed to load
				// attempt to reload
				LoadTox(toxPath, paramsLoadedDel, loadFailedDel, paramsLoadedDelHandle, loadFailedDelHandle);
			}
			else
			{
				// tox file is still loading
				params->BindOrCallDelegates(paramsLoadedDel, loadFailedDel, paramsLoadedDelHandle, loadFailedDelHandle);
			}
		}
	}
	else
	{
		// tox file has not started loading yet
		LoadTox(toxPath, paramsLoadedDel, loadFailedDel, paramsLoadedDelHandle, loadFailedDelHandle);
	}
}

void 
UTouchEngineSubsystem::UnbindDelegates(FString toxPath, FDelegateHandle paramsLoadedDelHandle, FDelegateHandle loadFailedDelHandle)
{
	if (loadedParams.Contains(toxPath))
	{
		UFileParams* params = loadedParams[toxPath];
		params->OnParamsLoaded.Remove(paramsLoadedDelHandle);
		params->OnFailedLoad.Remove(loadFailedDelHandle);
	}
}

bool
UTouchEngineSubsystem::UnbindDelegates(FDelegateHandle paramsLoadedDelHandle, FDelegateHandle loadFailedDelHandle)
{
	for (const TPair<FString, UFileParams*>& pair : loadedParams)
	{
		UFileParams* params = pair.Value;
		if (params->OnParamsLoaded.Remove(paramsLoadedDelHandle))
		{
			return params->OnFailedLoad.Remove(loadFailedDelHandle);
		}
	}

	return false;
}

bool 
UTouchEngineSubsystem::IsLoaded(FString toxPath)
{
	if (loadedParams.Contains(toxPath))
	{
		return loadedParams[toxPath]->isLoaded;
	}

	return false;
}

bool 
UTouchEngineSubsystem::HasFailedLoad(FString toxPath)
{
	if (loadedParams.Contains(toxPath))
	{
		return loadedParams[toxPath]->failedLoad;
	}

	return false;
}

bool UTouchEngineSubsystem::ReloadTox(FString toxPath, FTouchOnParametersLoaded::FDelegate paramsLoadedDel, FSimpleDelegate loadFailedDel,
													   FDelegateHandle& paramsLoadedDelHandle, FDelegateHandle& loadFailedDelHandle)
{
	UFileParams* params;

	if (loadedParams.Contains(toxPath))
	{
		params = loadedParams[toxPath];

		if (!params->isLoaded && !params->failedLoad)
		{
			// tox file is still loading, do nothing
			return false;
		}

		// reset currently stored data
		params->ResetEngine();
		params->engineInfo = NewObject<UTouchEngineInfo>();
		// attach delegates
		params->engineInfo->getOnParametersLoadedDelegate()->AddUFunction(params, "ParamsLoaded");
		params->engineInfo->getOnLoadFailedDelegate()->AddUFunction(params, "FailedLoad");
		params->BindOrCallDelegates(paramsLoadedDel, loadFailedDel, paramsLoadedDelHandle, loadFailedDelHandle);
		// reload tox
		params->engineInfo->load(toxPath);
		return true;
	}

	// tox was never loaded (can hit this if path is empty or invalid)
	return LoadTox(toxPath, paramsLoadedDel, loadFailedDel, paramsLoadedDelHandle, loadFailedDelHandle) != nullptr;
}


UFileParams* 
UTouchEngineSubsystem::LoadTox(FString toxPath, FTouchOnParametersLoaded::FDelegate paramsLoadedDel, FSimpleDelegate loadFailedDel,
												FDelegateHandle& paramsLoadedDelHandle, FDelegateHandle& loadFailedDelHandle)
{
	UFileParams* params;

	if (!loadedParams.Contains(toxPath))
	{
		// loading for the first time
		params = loadedParams.Add(toxPath, NewObject<UFileParams>());
	}
	else
	{
		// reloading
		params = loadedParams[toxPath];
		params->failedLoad = false;
		params->isLoaded = false;
	}

	// create engine info
	params->engineInfo = NewObject<UTouchEngineInfo>();
	// bind delegates
	params->engineInfo->getOnParametersLoadedDelegate()->AddUFunction(params, "ParamsLoaded");
	params->engineInfo->getOnLoadFailedDelegate()->AddUFunction(params, "FailedLoad");
	params->BindOrCallDelegates(paramsLoadedDel, loadFailedDel, paramsLoadedDelHandle, loadFailedDelHandle);
	// load tox
	if (params->engineInfo->load(toxPath))
	{
		// failed load immediately due to probably file path error
		loadedParams.Remove(toxPath);
		return nullptr;
	}
	return params;
}

void UFileParams::BindOrCallDelegates(FTouchOnParametersLoaded::FDelegate paramsLoadedDel, FSimpleDelegate failedLoadDel,
									  FDelegateHandle& paramsLoadedDelHandle, FDelegateHandle& loadFailedDelHandle)
{
	paramsLoadedDelHandle = OnParamsLoaded.Add(paramsLoadedDel);
	loadFailedDelHandle = OnFailedLoad.Add(failedLoadDel);

	if (isLoaded)
		paramsLoadedDel.Execute(Inputs, Outputs);
	if (failedLoad)
		failedLoadDel.Execute();
}

void UFileParams::ParamsLoaded(TArray<FTEDynamicVariableStruct> new_inputs, TArray<FTEDynamicVariableStruct> new_outputs)
{
	// set dynamic variable arrays
	Inputs = new_inputs;
	Outputs = new_outputs;
	// kill the touch engine instance
	if (engineInfo)
	{
		engineInfo->destroy();
		engineInfo = nullptr;
	}
	// set variables
	isLoaded = true;
	failedLoad = false;
	// call delegate for parameters being loaded
	if (OnParamsLoaded.IsBound())
		OnParamsLoaded.Broadcast(Inputs, Outputs);
}

void UFileParams::FailedLoad()
{
	isLoaded = false;
	failedLoad = true;

	if (engineInfo)
	{
		engineInfo->destroy();
		engineInfo = nullptr;
	}

	OnFailedLoad.Broadcast();
}

void UFileParams::ResetEngine()
{
	isLoaded = false;
	failedLoad = false;
	Inputs.Empty();
	Outputs.Empty();
	if (engineInfo)
	{
		engineInfo->destroy();
	}
	engineInfo = nullptr;
}
