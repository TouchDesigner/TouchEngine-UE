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
#include "PixelFormat.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchLoadResults.h"
#include "ToxDelegateInfo.h"
#include "Subsystems/EngineSubsystem.h"
#include "TouchEngineSubsystem.generated.h"

class UTouchEngineInfo;

namespace UE::TouchEngine
{
	struct TOUCHENGINE_API FCachedToxFileInfo
	{
		FTouchLoadResult LoadResult;

		static FCachedToxFileInfo MakeFailure(FString ErrorMessage)
		{
			return { FTouchLoadResult::MakeFailure(MoveTemp(ErrorMessage)) };
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
	 * 
	 * @params AbsoluteOrRelativeToContentFolder A path to the .tox file: either absolute or relative to the project's content folder.
	 * @params bForceReload Whether any current data should be discarded and reloaded (useful if the .tox file has changed).
	 */
	TFuture<UE::TouchEngine::FCachedToxFileInfo> GetOrLoadParamsFromTox(const FString& AbsoluteOrRelativeToContentFolder, bool bForceReload = false);

	/** Gives ans answer whether or not the given EPixelFormat is a supported one for this ResourceProvider. */
	bool IsSupportedPixelFormat(EPixelFormat PixelFormat) const;
	
	bool IsLoaded(const FString& AbsoluteOrRelativeToContentFolder) const;
	bool HasFailedLoad(const FString& AbsoluteOrRelativeToContentFolder) const;

	TObjectPtr<UTouchEngineInfo> GetTempEngineInfo() const { return EngineForLoading; }
	
private:

	using FAbsolutePath = FString;

	struct FLoadTask
	{
		FAbsolutePath AbsolutePath;
		TPromise<UE::TouchEngine::FCachedToxFileInfo> Promise;
	};
	
	TOptional<FLoadTask> ActiveTask;
	TArray<FLoadTask> TaskQueue;

	TMap<FAbsolutePath, UE::TouchEngine::FCachedToxFileInfo> CachedFileData;

	/** List of Supported EPixelFormat */
	UPROPERTY(Transient)
	TSet<TEnumAsByte<EPixelFormat>> CachedSupportedPixelFormats;

	/** TouchEngine instance used to load items into the details panel */
	UPROPERTY(Transient)
	TObjectPtr<UTouchEngineInfo> EngineForLoading;

	TFuture<UE::TouchEngine::FCachedToxFileInfo> EnqueueOrExecuteTask(const FString& AbsolutePath);
	void ExecuteTask(FLoadTask&& LoadTask);
};