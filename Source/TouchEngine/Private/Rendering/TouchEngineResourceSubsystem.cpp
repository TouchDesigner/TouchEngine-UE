// Copyright © Derivative Inc. 2022


#include "TouchEngineResourceSubsystem.h"

#include "TouchEngineDX11ResourceProvider.h"

DEFINE_LOG_CATEGORY(LogTouchEngineResourceProvider)

void UTouchEngineResourceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	{
		FString RHIName = GDynamicRHI->GetName();
#if PLATFORM_WINDOWS
		if(RHIName == TEXT("D3D11")) // DX11
		{
			ResourceProvider = MakeShared<FTouchEngineDX11ResourceProvider>();
		}
		else if(RHIName == TEXT("D3D12")) // DX12
		{

		}
#endif
		if(RHIName == TEXT("Vulkan"))
		{

		}

		if (ResourceProvider)
		{
			ResourceProvider->Initialize(
				[](const FString& Error)
				{
					UE_LOG(LogTouchEngineResourceProvider, Error, TEXT("ResourceProvider load error: %s"), *Error)
				},
				[](const TEResult Result, const FString& Error)
				{
					if (Result != TEResultSuccess)
					{
						UE_LOG(LogTouchEngineResourceProvider, Error, TEXT("ResourceProvider failed to create context: %s"), *Error)
					}
				});
		}
		else
		{
			UE_LOG(LogTouchEngineResourceProvider, Warning, TEXT("No ResourceProvider created. Unsupported RHI: %s"), *RHIName)
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
