// Copyright © Derivative Inc. 2021

#include "ToxAsset.h"

#include "HAL/PlatformFileManager.h"

bool UToxAsset::IsRelativePath() const
{
	FString TestPath = FPaths::ProjectContentDir() / FilePath;
	TestPath = FPaths::ConvertRelativePathToFull(TestPath);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	return PlatformFile.FileExists(*TestPath);
}

FString UToxAsset::GetAbsoluteFilePath() const
{
	if (IsRelativePath())
	{
		return FPaths::ProjectContentDir() / FilePath;
	}
	else
	{
		return FilePath;
	}
}

FString UToxAsset::GetRelativeFilePath() const
{
	if (IsRelativePath())
	{
		return FilePath;
	}
	else
	{
		FString RelativePath = FilePath;
		FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectContentDir());

		return RelativePath;
	}
}

void UToxAsset::SetFilePath(FString InPath)
{
	FilePath = InPath;
}