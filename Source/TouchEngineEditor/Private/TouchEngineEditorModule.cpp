// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TouchEngineEditorModule.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "DetailCustomizations.h"
#include "TouchEngineDynVarDetsCust.h"

#define LOCTEXT_NAMESPACE "FTouchEngineEditorModule"



 
void FTouchEngineEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(FName("TouchEngineDynamicVariableContainer"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&TouchEngineDynamicVariableStructDetailsCustomization::MakeInstance));
}

void FTouchEngineEditorModule::ShutdownModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomPropertyTypeLayout(FName("TouchEngineDynamicVariableContainer"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTouchEngineEditorModule, TouchEngineEditor)
