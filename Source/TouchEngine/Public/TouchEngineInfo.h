// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Misc/Paths.h"
#include "TouchEngineSubsystem.h"
#include "UTouchEngine.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "TouchEngineInfo.generated.h"

/**
 * 
 */

UCLASS(BlueprintType, Blueprintable, Category = "TouchEngine", DisplayName = "TouchEngineInfo Instance")
class TOUCHENGINE_API UTouchEngineInfo : public UObject
{
	GENERATED_BODY()

		//virtual void		 BeginPlay() override;

	friend class UTouchOnLoadTask;

public:

	UTouchEngineInfo();

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

	UFUNCTION(BlueprintCallable, Category = "TouchEngine")
	bool		isLoaded();

	FTouchOnLoadComplete getOnLoadCompleteDelegate();

private:
	UPROPERTY(Transient)
	UTouchEngine*			engine = nullptr;

	FString					myToxFile;
};



DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadComplete);

UCLASS(BlueprintType)
class TOUCHENGINE_API UTouchOnLoadTask : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UTouchOnLoadTask() : Super() {}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
		static UTouchOnLoadTask* WaitOnLoadTask(UObject* WorldContextObject, UTouchEngineInfo* EngineInfo, FString toxPath);

	virtual void Activate() override;

	UFUNCTION()
		virtual void OnLoadComplete();

	UPROPERTY(BlueprintAssignable)
	FOnLoadComplete LoadComplete;

protected:

	UPROPERTY()
		UTouchEngineInfo* engineInfo;
	UPROPERTY()
		FString ToxPath;
};