// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;
class IPropertyHandle;
class UToxAsset;

/**
 * Implements a details view customization for the UFileMediaSource class.
 */
class FToxAssetCustomization
	: public IDetailCustomization
{
public:

	//~ IDetailCustomization interface

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

public:

	/**
	 * Creates an instance of this class.
	 *
	 * @return The new instance.
	 */
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FToxAssetCustomization());
	}

private:
	static UToxAsset* GetToxAsset(const IDetailLayoutBuilder& InDetailBuilder);

	/** Callback for getting the selected path in the Tox file picker widget. */
	FString HandleFilePathPickerFilePath() const;

	/** Callback for getting the file type filter for the Tox file picker. */
	static FString HandleFilePathPickerFileTypeFilter();

	FString HandleBrowseDirectory() const;

	/** Callback for picking a path in the Tox file picker. */
	void HandleFilePathPickerPathPicked(const FString& PickedPath) const;

	/** Callback for getting the visibility of warning icon for external Tox Files (located outside the Content folder). */
	EVisibility HandleFilePathWarningIconVisibility() const;

private:

	/** Pointer to the FilePath property handle. */
	TSharedPtr<IPropertyHandle> FilePathProperty;

	/** Pointer to the Tox Asset whose Details View is currently being customized. */
	TWeakObjectPtr<UToxAsset> ToxAssetWeakPtr;
};
