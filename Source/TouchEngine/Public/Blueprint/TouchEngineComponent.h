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
#include "Components/ActorComponent.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "Engine/TouchEngine.h"
#include "TouchEngineComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTouchEngineComponent, Log, All)

class UTouchEngineInfo;
class UToxAsset;

namespace UE::TouchEngine
{
	struct FCookFrameResult;
}

DECLARE_MULTICAST_DELEGATE(FOnToxLoaded_Native)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToxLoaded);

DECLARE_MULTICAST_DELEGATE(FOnToxReset_Native);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToxReset);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnToxFailedLoad_Native, const FString& /*ErrorMessage*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToxFailedLoad, const FString&, ErrorMessage);

DECLARE_MULTICAST_DELEGATE(FOnToxUnloaded_Native)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToxUnloaded);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSetInputs);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOutputsReceived);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBeginPlay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEndPlay);

/*
* The different cook modes the TouchEngine component can run in
*/
UENUM(BlueprintType)
enum class ETouchEngineCookMode : uint8
{
	Synchronized = 0			UMETA(DisplayName = "Synchronized"),
	DelayedSynchronized = 1		UMETA(DisplayName = "Delayed Synchronized"),
	Independent = 2				UMETA(DisplayName = "Independent"),
	Max							UMETA(Hidden)
};

/*
* The different times the TouchEngine component will set / get variables from the TouchEngine instance
*/
UENUM(BlueprintType)
enum class ETouchEngineSendMode : uint8
{
	EveryFrame = 0		UMETA(DisplayName = "Every Frame"),
	OnAccess = 1		UMETA(DisplayName = "On Access"),
	Max					UMETA(Hidden)
};

/*
* Adds a TouchEngine instance to an object.
*/
UCLASS(Blueprintable, meta = (DisplayName = "TouchEngine Component"))
class TOUCHENGINE_API UTouchEngineComponentBase : public UActorComponent
{
	GENERATED_BODY()
	friend class FTouchEngineDynamicVariableStructDetailsCustomization;
public:
	
	/************** Vars **************/

	/** Our TouchEngine Info */
	UPROPERTY()
	TObjectPtr<UTouchEngineInfo> EngineInfo;

	/** Path to the Tox File to load. It is relative to the content directory */
	UPROPERTY()
	FString ToxFilePath_DEPRECATED;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File")
	TObjectPtr<UToxAsset> ToxAsset;

	/** Mode for component to run in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File")
	ETouchEngineCookMode CookMode = ETouchEngineCookMode::Independent;

	/** Mode for the component to set and get variables */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File")
	ETouchEngineSendMode SendMode = ETouchEngineSendMode::EveryFrame;

	/** TouchEngine framerate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tox File", meta = (DisplayName = "TE Frame Rate"))
	int64 TEFrameRate = 60;

	/** Multiplier applied to delta time before sending to TouchEngine */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tox File")
	int32 TimeScale = 10000;

	/** Whether or not to start the TouchEngine immediately on begin play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File", meta = (DisplayAfter="bAllowRunningInEditor"))
	bool LoadOnBeginPlay = true;

	/** Container for all dynamic variables */
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault), Category = "Tox File")
	FTouchEngineDynamicVariableContainer DynamicVariables;

	UPROPERTY()
	FString ErrorMessage;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Tox File", meta = (DisplayAfter="ToxAsset"))
	bool bAllowRunningInEditor = false;
#endif

	UTouchEngineComponentBase();

	/** Reloads the currently loaded tox file */
	UFUNCTION(BlueprintCallable, Category = "TouchEngine|States")
	void LoadTox(bool bForceReloadTox = false);

	/** Checks whether the component already has a tox file loaded */
	bool IsLoaded() const;

	/** Checks if the TouchEngine has been created, but has not yet loaded a tox file */
	bool IsLoading() const;

	/** Checks whether the component has failed to load a tox file */
	bool HasFailedLoad() const;

	/** Gets the path to the tox file relative to project dir */
	FString GetFilePath() const;

	/** Starts and creates the TouchEngine instance */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Start TouchEngine"), Category = "TouchEngine|States")
	void StartTouchEngine();

	/** Stops and deletes the TouchEngine instance */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Stop TouchEngine"), Category = "TouchEngine|States")
	void StopTouchEngine();

	/** Should UI, player, or other means be allowed to start the TouchEngine */
	UFUNCTION(BlueprintCallable, Category = "TouchEngine|States")
	bool CanStart() const;

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|States")
	bool IsRunning() const;

	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface
