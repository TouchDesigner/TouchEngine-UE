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

#include "TouchEngineEditorModule.h"

#include "AssetTypeActions_Base.h"
#include "Factory/ToxAssetFactoryNew.h"
#include "TouchEngineDynVarDetsCust.h"
#include "TouchEngineIntVector4StructCust.h"
#include "Customization/TouchEngineComponentCustomization.h"
#include "Customization/ToxAssetCustomization.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineIntVector4.h"
#include "ToxAsset.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogTouchEngineEditor);

#define LOCTEXT_NAMESPACE "FTouchEngineEditorModule"

EAssetTypeCategories::Type FTouchEngineEditorModule::TouchEngineAssetCategoryBit = EAssetTypeCategories::Misc;

void FTouchEngineEditorModule::StartupModule()
{
	RegisterCustomizations();
	
	RegisterAssetActions();

}

void FTouchEngineEditorModule::ShutdownModule()
{
	UnregisterCustomizations();

	UnregisterAssetActions();
	return;
}

void FTouchEngineEditorModule::RegisterAssetActions()
{
	// Acquire an asset category bit for our custom "TouchEngine" category
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	TouchEngineAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("TouchEngine")), LOCTEXT("TouchEngineCategory", "TouchEngine"));

	// Register New Tox Asset creation Action for Content Browser
	ToxAssetTypeActions = MakeShared<FToxAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(ToxAssetTypeActions.ToSharedRef());

}

void FTouchEngineEditorModule::UnregisterAssetActions()
{
	if (!FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return;
	}

	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(ToxAssetTypeActions.ToSharedRef());
}

void FTouchEngineEditorModule::RegisterCustomizations()
{
	using namespace UE::TouchEngineEditor::Private;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomClassLayout(UTouchEngineComponentBase::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FTouchEngineComponentCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(FTouchEngineDynamicVariableContainer::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTouchEngineDynamicVariableStructDetailsCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(FTouchEngineIntVector4::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTouchEngineIntVector4StructCust::MakeInstance));

	PropertyModule.RegisterCustomClassLayout(UToxAsset::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FToxAssetCustomization::MakeInstance));
}

void FTouchEngineEditorModule::UnregisterCustomizations()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomPropertyTypeLayout(FTouchEngineDynamicVariableContainer::StaticStruct()->GetFName());
	PropertyModule.UnregisterCustomPropertyTypeLayout(FTouchEngineIntVector4::StaticStruct()->GetFName());
	PropertyModule.UnregisterCustomClassLayout(UToxAsset::StaticClass()->GetFName());
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTouchEngineEditorModule, TouchEngineEditor)
