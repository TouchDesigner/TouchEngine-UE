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

#include "TouchEngineComponentCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Algo/AnyOf.h"
#include "Blueprint/TouchEngineComponent.h"
#include "DetailCategoryBuilder.h"
#include "TouchEngineEditorLog.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SThrobber.h"

#define LOCTEXT_NAMESPACE "FTouchEngineComponentCustomization"

namespace UE::TouchEngineEditor::Private
{
	TSharedRef<IDetailCustomization> FTouchEngineComponentCustomization::MakeInstance()
	{
		return MakeShared<FTouchEngineComponentCustomization>();
	}

	void FTouchEngineComponentCustomization::SetupHeader(IDetailCategoryBuilder& InCategoryBuilder)
	{
		if (!ensure(TouchEngineComponent.IsValid() && DynamicVariablesPropertyHandle.IsValid()))
		{
			UE_LOG(LogTouchEngineEditor, Warning, TEXT("FTouchEngineDynamicVariableContainer is designed to be contained by UTouchEngineComponentBase. Skipping customization..."));
			return;
		}

		TouchEngineComponent->GetOnToxStartedLoading().RemoveAll(this);
		TouchEngineComponent->GetOnToxLoaded().RemoveAll(this);
		TouchEngineComponent->GetOnToxReset().RemoveAll(this);
		TouchEngineComponent->GetOnToxFailedLoad().RemoveAll(this);
		TouchEngineComponent->GetOnToxStartedLoading().AddSP(this, &FTouchEngineComponentCustomization::OnToxStartedLoading);
		TouchEngineComponent->GetOnToxLoaded().AddSP(this, &FTouchEngineComponentCustomization::OnToxLoaded);
		TouchEngineComponent->GetOnToxReset().AddSP(this, &FTouchEngineComponentCustomization::OnToxReset);
		TouchEngineComponent->GetOnToxFailedLoad().AddSP(this, &FTouchEngineComponentCustomization::OnToxFailedLoad);

		SAssignNew(HeaderValueWidget, SBox);

		// As part of the Header (Known issue: lacks visual alignment)
		InCategoryBuilder.HeaderContent(HeaderValueWidget.ToSharedRef());

		RebuildHeaderValueWidgetContent();
	}

	void FTouchEngineComponentCustomization::RebuildHeaderValueWidgetContent()
	{
		if (!TouchEngineComponent.IsValid())
		{
			return;
		}

		TSharedPtr<SWidget> HeaderValueContent;

		const FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
		if (DynVars == nullptr)
		{
			HeaderValueContent = SNullWidget::NullWidget;
		}
		else if (TouchEngineComponent->GetFilePath().IsEmpty())
		{
			SAssignNew(HeaderValueContent, STextBlock)
				.Text(LOCTEXT("EmptyFilePath", "Empty file path."));
		}
		else if (TouchEngineComponent->HasFailedLoad())
		{
			// we have failed to load the tox file
			if (ErrorMessage.IsEmpty() && !TouchEngineComponent->ErrorMessage.IsEmpty())
			{
				ErrorMessage = TouchEngineComponent->ErrorMessage;
			}

			SAssignNew(HeaderValueContent, STextBlock)
				.AutoWrapText(true)
				.Text(FText::Format(LOCTEXT("ToxLoadFailed", "Failed to load TOX file: {0}"), FText::FromString(ErrorMessage)));
		}
		else if (TouchEngineComponent->IsLoading())
		{
			SAssignNew(HeaderValueContent, SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Loading", "Loading"))
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(5, 0)
				[
					SNew(SThrobber)
					.Animate(SThrobber::VerticalAndOpacity)
					.NumPieces(5)
				];
		}

		if (!HeaderValueContent.IsValid())
		{
			// If this path is reached it's either fully loaded or not supposed to be. Display nothing either way
			HeaderValueContent = SNullWidget::NullWidget;
		}

		HeaderValueWidget->SetContent(HeaderValueContent.ToSharedRef());
	}

	void FTouchEngineComponentCustomization::CustomizeDetails(IDetailLayoutBuilder& InDetailBuilder)
	{
		const TArray<TWeakObjectPtr<UObject>> SelectedObjects = InDetailBuilder.GetSelectedObjects();

		// Setup reference to TouchEngineComponent
		TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
		InDetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);
		check(ObjectsBeingCustomized.Num());
		TouchEngineComponent = Cast<UTouchEngineComponentBase>(ObjectsBeingCustomized[0]);

		const FName ToxAssetMemberName = GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxAsset);
		IDetailCategoryBuilder& CategoryBuilder = InDetailBuilder.EditCategory("Tox File", LOCTEXT("ToxParentCategory", "TouchEngine Component"));

