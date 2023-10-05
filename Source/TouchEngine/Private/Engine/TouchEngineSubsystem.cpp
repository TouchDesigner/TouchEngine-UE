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

#include "Engine/TouchEngineSubsystem.h"

#include "ToxAsset.h"
// #include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/TouchEngineInfo.h"
#include "Engine/TouchEngine.h"

#include "Misc/Paths.h"

// class FAssetRegistryModule;

namespace UE::TouchEngine::Private
{
	static TOptional<FString> GetAbsoluteToxPathIfExists(const UToxAsset* ToxAsset)
	{
		if (IsValid(ToxAsset))
		{
			const FString Path = ToxAsset->GetAbsoluteFilePath();
			if (FPaths::FileExists(Path))
			{
				return Path;
			}
		}
		return {};
	}
}

void UTouchEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	EngineForLoading = NewObject<UTouchEngineInfo>();
	//
	// FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	// TArray<FAssetData> AssetData;
	// AssetRegistryModule.Get().GetAssetsByClass(UToxAsset::StaticClass()->GetClassPathName(), AssetData, true);
	// TArray<UToxAsset*> ToxAssets;
	// for (int i = 0; i < AssetData.Num(); i++) {
	// 	if (UToxAsset* ToxAsset = Cast<UToxAsset>(AssetData[i].GetAsset()))
	// 	{
	// 		ToxAssets.Add(ToxAsset);
	// 		UE_LOG(LogTemp, Warning, TEXT("Asset '%s' with tox file: '%s'"), *GetNameSafe(ToxAsset), *ToxAsset->FilePath)
	// 	}
	// }
	// UE_LOG(LogTemp, Error, TEXT(" --- Fond %d Assets"), ToxAssets.Num())
}

void UTouchEngineSubsystem::Deinitialize()
{
	static const FString FailureReason = TEXT("TouchEngine Subsystem shutting down.");

	if (ActiveTask.IsSet())
	{
		ActiveTask->Promise.SetValue(UE::TouchEngine::FCachedToxFileInfo::MakeFailure(FailureReason));
		ActiveTask.Reset();
	}

	for (FLoadTask& Task : TaskQueue)
	{
		Task.Promise.SetValue(UE::TouchEngine::FCachedToxFileInfo::MakeFailure(FailureReason));
	}
	TaskQueue.Empty();
}

TFuture<UE::TouchEngine::FCachedToxFileInfo> UTouchEngineSubsystem::GetOrLoadParamsFromTox(UToxAsset* ToxAsset, double LoadTimeoutInSeconds, bool bForceReload)
{
	using namespace UE::TouchEngine;
	check(IsInGameThread());

	if (!IsValid(ToxAsset))
	{
		return MakeFulfilledPromise<FCachedToxFileInfo>(FCachedToxFileInfo::MakeFailure(TEXT("Invalid Tox Asset"))).GetFuture();
	}

	if (!bForceReload)
	{
		// if we have cached data, we don't really care if the file exist or not, and we don't want to run the events
		if (const FTouchLoadResult* LoadResult = CachedFileData.Find(ToxAsset->GetAbsoluteFilePath()))
		{
			ToxAssetToStartLoading.Reset();
			return MakeFulfilledPromise<FCachedToxFileInfo>(FCachedToxFileInfo{*LoadResult, true}).GetFuture();
		}
	}
	
	ToxAssetToStartLoading = ToxAsset;
#if WITH_EDITOR
	ToxAsset->GetOnToxStartedLoadingThroughSubsystem().Broadcast(ToxAsset);
#endif
	
	const FString AbsolutePath = ToxAsset->GetAbsoluteFilePath();
	if (AbsolutePath.IsEmpty() || !FPaths::FileExists(AbsolutePath))
	{
		const FCachedToxFileInfo FinalResult = FCachedToxFileInfo::MakeFailure(AbsolutePath.IsEmpty() ?
			TEXT("Invalid .tox file path. The path is empty") :
			FString::Printf(TEXT("Invalid .tox file path. The file is not found at location '%s'"), *AbsolutePath));
		CachedFileData.Add(AbsolutePath, FinalResult.LoadResult);
		ToxAssetToStartLoading.Reset();
#if WITH_EDITOR
		ToxAsset->GetOnToxLoadedThroughSubsystem().Broadcast(ToxAsset, FinalResult);
#endif
		return MakeFulfilledPromise<FCachedToxFileInfo>(FinalResult).GetFuture();
	}
	
	if (bForceReload)
	{
		CachedFileData.Remove(AbsolutePath);
	}

	ToxAssetToStartLoading.Reset();
	return EnqueueOrExecuteLoadTask(ToxAsset, LoadTimeoutInSeconds);
	
}

bool UTouchEngineSubsystem::IsSupportedPixelFormat(EPixelFormat PixelFormat) const
{
	// For this to return a non empty value, we would need to have called the Subsystem to load a TOX file, which might not always be the case.
	// The function LoadPixelFormats has been added to ensure that these are filled when a Tox is loaded
	return CachedSupportedPixelFormats.Contains(PixelFormat);
}

bool UTouchEngineSubsystem::IsSupportedPixelFormat(TEnumAsByte<ETextureRenderTargetFormat> PixelFormat) const
{
	return IsSupportedPixelFormat(GetPixelFormatFromRenderTargetFormat(PixelFormat));
}

