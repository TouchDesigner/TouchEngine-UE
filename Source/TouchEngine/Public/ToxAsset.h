// Copyright © Derivative Inc. 2021

#pragma once

#include "CoreMinimal.h"
#include "ToxAsset.generated.h"

class UAssetImportData;

/**
 *
 */
UCLASS(BlueprintType, hidecategories = (Object))
class TOUCHENGINE_API UToxAsset : public UObject
{
	GENERATED_BODY()

public:
	// File path to the tox file, relative to the project directory
	UPROPERTY(VisibleAnywhere, Category = ImportSettings)
	FString FilePath;
};