protected:
	virtual void OnRegister() override;

public:
	//~ Begin UActorComponent Interface
	virtual void PostLoad() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnComponentCreated() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	virtual void ExportCustomProperties(FOutputDevice& Out, uint32 Indent) override;
	virtual void ImportCustomProperties(const TCHAR* Buffer, FFeedbackContext* Warn) override;
	//~ End UActorComponent Interface

	FOnToxLoaded_Native& GetOnToxLoaded() { return OnToxLoaded_Native; }
	FOnToxReset_Native& GetOnToxReset() { return OnToxReset_Native; }
	FOnToxFailedLoad_Native& GetOnToxFailedLoad() { return OnToxFailedLoad_Native; }
	FOnToxUnloaded_Native& GetOnToxUnloaded() { return OnToxUnloaded_Native; }

protected:
	
	/** Called when the TouchEngine instance loads the tox file */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FOnToxLoaded OnToxLoaded;
	FOnToxLoaded_Native OnToxLoaded_Native;

	/** Called when the TouchEngine instance is reset, and data is cleared */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FOnToxReset OnToxReset;
	FOnToxReset_Native OnToxReset_Native;

	/** Called when the TouchEngine instance fails to load the tox file */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FOnToxFailedLoad OnToxFailedLoad;
	FOnToxFailedLoad_Native OnToxFailedLoad_Native;
	
	/** Called when the TouchEngine instance unloads the tox file, it might be called multiple times and it is also called after OnToxFailedLoad */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FOnToxUnloaded OnToxUnloaded;
	FOnToxUnloaded_Native OnToxUnloaded_Native;

	/** Called before sending the inputs to the TouchEngine */
	UPROPERTY(BlueprintAssignable, Category = "Components|Parameters")
	FSetInputs OnSetInputs;

	/** Called after receiving the outputs from the TouchEngine */
	UPROPERTY(BlueprintAssignable, Category = "Components|Parameters")
	FOnOutputsReceived OnOutputsReceived;

	/**
	 * Begins Play for the component that also fires in the Editor.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation", meta=(DisplayName = "Begin Play"))
	FBeginPlay CustomBeginPlay;
	/**
	 * End Play for the component that also fires in the Editor.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation", meta=(DisplayName = "End Play"))
	FEndPlay CustomEndPlay;
	
	void BroadcastOnToxLoaded(bool bInSkipBlueprintEvent = false);
	void BroadcastOnToxReset(bool bInSkipUIEvent = false);
	void BroadcastOnToxFailedLoad(const FString& Error, bool bInSkipUIEvent = false);
	void BroadcastOnToxUnloaded(bool bInSkipUIEvent = false);
	void BroadcastOnSetInputs();
	void BroadcastOnOutputsReceived() const;

	void BroadcastCustomBeginPlay();
	void BroadcastCustomEndPlay();
	
private:

	FDelegateHandle ParamsLoadedDelegateHandle;
	FDelegateHandle LoadFailedDelegateHandle;
	FDelegateHandle BeginFrameDelegateHandle;

	/** Set if a frame cooking request is in progress. Used for waiting. */
	TOptional<TFuture<UE::TouchEngine::FCookFrameResult>> PendingCookFrame;
	
	void StartNewCook(float DeltaTime);

	// Called at the beginning of a frame.
	void OnBeginFrame();

	void LoadToxInternal(bool bForceReloadTox, bool bInSkipBlueprintEvents = false);
	/** Attempts to create an engine instance for this object. Should only be used for in world objects. */
	TFuture<UE::TouchEngine::FTouchLoadResult> LoadToxThroughComponentInstance();
	/** Loads or gets the cached data from the loading subsystem */
	TFuture<UE::TouchEngine::FTouchLoadResult> LoadToxThroughCache(bool bForceReloadTox);
	
	void CreateEngineInfo();

	FString GetAbsoluteToxPath() const;

	void VarsSetInputs();
	void VarsGetOutputs();

	bool ShouldUseLocalTouchEngine() const;

	enum class EReleaseTouchResources
	{
		/** Completely destroys the TE process - TERelease will be called on the instance */
		KillProcess,
		/** Just calls TEInstanceUnload so the engine can be reused later. */
		Unload
	};
	
	/** Shared logic for releasing the touch engine resources. */
	void ReleaseResources(EReleaseTouchResources ReleaseMode);

	/** Used to skip Blueprint Events when broadcasting */
	bool bSkipBlueprintEvents;
};