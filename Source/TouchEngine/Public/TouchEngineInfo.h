// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Misc/Paths.h"
#include "TouchEngineSubsystem.h"
#include "UTouchEngine.h"
#include "TouchEngineInfo.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, Category = "TouchEngine", DisplayName = "TouchEngineInfo Instance")
class TOUCHENGINE_API ATouchEngineInfo : public AInfo
{
	GENERATED_BODY()

	//virtual void		 BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	bool		load(FString toxPath);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	FString		getToxPath() const;

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	FTouchCHOPSingleSample	getCHOPOutputSingleSample(const FString &identifier);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	void		setCHOPInputSingleSample(const FString &identifier, const FTouchCHOPSingleSample &chop);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	FTouchTOP	getTOPOutput(const FString &identifier);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	void		setTOPInput(const FString &identifier, UTexture *texture);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	void		cookFrame();

private:
	UPROPERTY(Transient)
	UTouchEngine*			myEngine = nullptr;

	FString					myToxFile;
};
