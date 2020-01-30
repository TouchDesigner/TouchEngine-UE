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

	static UTouchEngine*			createEngine(FString toxPath);
	//static void						destroyEngine(UTouchEngine**);


private:
	
};
