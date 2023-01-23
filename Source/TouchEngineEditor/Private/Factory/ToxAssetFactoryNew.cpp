// Copyright © Derivative Inc. 2021


#include "ToxAssetFactoryNew.h"

#include "ToxAsset.h"

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

UToxAssetFactoryNew::UToxAssetFactoryNew(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UToxAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;
}

UObject* UToxAssetFactoryNew::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UToxAsset* NewAsset = NewObject<UToxAsset>(InParent, InClass, InName, Flags);

	return NewAsset;
}

uint32 UToxAssetFactoryNew::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}