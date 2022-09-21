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
#include "Components/ActorComponent.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineResourceProvider.h"
#include "TouchEngineComponent.generated.h"

class UTouchEngineInfo;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToxLoaded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToxFailedLoad, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSetInputs);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGetOutputs);

/*
* The different cook modes the TouchEngine component can run in
*/
UENUM(BlueprintType)
enum class ETouchEngineCookMode : uint8
{
	Synchronized = 0			UMETA(DisplayName = "Synchronized"),
	DelayedSynchronized = 1		UMETA(DisplayName = "Delayed Synchronized"),
	Independent = 2				UMETA(DisplayName = "Independent"),
	Max							UMETA(Hidden)
};

/*
* The different times the TouchEngine component will set / get variables from the TouchEngine instance
*/
UENUM(BlueprintType)
enum class ETouchEngineSendMode : uint8
{
	EveryFrame = 0		UMETA(DisplayName = "Every Frame"),
	OnAccess = 1		UMETA(DisplayName = "On Access"),
	Max					UMETA(Hidden)
};

/*
* Adds a TouchEngine instance to an object.
*/
UCLASS(Blueprintable, meta = (DisplayName = "TouchEngine Component"))
class TOUCHENGINE_API UTouchEngineComponentBase : public UActorComponent
{
	GENERATED_BODY()
	friend class FTouchEngineDynamicVariableStructDetailsCustomization;
public:

	/************** Delegates **************/ 
	
	/** Called when the TouchEngine instance loads the tox file */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FOnToxLoaded OnToxLoaded;

	/** Called when the TouchEngine instance fails to load the tox file */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FOnToxFailedLoad OnToxFailedLoad;

	UPROPERTY(BlueprintAssignable, Category = "Components|Parameters")
	FSetInputs SetInputs;

	UPROPERTY(BlueprintAssignable, Category = "Components|Parameters")
	FGetOutputs GetOutputs;
	
	/************** Vars **************/ 

	/** Our TouchEngine Info */
	UPROPERTY()
	TObjectPtr<UTouchEngineInfo> EngineInfo;

	/** Path to the Tox File to load */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	FString ToxFilePath;

	/** Mode for component to run in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	ETouchEngineCookMode CookMode = ETouchEngineCookMode::Independent;

	/** Mode for the component to set and get variables */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	ETouchEngineSendMode SendMode = ETouchEngineSendMode::EveryFrame;

	/** TouchEngine framerate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Category = "ToxFile", DisplayName = "TE Frame Rate"))
	int64 TEFrameRate = 60;

	/** Whether or not to start the TouchEngine immediately on begin play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	bool LoadOnBeginPlay = true;

	/** Container for all dynamic variables */
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault, Category = "ToxFile"))
	FTouchEngineDynamicVariableContainer DynamicVariables;

	UPROPERTY()
	FString ErrorMessage;
	
	UTouchEngineComponentBase();

	virtual void PostInitProperties() override
	{
		if (HasAnyFlags(RF_ClassDefaultObject))
		{
			Test = MakeShared<int>(42);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Nope"));
		}

		Super::PostInitProperties();
	}
	TSharedPtr<int32> Test;

	/** Reloads the currently loaded tox file */
	UFUNCTION(BlueprintCallable, meta = (Category = "ToxFile"))
	void ReloadTox();

	/** Checks whether the component already has a tox file loaded */
	bool IsLoaded() const;
	/** Checks whether the component has failed to load a tox file */
	bool HasFailedLoad() const;

	/** Starts and creates the TouchEngine instance */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Start TouchEngine"), Category = "TouchEngine")
	void StartTouchEngine();

	/** Stops and deletes the TouchEngine instance */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Stop TouchEngine"), Category = "TouchEngine")
	void StopTouchEngine();

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	bool IsRunning() const;
	
	void UnbindDelegates();

	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	//~ End UActorComponent Interface

private:

	FDelegateHandle ParamsLoadedDelegateHandle;
	FDelegateHandle LoadFailedDelegateHandle;

	FDelegateHandle BeginFrameDelegateHandle;
	
	// Called at the beginning of a frame.
	void OnBeginFrame();
	
	// Attempts to grab the parameters from the TouchEngine engine subsystem. Should only be used for objects in blueprint.
	void LoadParameters();
	// Ensures that the stored parameters match the parameters stored in the TouchEngine engine subsystem.
	void ValidateParameters();
	// Attempts to create an engine instance for this object. Should only be used for in world objects.
	void LoadTox();
	void CreateEngineInfo();
	
	// Returns the absolute path of the stored ToxFilePath.
	FString GetAbsoluteToxPath() const;
	
	void VarsSetInputs();
	void VarsGetOutputs();
};
