// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TouchEngineModule.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "DetailCustomizations.h"

#define LOCTEXT_NAMESPACE "FTouchEngineModule"




void FTouchEngineModule::StartupModule()
{
}

void FTouchEngineModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTouchEngineModule, TouchEngine)
