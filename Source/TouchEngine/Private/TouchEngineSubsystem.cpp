// Fill out your copyright notice in the Description page of Project Settings.

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

	// tox was never loaded (should never hit this)
	return false;
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
	params->engineInfo->load(toxPath);
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

void UFileParams::ParamsLoaded(TArray<FTouchDynamicVariable> new_inputs, TArray<FTouchDynamicVariable> new_outputs)
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
