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
UCLASS(BlueprintType, Blueprintable, DisplayName = "TouchEngine Instance")
class TOUCHENGINE_API ATouchEngineInstance : public AInfo
{
	GENERATED_BODY()

	//virtual void		 BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	float		getValue();

	UFUNCTION(BlueprintCallable)
	void		load(FString toxPath);

	UFUNCTION(BlueprintCallable)
	FString		getToxPath() const;

	UFUNCTION(BlueprintCallable)
	FTouchCHOPSingleSample	getCHOPOutputSingleSample(const FString &identifier);

	UFUNCTION(BlueprintCallable)
	void		setCHOPInputSingleSample(const FString &identifier, const FTouchCHOPSingleSample &chop);

	UFUNCTION(BlueprintCallable)
	FTouchTOP	getTOPOutput(const FString &identifier);

	UFUNCTION(BlueprintCallable)
	void		cookFrame();

private:
	UPROPERTY(Transient)
	UTouchEngine*			myEngine = nullptr;

	UPROPERTY(EditAnywhere)
	FString					myToxFile;
};
