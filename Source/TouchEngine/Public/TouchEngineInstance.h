// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "TouchEngineSubsystem.h"
#include "UTouchEngine.h"
#include "TouchEngineInstance.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, Category = "TouchEngine", DisplayName = "TouchEngine Instance")
class TOUCHENGINE_API ATouchEngineInstance : public AInfo
{
	GENERATED_BODY()

	//virtual void		 BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	void		load(FString toxPath);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	FString		getToxPath() const;

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	FTouchCHOPSingleSample	getCHOPOutputSingleSample(const FString &identifier);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	void		setCHOPInputSingleSample(const FString &identifier, const FTouchCHOPSingleSample &chop);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	FTouchTOP	getTOPOutput(const FString &identifier);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	void		cookFrame();

private:
	UPROPERTY(Transient)
	UTouchEngine*			myEngine = nullptr;

	FString					myToxFile;
};
