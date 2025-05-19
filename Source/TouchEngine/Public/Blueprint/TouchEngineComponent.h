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
#include "Engine/Util/CookFrameData.h"
#include "TouchEngineComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTouchEngineComponent, Display, All)

class UTouchEngineInfo;
class UToxAsset;

namespace UE::TouchEngine
{
	struct FCachedToxFileInfo;
	struct FCookFrameResult;
}


DECLARE_MULTICAST_DELEGATE(FOnToxStartedLoading_Native)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToxStartedLoading);

DECLARE_MULTICAST_DELEGATE(FOnToxLoaded_Native)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToxLoaded);

DECLARE_MULTICAST_DELEGATE(FOnToxReset_Native);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToxReset);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnToxFailedLoad_Native, const FString& /*ErrorMessage*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToxFailedLoad, const FString&, ErrorMessage);

DECLARE_MULTICAST_DELEGATE(FOnToxUnloaded_Native)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToxUnloaded);


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartFrame, const FTouchEngineInputFrameData&, FrameData);
// The comment after FrameData was the only way found to give comments to event parameters
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEndFrame,	bool, IsSuccessful,	ECookFrameResult, Result, const FTouchEngineOutputFrameData&, FrameData
	/* @param IsSuccessful Returns true if Result == Success and if FrameData.WasFrameDropped == false */);
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
* The different times the TouchEngine component will set / get variables from the TouchEngine instance. todo: to deprecate
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
	
	/** Our TouchEngine Info */
	UPROPERTY(Transient)
	TObjectPtr<UTouchEngineInfo> EngineInfo;

	/** Path to the Tox File to load. It is relative to the content directory */
	UPROPERTY()
	FString ToxFilePath_DEPRECATED;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File")
	TObjectPtr<UToxAsset> ToxAsset;

	/** Mode for component to run in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File")
	ETouchEngineCookMode CookMode = ETouchEngineCookMode::Independent;

	/** Mode for the component to set and get variables. Deprecated as a there shouldn't be a send mode and we should just  */
	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage="There shouldn't be the need for a SendMode available to the user, the backend of the component will deal with this."))
	ETouchEngineSendMode SendMode_DEPRECATED = ETouchEngineSendMode::EveryFrame;

	/** TouchEngine framerate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tox File", meta = (DisplayName = "TE Frame Rate"))
	int64 TEFrameRate = 60;

	/** Multiplier applied to delta time before sending to TouchEngine. Deprecated as it shouldn't be set by the user */
	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage="There shouldn't be the need for the TimeScale to be adjustable by the user, it is automatically computed by the backend."))
	int32 TimeScale_DEPRECATED = 10000;

	/** Whether or not to start the TouchEngine immediately on begin play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File", meta = (DisplayAfter="bAllowRunningInEditor"))
	bool LoadOnBeginPlay = true;
	
	/**
	 * Sets the maximum number of cooks we will enqueue while another cook is processing by TouchEngine. This happens in DelayedSynchronized and Independent modes.
	 * When the limit is reached, older cooks will be discarded.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File", meta=(ClampMin=1, UIMin=1, UIMax=30))
	int32 InputBufferLimit = 10;

	/** Container for all dynamic variables */
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault), Category = "Tox File")
	FTouchEngineDynamicVariableContainer DynamicVariables;
	
	UPROPERTY()
	FString ErrorMessage;

