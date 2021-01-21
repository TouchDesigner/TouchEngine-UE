// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineDynVarDetsCust.h"
#include "Engine/GameViewportClient.h"
#include "PropertyHandle.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineComponent.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "DetailWidgetRow.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "PropertyCustomizationHelpers.h"
#include "UObject/PropertyIterator.h"
#include "SResetToDefaultMenu.h"
#include "..\Public\TouchEngineDynVarDetsCust.h"

TouchEngineDynamicVariableStructDetailsCustomization::TouchEngineDynamicVariableStructDetailsCustomization()
{

}

TouchEngineDynamicVariableStructDetailsCustomization::~TouchEngineDynamicVariableStructDetailsCustomization()
{
	DynVars->Unbind_OnToxLoaded(ToxLoaded_DelegateHandle);
	DynVars->Unbind_OnToxFailedLoad(ToxFailedLoad_DelegateHandle);
}

void TouchEngineDynamicVariableStructDetailsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	PropertyHandle = StructPropertyHandle;

	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);

	if (RawData.Num() != 1)
	{
		//multiple values
		return;
	}

	DynVars = static_cast<FTouchEngineDynamicVariableContainer*>(RawData[0]);

	if (!DynVars)
		return;

	if (!DynVars->parent)
	{
		TArray<UObject*> outers;
		PropertyHandle->GetOuterObjects(outers);

		for (int i = 0; i < outers.Num(); i++)
		{
			UTouchEngineComponentBase* parent = static_cast<UTouchEngineComponentBase*>(outers[i]);
			if (parent)
			{
				DynVars->parent = parent;

				break;
			}
		}
	}

	DynVars->parent->CreateEngineInfo();

	ToxLoaded_DelegateHandle = DynVars->CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded));
	ToxFailedLoad_DelegateHandle = DynVars->CallOrBind_OnToxFailedLoad(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad));

	// tox file is not loaded yet
	if (!DynVars->parent->EngineInfo->isLoaded())
	{
		// file still loading, run throbber
		if (!DynVars->parent->EngineInfo->hasFailedLoad())
		{
			HeaderRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(FString("Variables")), FText::FromString(FString("Input and output variables as read from the TOX file")), false)
				]
			.ValueContent()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SThrobber)
					.Animate(SThrobber::VerticalAndOpacity)
				.NumPieces(5)
				]
			;
		}
		// we have failed to load the tox file
		else
		{
			HeaderRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(FString("Variables")), FText::FromString(FString("Input and output variables as read from the TOX file")), false)
				]
			.ValueContent()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Failed to load TOX file"))
				]
			;
		}
	}
	// tox file is loaded, do not run throbber
	else
	{
		HeaderRow.NameContent()
			[
				StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(FString("Variables")), FText::FromString(FString("Input and output variables as read from the TOX file")), false)
			]
		;
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	PropertyHandle = StructPropertyHandle;
	PropUtils = StructCustomizationUtils.GetPropertyUtilities();


	// handle input variables 
	TSharedPtr<IPropertyHandleArray> inputsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Input))->AsArray();
	uint32 numInputs = 0u;
	inputsHandle->GetNumElements(numInputs);

	for (uint32 i = 0; i < numInputs; i++)
	{
		auto dynVarHandle = inputsHandle->GetElement(i);
		FTEDynamicVariable* dynVar;

		{
			TArray<void*> RawData;
			PropertyHandle->AccessRawData(RawData);
			dynVarHandle->AccessRawData(RawData);
			dynVar = static_cast<FTEDynamicVariable*>(RawData[0]);
		}

		switch (dynVar->VarType)
		{
		case EVarType::VARTYPE_BOOL:
		{
			FDetailWidgetRow& newRow = StructBuilder.AddCustomRow(FText::FromString(dynVar->VarName));

			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
				]
			.ValueContent()
				[
					SNew(SCheckBox)
					.OnCheckStateChanged_Raw(dynVar, &FTEDynamicVariable::HandleChecked)
				.IsChecked_Raw(dynVar, &FTEDynamicVariable::GetValueAsCheckState)
				];
		}
		break;
		case EVarType::VARTYPE_INT:
		{
			FDetailWidgetRow& newRow = StructBuilder.AddCustomRow(FText::FromString(dynVar->VarName));

			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
				]
			.ValueContent()
				[
					SNew(SNumericEntryBox<int>)
					.OnValueCommitted_Raw(dynVar, &FTEDynamicVariable::HandleValueChanged<int>)
				.AllowSpin(false)
				.Value_Raw(dynVar, &FTEDynamicVariable::GetValueAsOptionalInt)
				];
		}
		break;
		case EVarType::VARTYPE_DOUBLE:
		{
			if (dynVar->count == 1)
			{
				FDetailWidgetRow& newRow = StructBuilder.AddCustomRow(FText::FromString(dynVar->VarName));

				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
					]
				.ValueContent()
					[
						SNew(SNumericEntryBox<double>)
						.OnValueCommitted_Raw(dynVar, &FTEDynamicVariable::HandleValueChanged<double>)
					.AllowSpin(false)
					.Value_Raw(dynVar, &FTEDynamicVariable::GetValueAsOptionalDouble)
					]
				;
			}
			else
			{
				for (int j = 0; j < dynVar->count; j++)
				{
					FDetailWidgetRow& newRow = StructBuilder.AddCustomRow(FText::FromString(FString(dynVar->VarName).Append(FString::FromInt(j + 1))));

					newRow.NameContent()
						[
							CreateNameWidget((FString(dynVar->VarName).Append(FString::FromInt(j + 1))), dynVar->VarIdentifier, StructPropertyHandle)
						]
					.ValueContent()
						[
							SNew(SNumericEntryBox<double>)
							.OnValueCommitted(SNumericEntryBox<double>::FOnValueCommitted::CreateRaw(dynVar, &FTEDynamicVariable::HandleValueChangedWithIndex, j))
						.AllowSpin(false)
						//_##AttrName = TAttribute< AttrType >::Create(TAttribute< AttrType >::FGetter::CreateRaw(InUserObject, InFunc)); 
						.Value(TAttribute<TOptional<double>>::Create(TAttribute<TOptional<double>>::FGetter::CreateRaw(dynVar, &FTEDynamicVariable::GetIndexedValueAsOptionalDouble, j)))
						]
					;
				}
			}
		}
		break;
		case EVarType::VARTYPE_FLOAT:
		{
			FDetailWidgetRow& newRow = StructBuilder.AddCustomRow(FText::FromString(dynVar->VarName));

			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
				]
			.ValueContent()
				[
					SNew(SNumericEntryBox<float>)
					.OnValueCommitted_Raw(dynVar, &FTEDynamicVariable::HandleValueChanged<float>)
				.AllowSpin(false)
				.Value_Raw(dynVar, &FTEDynamicVariable::GetValueAsOptionalFloat)
				];
		}
		break;
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			FDetailWidgetRow& newRow = StructBuilder.AddCustomRow(FText::FromString(dynVar->VarName));
			auto floatsHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTEDynamicVariable, floatBufferProperty));

			floatsHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTEDynamicVariable::HandleFloatBufferChanged));
			floatsHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTEDynamicVariable::HandleFloatBufferChildChanged));
			floatsHandle->SetToolTipText(FText::FromString(dynVar->VarIdentifier));

			auto floatsArrayHandle = floatsHandle->AsArray();

			TSharedRef<FDetailArrayBuilder> arrayBuilder = MakeShareable(new FDetailArrayBuilder(floatsHandle.ToSharedRef()));
			arrayBuilder->SetDisplayName(FText::FromString(dynVar->VarName));
			arrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild));

			StructBuilder.AddCustomBuilder(arrayBuilder);
		}
		break;
		case EVarType::VARTYPE_STRING:
		{
			FDetailWidgetRow& newRow = StructBuilder.AddCustomRow(FText::FromString(dynVar->VarName));

			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
				]
			.ValueContent()
				.MaxDesiredWidth(0.0f)
				.MinDesiredWidth(125.0f)
				[
					SNew(SEditableTextBox)
					.ClearKeyboardFocusOnCommit(false)
				.IsEnabled(!PropertyHandle->IsEditConst())
				.ForegroundColor(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor)
				.OnTextChanged_Raw(dynVar, &FTEDynamicVariable::HandleTextBoxTextChanged)
				.OnTextCommitted_Raw(dynVar, &FTEDynamicVariable::HandleTextBoxTextCommited)
				.SelectAllTextOnCommit(true)
				.Text_Raw(dynVar, &FTEDynamicVariable::HandleTextBoxText)
				];
		}
		break;
		case EVarType::VARTYPE_TEXTURE:
		{
			FDetailWidgetRow& newRow = StructBuilder.AddCustomRow(FText::FromString(dynVar->VarName));
			TSharedPtr<IPropertyHandle> textureHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTEDynamicVariable, textureProperty));
			textureHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTEDynamicVariable::HandleTextureChanged));

			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
				]
			.ValueContent()
				[
					textureHandle->CreatePropertyValueWidget()
				]
			;
		}
		break;
		default:
			// VARTYPE_NOT_SET or VARTYPE_MAX
			break;
		}
	}

	// handle output variables
	TSharedPtr<IPropertyHandleArray> outputsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Output))->AsArray();
	uint32 numOutputs = 0u;
	outputsHandle->GetNumElements(numOutputs);

	for (uint32 i = 0; i < numOutputs; i++)
	{
		auto dynVarHandle = outputsHandle->GetElement(i);
		FTEDynamicVariable* dynVar;

		{
			TArray<void*> RawData;
			PropertyHandle->AccessRawData(RawData);
			dynVarHandle->AccessRawData(RawData);
			dynVar = static_cast<FTEDynamicVariable*>(RawData[0]);
		}

		FDetailWidgetRow& newRow = StructBuilder.AddCustomRow(FText::FromString(dynVar->VarName));

		switch (dynVar->VarType)
		{
			/*
		case EVarType::VARTYPE_BOOL:
		{
			newRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(dynVar->VarName))
				]
			.ValueContent()
				[
					SNew(SCheckBox)
					.OnCheckStateChanged_Raw(dynVar, &FTouchEngineDynamicVariableStruct::HandleChecked)
				.IsChecked(dynVar->GetValueAsBool())
				.IsEnabled(false)
				];
		}
		break;
		case EVarType::VARTYPE_INT:
		{
			newRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(dynVar->VarName))
				]
			.ValueContent()
				[
					SNew(SNumericEntryBox<int>)
					.OnValueCommitted_Raw(dynVar, &FTouchEngineDynamicVariableStruct::HandleValueChanged<int>)
				.AllowSpin(false)
				.Value(dynVar->GetValueAsInt())
				.IsEnabled(false)
				];
		}
		break;
		case EVarType::VARTYPE_DOUBLE:
		{
			newRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(dynVar->VarName))
				]
			.ValueContent()
				[
					SNew(SNumericEntryBox<double>)
					.OnValueCommitted_Raw(dynVar, &FTouchEngineDynamicVariableStruct::HandleValueChanged<double>)
				.AllowSpin(false)
				.Value(dynVar->GetValueAsDouble())
				.IsEnabled(false)
				];
		}
		break;
		case EVarType::VARTYPE_FLOAT:
		{
			newRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(dynVar->VarName))
				]
			.ValueContent()
				[
					SNew(SNumericEntryBox<float>)
					.OnValueCommitted_Raw(dynVar, &FTouchEngineDynamicVariableStruct::HandleValueChanged<float>)
				.AllowSpin(false)
				.Value_Raw(dynVar, &FTouchEngineDynamicVariableStruct::GetValueAsOptionalFloat)
				.IsEnabled(false)
				];
		}
		break;
			*/
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
				]
			.ValueContent()
				[
					SNew(STextBlock)
					.Text(FText::FromString("Float array will be filled at runtime"))
				]

			;
		}
		break;
		/*
		case EVarType::VARTYPE_STRING:
		{
			newRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(dynVar->VarName))
				]
			.ValueContent()
				.MaxDesiredWidth(0.0f)
				.MinDesiredWidth(125.0f)
				[
					SNew(SEditableTextBox)
					.ClearKeyboardFocusOnCommit(false)
				.IsEnabled(!PropertyHandle->IsEditConst())
				.ForegroundColor(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor)
				.OnTextChanged_Raw(dynVar, &FTouchEngineDynamicVariableStruct::HandleTextBoxTextChanged)
				.OnTextCommitted_Raw(dynVar, &FTouchEngineDynamicVariableStruct::HandleTextBoxTextCommited)
				.SelectAllTextOnCommit(true)
				.Text_Raw(dynVar, &FTouchEngineDynamicVariableStruct::HandleTextBoxText)
				];
		}
		break;
		*/
		case EVarType::VARTYPE_TEXTURE:
		{
			TSharedPtr<IPropertyHandle> textureHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTEDynamicVariable, textureProperty));
			textureHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTEDynamicVariable::HandleTextureChanged));
			auto textureWidget = textureHandle->CreatePropertyValueWidget();
			textureWidget->SetEnabled(false);

			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
				]
			.ValueContent()
				[
					textureWidget
				]
			;
		}
		break;
		// string data
		case EVarType::VARTYPE_STRING:
		{
			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
				]
			.ValueContent()
				[
					SNew(STextBlock)
					.Text(FText::FromString("String array will be filled at runtime"))
				]

			;
		}
		default:
			// VARTYPE_NOT_SET or VARTYPE_MAX
			break;
		}
	}
}


