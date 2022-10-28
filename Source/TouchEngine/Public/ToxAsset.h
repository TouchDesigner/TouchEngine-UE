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
	virtual void PostInitProperties() override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

#if WITH_EDITORONLY_DATA
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
#endif

	// File path to the tox file, relative to the project directory
	UPROPERTY(VisibleAnywhere, Category = ImportSettings)
	FString FilePath;

#if WITH_EDITORONLY_DATA
	/** Import options used for this ToxAsset */
	UPROPERTY(Category = ImportSettings, VisibleAnywhere, Instanced)
	TObjectPtr<UAssetImportData> AssetImportData;
#endif
};
