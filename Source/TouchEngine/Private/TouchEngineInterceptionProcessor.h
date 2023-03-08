// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ITouchEngineInterceptionFeature.h"


/**
 * Remote control interception processor feature
 */
class FTouchEngineInterceptionProcessor :
	public ITouchEngineInterceptionFeatureProcessor
{
public:
	FTouchEngineInterceptionProcessor() = default;
	virtual ~FTouchEngineInterceptionProcessor() = default;

public:
	// ITouchEngineInterceptionCommands interface
	virtual void VarsSetInputs(FTEToxAssetMetadata& InTEToxAssetMetadata) override;
	virtual void VarsGetOutputs(FTEToxAssetMetadata& InTEToxAssetMetadata) override;
	// ~ITouchEngineInterceptionCommands interface
};
