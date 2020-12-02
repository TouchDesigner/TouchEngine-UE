// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineDynVarDetsCust.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Engine/GameViewportClient.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineComponent.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "..\Public\TouchEngineDynVarDetsCust.h"

void TouchEngineDynamicVariableStructDetailsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	PropertyHandle = StructPropertyHandle;

	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);

	if (RawData.Num() != 1)
	{
		//multiple values? error I think
		return;
	}

	auto DynVars = static_cast<FTouchEngineDynamicVariableStruct*>(RawData[0]);

	if (!DynVars)
		return;


	//if (DynVars->parent && !DynVars->parent->EngineInfo->isLoaded())
	//{
		//DynVars->OnToxLoaded.AddRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded);
	DynVars->CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded));
	//}

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Is TOX loaded? %s"), DynVars->parent->EngineInfo->isLoaded() ? TEXT("true") : TEXT("false")));


	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
	.ValueContent()
		.MaxDesiredWidth(0.0f)
		.MinDesiredWidth(125.0f)
		[
			SNew(SEditableTextBox)
			.ClearKeyboardFocusOnCommit(false)
		.IsEnabled(!PropertyHandle->IsEditConst())
		.ForegroundColor(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor)
		.OnTextChanged(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextChanged)
		.OnTextCommitted(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommited)
		.SelectAllTextOnCommit(true)
		.Text(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxText)
		];
}

void TouchEngineDynamicVariableStructDetailsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	CachedLayoutBuilder = &StructBuilder.GetParentCategory().GetParentLayout();
	//CachedLayoutBuilder->ForceRefreshDetails();
}


TSharedRef<IPropertyTypeCustomization> TouchEngineDynamicVariableStructDetailsCustomization::MakeInstance()
{
	return MakeShareable(new TouchEngineDynamicVariableStructDetailsCustomization);
}




FSlateColor TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor() const
{
	//if (InputValid)
	//{
	//	static const FName InvertedForegroundName("InvertedForeground");
	//	return FEditorStyle::GetSlateColor(InvertedForegroundName);
	//}

	return FLinearColor::Red;
}

/** Handles getting the text to be displayed in the editable text box. */
FText TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxText() const
{
	//TArray<void*> RawData;
	//PropertyHandle->AccessRawData(RawData);
	//
	//if (RawData.Num() != 1)
	//{
	//	return LOCTEXT("MultipleValues", "Multiple Values");
	//}
	//
	//auto DateTimePtr = static_cast<FDateTime*>(RawData[0]);
	//if (!DateTimePtr)
	//{
	//	return FText::GetEmpty();
	//}

	return FText::AsCultureInvariant(FString("reddtest"));
}

/** Handles changing the value in the editable text box. */
void TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextChanged(const FText& NewText)
{
	/*
	FDateTime DateTime;
	InputValid = ParseDateTimeZone(NewText.ToString(), DateTime);
	*/
}

/** Handles committing the text in the editable text box. */
void TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo)
{
	/*
	FDateTime ParsedDateTime;

	InputValid = ParseDateTimeZone(NewText.ToString(), ParsedDateTime);
	if (InputValid && PropertyHandle.IsValid())
	{
		TArray<void*> RawData;
		PropertyHandle->AccessRawData(RawData);

		PropertyHandle->NotifyPreChange();
		for (auto RawDataInstance : RawData)
		{
			*(FDateTime*)RawDataInstance = ParsedDateTime;
		}
		PropertyHandle->NotifyPostChange();
		PropertyHandle->NotifyFinishedChangingProperties();
	}
	*/
}

void TouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded()
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Purple, FString::Printf(TEXT("ToxLoaded")));
}

void TouchEngineDynamicVariableStructDetailsCustomization::RerenderPanel()
{
	//FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	//PropertyEditorModule.NotifyCustomizationModuleChanged();

	if (CachedLayoutBuilder)
	{
		CachedLayoutBuilder->ForceRefreshDetails();
	}
}