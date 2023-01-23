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
	bool IsRelativePath() const;
	FString GetAbsoluteFilePath() const;
	FString GetRelativeFilePath() const;

	void SetFilePath(FString InPath);

	/* File path to the tox file.
	* This is relative to the project directory for Tox files located within the project's Content folder
	* For external Tox files (located outside the Content folder) this is an absolute path.
	*/
	UPROPERTY(EditAnywhere, Category = ImportSettings)
	FString FilePath;
};
