// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineComponent.generated.h"

class UTouchEngineInfo;

/*
* The different cook modes the TouchEngine component can run in
*/
UENUM(BlueprintType)
enum class ETouchEngineCookMode : uint8
{
	COOKMODE_SYNCHRONIZED = 0			UMETA(DisplayName = "Synchronized"),
	COOKMODE_DELAYEDSYNCHRONIZED = 1	UMETA(DisplayName = "Delayed Synchronized"),
	COOKMODE_INDEPENDENT = 2			UMETA(DisplayName = "Independent"),
	COOKMODE_MAX
};

/*
* Adds a TouchEngine instance to an object. 
*/
UCLASS(DefaultToInstanced, Blueprintable, abstract)
class TOUCHENGINE_API UTouchEngineComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTouchEngineComponentBase();
	~UTouchEngineComponentBase();

	// Our TouchEngine Info
	UPROPERTY()
	UTouchEngineInfo* EngineInfo;
	// Path to the Tox File to load
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	FString ToxFilePath = "";
	// Mode for component to run in
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	TEnumAsByte<ETouchEngineCookMode> cookMode = ETouchEngineCookMode::COOKMODE_INDEPENDENT;
	// TouchEngine framerate
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Category = "ToxFile", DisplayName = "TE Frame Rate"))
	int64 TEFrameRate = 60;
	// Whether or not to start the TouchEngine immediately on begin play
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	bool LoadOnBeginPlay = true;
	// Container for all dynamic variables
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault, Category = "ToxFile"))
	FTouchEngineDynamicVariableContainer dynamicVariables;


protected:

	// delegate handles for the tox file either loading parameters successfully or failing
	FDelegateHandle paramsLoadedDelHandle, loadFailedDelHandle;
	// delegate handles for the call to begin frame and end frame
	FDelegateHandle beginFrameDelHandle, endFrameDelHandle;

	// Used to determine the time since last frame if we're cooking outside of TickComponent
	float cookTime = 0, lastCookTime = 0;

	// Called when the game starts
	virtual void BeginPlay() override;
	// Called at the beginning of a frame.
	void OnBeginFrame();
	// Called at the end of a frame.
	void OnEndFrame();
	// Called when a component is created. This may happen in editor for both blueprints and for world objects.
	virtual void OnComponentCreated() override;
	// Called when a component is destroyed. This may happen in editor for both blueprints and for world objects.
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy);
	// Called when a property on this object has been modified externally
	virtual void PostEditChangeProperty(FPropertyChangedEvent& e);
	// Attemps to grab the parameters from the TouchEngine engine subsytem. Should only be used for objects in blueprint.
	void LoadParameters();
	// Attempts to create an engine instance for this object. Should only be used for in world objects.
	void LoadTox();
	// Returns the absolute path of the stored ToxFilePath.
	FString GetAbsoluteToxPath();

public:	

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// Creates a TouchEngine instance for this object
	virtual void CreateEngineInfo();
	// Reloads the currently loaded tox file
	UFUNCTION(BlueprintCallable, meta = (Category = "ToxFile"))
	void ReloadTox();
	// Checks whether the component already has a tox file loaded
	bool IsLoaded();
	// Checks whether the component has failed to load a tox file
	bool HasFailedLoad();
	// Starts and creates the TouchEngine instnace
	UFUNCTION(BlueprintCallable)
	void StartTouchEngine();
	// Stops and deletes the TouchEngine instance
	UFUNCTION(BlueprintCallable)
	void StopTouchEngine();
};
