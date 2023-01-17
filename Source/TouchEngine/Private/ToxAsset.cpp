// Copyright © Derivative Inc. 2021

#include "ToxAsset.h"

void UToxAsset::SetFilePath(FString InPath)
{
	ToxSource.FilePath = InPath;
}

void UToxAsset::PostLoad()
{
	// Backward compatibility - migrate any assets configured using the previous setup (via FilePath)
	if (!FilePath.IsEmpty())
	{
		if (ToxSource.FilePath.IsEmpty())
		{
			ToxSource.FilePath = FPaths::ProjectContentDir() / FilePath;
			ToxSource.FilePath = FPaths::ConvertRelativePathToFull(ToxSource.FilePath);
		}
	}

	Super::PostLoad();
}
