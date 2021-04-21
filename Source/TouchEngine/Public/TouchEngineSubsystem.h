// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "TouchEngineSubsystem.generated.h"

struct FTouchDynVar;
class UTouchEngineInfo;

DECLARE_MULTICAST_DELEGATE_TwoParams(FTouchOnParametersLoaded, TArray<FTouchDynVar>, TArray<FTouchDynVar>);
DECLARE_MULTICAST_DELEGATE(FTouchOnFailedLoad);

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
	TArray<FTouchDynVar> Inputs;
	// Dynamic variable outputs
	UPROPERTY(Transient)
	TArray<FTouchDynVar>	Outputs;

	// Engine instance to load parameter list. Deleted once parameters are retreived.
	UPROPERTY(Transient)
	UTouchEngineInfo* engineInfo;

	// if parameters have been loaded
	bool isLoaded = false;
	// if parameters failed to load
	bool failedLoad = false;

	// Delegate called when parameters have been loaded 
	FTouchOnParametersLoaded OnParamsLoaded;
	// Delegate called when tox files fails load
	FTouchOnFailedLoad OnFailedLoad;
	// Binds or calls the delegates passed in based on if the parameters have been successfuly loaded
	void BindOrCallDelegates(FTouchOnParametersLoaded::FDelegate paramsLoadedDel, FSimpleDelegate failedLoadDel, 
							 FDelegateHandle& paramsLoadedDelHandle, FDelegateHandle& loadFailedDelHandle);

	// Callback for when the stored engine info loads the parameter list
	UFUNCTION()
	void ParamsLoaded(TArray<FTouchDynVar> new_inputs, TArray<FTouchDynVar> new_outputs);
	// Callback for when the stored engine info fails to load tox file
	UFUNCTION()
	void FailedLoad();

	// deletes stored variable data
	void ResetEngine();
};



UCLASS()
class TOUCHENGINE_API UTouchEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:

	// Implement this for initialization of instances of the system
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	// Implement this for deinitialization of instances of the system
	virtual void Deinitialize() override;

	// Calls the passed in delegate when the parameters for the specified tox path have been loaded
	void GetParamsFromTox(FString toxPath, FTouchOnParametersLoaded::FDelegate paramsLoadedDel, FSimpleDelegate loadFailedDel, 
										   FDelegateHandle& paramsLoadedDelHandle, FDelegateHandle& loadFailedDelHandle);
	// Unbinds the passed in handles from the UFileParam object associated with the passed in tox path
	void UnbindDelegates(FString toxPath, FDelegateHandle paramsLoadedDelHandle, FDelegateHandle loadFailedDelHandle);
	// Attempts to unbind the passed in handles from any UFileParams they may be bound to
	bool UnbindDelegates(FDelegateHandle paramsLoadedDelHandle, FDelegateHandle loadFailedDelHandle);
	// Returns if the passed in tox path has any parameters associated with it
	bool IsLoaded(FString toxPath);
	// Returns if the passed in tox path has failed to load parameters
	bool HasFailedLoad(FString toxPath);
	// Deletes the parameters associated with the passed in tox path and attempts to reload
	bool ReloadTox(FString toxPath, FTouchOnParametersLoaded::FDelegate paramsLoadedDel, FSimpleDelegate loadFailedDel,
									FDelegateHandle& paramsLoadedDelHandle, FDelegateHandle& loadFailedDelHandle);

private:

	// Pointer to lib file handle
	void* myLibHandle = nullptr;
	// Map of files loaded to their parameters
	UPROPERTY(Transient)
	TMap<FString, UFileParams*> loadedParams;
	// Loads a tox file and stores its parameters in the "loadedParams" map
	UFileParams* LoadTox(FString toxPath, FTouchOnParametersLoaded::FDelegate paramsLoadedDel, FSimpleDelegate loadFailedDel, 
										  FDelegateHandle& paramsLoadedDelHandle, FDelegateHandle& loadFailedDelHandle);

};