TSharedRef<IPropertyTypeCustomization> TouchEngineDynamicVariableStructDetailsCustomization::MakeInstance()
{
	return MakeShareable(new TouchEngineDynamicVariableStructDetailsCustomization);
}

void TouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded()
{
	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Purple, FString::Printf(TEXT("ToxLoaded")));

	RerenderPanel();
}

void TouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad()
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Purple, FString::Printf(TEXT("Tox Failed Load")));

	RerenderPanel();
}

void TouchEngineDynamicVariableStructDetailsCustomization::RerenderPanel()
{
	if (PropUtils.IsValid())
	{
		PropUtils->ForceRefresh();
	}
}


FSlateColor TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor() const
{
	//if (InputValid)
	//{
	//	static const FName InvertedForegroundName("InvertedForeground");
	//	return FEditorStyle::GetSlateColor(InvertedForegroundName);
	//}

	return FSlateColor(FLinearColor::Red);
}

void TouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild(TSharedRef<IPropertyHandle> ElementHandle, int32 ChildIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	FDetailWidgetRow& ElementRow = ChildrenBuilder.AddCustomRow(FText::FromString(FString::Printf(TEXT("Row%i"), ChildIndex)));

	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);
	float value = *(float*)(RawData[0]);


	ElementRow
		.NameContent()
		[
			ElementHandle->CreatePropertyNameWidget()
		]
	.ValueContent()
		.MaxDesiredWidth(0.f)
		.MinDesiredWidth(125.f)
		[
			ElementHandle->CreatePropertyValueWidget()
		]
	;
}

TSharedRef<SWidget> TouchEngineDynamicVariableStructDetailsCustomization::CreateNameWidget(FString name, FString tooltip, TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	auto nameContent = StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(name), FText::FromString(tooltip));
	//nameContent->SetToolTipText(FText::FromString(tooltip));
	//SetChildWidgetsTooltips(tooltip, nameContent);

	return nameContent;
}

void TouchEngineDynamicVariableStructDetailsCustomization::SetChildWidgetsTooltips(FString tooltip, TSharedRef<SWidget> widget)
{
	FChildren* children = widget->GetAllChildren();

	for (int i = 0; i < children->Num(); i++)
	{
		TSharedRef<SWidget> child = children->GetChildAt(i);

		child->SetToolTipText(FText::FromString(tooltip));
		SetChildWidgetsTooltips(tooltip, child);
	}
}
