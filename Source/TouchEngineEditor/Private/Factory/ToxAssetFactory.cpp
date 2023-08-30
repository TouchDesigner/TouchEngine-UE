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
// #include "EditorFramework/AssetImportData.h"
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
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport( this, InClass, InParent, InName, Parms );
	UToxAsset* ToxAsset = NewObject<UToxAsset>(InParent, InClass, InName, Flags);
	ToxAsset->SetFilePath(Filename);
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, ToxAsset);

	bOutOperationCanceled = false;

	return ToxAsset;
}

bool UToxAssetFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	const UToxAsset* ToxAsset = Cast<UToxAsset>(Obj);
	if (IsValid(ToxAsset))
	{
		OutFilenames.Add(ToxAsset->GetAbsoluteFilePath());
		return true;
	}
	return false;
}

void UToxAssetFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UToxAsset* ToxAsset = Cast<UToxAsset>(Obj);
	if (IsValid(ToxAsset) && ensure(NewReimportPaths.Num() == 1))
	{
		ToxAsset->SetFilePath(NewReimportPaths[0]);
	}
}

EReimportResult::Type UToxAssetFactory::Reimport(UObject* Obj)
{
	const UToxAsset* ToxAsset = Cast<UToxAsset>(Obj);
	if (!IsValid(ToxAsset) || !FPaths::FileExists(ToxAsset->GetAbsoluteFilePath()))
	{
		return EReimportResult::Failed;
	}
	
	// const FString Filename = ToxAsset->AssetImportData->GetFirstFilename();
	// ToxAsset->AssetImportData->Update(Filename);
	
	UE_LOG(LogTemp, Warning, TEXT("REIMPORT"));
	return EReimportResult::Succeeded;
}

#undef LOCTEXT_NAMESPACE
