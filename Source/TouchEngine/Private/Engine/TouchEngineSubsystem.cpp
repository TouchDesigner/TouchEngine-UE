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

#include "Engine/TouchEngineSubsystem.h"

#include "Logging.h"
#include "Engine/TouchEngineInfo.h"
#include "Engine/FileParams.h"
#include "Engine/TouchEngine.h"

void UTouchEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	TempEngineInfo = NewObject<UTouchEngineInfo>();
}

void UTouchEngineSubsystem::GetOrLoadParamsFromTox(FString ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle)
{
	if (const TObjectPtr<UFileParams>* PotentialParams = LoadedParams.Find(ToxPath))
	{
		UFileParams* Params = *PotentialParams;
		if (Params->bHasFailedLoad)
		{
			// Attempt to reload
			LoadTox(ToxPath, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
		}
		else // Tox file is either loading or has already loaded
		{
			Params->BindOrCallDelegates(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
		}
	}
	else
	{
		// tox file has not started loading yet
		LoadTox(ToxPath, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
	}
}

bool UTouchEngineSubsystem::ReloadTox(const FString& ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle)
{
	if (const TObjectPtr<UFileParams>* PotentialParams = LoadedParams.Find(ToxPath))
	{
		UFileParams* Params = *PotentialParams;
		const bool bQueueForLater = TempEngineInfo->Engine->IsLoading(); 
		if (bQueueForLater)
		{
			RemainingToxPaths.Add(ToxPath, FToxDelegateInfo(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle));
			return true;
		}
		
		// Reset currently stored data
		Params->ResetEngine();
		TempEngineInfo->Unload();
		if (ParamsLoadedDelHandle.IsValid())
		{
			UnbindDelegates(ParamsLoadedDelHandle, LoadFailedDelHandle);
		}
		return LoadTox(ToxPath, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle) != nullptr;
	}

	// tox was never loaded (can hit this if path is empty or invalid)
	return LoadTox(ToxPath, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle) != nullptr;
}

bool UTouchEngineSubsystem::IsSupportedPixelFormat(EPixelFormat PixelFormat) const
{
	bool bResult = CachedSupportedPixelFormats.Contains(PixelFormat);
	return bResult;
}

TObjectPtr<UFileParams> UTouchEngineSubsystem::GetParamsFromToxIfLoaded(FString ToxPath)
{
	const TObjectPtr<UFileParams>* Value = LoadedParams.Find(ToxPath);
	return Value && Value->Get()
		? Value->Get()
		: nullptr; 
}

bool UTouchEngineSubsystem::IsLoaded(const FString& ToxPath) const
{
	const TObjectPtr<UFileParams>* Value = LoadedParams.Find(ToxPath);
	return Value && Value->Get()
		? Value->Get()->bIsLoaded
		: false; 
}

bool UTouchEngineSubsystem::HasFailedLoad(const FString& ToxPath) const
{
	const TObjectPtr<UFileParams>* Value = LoadedParams.Find(ToxPath);
	return Value && Value->Get()
		? Value->Get()->bHasFailedLoad
		: false; 
}

bool UTouchEngineSubsystem::UnbindDelegates(FDelegateHandle ParamsLoadedDelHandle, FDelegateHandle LoadFailedDelHandle)
{
	for (const TPair<FString, UFileParams*>& Pair : LoadedParams)
	{
		UFileParams* Params = Pair.Value;
		if (Params->ParamsLoadedDelegate.Remove(ParamsLoadedDelHandle))
		{
			return Params->FailedLoadDelegate.Remove(LoadFailedDelHandle);
		}
	}
	return false;
}

UFileParams* UTouchEngineSubsystem::LoadTox(FString ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle)
{
	if (ToxPath.IsEmpty() || !ensure(TempEngineInfo && TempEngineInfo->Engine))
	{
		return nullptr;
	}

	UFileParams* Params = nullptr;
	if (!TempEngineInfo->Engine->HasAttemptedToLoad())
	{
		if (!LoadedParams.Contains(ToxPath))
		{
			// load tox
			Params = LoadedParams.Add(ToxPath, NewObject<UFileParams>());
			Params->bHasFailedLoad = false;
			Params->bIsLoaded = false;

			// bind delegates
			SetupCallbacks(Params, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
			if (TempEngineInfo->Load(ToxPath))
			{
				// failed load immediately due to probably file path error
				LoadedParams.Remove(ToxPath);
			}
		}
		else
		{
			// reloading
			Params = LoadedParams[ToxPath];

			// load tox
			Params->bHasFailedLoad = false;
			Params->bIsLoaded = false;

			SetupCallbacks(Params, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
			if (TempEngineInfo->Load(ToxPath))
			{
				// failed load immediately due to probably file path error
				LoadedParams.Remove(ToxPath);
				return nullptr;
			}
		}
	}
	else
	{
		if (LoadedParams.Contains(ToxPath))
		{
			// adding to file path either cached to load or currently loading
			Params = LoadedParams[ToxPath];
		}
		else
		{
			// reloading
			Params = LoadedParams.Add(ToxPath, NewObject<UFileParams>());
			Params->bHasFailedLoad = false;
			Params->bIsLoaded = false;
		}

		RemainingToxPaths.Add(ToxPath, FToxDelegateInfo(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle));
	}

	return Params;
}

void UTouchEngineSubsystem::LoadNext()
{
	if (TempEngineInfo && TempEngineInfo->Engine)
	{
		FString JustLoaded = TempEngineInfo->Engine->GetToxPath();
		RemainingToxPaths.Remove(JustLoaded);
		TempEngineInfo->Unload();
	}

	if (RemainingToxPaths.Num() > 0)
	{
		FString ToxPath = RemainingToxPaths.begin().Key();
		FToxDelegateInfo DelegateInfo = RemainingToxPaths.begin().Value();

		UFileParams* Params = LoadedParams[ToxPath];
		SetupCallbacks(Params, DelegateInfo.Owner, DelegateInfo.ParamsLoadedDelegate, DelegateInfo.FailedLoadDelegate, DelegateInfo.ParamsLoadedDelegateHandle, DelegateInfo.LoadFailedDelegateHandle);
		// Remove now, since Load(ToxPath) might fail, and cause LoadNext to try to load the same path again, crashing Unreal.
		RemainingToxPaths.Remove(ToxPath);

		if (TempEngineInfo->Load(ToxPath))
		{
			LoadedParams.Remove(ToxPath);
		}
	}
	else
	{
		TempEngineInfo->Destroy();
	}
}

void UTouchEngineSubsystem::SetupCallbacks(
	UFileParams* Params,
	UObject* Owner,
	FTouchOnParametersLoaded::FDelegate ParamsLoadedDel,
	FTouchOnFailedLoad::FDelegate LoadFailedDel,
	FDelegateHandle& ParamsLoadedDelHandle,
	FDelegateHandle& LoadFailedDelHandle
	)
{
	TempEngineInfo->GetOnParametersLoadedDelegate()->AddUObject(this, &UTouchEngineSubsystem::OnParamsLoaded, Params);
	TempEngineInfo->GetOnLoadFailedDelegate()->AddUObject(this, &UTouchEngineSubsystem::OnFailedLoad, Params);
	Params->BindOrCallDelegates(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
}

void UTouchEngineSubsystem::OnParamsLoaded(const TArray<FTouchEngineDynamicVariableStruct>& InInputs, const TArray<FTouchEngineDynamicVariableStruct>& InOutputs, UFileParams* InLoadedParams)
{
	// The TE instance has fully loaded - not it is safe to call this functions
	if (ensure(TempEngineInfo))
	{
		const bool bSuccess = TempEngineInfo->GetSupportedPixelFormats(CachedSupportedPixelFormats);
		UE_CLOG(!bSuccess, LogTouchEngine, Warning, TEXT("Failed to obtain supported pixel formats"));
	}
	InLoadedParams->OnParamsLoaded(InInputs, InOutputs);
}

void UTouchEngineSubsystem::OnFailedLoad(const FString& Error, UFileParams* InLoadedParams)
{
	InLoadedParams->OnFailedLoad(Error);
}
