// Copyright Epic Games, Inc. All Rights Reserved.

#include "ToxAssetCustomization.h"
#include "Misc/Paths.h"
#include "ToxAsset.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Input/SFilePathPicker.h"
#include "Widgets/Images/SImage.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

#define LOCTEXT_NAMESPACE "FToxAssetCustomization"


/* IDetailCustomization interface
 *****************************************************************************/

void FToxAssetCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	UToxAsset* ToxAsset = GetToxAsset(DetailBuilder);
	check(ToxAsset);
	ToxAssetWeakPtr = ToxAsset;

	// customize 'File' category
	IDetailCategoryBuilder& FileCategory = DetailBuilder.EditCategory("ImportSettings");
	{
		// FilePath
		FilePathProperty = DetailBuilder.GetProperty("FilePath");
		{
			IDetailPropertyRow& FilePathRow = FileCategory.AddProperty(FilePathProperty);

			FilePathRow
				.ShowPropertyButtons(false)
				.CustomWidget()
				.NameContent()
					[
						SNew(STextBlock)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.Text(LOCTEXT("FilePathPropertyName", "File Path"))
						.ToolTipText(FilePathProperty->GetToolTipText())
					]
				.ValueContent()
					.MaxDesiredWidth(0.0f)
					.MinDesiredWidth(125.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(5.0f, 0.0f)
						[	
							SNew(SImage)
							.Image(FCoreStyle::Get().GetBrush("Icons.Warning"))
							.ToolTipText(LOCTEXT("FilePathWarning", "The selected Tox file will not get packaged because it is located outside the project's Content directory."))
							.Visibility(this, &FToxAssetCustomization::HandleFilePathWarningIconVisibility)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SFilePathPicker)
							.BrowseButtonImage(FAppStyle::GetBrush("PropertyWindow.Button_Ellipsis"))
							.BrowseButtonStyle(FAppStyle::Get(), "HoverHintOnly")
							.BrowseButtonToolTip(LOCTEXT("FilePathBrowseButtonToolTip", "Choose a file from this computer"))
							.BrowseDirectory(this, &FToxAssetCustomization::HandleBrowseDirectory) //FPaths::ProjectContentDir() / TEXT("Movies"))
							.FilePath(this, &FToxAssetCustomization::HandleFilePathPickerFilePath)
							.FileTypeFilter_Static(&FToxAssetCustomization::HandleFilePathPickerFileTypeFilter)
							.OnPathPicked(this, &FToxAssetCustomization::HandleFilePathPickerPathPicked)
							.ToolTipText(LOCTEXT("FilePathToolTip", "The path to a Tox file on this computer"))
						]
					];
		}
	}
}


UToxAsset* FToxAssetCustomization::GetToxAsset(const IDetailLayoutBuilder& InDetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> LayoutObjects = InDetailBuilder.GetSelectedObjects();
	if (LayoutObjects.Num())
	{
		return Cast<UToxAsset>(LayoutObjects[0].Get());
	}

	return nullptr;
}

/* FToxAssetCustomization callbacks
 *****************************************************************************/

FString FToxAssetCustomization::HandleFilePathPickerFilePath() const
{
	FString FilePath;
	FilePathProperty->GetValue(FilePath);

	return FilePath;
}


FString FToxAssetCustomization::HandleFilePathPickerFileTypeFilter()
{
	FString Filter = TEXT("Tox Files (*.tox)|*.tox");

	return Filter;
}

FString FToxAssetCustomization::HandleBrowseDirectory() const
{
	if (const UToxAsset* ToxAsset = ToxAssetWeakPtr.Get())
	{
		const FString FilePath = ToxAsset->GetAbsoluteFilePath();
		FString Dir = FPaths::GetPath(FilePath);
		if (FPaths::DirectoryExists(Dir))
		{
			return Dir;
		}
	}
	
	return FPaths::ProjectContentDir();
}


void FToxAssetCustomization::HandleFilePathPickerPathPicked(const FString& PickedPath) const
{
	FilePathProperty->SetValue(PickedPath);
}


EVisibility FToxAssetCustomization::HandleFilePathWarningIconVisibility() const
{
	FString FilePath;

	if ((FilePathProperty->GetValue(FilePath) != FPropertyAccess::Success) || FilePath.IsEmpty() || FilePath.Contains(TEXT("://")))
	{
		return EVisibility::Hidden;
	}

	// Relative Paths - these are guaranteed to be inside the Content folder always
	if (const UToxAsset* ToxAsset = ToxAssetWeakPtr.Get())
	{
		if (ToxAsset->IsRelativePath())
		{
			return EVisibility::Hidden; 
		}
	}

	// External tox - must display warning to the user
	return EVisibility::Visible;
}

#undef LOCTEXT_NAMESPACE
