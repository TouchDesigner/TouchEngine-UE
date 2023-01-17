// Copyright © Derivative Inc. 2021

#include "ToxAssetFactory.h"

#include "ToxAsset.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogToxFactory);

#define LOCTEXT_NAMESPACE "ToxFactory"

UClass* FToxAssetTypeActions::GetSupportedClass() const
{
	return UToxAsset::StaticClass();
}

FText FToxAssetTypeActions::GetName() const
{
	return INVTEXT("Tox Asset");
}

FColor FToxAssetTypeActions::GetTypeColor() const
{
	return FColor::Yellow;
}

uint32 FToxAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

UToxAssetFactory::UToxAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Formats.Add(FString(TEXT("tox;")) + LOCTEXT("ToxFile", "Tox File").ToString());
	SupportedClass = UToxAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = true;
}

UObject* UToxAssetFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UToxAsset* ToxAsset = NewObject<UToxAsset>(InParent, InClass, InName, Flags);

	FString Path = Filename;
	FPaths::MakePathRelativeTo(Path, *FPaths::ProjectContentDir());
	ToxAsset->FilePath = Path;

	bOutOperationCanceled = false;
	return ToxAsset;
}

UObject* UToxAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UToxAsset* NewAsset = NewObject<UToxAsset>(InParent, InClass, InName, Flags);

	return NewAsset;
}

uint32 UToxAssetFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}

#undef LOCTEXT_NAMESPACE