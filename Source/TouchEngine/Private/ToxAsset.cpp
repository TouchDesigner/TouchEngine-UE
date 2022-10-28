// Copyright © Derivative Inc. 2021

#include "ToxAsset.h"

#include "EditorFramework/AssetImportData.h"
#include "UObject/ObjectSaveContext.h"

void UToxAsset::PostInitProperties()
{
	UObject::PostInitProperties();

#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif
}

void UToxAsset::PreSave(FObjectPreSaveContext SaveContext)
{
	UObject::PreSave(SaveContext);

#if WITH_EDITORONLY_DATA
	if (AssetImportData)
	{
		FString Path = AssetImportData->GetFirstFilename();
		FPaths::MakePathRelativeTo(Path, *FPaths::ProjectContentDir());
		FilePath = Path;
	}
#endif
}

#if WITH_EDITORONLY_DATA
void UToxAsset::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	if (AssetImportData)
	{
		OutTags.Add(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
	}

	Super::GetAssetRegistryTags(OutTags);
}
#endif