#if WITH_EDITORONLY_DATA
	/** When set to true, this TouchEngine Component will load and start running in the Unreal Editor. Does not work on packaged games. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tox File", meta = (DisplayAfter="ToxAsset"))
	bool bAllowRunningInEditor = false;
#endif

	/** If set to true, the component will pause Unreal Editor every time every time a frame was done processing. Useful for debugging. Only has an effect in Editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File", AdvancedDisplay)
	bool bPauseOnEndFrame = false;
	
	/**
	 * To export textures to TouchEngine, we need to create temporary textures to copy into and share with TouchEngine.
	 * For better performances, these temporary textures are returned to a texture pool once done to be reused.
	 * This parameters sets how many textures can be kept in the pool.
	 * This will only have an effect if changed before loading a tox file.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File", AdvancedDisplay, meta=(ClampMin=1, UIMin=1, UIMax=30))
	int32 ExportedTexturePoolSize = 20;
	/**
	 * To import textures from TouchEngine, we need to create Frame UTextures into which we will copy the textures returned by TouchEngine.
	 * For better performances, these Frame UTextures are returned to a texture pool once done to be reused.
	 * This parameters sets how many Frame UTextures can be kept in the pool.
	 * This will only have an effect if changed before loading a tox file.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File", AdvancedDisplay, meta=(ClampMin=1, UIMin=1, UIMax=30))
	int32 ImportedTexturePoolSize = 20;
	
	/**
	 * The number of second to wait for the tox file to load before cancelling.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File", AdvancedDisplay, meta=(ClampMin=0.01, UIMin=5, UIMax=60, ForceUnits="s"))
	double ToxLoadTimeout = 30.0;

	/**
	 * The number of second to wait for a cook before cancelling it.
	 * If the cook is not done by that time, the component will raise a TouchEngineCookTimeout error and will continue running.
	 * Be careful of not using too high values in Synchronized mode as we are stalling the GameThread, the application could become unusable
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tox File", AdvancedDisplay, meta=(ClampMin=0.01, UIMin=0.01, UIMax=0.5, ForceUnits="s"))
	double CookTimeout = 0.3;
	
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
	
	/** Checks whether the component is ready to load, with the Engine being either Unloaded or NoTouchEngine */
	bool IsReadyToLoad() const;

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

	/**
	 * Keeps the frame texture retrieved from Get TouchEngine Output.
	 * When retrieving a TOP, Get TouchEngine Output returns a temporary texture that will go back into a texture pool after another value has been retrieved from TouchEngine, for performance.
	 * If the texture needs to be kept alive for longer, this function needs to be called to ensure the frame texture is removed from the pool and will not be overriden.
	 * @param FrameTexture The Texture retrieved by Get TouchEngine Output
	 * @param Texture if successful, returns the texture made permanent (will be the same pointer as the Frame Texture, this is for ease of use in Blueprint), otherwise returns nullptr
	 * @return true if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "TouchEngine|TOP")
	bool KeepFrameTexture(UTexture2D* FrameTexture, UTexture2D*& Texture);
	
	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PreEditUndo() override;
	virtual void PostEditUndo() override;
	virtual void PostReinitProperties() override;

private:
	struct FTEPreUndoValues
	{
		FTouchEngineDynamicVariableContainer DynamicVariables;
		bool bAllowRunningInEditor; //to know if it was ticked or unticked
		TObjectPtr<UTouchEngineInfo> EngineInfo; // otherwise it might disappear between undo and redo
	};
	FTEPreUndoValues PreUndoValues = {};

	void HandleAllowRunningInEditorChanged();
#endif
	//~ End UObject Interface
protected:
	//~ Begin UActorComponent Interface
	virtual void OnRegister() override;
public:
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	virtual void ExportCustomProperties(FOutputDevice& Out, uint32 Indent) override;
	virtual void ImportCustomProperties(const TCHAR* Buffer, FFeedbackContext* Warn) override;
	virtual void PostEditImport() override;
	//~ End UActorComponent Interface
	
	FOnToxStartedLoading_Native& GetOnToxStartedLoading() { return OnToxStartedLoading_Native; }
	FOnToxLoaded_Native& GetOnToxLoaded() { return OnToxLoaded_Native; }
	FOnToxReset_Native& GetOnToxReset() { return OnToxReset_Native; }
	FOnToxFailedLoad_Native& GetOnToxFailedLoad() { return OnToxFailedLoad_Native; }
	FOnToxUnloaded_Native& GetOnToxUnloaded() { return OnToxUnloaded_Native; }

protected:
	/** Called when the TouchEngine instance starts to load the tox file */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FOnToxStartedLoading OnToxStartedLoading;
	FOnToxStartedLoading_Native OnToxStartedLoading_Native;
	
	/** Called when the TouchEngine instance finished loading the tox file */
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
	FOnStartFrame OnStartFrame;

	/** Called after receiving the outputs from the TouchEngine */
	UPROPERTY(BlueprintAssignable, Category = "Components|Parameters")
	FOnEndFrame OnEndFrame;

	/** Begins Play for the component that also fires in the Editor. */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation", meta=(DisplayName = "Begin Play"))
	FBeginPlay CustomBeginPlay;
	/** End Play for the component that also fires in the Editor. */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation", meta=(DisplayName = "End Play"))
	FEndPlay CustomEndPlay;
	
	void BroadcastOnToxStartedLoading(bool bInSkipBlueprintEvent = false);
	void BroadcastOnToxLoaded(bool bInSkipBlueprintEvent = false);
	void BroadcastOnToxReset(bool bInSkipBlueprintEvent = false);
	void BroadcastOnToxFailedLoad(const FString& Error, bool bInSkipBlueprintEvent = false);
	void BroadcastOnToxUnloaded(bool bInSkipBlueprintEvent = false);
	void BroadcastOnStartFrame(const FTouchEngineInputFrameData& FrameData) const;
	void BroadcastOnEndFrame(ECookFrameResult Result, const FTouchEngineOutputFrameData& FrameData) const;

	void BroadcastCustomBeginPlay() const;
	void BroadcastCustomEndPlay() const;

