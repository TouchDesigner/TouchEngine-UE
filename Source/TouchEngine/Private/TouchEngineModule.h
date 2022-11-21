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
#include "ITouchEngineModule.h"

namespace UE::TouchEngine
{
	class FTouchEngineModule : public ITouchEngineModule
	{
	public:

		//~ Begin IModuleInterface Interface
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;
		//~ End IModuleInterface Interface

		//~ Begin ITouchEngineModule Interface
		virtual bool IsTouchEngineLibInitialized() const override;
		virtual void BindResourceProvider(const FString& NameOfRHI, FResourceProviderFactory FactoryDelegate) override;
		virtual void UnbindResourceProvider(const FString& NameOfRHI) override;
		virtual TSharedPtr<FTouchResourceProvider> CreateResourceProvider(const FString& NameOfRHI) override;
		//~ End ITouchEngineModule Interface

	private:

		TMap<FString, FResourceProviderFactory> ResourceFactories;
		
		/** Result of loading lib */
		void* TouchEngineLibHandle = nullptr;
		
		void LoadTouchEngineLib();
		void UnloadTouchEngineLib();
	};

}
