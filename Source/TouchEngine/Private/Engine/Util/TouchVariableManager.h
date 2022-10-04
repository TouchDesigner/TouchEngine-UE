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
#include "Engine/TouchEngine.h"
#include "TouchEngine/TouchObject.h"

namespace UE::TouchEngine
{
	class FTouchErrorLog;
	class FTouchResourceProvider;
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FTextureInputUpdateEvent, FName /*Identifier*/);
	
	class FTouchVariableManager
	{
	public:

		FTouchVariableManager(TouchObject<TEInstance> TouchEngineInstance, TSharedPtr<FTouchResourceProvider> ResourceProvider, FTouchErrorLog& ErrorLog);
		
		void AllocateLinkedTop(FName ParamName);
		void UpdateLinkedTOP(FName ParamName, UTexture2D* Texture);
		
		FTouchCHOPFull GetCHOPOutputSingleSample(const FString& Identifier);
		FTouchCHOPFull GetCHOPOutputs(const FString& Identifier);
		UTexture2D* GetTOPOutput(const FString& Identifier);
		TTouchVar<bool> GetBooleanOutput(const FString& Identifier);
		TTouchVar<double> GetDoubleOutput(const FString& Identifier);
		TTouchVar<int32_t> GetIntegerOutput(const FString& Identifier);
		TTouchVar<TEString*> GetStringOutput(const FString& Identifier);
		FTouchDATFull GetTableOutput(const FString& Identifier);
		TArray<FString> GetCHOPChannelNames(const FString& Identifier) const;

		void SetCHOPInputSingleSample(const FString &Identifier, const FTouchCHOPSingleSample& CHOP);
		void SetCHOPInput(const FString& Identifier, const FTouchCHOPFull& CHOP);
		void SetTOPInput(const FString& Identifier, UTexture* Texture);
		void SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op);
		void SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op);
		void SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32_t>>& Op);
		void SetStringInput(const FString& Identifier, TTouchVar<char*>& Op);
		void SetTableInput(const FString& Identifier, FTouchDATFull& Op);

		FTextureInputUpdateEvent& OnStartInputTextureUpdate() { return OnStartInputTextureUpdateDelegate; }
		FTextureInputUpdateEvent& OnFinishInputTextureUpdate() { return OnFinishInputTextureUpdateDelegate; }
		
	private:
		
		TouchObject<TEInstance>	TouchEngineInstance;
		TSharedPtr<FTouchResourceProvider> ResourceProvider;
		FTouchErrorLog& ErrorLog;
		
		FCriticalSection TOPLock;
		TMap<FString, FTouchCHOPSingleSample> CHOPSingleOutputs;
		TMap<FString, FTouchCHOPFull> CHOPFullOutputs;
		TMap<FName, UTexture2D*> TOPOutputs;

		FTextureInputUpdateEvent OnStartInputTextureUpdateDelegate;
		FTextureInputUpdateEvent OnFinishInputTextureUpdateDelegate;
	};
}


