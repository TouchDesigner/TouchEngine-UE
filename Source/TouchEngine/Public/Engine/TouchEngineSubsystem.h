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
#include "Async/Future.h"
#include "PixelFormat.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchLoadResults.h"
#include "Subsystems/EngineSubsystem.h"
#include "TouchEngineSubsystem.generated.h"

class UToxAsset;
class UTouchEngineInfo;

namespace UE::TouchEngine
{
	struct TOUCHENGINE_API FCachedToxFileInfo
	{
		const FTouchLoadResult LoadResult;
		/** Set to true if the load result is returning previously cached data */
		const bool bWasCached = false;

		static FCachedToxFileInfo MakeFailure(FString ErrorMessage)
		{
			return { FTouchLoadResult::MakeFailure(MoveTemp(ErrorMessage)), false };
		} 
	};
	
}

/** Keeps a global list of loaded tox files. */
UCLASS()
class TOUCHENGINE_API UTouchEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:

	//~ Begin UEngineSubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End UEngineSubsystem Interface

	/**
	 * Gets or loads the params from the given tox file path. Executes the future (possibly immediately) once the data is available.
	 * The Subsystem is used to load Tox files and to cache the values so the details panel could quickly display the values in the Editor UI without having to reload the files everytime.
	 * 
	 * @params AbsoluteOrRelativeToContentFolder A path to the .tox file: either absolute or relative to the project's content folder.
	 * @params LoadTimeoutInSeconds The number of seconds to wait for the load to complete before timing out
	 * @params bForceReload Whether any current data should be discarded and reloaded (useful if the .tox file has changed).
	 */
	TFuture<UE::TouchEngine::FCachedToxFileInfo> GetOrLoadParamsFromTox(UToxAsset* ToxAsset, double LoadTimeoutInSeconds, bool bForceReload = false);

	/** Gives ans answer whether or not the given EPixelFormat is a supported one for this ResourceProvider. The subsystem needs to have a loaded a file for this to be valid */
	bool IsSupportedPixelFormat(EPixelFormat PixelFormat) const;
	bool IsSupportedPixelFormat(TEnumAsByte<ETextureRenderTargetFormat> PixelFormat) const;
	
	bool IsLoaded(const UToxAsset* ToxAsset) const;
	bool IsLoading(const UToxAsset* ToxAsset) const;
	/** Check if the given asset has failed loading. Make sure to check that the file is not loading before calling this function */
	bool HasFailedLoad(const UToxAsset* ToxAsset) const;

	/**
	 * Cache the loaded data from a TouchEngine Component.
	 * Since a component can be playing directly in Editor, it can load a Tox file from its own engine instead of loading it from the subsystem.
	 * This function allows the Subsystem to cache the loaded data from the component.
	 * @param ToxAsset The ToxAsset containing the path to the tox file
	 * @param LoadResult The load result from the Component
	 */
	void CacheLoadedDataFromComponent(UToxAsset* ToxAsset,const UE::TouchEngine::FTouchLoadResult& LoadResult);
	
	/**
	 * Load the Pixel Data from the given TouchEngineInfo.
	 * Since a component can be playing directly in Editor, it can load a Tox file from its own engine instead of loading it from the subsystem.
	 * This would not allow the subsystem to have Pixel Formats information if no file was loaded from the subsystem.
	 */
	void LoadPixelFormats(const UTouchEngineInfo* ComponentEngineInfo);

	TObjectPtr<UTouchEngineInfo> GetTempEngineInfo() const { return EngineForLoading; }
	
private:
	struct FLoadTask
	{
		UToxAsset* ToxAsset;
		TPromise<UE::TouchEngine::FCachedToxFileInfo> Promise;
		double LoadTimeoutInSeconds;
	};
	
	TOptional<FLoadTask> ActiveTask;
	TArray<FLoadTask> TaskQueue;

	TMap<FString, UE::TouchEngine::FTouchLoadResult> CachedFileData;

	// Temporary pointer needed to have IsLoading return true for this asset when it is starting to be processed in GetOrLoadParamsFromTox.
	TWeakObjectPtr<UToxAsset> ToxAssetToStartLoading;

	/** List of Supported EPixelFormat */
	UPROPERTY(Transient)
	TSet<TEnumAsByte<EPixelFormat>> CachedSupportedPixelFormats;

	/** TouchEngine instance used to load items into the details panel */
	UPROPERTY(Transient)
	TObjectPtr<UTouchEngineInfo> EngineForLoading;

	TFuture<UE::TouchEngine::FCachedToxFileInfo> EnqueueOrExecuteLoadTask(UToxAsset* ToxAsset, double LoadTimeoutInSeconds);
	void ExecuteLoadTask(FLoadTask&& LoadTask);
};