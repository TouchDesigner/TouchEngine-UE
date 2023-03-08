// Copyright Epic Games, Inc. All Rights Reserved.

#include "TouchEngineInterceptionProcessor.h"

#include "Blueprint/TouchEngineComponent.h"

// Interception related headers

PRAGMA_DISABLE_OPTIMIZATION

void FTouchEngineInterceptionProcessor::VarsSetInputs(FTEToxAssetMetadata& InTEToxAssetMetadata)
{
	UObject* LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *InTEToxAssetMetadata.ObjectPath, nullptr, LOAD_None, nullptr, true);

	if (LoadedObject)
	{
	}

	if (UTouchEngineComponentBase* TouchEngineComponentBase = Cast<UTouchEngineComponentBase>(LoadedObject))
	{
		// TODO. no needed now, we just call the same class on master
		if (InTEToxAssetMetadata.bApplyDynamicVariables)
		{
			TouchEngineComponentBase->DynamicVariables = InTEToxAssetMetadata.DynamicVariables;
		}
		TouchEngineComponentBase->VarsSetInputs_Internal();
	}
}

void FTouchEngineInterceptionProcessor::VarsGetOutputs(FTEToxAssetMetadata& InTEToxAssetMetadata)
{
	UObject* LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *InTEToxAssetMetadata.ObjectPath, nullptr, LOAD_None, nullptr, true);

	if (LoadedObject)
	{
	}

	if (UTouchEngineComponentBase* TouchEngineComponentBase = Cast<UTouchEngineComponentBase>(LoadedObject))
	{
		TouchEngineComponentBase->DynamicVariables = InTEToxAssetMetadata.DynamicVariables;
		TouchEngineComponentBase->VarsGetOutputs_Internal();
	}
}

PRAGMA_ENABLE_OPTIMIZATION