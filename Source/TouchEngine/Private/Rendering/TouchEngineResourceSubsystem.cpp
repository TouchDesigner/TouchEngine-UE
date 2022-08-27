// Copyright © Derivative Inc. 2022


#include "TouchEngineResourceSubsystem.h"

#include "TouchEngineDX11ResourceProvider.h"
#include "TouchEngineSubsystem.h"

DEFINE_LOG_CATEGORY(LogTouchEngineResourceProvider)

void UTouchEngineResourceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Cannot call InitializeResourceProvider yet, since UTouchEngineResourceSubsystem may Initialize before UTouchEngineSubsystem does :/
	// So we wait until its needed and create it lazy-load-style.
	// -Drakynfly
}

void UTouchEngineResourceSubsystem::Deinitialize()
{
	ENQUEUE_RENDER_COMMAND(UTouchEngineResourceSubsystem_Deinitialize)(
	[this](FRHICommandListImmediate& RHICmdList)
	{
		if (!IsValid(this))
		{
			return;
		}

		FScopeLock Lock(&ResourceSubsystemCriticalSection);

		if (ResourceProvider)
		{
			ResourceProvider->Release();
			ResourceProvider = nullptr;
		}
	});

	Super::Deinitialize();
}

const TSharedPtr<FTouchEngineResourceProvider>& UTouchEngineResourceSubsystem::GetResourceProvider()
{
	if (!ResourceProvider)
	{
		InitializeResourceProvider();
	}

	return ResourceProvider;
}

void UTouchEngineResourceSubsystem::InitializeResourceProvider()
{
	UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();

	if (!TESubsystem || !TESubsystem->IsTouchEngineLibInitialized())
	{
		UE_LOG(LogTouchEngineResourceProvider, Warning, TEXT("Cannot initialize Resource Provider until Touch Engine Lib has been initialized!"))
		return;
	}

	FString RHIName = GDynamicRHI->GetName();

	{
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
	}

	if (ResourceProvider)
	{
		if (!ResourceProvider->Initialize(
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
			}))
		{
			UE_LOG(LogTouchEngineResourceProvider, Error, TEXT("ResourceProvider Initialize failed!"))
		}
	}
	else
	{
		UE_LOG(LogTouchEngineResourceProvider, Warning, TEXT("No ResourceProvider created. Unsupported RHI: %s"), *RHIName)
	}
}