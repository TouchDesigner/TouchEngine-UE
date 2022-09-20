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
#include "TouchEngineResourceProvider.h"
#include "Interfaces/IPluginManager.h"
#include "TouchEngine/TEResult.h"

namespace UE::TouchEngine
{
	void FTouchEngineModule::StartupModule()
	{
		LoadTouchEngineLib();
	}

	void FTouchEngineModule::ShutdownModule()
	{
		ReleaseAllResourceProviders();
		
		UnloadTouchEngineLib();
	}
	
	void FTouchEngineModule::BindResourceProvider(const FString& NameOfRHI, FResourceProviderFactory FactoryDelegate)
	{
		if (ensure(!ResourceProviders.Contains(NameOfRHI) && FactoryDelegate.IsBound()))
		{
			ResourceProviders.Add(NameOfRHI, { FactoryDelegate });
		}
	}

	bool FTouchEngineModule::IsTouchEngineLibInitialized() const
	{
		return TouchEngineLibHandle != nullptr;
	}

	void FTouchEngineModule::UnbindResourceProvider(const FString& NameOfRHI)
	{
		FResourceProviderData Data;
		if (ResourceProviders.RemoveAndCopyValue(NameOfRHI, Data))
		{
			ReleaseResourceProvider(Data.Instance.ToSharedRef());
		}
	}

	TSharedPtr<FTouchEngineResourceProvider> FTouchEngineModule::GetResourceProvider(const FString& NameOfRHI)
	{
		if (FResourceProviderData* ProviderData = ResourceProviders.Find(NameOfRHI)
			; ensure(IsTouchEngineLibInitialized()) && ensure(ProviderData))
		{
			return InitResourceProvider(*ProviderData);
		}

		UE_LOG(LogTouchEngine, Error, TEXT("RHI %s is unsupported"), *NameOfRHI);
		return nullptr;
	}

	TSharedPtr<FTouchEngineResourceProvider> FTouchEngineModule::InitResourceProvider(FResourceProviderData& ProviderData)
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
		ProviderData.Instance = ProviderData.Factory.Execute(Args);
		UE_CLOG(!ProviderData.Instance, LogTouchEngine, Error, TEXT("ResourceProvider Initialize failed"))

		return ProviderData.Instance;
	}

	void FTouchEngineModule::ReleaseResourceProvider(TSharedRef<FTouchEngineResourceProvider> Instance)
	{
		ENQUEUE_RENDER_COMMAND(UTouchEngineResourceSubsystem_Deinitialize)(
			[ProviderInstance = MoveTemp(Instance)](FRHICommandListImmediate& RHICmdList)
			{
				ProviderInstance->Release_RenderThread();
			}
		);
	}

	void FTouchEngineModule::ReleaseAllResourceProviders()
	{
		for (auto Pair : ResourceProviders)
		{
			if (Pair.Value.Instance)
			{
				ReleaseResourceProvider(Pair.Value.Instance.ToSharedRef());
				Pair.Value.Instance.Reset();
			}
		}
	}

	void FTouchEngineModule::LoadTouchEngineLib()
	{
		const FString BasePath = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("TouchEngine"))->GetBaseDir(), TEXT("/Binaries/ThirdParty/Win64"));
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