#if WITH_EDITOR
	void OnToxStartedLoadingThroughSubsystem(UToxAsset* ReloadedToxAsset);
	void OnToxLoadedThroughSubsystem(UToxAsset* ReloadedToxAsset, const UE::TouchEngine::FCachedToxFileInfo& LoadResult);
#endif
	
private:

	FDelegateHandle ParamsLoadedDelegateHandle;
	FDelegateHandle LoadFailedDelegateHandle;
	
	void StartNewCook(double TimeInSeconds);
	void OnCookFinished(const UE::TouchEngine::FCookFrameResult& CookFrameResult);

	/**
	 * Internal function to load the current ToxAsset
	 * @param bForceReloadTox If set to true, we will force the asset to be reloaded, either through the Subsystem if in Editor, or through the component
	 * @param bInSkipBlueprintEvents If set to true, the BP events will not be fired. Should only be done when the component is loaded
	 * @param bForceReloadFromCache If set to true and the function decides to not reload the tox file, we will gather the latest data from the cache.
	 * This is useful in PostLoad if the subsystem already has data saved, which would stop the component from reloading it.
	 */
	void LoadToxInternal(bool bForceReloadTox, bool bInSkipBlueprintEvents = false, bool bForceReloadFromCache = false);
	void HandleToxLoaded(const UE::TouchEngine::FTouchLoadResult& LoadResult, bool bLoadedLocalTouchEngine, bool bInSkipBlueprintEvents);
	/* Copies the Default, Min, Max and Dropdown values of all input DynamicVariables from the given load result */
	void EnsureToxMetadataIsSet(const UE::TouchEngine::FTouchLoadResult& LoadResult);
	/** Attempts to create an engine instance for this object. Should only be used for in world objects. */
	TFuture<UE::TouchEngine::FTouchLoadResult> LoadToxThroughComponentInstance();
	/** Loads or gets the cached data from the loading subsystem */
	TFuture<UE::TouchEngine::FCachedToxFileInfo> LoadToxThroughCache(bool bForceReloadTox);
	
	void CreateEngineInfo();

	FString GetAbsoluteToxPath() const;
	
	bool ShouldUseLocalTouchEngine() const;

	enum class EReleaseTouchResources
	{
		/** Completely destroys the TE process - TERelease will be called on the instance */
		KillProcess,
		/** Just calls TEInstanceUnload so the engine can be reused later. */
		Unload
	};
	
	/** Shared logic for releasing the TouchEngine resources. */
	void ReleaseResources(EReleaseTouchResources ReleaseMode);
};