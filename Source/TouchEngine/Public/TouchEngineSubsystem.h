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
#include "TouchEngineDynamicVariableStruct.h"
#include "Subsystems/EngineSubsystem.h"
#include "TouchEngineSubsystem.generated.h"

class UTouchEngineInfo;

DECLARE_MULTICAST_DELEGATE_TwoParams(FTouchOnParametersLoaded, const TArray<FTouchEngineDynamicVariableStruct>&, const TArray<FTouchEngineDynamicVariableStruct>&);
DECLARE_MULTICAST_DELEGATE_OneParam(FTouchOnFailedLoad, const FString&);

USTRUCT()
struct FToxDelegateInfo
{
	GENERATED_BODY()

	FToxDelegateInfo() = default;
	FToxDelegateInfo(
		UObject* Owner,
		FTouchOnParametersLoaded::FDelegate ParamsLoadedDelegate,
		FTouchOnFailedLoad::FDelegate FailedLoadDelegate,
		FDelegateHandle ParamsLoadedDelegateHandle,
		FDelegateHandle In_LoadFailedDelHandle)
		: Owner(Owner)
		, ParamsLoadedDelegate(ParamsLoadedDelegate)
		, FailedLoadDelegate(FailedLoadDelegate)
		, ParamsLoadedDelegateHandle(ParamsLoadedDelegateHandle)
		, LoadFailedDelegateHandle(LoadFailedDelegateHandle)
	{}
	
	UObject* Owner;
	FTouchOnParametersLoaded::FDelegate ParamsLoadedDelegate;
	FTouchOnFailedLoad::FDelegate FailedLoadDelegate;
	FDelegateHandle ParamsLoadedDelegateHandle;
	FDelegateHandle LoadFailedDelegateHandle;
};


/*
* Holds information for what parameters are loaded for a specific tox file
*/
UCLASS()
class TOUCHENGINE_API UFileParams : public UObject
{
	GENERATED_BODY()

public:

	// Dynamic variable inputs
	UPROPERTY(Transient)
	TArray<FTouchEngineDynamicVariableStruct> Inputs;

	// Dynamic variable outputs
	UPROPERTY(Transient)
	TArray<FTouchEngineDynamicVariableStruct> Outputs;

	// Engine instance to load parameter list. Deleted once parameters are retrieved.
	//UPROPERTY(Transient)
	//UTouchEngineInfo* EngineInfo;

	UPROPERTY(Transient)
	FString ErrorString = "";

	// if parameters have been loaded
	bool bIsLoaded = false;
	// if parameters failed to load
	bool bHasFailedLoad = false;

	// Delegate called when parameters have been loaded
	FTouchOnParametersLoaded OnParamsLoaded;
	// Delegate called when tox files fails load
	FTouchOnFailedLoad OnFailedLoad;
	
	// Binds or calls the delegates passed in based on if the parameters have been successfully loaded
	void BindOrCallDelegates(UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate FailedLoadDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle);

	// Callback for when the stored engine info loads the parameter list
	void ParamsLoaded(const TArray<FTouchEngineDynamicVariableStruct>& InInputs, const TArray<FTouchEngineDynamicVariableStruct>& InOutputs);

	// Callback for when the stored engine info fails to load tox file
	void FailedLoad(const FString& Error);

	// deletes stored variable data
	void ResetEngine();
};



UCLASS()
class TOUCHENGINE_API UTouchEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

	friend class UFileParams;

public:

	// Tox files that still need to be loaded
	UPROPERTY(Transient)
	TMap<FString, FToxDelegateInfo> CachedToxPaths;

	// Implement this for initialization of instances of the system
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Calls the passed in delegate when the parameters for the specified tox path have been loaded
	void GetParamsFromTox(FString ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle);
	UFileParams* GetParamsFromTox(FString ToxPath);
	// Unbinds the passed in handles from the UFileParam object associated with the passed in tox path
	void UnbindDelegates(FString ToxPath, FDelegateHandle ParamsLoadedDelHandle, FDelegateHandle LoadFailedDelHandle);
	// Attempts to unbind the passed in handles from any UFileParams they may be bound to
	bool UnbindDelegates(FDelegateHandle ParamsLoadedDelHandle, FDelegateHandle LoadFailedDelHandle);
	// Returns if the passed in tox path has any parameters associated with it
	bool IsLoaded(FString ToxPath) const;
	// Returns if the passed in tox path has failed to load parameters
	bool HasFailedLoad(FString ToxPath) const;
	// Deletes the parameters associated with the passed in tox path and attempts to reload
	bool ReloadTox(FString ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle);

private:

	// TouchEngine instance used to load items into the details panel
	UPROPERTY(Transient)
	TObjectPtr<UTouchEngineInfo> TempEngineInfo;

	// Map of files loaded to their parameters
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UFileParams>> LoadedParams;

	// Loads a tox file and stores its parameters in the "loadedParams" map
	UFileParams* LoadTox(FString ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle);

	void LoadNext();

};
