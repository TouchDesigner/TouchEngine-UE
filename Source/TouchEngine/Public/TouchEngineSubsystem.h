// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "UTouchEngine.h"
#include "TouchEngineSubsystem.generated.h"

/**
 * 
 */

UCLASS()
class TOUCHENGINE_API UTouchEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:

	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Implement this for deinitialization of instances of the system */
	virtual void Deinitialize() override;

	static UTouchEngine*			createEngine(FString toxPath);

private:

	void*		myLibHandle = nullptr;
	
};
