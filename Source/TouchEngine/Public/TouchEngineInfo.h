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
class TOUCHENGINE_API ATouchEngineInfo : public AInfo
{
	GENERATED_BODY()

		//virtual void		 BeginPlay() override;

	friend class UTouchOnLoadTask;

public:

	ATouchEngineInfo();

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

private:
	UPROPERTY(Transient)
	UTouchEngine*			engine = nullptr;

	FString					myToxFile;
};


UCLASS(BlueprintType)
class TOUCHENGINE_API UTouchOnLoadTask : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UTouchOnLoadTask() : Super() {}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
		static UTouchOnLoadTask* WaitOnLoadTask(UObject* WorldContextObject, ATouchEngineInfo* EngineInfo, FString toxPath);

	virtual void Activate() override;

	UFUNCTION()
		virtual void OnLoadComplete();

	UPROPERTY(BlueprintAssignable)
		FTouchOnLoadComplete LoadComplete;

protected:

	UPROPERTY()
		ATouchEngineInfo* engineInfo;
	UPROPERTY()
		FString ToxPath;
};