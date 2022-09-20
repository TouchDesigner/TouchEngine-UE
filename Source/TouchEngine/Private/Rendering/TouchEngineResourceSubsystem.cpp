// Copyright © Derivative Inc. 2022


#include "TouchEngineResourceSubsystem.h"

#include "ITouchEngineModule.h"

TSharedPtr<UE::TouchEngine::FTouchEngineResourceProvider> UTouchEngineResourceSubsystem::GetResourceProvider()
{
	return UE::TouchEngine::ITouchEngineModule::Get().GetResourceProvider();
}