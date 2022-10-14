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
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchVariables.h"
#include "Util/TouchErrorLog.h"

#include "Engine/Util/TouchVariableManager.h"

#include "TouchEngine/TouchObject.h"
#include "TouchEngine.generated.h"

class UTexture;
class UTexture2D;
class UTouchEngineInfo;
template <typename T> struct TTouchVar;
struct FTouchEngineDynamicVariableStruct;

namespace UE::TouchEngine
{
	class FTouchFrameCooker;
	class FTouchVariableManager;
	class FTouchResourceProvider;
	
	struct FCookFrameRequest;
	struct FCookFrameResult;
}

DECLARE_MULTICAST_DELEGATE(FTouchOnLoadComplete);
DECLARE_MULTICAST_DELEGATE_OneParam(FTouchOnLoadFailed, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTouchOnParametersLoaded, const TArray<FTouchEngineDynamicVariableStruct>&, const TArray<FTouchEngineDynamicVariableStruct>&);
DECLARE_MULTICAST_DELEGATE(FTouchOnCookFinished);

UCLASS()
class TOUCHENGINE_API UTouchEngine : public UObject
{
	GENERATED_BODY()
	friend class UTouchEngineInfo;
public:

	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
	//~ End UObject Interface
	
	void LoadTox(const FString& InToxPath);
	void Unload();

	TFuture<UE::TouchEngine::FCookFrameResult> CookFrame_GameThread(const UE::TouchEngine::FCookFrameRequest& CookFrameRequest);
	void SetCookMode(bool bIsIndependent);
	bool SetFrameRate(int64 FrameRate);
	
	FTouchCHOPFull GetCHOPOutputSingleSample(const FString& Identifier) const { return ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetCHOPOutputSingleSample(Identifier) : FTouchCHOPFull{}; }
	FTouchCHOPFull GetCHOPOutputs(const FString& Identifier) const { return ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetCHOPOutputs(Identifier) : FTouchCHOPFull{}; }
	UTexture2D* GetTOPOutput(const FString& Identifier) const { return ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetTOPOutput(Identifier) : nullptr; }
	TTouchVar<bool> GetBooleanOutput(const FString& Identifier) const { return ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetBooleanOutput(Identifier) : TTouchVar<bool>{}; }
	TTouchVar<double> GetDoubleOutput(const FString& Identifier) const { return ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetDoubleOutput(Identifier) : TTouchVar<double>{}; }
	TTouchVar<int32_t> GetIntegerOutput(const FString& Identifier) const { return ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetIntegerOutput(Identifier) : TTouchVar<int32_t>{}; }
	TTouchVar<TEString*> GetStringOutput(const FString& Identifier) const { return ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetStringOutput(Identifier) : TTouchVar<TEString*>{}; }
	FTouchDATFull GetTableOutput(const FString& Identifier) const { return ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetTableOutput(Identifier) : FTouchDATFull{}; }
	TArray<FString> GetCHOPChannelNames(const FString& Identifier) const { return ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetCHOPChannelNames(Identifier) : TArray<FString>{}; }

	void SetCHOPInputSingleSample(const FString& Identifier, const FTouchCHOPSingleSample& CHOP) { if (ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetCHOPInputSingleSample(Identifier, CHOP); } }
	void SetCHOPInput(const FString& Identifier, const FTouchCHOPFull& CHOP) { if (ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetCHOPInput(Identifier, CHOP); } }
	void SetTOPInput(const FString& Identifier, UTexture* Texture) { if (ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetTOPInput(Identifier, Texture); } }
	void SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op) { if (ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetBooleanInput(Identifier, Op); } }
	void SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op) { if (ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetDoubleInput(Identifier, Op); } }
	void SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32_t>>& Op) { if (ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetIntegerInput(Identifier, Op); } }
	void SetStringInput(const FString& Identifier, TTouchVar<const char*>& Op) { if (ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetStringInput(Identifier, Op); } }
	void SetTableInput(const FString& Identifier, FTouchDATFull& Op) { if (ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetTableInput(Identifier, Op); } }

	const FString& GetToxPath() const { return ToxPath; }
	bool IsLoading() const;
	bool HasAttemptedToLoad() const { return bDidLoad; }
	bool HasFailedToLoad() const { return bFailedLoad; }

private:

	struct FTouchResources
	{
		TouchObject<TEInstance> TouchEngineInstance = nullptr;
		
		/** Helps print messages to the message log. */
		TSharedPtr<UE::TouchEngine::FTouchErrorLog> ErrorLog;
	
		/** Created when the TE is spun up. */
		TSharedPtr<UE::TouchEngine::FTouchResourceProvider> ResourceProvider;
		/** Only valid when there is a valid TE running. */
		TSharedPtr<UE::TouchEngine::FTouchVariableManager> VariableManager;
		/** Handles cooking frames */
		TSharedPtr<UE::TouchEngine::FTouchFrameCooker> FrameCooker;
	};
	
	FTouchOnLoadFailed OnLoadFailed;
	FTouchOnParametersLoaded OnParametersLoaded;

	FString FailureMessage;

	FString	ToxPath;

	bool bIsDestroyingTouchEngine = false;
	std::atomic<bool> bDidLoad = false;
	bool bFailedLoad = false;
	bool bConfiguredWithTox = false;
	bool bLoadCalled = false;

	float TargetFrameRate = 60.f;
	TETimeMode TimeMode = TETimeInternal;

	/** Systems that are only valid while there is a Touch Engine (being) loaded. */
	FTouchResources TouchResources;
	
	/** Create a touch engine instance, if none exists, and set up the engine with the tox path. This won't call TEInstanceLoad. */
	bool InstantiateEngineWithToxFile(const FString& InToxPath);

	// Handlers for loading tox
	static void TouchEventCallback_AnyThread(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale, void* Info);
	void OnInstancedLoaded_AnyThread(TEInstance* Instance, TEResult Result);
	void FinishLoadInstance_AnyThread(TEInstance* Instance);
	void OnLoadError_AnyThread(TEResult Result, const FString& BaseErrorMessage = {});
	TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> ProcessTouchVariables(TEInstance* Instance, TEScope Scope);
	void SetDidLoad() { bDidLoad = true; }

	// Handle linking of vars: there is no restriction on the thread - it can  happen on Game, Touch or Rendering thread!
	static void	LinkValueCallback_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier, void* Info);
	void LinkValue_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier);
	void ProcessLinkTextureValueChanged_AnyThread(const char* Identifier);
	
	void Clear_GameThread(bool bIsThisGettingDestroyed = false);

	bool OutputResultAndCheckForError(const TEResult Result, const FString& ErrMessage);
};
