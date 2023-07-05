/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

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
