// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TouchEngineInfo.h"
#include "UTouchEngine.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineComponent.generated.h"



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
	UPROPERTY(EditDefaultsOnly)
	FString ToxPath;
	// Container for all dynamic variables
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault))
	FTouchEngineDynamicVariableContainer dynamicVariables;


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void OnComponentCreated() override;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy);

	virtual void PostEditChangeProperty(FPropertyChangedEvent& e);

	void LoadTox();
	 
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void CreateEngineInfo();
};