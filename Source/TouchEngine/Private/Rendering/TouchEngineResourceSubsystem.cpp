// Copyright © Derivative Inc. 2022


#include "TouchEngineResourceSubsystem.h"

#include "TouchEngineDX11ResourceProvider.h"

void UTouchEngineResourceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	{
		FString RHIName = GDynamicRHI->GetName();
#if PLATFORM_WINDOWS
		if(RHIName == TEXT("DX11"))
		{
			ResourceProvider = MakeShared<FTouchEngineDX11ResourceProvider>();
		}
		else if(RHIName == TEXT("DX12"))
		{
			
		}
#endif
		if(RHIName == TEXT("Vulkan"))
		{
			
		}
	}
}

void UTouchEngineResourceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

const TSharedPtr<FTouchEngineResourceProvider>& UTouchEngineResourceSubsystem::GetResourceProvider()
{
	return ResourceProvider;
}
