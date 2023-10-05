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

#include "ToxAsset.h"

#include "EditorFramework/AssetImportData.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFileManager.h"

bool UToxAsset::IsRelativePath() const
{
	return FPaths::IsRelative(FilePath);
}

FString UToxAsset::GetAbsoluteFilePath() const
{
	if (IsRelativePath())
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir(), FilePath);
	}
	else
	{
		return FilePath;
	}
}

FString UToxAsset::GetRelativeFilePath() const
{
	if (IsRelativePath())
	{
		return FilePath;
	}
	else
	{
		FString RelativePath = FilePath;
		FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectContentDir());
		return RelativePath;
	}
}

void UToxAsset::SetFilePath(FString InPath)
{
	const FString ContentDir = FPaths::ProjectContentDir();
	if (FPaths::MakePathRelativeTo(InPath, *ContentDir))
	{
		InPath = FString(TEXT("./")) + InPath;
	}
	// Else this is an External Tox (i.e. located outside the Content folder)

	FilePath = InPath;
	
	// if (!AssetImportData)
	// {
	// 	// AssetImportData should always be valid
	// 	AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	// }
	// AssetImportData->Update(GetAbsoluteFilePath());
}

void UToxAsset::PostInitProperties()
{
// #if WITH_EDITORONLY_DATA
// 	if (!HasAnyFlags(RF_ClassDefaultObject))
// 	{
// 		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
// 		AssetImportData->Update(GetAbsoluteFilePath());
// 	}
// #endif
	Super::PostInitProperties();
}

void UToxAsset::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	// the code below makes the reimporting work, but forces the tox file and the asset file to move together
	// FAssetImportInfo ImportInfo;
	// ImportInfo.Insert(FAssetImportInfo::FSourceFile(GetAbsoluteFilePath()));
	// OutTags.Add( FAssetRegistryTag(SourceFileTagName(), ImportInfo.ToJson(), FAssetRegistryTag::TT_Hidden) );

	Super::GetAssetRegistryTags(OutTags);
}

void UToxAsset::Serialize(FArchive& Ar)
{
	// if (Ar.IsSaving() && !HasAnyFlags(RF_ClassDefaultObject))
	// {
	// 	if (!AssetImportData)
	// 	{
	// 		// AssetImportData should always be valid
	// 		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	// 	}
	// 	AssetImportData->Update(GetAbsoluteFilePath());
	// }
	
	Super::Serialize(Ar);

	// if (Ar.IsLoading() && !AssetImportData && !HasAnyFlags(RF_ClassDefaultObject))
	// {
	// 	// AssetImportData should always be valid
	// 	AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	// 	// AssetImportData->UpdateFilenameOnly(GetAbsoluteFilePath());
	// 	AssetImportData->Update(GetAbsoluteFilePath());
	// }
}

#if WITH_EDITOR
void UToxAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UToxAsset, FilePath))
	{
		SetFilePath(FilePath);
	}
}
#endif
