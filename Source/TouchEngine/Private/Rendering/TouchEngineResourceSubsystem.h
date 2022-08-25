// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TouchEngineResourceSubsystem.generated.h"

class FTouchEngineResourceProvider;

DECLARE_LOG_CATEGORY_EXTERN(LogTouchEngineResourceProvider, Log, All)

/**
 *
 */
UCLASS()
class TOUCHENGINE_API UTouchEngineResourceSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	const TSharedPtr<FTouchEngineResourceProvider>& GetResourceProvider();

private:
	TSharedPtr<FTouchEngineResourceProvider> ResourceProvider;
};