bool UTouchEngineSubsystem::IsLoaded(const UToxAsset* ToxAsset) const
{
	using namespace UE::TouchEngine;
	if (!IsValid(ToxAsset))
	{
		return false;
	}
	if (ToxAssetToStartLoading == ToxAsset)
	{
		return false;
	}
	
	const TOptional<FString> AbsolutePath = Private::GetAbsoluteToxPathIfExists(ToxAsset);
	if (AbsolutePath.IsSet())
	{
		const FTouchLoadResult* LoadResult = CachedFileData.Find(AbsolutePath.GetValue());
		return LoadResult && LoadResult->IsSuccess();
	}

	return false;
}

bool UTouchEngineSubsystem::IsLoading(const UToxAsset* ToxAsset) const
{
	if (!IsValid(ToxAsset))
	{
		return false;
	}
	if (ToxAssetToStartLoading == ToxAsset)
	{
		return true;
	}
	if (IsLoaded(ToxAsset)) //check if we should check this last due to the promise
	{
		return false;
	}
	
	using namespace UE::TouchEngine;

	const TOptional<FString> AbsolutePath = Private::GetAbsoluteToxPathIfExists(ToxAsset);
	if (AbsolutePath.IsSet())
	{
		if (ActiveTask && ActiveTask->ToxAsset == ToxAsset)
		{
			return true;
		}
		for (const FLoadTask& Task :TaskQueue)
		{
			if (Task.ToxAsset == ToxAsset)
			{
				return true;
			}
		}
	}

	return false;
}

bool UTouchEngineSubsystem::HasFailedLoad(const UToxAsset* ToxAsset) const
{
	using namespace UE::TouchEngine;
	if (ToxAssetToStartLoading == ToxAsset)
	{
		return false; // what to do for assets that are loading?
	}
	
	const TOptional<FString> AbsolutePath = Private::GetAbsoluteToxPathIfExists(ToxAsset);
	if (AbsolutePath.IsSet())
	{
		const FTouchLoadResult* LoadResult = CachedFileData.Find(AbsolutePath.GetValue());
		if (LoadResult)
		{
			return LoadResult->IsFailure();
		}
	}

	return true; // This path is reached if the user creates an Empty Tox asset with a blank path (or if the path is invalid)
}

void UTouchEngineSubsystem::CacheLoadedDataFromComponent(UToxAsset* ToxAsset, const UE::TouchEngine::FTouchLoadResult& LoadResult)
{
	if (IsValid(ToxAsset))
	{
		CachedFileData.Add(ToxAsset->GetAbsoluteFilePath(), LoadResult);
	}
}

void UTouchEngineSubsystem::LoadPixelFormats(const UTouchEngineInfo* ComponentEngineInfo)
{
	if (ComponentEngineInfo)
	{
		TSet<TEnumAsByte<EPixelFormat>> SupportedPixelFormats;
		ComponentEngineInfo->GetSupportedPixelFormats(SupportedPixelFormats);
		
		if (!SupportedPixelFormats.IsEmpty()) //if we had an issue getting the pixel formats, do not update the cached ones
		{
			CachedSupportedPixelFormats = SupportedPixelFormats;
		}
	}
}

TFuture<UE::TouchEngine::FCachedToxFileInfo> UTouchEngineSubsystem::EnqueueOrExecuteLoadTask(UToxAsset* ToxAsset, double LoadTimeoutInSeconds)
{
	using namespace UE::TouchEngine;
	
	TPromise<FCachedToxFileInfo> Promise;
	TFuture<FCachedToxFileInfo> Future = Promise.GetFuture();
	if (ActiveTask)
	{
		TaskQueue.Emplace(FLoadTask{ToxAsset, MoveTemp(Promise), LoadTimeoutInSeconds});
	}
	else
	{
		ExecuteLoadTask(FLoadTask{ToxAsset, MoveTemp(Promise), LoadTimeoutInSeconds});
	}

	return Future;
}

void UTouchEngineSubsystem::ExecuteLoadTask(FLoadTask&& LoadTask)
{
	using namespace UE::TouchEngine;
	ActiveTask = MoveTemp(LoadTask);
	EngineForLoading->LoadTox(*ActiveTask->ToxAsset->GetAbsoluteFilePath(), nullptr, ActiveTask->LoadTimeoutInSeconds)
		.Next([this](const FTouchLoadResult& LoadResult)
		{
			check(IsInGameThread());

			// We do not expect this to fire because the only Reset points should be in this if and in Deinitialize()
			if (ensure(ActiveTask.IsSet()))
			{
				const FCachedToxFileInfo FinalResult { LoadResult, false };
				CachedFileData.Add(ActiveTask->ToxAsset->GetAbsoluteFilePath(), LoadResult);
				
				// This is only safe to call after TE has sent the load success event - which has if it has told us the file is loaded.
				EngineForLoading->GetSupportedPixelFormats(CachedSupportedPixelFormats);
				
				ActiveTask->Promise.EmplaceValue(FinalResult);
#if WITH_EDITOR
				ActiveTask->ToxAsset->GetOnToxLoadedThroughSubsystem().Broadcast(ActiveTask->ToxAsset, FinalResult);
#endif
				ActiveTask.Reset();
			}
			
			if (TaskQueue.Num() > 0)
			{
				FLoadTask Task = MoveTemp(TaskQueue[0]);
				TaskQueue.RemoveAt(0);
				ExecuteLoadTask(MoveTemp(Task));
			}
			else
			{
				// If there are no more tasks, prevent the engine locking up rendering resources.
				// Some .tox files when loaded lock shared hardware resources which we'd block.
				EngineForLoading->Destroy();
			}
		});
}
