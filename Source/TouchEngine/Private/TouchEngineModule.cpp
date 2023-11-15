/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "TouchEngineModule.h"

#include "Logging.h"
#if WITH_EDITOR
#include "MessageLogModule.h"
#endif
#include "Interfaces/IPluginManager.h"
#include "Rendering/TouchResourceProvider.h"
#include "TouchEngine/TEResult.h"

#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "TouchEngineModule"

namespace UE::TouchEngine
{
	void FTouchEngineModule::StartupModule()
	{
		LoadTouchEngineLib();

#if WITH_EDITOR
		// Register the Message Log Category
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		FMessageLogInitializationOptions InitOptions;
		InitOptions.bShowPages = true;
		InitOptions.bAllowClear = true;
		InitOptions.bShowFilters = true;
		MessageLogModule.RegisterLogListing(MessageLogName, LOCTEXT("TouchEngineLog", "TouchEngine"), InitOptions);
#endif
	}

	void FTouchEngineModule::ShutdownModule()
	{
		ResourceFactories.Reset();
		UnloadTouchEngineLib();

#if WITH_EDITOR
		// Unregister the Message Log Category
		if (FModuleManager::Get().IsModuleLoaded("MessageLog"))
		{
			FMessageLogModule& MessageLogModule = FModuleManager::GetModuleChecked<FMessageLogModule>("MessageLog");
			MessageLogModule.UnregisterLogListing(MessageLogName);
		}
#endif
	}
	
	void FTouchEngineModule::BindResourceProvider(const FString& NameOfRHI, FResourceProviderFactory FactoryDelegate)
	{
		if (ensure(!ResourceFactories.Contains(NameOfRHI) && FactoryDelegate.IsBound()))
		{
			ResourceFactories.Add(NameOfRHI, { FactoryDelegate });
		}
	}

	bool FTouchEngineModule::IsTouchEngineLibInitialized() const
	{
		return TouchEngineLibHandle != nullptr;
	}

	void FTouchEngineModule::UnbindResourceProvider(const FString& NameOfRHI)
	{
		ResourceFactories.Remove(NameOfRHI);
	}

	TSharedPtr<FTouchResourceProvider> FTouchEngineModule::CreateResourceProvider(const FString& NameOfRHI)
	{
		if (const FResourceProviderFactory* Factory = ResourceFactories.Find(NameOfRHI)
			; ensure(IsTouchEngineLibInitialized()) && ensure(Factory))
		{
			auto OnLoadError = [](const FString& Error)
			{
				UE_LOG(LogTouchEngine, Error, TEXT("ResourceProvider load error: %s"), *Error)
			};
			auto OnResultError = [](const TEResult Result, const FString& Error)
			{
				if (Result != TEResultSuccess)
				{
					UE_LOG(LogTouchEngine, Error, TEXT("ResourceProvider failed to create context: %s"), *Error)
				}
			};
			
			const FResourceProviderInitArgs Args{ OnLoadError, OnResultError };
			return Factory->Execute(Args);
		}

		UE_LOG(LogTouchEngine, Error, TEXT("RHI %s is unsupported"), *NameOfRHI);
		return nullptr;
	}

	void FTouchEngineModule::LoadTouchEngineLib()
	{
#if WITH_EDITOR
		const FString BasePath = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("TouchEngine"))->GetBaseDir(), TEXT("/Binaries/ThirdParty/Win64"));
#else
		const FString BasePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("/Binaries/Win64"));
#endif
		const FString FullPathToDLL = FPaths::Combine(BasePath, TEXT("TouchEngine.dll"));
		if (!FPaths::FileExists(FullPathToDLL))
		{
			UE_LOG(LogTouchEngine, Error, TEXT("Invalid path to TouchEngine.dll: %s"), *FullPathToDLL);
			return;
		}
		
		FPlatformProcess::PushDllDirectory(*BasePath);
		TouchEngineLibHandle = FPlatformProcess::GetDllHandle(*FullPathToDLL);
		FPlatformProcess::PopDllDirectory(*BasePath);
		
		UE_CLOG(!IsTouchEngineLibInitialized(), LogTouchEngine, Error, TEXT("Failed to load TouchEngine library: %s"), *FullPathToDLL);
	}

	void FTouchEngineModule::UnloadTouchEngineLib()
	{
		if (IsTouchEngineLibInitialized())
		{
			FPlatformProcess::FreeDllHandle(TouchEngineLibHandle);
			TouchEngineLibHandle = nullptr;
		}
	}
}

IMPLEMENT_MODULE(UE::TouchEngine::FTouchEngineModule, TouchEngine)

#undef LOCTEXT_NAMESPACE