		const FName DynamicVariablesMemberName = GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, DynamicVariables);
		DynamicVariablesPropertyHandle = InDetailBuilder.GetProperty(DynamicVariablesMemberName);

		// Header (<Reload Tox Throbber>)
		SetupHeader(CategoryBuilder);

		// 1. Tox Asset (Property Row)
		CategoryBuilder.AddProperty(ToxAssetMemberName);

		// 2. [Reload Tox Button]
		if (SelectedObjects.IsEmpty() || SelectedObjects.Num() > 1 || !ensure(TouchEngineComponent.IsValid() && DynamicVariablesPropertyHandle))
		{
			CategoryBuilder.AddCustomRow(LOCTEXT("MultiSelectionInvalid", "Selection is invalid"))
				.ValueContent()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(LOCTEXT("MultiSelectionInvalid_Text", "Tox Parameters does not support editing multiple objects"))
				];
		}
		else
		{
			CategoryBuilder.AddCustomRow(LOCTEXT("Reload", "Reload"), false)
				.ValueContent()
				[
					SNew(SBox).Padding(0, 10)
					[
						SNew(SButton)
						.Text(TAttribute<FText>(LOCTEXT("ReloadTox", "Reload Tox")))
						.ContentPadding(3)
						.OnClicked(FOnClicked::CreateSP(this, &FTouchEngineComponentCustomization::OnReloadClicked))
					]
				];
		}

		const TSharedRef<IPropertyHandle> ToxAssetProperty = InDetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxAsset));
		ToxAssetProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FTouchEngineComponentCustomization::OnToxAssetChanged));
		if (IDetailPropertyRow* ToxAssetPropertyRow = InDetailBuilder.EditDefaultProperty(ToxAssetProperty))
		{
			TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
			InDetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);
			ToxAssetPropertyRow
				->IsEnabled(TAttribute<bool>::CreateLambda([CustomizedObjects]()
				{
					const bool bIsAnyRunningTouchEngine = Algo::AnyOf(CustomizedObjects, [](TWeakObjectPtr<UObject> Object)
					{
						return Object.IsValid() && Cast<UTouchEngineComponentBase>(Object.Get())->IsRunning();
					});
					return !bIsAnyRunningTouchEngine;
				}))
				.ToolTip(FText::Format(
					LOCTEXT("ToxAssetTooltipFmt", "{0}\nThis property can only be modified while the file is not loaded by this component's TouchEngine instance"),	
					ToxAssetProperty->GetToolTipText()
					)
				);
		}

		if (TouchEngineComponent.IsValid())
		{
			if (TouchEngineComponent->HasFailedLoad())
			{
				RebuildHeaderValueWidgetContent();
			}
			else if (TouchEngineComponent->IsReadyToLoad())
			{
				TouchEngineComponent->LoadTox(false); // Only try to load latest data if it did not fail the last time
			}
		}
	}

	void FTouchEngineComponentCustomization::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& InDetailBuilder)
	{
		DetailBuilder = InDetailBuilder;
		IDetailCustomization::CustomizeDetails(InDetailBuilder);
	}


	FReply FTouchEngineComponentCustomization::OnReloadClicked()
	{
		if (!ensure(TouchEngineComponent.IsValid() && DynamicVariablesPropertyHandle.IsValid()))
		{
			return FReply::Handled();
		}

		DynamicVariablesPropertyHandle->NotifyPreChange();
		TouchEngineComponent->LoadTox(true);
		DynamicVariablesPropertyHandle->NotifyPostChange(EPropertyChangeType::Unspecified);
		
		return FReply::Handled();
	}

	void FTouchEngineComponentCustomization::OnToxAssetChanged() const
	{
		// Here we can only take the ptr as ForceRefreshDetails() checks that the reference is unique.
		if (IDetailLayoutBuilder* DetailBuilderValuePtr = DetailBuilder.Pin().Get())
		{
			DetailBuilderValuePtr->ForceRefreshDetails();
		}
	}

	FTouchEngineDynamicVariableContainer* FTouchEngineComponentCustomization::GetDynamicVariables() const
	{
		if (DynamicVariablesPropertyHandle->IsValidHandle())
		{
			TArray<void*> RawData;
			DynamicVariablesPropertyHandle->AccessRawData(RawData);
			if (RawData.Num() == 1)
			{
				return static_cast<FTouchEngineDynamicVariableContainer*>(RawData[0]);
			}
		}
		return nullptr;
	}

	void FTouchEngineComponentCustomization::OnToxStartedLoading()
	{
		RebuildHeaderValueWidgetContent();
	}
	
	void FTouchEngineComponentCustomization::OnToxLoaded()
	{
		RebuildHeaderValueWidgetContent();
	}

	void FTouchEngineComponentCustomization::OnToxReset()
	{
		RebuildHeaderValueWidgetContent();
	}

	void FTouchEngineComponentCustomization::OnToxFailedLoad(const FString& Error)
	{
		ErrorMessage = Error;
		if (TouchEngineComponent.IsValid())
		{
			TouchEngineComponent->ErrorMessage = Error;
		}

		RebuildHeaderValueWidgetContent();
	}
}

#undef LOCTEXT_NAMESPACE
