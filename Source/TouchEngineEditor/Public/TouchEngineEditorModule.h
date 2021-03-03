// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/*
* Module to bind the TouchEngineDynamicVariableDetailsCustomizationPanel to the TouchEngineDynamicVariableContainer class
*/
class FTouchEngineEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
};
