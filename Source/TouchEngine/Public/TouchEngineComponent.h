// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TouchEngineInfo.h"
#include "UTouchEngine.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "Containers\EnumAsByte.h"
#include "TouchEngineComponent.generated.h"



UENUM(BlueprintType)
enum class ETouchEngineCookMode : uint8
{
	COOKMODE_SYNCHRONIZED = 0			UMETA(DisplayName = "SYNCHRONIZED"),
	COOKMODE_DELAYEDSYNCHRONIZED = 1	UMETA(DisplayName = "DELAYED SYNCHRONIZED"),
	COOKMODE_INDEPENDENT = 2			UMETA(DisplayName = "INDEPENDENT"),
	COOKMODE_MAX
};


UCLASS(DefaultToInstanced, Blueprintable, abstract)
class TOUCHENGINE_API UTouchEngineComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTouchEngineComponentBase();

	// Our TouchEngine Info
	UPROPERTY()
	UTouchEngineInfo* EngineInfo;
	// Path to the Tox File to load
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	FString ToxFilePath = "";
	// Mode for component to run in
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	TEnumAsByte<ETouchEngineCookMode> cookMode = ETouchEngineCookMode::COOKMODE_DELAYEDSYNCHRONIZED;
	// Container for all dynamic variables
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault, Category = "ToxFile"))
	FTouchEngineDynamicVariableContainer dynamicVariables;


protected:

	// delegate handles for the tox file either loading parameters successfully or failing
	FDelegateHandle paramsLoadedDelHandle, loadFailedDelHandle;

	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void OnComponentCreated() override;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy);

	virtual void PostEditChangeProperty(FPropertyChangedEvent& e);

	void LoadParameters();

	void LoadTox();

	FString GetRelativeToxPath();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	virtual void CreateEngineInfo();

	UFUNCTION(BlueprintCallable, meta = (Category = "ToxFile"))
	void ReloadTox();

	bool IsLoaded();

	bool HasFailedLoad();
};
