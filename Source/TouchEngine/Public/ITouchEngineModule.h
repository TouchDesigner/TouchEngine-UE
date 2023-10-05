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

#pragma once

#include "CoreMinimal.h"
#include "DynamicRHI.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "TouchEngine/TEResult.h"

namespace UE::TouchEngine
{
	class FTouchResourceProvider;
	
	using FLoadErrorFunc = TFunctionRef<void(const FString&)>;
	using FResultErrorFunc = TFunctionRef<void(const TEResult, const FString&)>;
	
	struct FResourceProviderInitArgs
	{
		FLoadErrorFunc LoadErrorCallback;
		FResultErrorFunc ResultCallback;
	};
	
	DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<FTouchResourceProvider>, FResourceProviderFactory, const FResourceProviderInitArgs&)
	
	class ITouchEngineModule : public IModuleInterface
	{
	public:

		static ITouchEngineModule& Get()
		{
			return FModuleManager::Get().GetModuleChecked<ITouchEngineModule>("TouchEngine");
		}

		/** Used when the module might already have been unloaded */
		static ITouchEngineModule* GetSafe()
		{
			return static_cast<ITouchEngineModule*>(FModuleManager::Get().GetModule("TouchEngine"));
		}

		/** Whether the TouchEngine library was initialized successfully. You can use this to avoid certain calls in error state. */
		virtual bool IsTouchEngineLibInitialized() const = 0;
		
		/** Registers a resource provider for the given RHI */
		virtual void BindResourceProvider(const FString& NameOfRHI, FResourceProviderFactory FactoryDelegate) = 0;
		virtual void UnbindResourceProvider(const FString& NameOfRHI) = 0;

		/** Gets the resource provider for the given RHI */
		virtual TSharedPtr<FTouchResourceProvider> CreateResourceProvider(const FString& NameOfRHI) = 0;
		/** Gets the resource provider for the current configuration of this application's Unreal Engine instance. */
		TSharedPtr<FTouchResourceProvider> CreateResourceProvider() { return CreateResourceProvider(GDynamicRHI->GetName()); }
	};
}
