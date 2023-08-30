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
#if WITH_EDITOR
#include "Engine/TouchEngineSubsystem.h"
#endif
#include "ToxAsset.generated.h"

// class UAssetImportData;
#if WITH_EDITOR
DECLARE_MULTICAST_DELEGATE_OneParam(FOnToxStartedLoadingThroughSubsystem_Native, class UToxAsset*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnToxLoadedThroughSubsystem_Native, class UToxAsset*, const UE::TouchEngine::FCachedToxFileInfo&);
#endif

/**
 *
 */
UCLASS(BlueprintType, hidecategories = (Object))
class TOUCHENGINE_API UToxAsset : public UObject
{
	GENERATED_BODY()

public:
	bool IsRelativePath() const;
	UFUNCTION(BlueprintGetter, BlueprintPure, Category = ImportSettings)
	FString GetAbsoluteFilePath() const;
	UFUNCTION(BlueprintGetter, BlueprintPure, Category = ImportSettings)
	FString GetRelativeFilePath() const;

	UFUNCTION(BlueprintSetter, Category = ImportSettings)
	void SetFilePath(FString InPath);

private:
	/*
	 * File path to the tox file.
	* This is relative to the project directory for Tox files located within the project's Content folder
	* For external Tox files (located outside the Content folder) this is an absolute path.
	*/
	UPROPERTY(EditAnywhere, BlueprintGetter=GetRelativeFilePath, BlueprintSetter=SetFilePath, Category = ImportSettings)
	FString FilePath;
	
public:
// #if WITH_EDITORONLY_DATA
// 	/** Importing data and options used for this Tox file. Needed to work within the reimport framework of UE */
// 	UPROPERTY(VisibleAnywhere, Instanced, Category = ImportSettings, meta=(FullyExpand))
// 	TObjectPtr<class UAssetImportData> AssetImportData;
// #endif
	
	// UObject interface
	virtual void PostInitProperties() override;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End of UObject interface

#if WITH_EDITOR
	FOnToxStartedLoadingThroughSubsystem_Native& GetOnToxStartedLoadingThroughSubsystem() { return OnToxStartedLoadingThroughSubsystem_Native; }
	FOnToxLoadedThroughSubsystem_Native& GetOnToxLoadedThroughSubsystem() { return OnToxLoadedThroughSubsystem_Native; }

protected:
	/** Called when the Editor is starting to load the tox file */
	FOnToxStartedLoadingThroughSubsystem_Native OnToxStartedLoadingThroughSubsystem_Native;
	/** Called when the Editor has finished loading the tox file */
	FOnToxLoadedThroughSubsystem_Native OnToxLoadedThroughSubsystem_Native;
#endif
};
