// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "TouchEngineResourceSubsystem.generated.h"

class FSubsystemCollectionBase;

namespace UE::TouchEngine
{
	class FTouchEngineResourceProvider;
}

/**
 *
 */
UCLASS()
class TOUCHENGINE_API UTouchEngineResourceSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:

	// Gets or creates the resource provider for the correct RHI
	TSharedPtr<UE::TouchEngine::FTouchEngineResourceProvider> GetResourceProvider();
};
