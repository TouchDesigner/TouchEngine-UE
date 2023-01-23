// Copyright © Derivative Inc. 2021

#include "ToxAssetFactory.h"

#include "ToxAsset.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogToxFactory);

#define LOCTEXT_NAMESPACE "ToxFactory"

UToxAssetFactory::UToxAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Formats.Add(FString(TEXT("tox;")) + LOCTEXT("ToxFile", "Tox File").ToString());
	SupportedClass = UToxAsset::StaticClass();
	bCreateNew = false;
	bEditAfterNew = true;
	bEditorImport = true;
}

UObject* UToxAssetFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UToxAsset* ToxAsset = NewObject<UToxAsset>(InParent, InClass, InName, Flags);

	FString FilePath = Filename;

	const FString ContentDirRelativePath = FPaths::ProjectContentDir();
	const FString ContentDirFullPath = FPaths::ConvertRelativePathToFull(ContentDirRelativePath);
	if (Filename.StartsWith(ContentDirFullPath))
	{
		FPaths::MakePathRelativeTo(FilePath, *ContentDirRelativePath);
		FilePath = FString(TEXT("./")) + FilePath;
	}
	// Else this is an External Tox (i.e. located outside the Content folder)

	ToxAsset->SetFilePath(FilePath);

	bOutOperationCanceled = false;

	return ToxAsset;
}

#undef LOCTEXT_NAMESPACE