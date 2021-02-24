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



UCLASS(Category = "TouchEngine", DisplayName = "TouchEngineInfo Instance")
class TOUCHENGINE_API UTouchEngineInfo : public UObject
{
	GENERATED_BODY()

	friend class UTouchOnLoadTask;

public:

	UTouchEngineInfo();

	bool		load(FString toxPath);

	void		clear();

	void		destroy();

	FString		getToxPath() const;

	bool		setCookMode(bool isIndependent);

	bool		setFrameRate(int64 frameRate);

	FTouchCHOPSingleSample	getCHOPOutputSingleSample(const FString &identifier);

	void		setCHOPInputSingleSample(const FString &identifier, const FTouchCHOPSingleSample &chop);

	FTouchTOP	getTOPOutput(const FString &identifier);

	void		setTOPInput(const FString &identifier, UTexture *texture);

	FTouchVar<bool>				getBooleanOutput(const FString& identifier);
	void						setBooleanInput(const FString& identifier, FTouchVar<bool>& op);
	FTouchVar<double>			getDoubleOutput(const FString& identifier);
	void						setDoubleInput(const FString& identifier, FTouchVar<TArray<double>>& op);
	FTouchVar<int32_t>			getIntegerOutput(const FString& identifier);
	void						setIntegerInput(const FString& identifier, FTouchVar<int32_t>& op);
	FTouchVar<TEString*>		getStringOutput(const FString& identifier);
	void						setStringInput(const FString& identifier, FTouchVar<char*>& op);
	FTouchVar<TETable*>			getTableOutput(const FString& identifier);
	void						setTableInput(const FString& identifier, FTouchVar<TETable*>& op);


	void		cookFrame(int64 FrameTime_Mill);

	bool		isLoaded();

	bool		isCookComplete();

	bool		hasFailedLoad();

	void		logTouchEngineError(FString error);

	FTouchOnLoadComplete* getOnLoadCompleteDelegate();
	FTouchOnLoadFailed* getOnLoadFailedDelegate();
	FTouchOnParametersLoaded* getOnParametersLoadedDelegate();

private:
	UPROPERTY(Transient)
	UTouchEngine*			engine = nullptr;

	FString					myToxFile;
};