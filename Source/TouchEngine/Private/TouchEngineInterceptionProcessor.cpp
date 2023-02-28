// Copyright Epic Games, Inc. All Rights Reserved.

#include "TouchEngineInterceptionProcessor.h"

#include "Blueprint/TouchEngineComponent.h"

// Interception related headers

PRAGMA_DISABLE_OPTIMIZATION

void FTouchEngineInterceptionProcessor::VarsGetOutputs(FTEToxAssetMetadata& InTEToxAssetMetadata)
{

	UE_LOG(LogTemp, Warning, TEXT("1 >>>>> ObjectPath %s"), *InTEToxAssetMetadata.TouchEngineContainer->ObjectPath);

	UObject* LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *InTEToxAssetMetadata.ObjectPath, nullptr, LOAD_None, nullptr, true);
	//check(LoadedObject);

	if (LoadedObject)
	{
		UE_LOG(LogTemp, Warning, TEXT("2 >>>>> LoadedObject %s, DynVars_Input %d, DynVars_Output %d"), *LoadedObject->GetPathName(), InTEToxAssetMetadata.TouchEngineContainer->DynamicVariables.DynVars_Input.Num(), InTEToxAssetMetadata.TouchEngineContainer->DynamicVariables.DynVars_Output.Num());
	}

	if (UTouchEngineComponentBase* TouchEngineComponentBase = Cast<UTouchEngineComponentBase>(LoadedObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("3 >>>>> TouchEngineComponentBase %s"), *TouchEngineComponentBase->GetPathName());


		TouchEngineComponentBase->DynamicVariables = InTEToxAssetMetadata.DynamicVariables;
		TouchEngineComponentBase->VarsGetOutputs_Internal();
	}
}

PRAGMA_ENABLE_OPTIMIZATION