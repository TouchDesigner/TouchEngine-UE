// Copyright © Derivative Inc. 2021


#include "ToxAssetFactory.h"

#include "ToxAsset.h"
#include "Misc/Paths.h"
#include "EditorFramework/AssetImportData.h"
#include "AssetRegistryModule.h"

DEFINE_LOG_CATEGORY(LogToxFactory);

#define LOCTEXT_NAMESPACE "ToxFactory"

UToxAssetFactory::UToxAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Formats.Add(FString(TEXT("tox;")) + LOCTEXT("ToxFile", "Tox File").ToString());
	SupportedClass = UToxAsset::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
}

UObject* UToxAssetFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UToxAsset* ToxAsset = NewObject<UToxAsset>(InParent, InClass, InName, Flags);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked< FAssetRegistryModule >(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	TArray<FAssetData> OutAssetData;
	AssetRegistry.GetAssetsByClass(InParent->GetClass()->GetFName(), OutAssetData);
	//UObject* InParentBlueprint = OutAssetData.GetAsset();

	FString Path = Filename;
	FPaths::MakePathRelativeTo(Path, *FPaths::ProjectContentDir());
	ToxAsset->FilePath = Path;

	bOutOperationCanceled = false;
	return ToxAsset;
}

/*
UReimportToxAssetFactory::UReimportToxAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UToxAsset::StaticClass();
	bCreateNew = false;
}

bool UReimportToxAssetFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UToxAsset* pTox = Cast<UToxAsset> (Obj);
	if (pTox)
	{
		pTox->FilePath->ExtractFilenames(OutFilenames);
		return true;
	}
	return false;
}

void UReimportToxAssetFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UToxAsset* pTox = Cast<UToxAsset>(Obj);
	if (pTox && ensure(NewReimportPaths.Num() == 1))
	{
		pTox->FilePath->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type UReimportToxAssetFactory::Reimport(UObject* Obj)
{
	if (!Obj || !Obj->IsA(UToxAsset::StaticClass()))
	{
		return EReimportResult::Failed;
	}

	UToxAsset* pTox = Cast<UToxAsset>(Obj);

	TGuardValue<UToxAsset*> OriginalToxGuardValue(pOriginalTox, pTox);

	const FString ResolvedSourceFilePath = pTox->FilePath->GetFirstFilename();
	if (!ResolvedSourceFilePath.Len())
	{
		return EReimportResult::Failed;
	}

	bool OutCanceled = false;
	if (ImportObject(pTox->GetClass(), pTox->GetOuter(), *pTox->GetName(), RF_Public | RF_Standalone, ResolvedSourceFilePath, nullptr, OutCanceled) != nullptr)
	{

	}
	else if (OutCanceled)
	{
		UE_LOG(LogToxFactory, Warning, TEXT("-- import canceled"));
		return EReimportResult::Cancelled;
	}
	else
	{
		UE_LOG(LogToxFactory, Warning, TEXT("-- import failed"));
		return EReimportResult::Failed;
	}
	return EReimportResult::Succeeded;
}

int32 UReimportToxAssetFactory::GetPriority() const
{
	return ImportPriority;
}

bool UReimportToxAssetFactory::IsAutomatedImport() const
{
	return Super::IsAutomatedImport();
}

*/

#undef LOCTEXT_NAMESPACE