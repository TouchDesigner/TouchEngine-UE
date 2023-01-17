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
	FString GetAbsoluteFilePath() const
	{
		return ToxSource.FilePath;
	}

	FString GetRelativeFilePath() const
	{ 
		FString RelativePath = ToxSource.FilePath;
		FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectContentDir());

		return RelativePath;
	}

	void SetFilePath(FString InPath);

	UPROPERTY(EditAnywhere, Category = ImportSettings)
	FFilePath ToxSource;

	void PostLoad() override;

private:
	/* Deprecated and now private. Retained for providing backward compatibility
	* Users can now manually edit the file location using the ToxSource file picker
	*
	* Original comment: File path to the tox file, relative to the project directory */
	UPROPERTY()
	FString FilePath;
};
