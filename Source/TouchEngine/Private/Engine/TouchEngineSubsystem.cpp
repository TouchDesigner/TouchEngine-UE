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

#include "Engine/TouchEngineInfo.h"
#include "Engine/TouchEngine.h"

#include "Misc/Paths.h"

namespace UE::TouchEngine::Private
{
	static TOptional<FString> ConvertToAbsolutePathIfRelativeAndExists(const FString& AbsoluteOrRelativeToxPath)
	{
		FString AbsoluteFilePath = AbsoluteOrRelativeToxPath;
		if (!FPaths::FileExists(AbsoluteOrRelativeToxPath))
		{
			AbsoluteFilePath = FPaths::ProjectContentDir() + AbsoluteOrRelativeToxPath;
			if (!FPaths::FileExists(AbsoluteOrRelativeToxPath))
			{
				return {};
			}
		}
		return AbsoluteFilePath;
	}
}

void UTouchEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	EngineForLoading = NewObject<UTouchEngineInfo>();
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

TFuture<UE::TouchEngine::FCachedToxFileInfo> UTouchEngineSubsystem::GetOrLoadParamsFromTox(const FString& AbsoluteOrRelativeToContentFolder, bool bForceReload)
{
	using namespace UE::TouchEngine;
	check(IsInGameThread());

	const TOptional<FAbsolutePath> AbsolutePath = Private::ConvertToAbsolutePathIfRelativeAndExists(AbsoluteOrRelativeToContentFolder);
	if (!AbsolutePath)
	{
		return MakeFulfilledPromise<FCachedToxFileInfo>(FCachedToxFileInfo::MakeFailure(TEXT("Invalid path"))).GetFuture();
	}
	
	if (bForceReload)
	{
		CachedFileData.Remove(*AbsolutePath);
	}
	else if (const FCachedToxFileInfo* FileInfo = CachedFileData.Find(*AbsolutePath))
	{
		return MakeFulfilledPromise<FCachedToxFileInfo>(*FileInfo).GetFuture();
	}

	return EnqueueOrExecuteTask(AbsoluteOrRelativeToContentFolder);
	
}

bool UTouchEngineSubsystem::IsSupportedPixelFormat(EPixelFormat PixelFormat) const
{
	bool bResult = CachedSupportedPixelFormats.Contains(PixelFormat);
	return bResult;
}

bool UTouchEngineSubsystem::IsLoaded(const FString& AbsoluteOrRelativeToContentFolder) const
{
	using namespace UE::TouchEngine;
	const TOptional<FAbsolutePath> AbsolutePath = Private::ConvertToAbsolutePathIfRelativeAndExists(AbsoluteOrRelativeToContentFolder);
	const FCachedToxFileInfo* FileInfo = CachedFileData.Find(*AbsolutePath);
	return FileInfo && FileInfo->LoadResult.IsSuccess();
}

bool UTouchEngineSubsystem::HasFailedLoad(const FString& AbsoluteOrRelativeToContentFolder) const
{
	using namespace UE::TouchEngine;
	const TOptional<FAbsolutePath> AbsolutePath = Private::ConvertToAbsolutePathIfRelativeAndExists(AbsoluteOrRelativeToContentFolder);
	const FCachedToxFileInfo* FileInfo = CachedFileData.Find(*AbsolutePath);
	return FileInfo && FileInfo->LoadResult.IsFailure();
}

TFuture<UE::TouchEngine::FCachedToxFileInfo> UTouchEngineSubsystem::EnqueueOrExecuteTask(const FString& AbsolutePath)
{
	using namespace UE::TouchEngine;
	
	TPromise<FCachedToxFileInfo> Promise;
	TFuture<FCachedToxFileInfo> Future = Promise.GetFuture();
	if (ActiveTask)
	{
		TaskQueue.Emplace(FLoadTask{ *AbsolutePath, MoveTemp(Promise) });
	}
	else
	{
		ExecuteTask({ AbsolutePath, MoveTemp(Promise) });
	}

	return Future;
}

void UTouchEngineSubsystem::ExecuteTask(FLoadTask&& LoadTask)
{
	using namespace UE::TouchEngine;
	ActiveTask = MoveTemp(LoadTask);
	EngineForLoading->LoadTox(*ActiveTask->AbsolutePath)
		.Next([this](FTouchLoadResult LoadResult)
		{
			check(IsInGameThread());

			// We do not expect this to fire because the only Reset points should be in this if and in Deinitialize()
			if (ensure(ActiveTask.IsSet()))
			{
				const FCachedToxFileInfo FinalResult { LoadResult };
				CachedFileData.Add(ActiveTask->AbsolutePath, FinalResult);
				
				// This is only safe to call after TE has sent the load success event - which has if it has told us the file is loaded.
				EngineForLoading->GetSupportedPixelFormats(CachedSupportedPixelFormats);
				
				ActiveTask->Promise.EmplaceValue(FinalResult);
				ActiveTask.Reset();
			}
			
			if (TaskQueue.Num() > 0)
			{
				FLoadTask Task = MoveTemp(TaskQueue[0]);
				TaskQueue.RemoveAt(0);
				ExecuteTask(MoveTemp(Task));
			}
			else
			{
				// If there are no more tasks, prevent the engine locking up rendering resources.
				// Some .tox files when loaded lock shared hardware resources which we'd block.
				EngineForLoading->Destroy();
			}
		});
}