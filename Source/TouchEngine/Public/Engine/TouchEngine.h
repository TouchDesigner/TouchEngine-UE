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
#include "TouchVariables.h"
#include "Logging/MessageLog.h"
#include "TouchEngine/TouchObject.h"
#include "Util/TouchErrorLog.h"
#include "TouchEngine.generated.h"

class UTexture;
class UTexture2D;
class UTouchEngineInfo;
template <typename T> struct TTouchVar;
struct FTouchEngineDynamicVariableStruct;

namespace UE::TouchEngine
{
	class FTouchResourceProvider;
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

	void LoadTox(const FString& ToxPath);
	void Unload();
	
	const FString& GetToxPath() const { return MyToxPath; }

	void CookFrame_GameThread(int64 FrameTime_Mill);
	bool SetCookMode(bool IsIndependent);
	bool SetFrameRate(int64 FrameRate);
	
	FTouchCHOPFull GetCHOPOutputSingleSample(const FString& Identifier);
	FTouchCHOPFull GetCHOPOutputs(const FString& Identifier);
	UTexture2D* GetTOPOutput(const FString& Identifier);
	TTouchVar<bool> GetBooleanOutput(const FString& Identifier);
	TTouchVar<double> GetDoubleOutput(const FString& Identifier);
	TTouchVar<int32_t> GetIntegerOutput(const FString& Identifier);
	TTouchVar<TEString*> GetStringOutput(const FString& Identifier);
	FTouchDATFull GetTableOutput(const FString& Identifier);

	void SetCHOPInputSingleSample(const FString &Identifier, const FTouchCHOPSingleSample& CHOP);
	void SetCHOPInput(const FString& Identifier, const FTouchCHOPFull& CHOP);
	void SetTOPInput(const FString& Identifier, UTexture* Texture);
	void SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op);
	void SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op);
	void SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32_t>>& Op);
	void SetStringInput(const FString& Identifier, TTouchVar<char*>& Op);
	void SetTableInput(const FString& Identifier, FTouchDATFull& Op);

	bool GetDidLoad() const { return MyDidLoad; }

	bool GetIsLoading() const;
	bool GetFailedLoad() const { return MyFailedLoad; }

private:
	
	enum class EFinalClean
	{
		False,
		True
	};
	
	FTouchOnLoadFailed OnLoadFailed;
	FTouchOnParametersLoaded OnParametersLoaded;
	FTouchOnCookFinished OnCookFinished;

	FString FailureMessage;

	FString									MyToxPath;
	TouchObject<TEInstance>					MyTouchEngineInstance = nullptr;
	TSharedPtr<UE::TouchEngine::FTouchResourceProvider> MyResourceProvider = nullptr;;

	TMap<FString, FTouchCHOPSingleSample>	MyCHOPSingleOutputs;
	TMap<FString, FTouchCHOPFull>			MyCHOPFullOutputs;
	FCriticalSection						MyTOPLock;
	TMap<FName, UTexture2D*>				MyTOPOutputs;


	std::atomic<bool>						MyDidLoad = false;
	bool									MyFailedLoad = false;
	bool									MyCooking = false;
	TETimeMode								MyTimeMode = TETimeMode::TETimeInternal;
	float									MyFrameRate = 60.f;
	int64									MyTime = 0;

	bool									MyConfiguredWithTox = false;
	bool									MyLoadCalled = false;
	// TODO DP: This variables are read and written to concurrently -> Data race
	int64									MyNumOutputTexturesQueued = 0, MyNumInputTexturesQueued = 0;

	UE::TouchEngine::FTouchErrorLog ErrorLog;
	
	/** Create a touch engine instance, if none exists, and set up the engine with the tox path. This won't call TEInstanceLoad. */
	bool InstantiateEngineWithToxFile(const FString& ToxPath);

	// Handlers for loading tox
	static void TouchEventCallback_GameOrTouchThread(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale, void* Info);
	void OnInstancedLoaded_GameOrTouchThread(TEInstance* Instance, TEResult Result);
	void LoadInstance_GameOrTouchThread(TEInstance* Instance);
	void OnLoadError_GameOrTouchThread(TEResult Result, const FString& BaseErrorMessage = {});
	
	TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> ProcessTouchVariables(TEInstance* Instance, TEScope Scope);
	void SetDidLoad() { MyDidLoad = true; }

	// Handle linking of vars: there is no restriction on the thread - it can  happen on Game, Touch or Rendering thread!
	static void	LinkValueCallback_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier, void* Info);
	void LinkValue_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier);
	void ProcessLinkTextureValueChanged_AnyThread(const char* Identifier);

	void AllocateLinkedTop(FName ParamName);
	void UpdateLinkedTOP(FName ParamName, UTexture2D* Texture);
	
	void Clear();

	static bool IsInGameOrTouchThread();

	bool OutputResultAndCheckForError(const TEResult Result, const FString& ErrMessage);
};
