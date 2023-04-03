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