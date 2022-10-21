// Copyright © Derivative Inc. 2022

#include "TouchEngineVulkanRHI.h"

#include "ITouchEngineModule.h"
#include "Logging.h"
#include "TouchEngineVulkanResourceProvider.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FTouchEngineVulkanRHI"

namespace UE::TouchEngine::Vulkan
{
	void FTouchEngineVulkanRHI::StartupModule()
	{
		ITouchEngineModule::Get().BindResourceProvider(
			TEXT("Vulkan"),
			FResourceProviderFactory::CreateLambda([](const FResourceProviderInitArgs& Args)
			{
				return MakeVulkanResourceProvider(Args);
			})
		);
	}

	void FTouchEngineVulkanRHI::ShutdownModule()
	{
		ITouchEngineModule::Get().UnbindResourceProvider(TEXT("Vulkan"));
	}
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(UE::TouchEngine::Vulkan::FTouchEngineVulkanRHI, TouchEngineVulkanRHI);