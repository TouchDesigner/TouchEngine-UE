// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineSubsystem.generated.h"

class UTouchEngineInfo;

DECLARE_MULTICAST_DELEGATE_TwoParams(FTouchOnParametersLoaded, TArray<FTouchEngineDynamicVariable>, TArray<FTouchEngineDynamicVariable>);
DECLARE_MULTICAST_DELEGATE(FTouchOnFailedLoad);

UCLASS()
class TOUCHENGINE_API UFileParams : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TArray<FTouchEngineDynamicVariable> Inputs;
	UPROPERTY()
	TArray<FTouchEngineDynamicVariable>	Outputs;

	UPROPERTY()
	UTouchEngineInfo* engineInfo;

	bool isLoaded = false;
	bool failedLoad = false;

	void BindOrCallDelegates(FTouchOnParametersLoaded::FDelegate paramsLoadedDel, FSimpleDelegate failedLoadDel, 
							 FDelegateHandle& paramsLoadedDelHandle, FDelegateHandle& loadFailedDelHandle);

	/** Delegate called when parameters have been loaded */
	FTouchOnParametersLoaded OnParamsLoaded;
	/** Delegate called when tox files fails load */
	FTouchOnFailedLoad OnFailedLoad;
	/** Callback for when the stored engine info loads the parameter list */
	UFUNCTION()
	void ParamsLoaded(TArray<FTouchEngineDynamicVariable> new_inputs, TArray<FTouchEngineDynamicVariable> new_outputs);
	/** Callback for when the stored engine info fails to load tox file */
	UFUNCTION()
	void FailedLoad();

	void ResetEngine();
};



UCLASS()
class TOUCHENGINE_API UTouchEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:

	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Implement this for deinitialization of instances of the system */
	virtual void Deinitialize() override;

	/** Calls the passed in delegate when the parameters for the specified tox file have been loaded*/
	void GetParamsFromTox(FString toxPath, FTouchOnParametersLoaded::FDelegate paramsLoadedDel, FSimpleDelegate loadFailedDel, 
										   FDelegateHandle& paramsLoadedDelHandle, FDelegateHandle& loadFailedDelHandle);

	void UnbindDelegates(FString toxPath, FDelegateHandle paramsLoadedDelHandle, FDelegateHandle loadFailedDelHandle);

	bool IsLoaded(FString toxPath);

	bool HasFailedLoad(FString toxPath);

	bool ReloadTox(FString toxPath, FTouchOnParametersLoaded::FDelegate paramsLoadedDel, FSimpleDelegate loadFailedDel,
									FDelegateHandle& paramsLoadedDelHandle, FDelegateHandle& loadFailedDelHandle);

private:

	void* myLibHandle = nullptr;
	/** Map of files loaded to their parameters */
	UPROPERTY(Transient)
	TMap<FString, UFileParams*> loadedParams;
	/** Loads a tox file and stores its parameters in the "loadedParams" map */
	UFileParams* LoadTox(FString toxPath, FTouchOnParametersLoaded::FDelegate paramsLoadedDel, FSimpleDelegate loadFailedDel, 
										  FDelegateHandle& paramsLoadedDelHandle, FDelegateHandle& loadFailedDelHandle);

};


