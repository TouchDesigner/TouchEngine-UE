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

#include "ToxAsset.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformFile.h"
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