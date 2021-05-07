// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineDynVarDetsCust.h"
#include "PropertyHandle.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineComponent.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "PropertyCustomizationHelpers.h"
#include "IDetailGroup.h"

TouchEngineDynamicVariableStructDetailsCustomization::TouchEngineDynamicVariableStructDetailsCustomization()
{

}

TouchEngineDynamicVariableStructDetailsCustomization::~TouchEngineDynamicVariableStructDetailsCustomization()
{
	if (ToxLoaded_DelegateHandle.IsValid())
	{

		DynVars->Unbind_OnToxLoaded(ToxLoaded_DelegateHandle);
	}
	if (ToxFailedLoad_DelegateHandle.IsValid())
	{
		//if ()
		DynVars->Unbind_OnToxFailedLoad(ToxFailedLoad_DelegateHandle);
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	PropertyHandle = StructPropertyHandle;

	pendingRedraw = false;

	TArray<UObject*> objs;
	StructPropertyHandle->GetOuterObjects(objs);
	for (UObject* obj : objs)
	{
		blueprintObject = obj->GetOuter();
	}

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

	//DynVars->parent->CreateEngineInfo();
	if (ToxFailedLoad_DelegateHandle.IsValid())
	{
		DynVars->Unbind_OnToxFailedLoad(ToxFailedLoad_DelegateHandle);
		ToxFailedLoad_DelegateHandle.Reset();
	}

	ToxFailedLoad_DelegateHandle = DynVars->CallOrBind_OnToxFailedLoad(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad));

	// check tox file load state
	if (!DynVars->parent->IsLoaded())
	{
		// tox file is not loaded yet
		if (!DynVars->parent->HasFailedLoad())
		{
			// file still loading, run throbber
			HeaderRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(FString("Tox Parameters")), FText::FromString(FString("Input and output variables as read from the TOX file")), false)
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
		else
		{
			// we have failed to load the tox file
			HeaderRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(FString("Tox Parameters")), FText::FromString(FString("Input and output variables as read from the TOX file")), false)
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
	else
	{
		// tox file has finished loading
		if (!DynVars->parent->HasFailedLoad())
		{
			// tox file is loaded, do not run throbber
			HeaderRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(FString("Tox Parameters")), FText::FromString(FString("Input and output variables as read from the TOX file")), false)
				]
			;
		}
		else
		{
			// this should not be hit, just here to ensure throbber does not run forever in unexpected state
			// we have failed to load the tox file
			HeaderRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(FString("Tox Parameters")), FText::FromString(FString("Input and output variables as read from the TOX file")), false)
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
}

void TouchEngineDynamicVariableStructDetailsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	PropertyHandle = StructPropertyHandle;

	TArray<UObject*> objs;
	StructPropertyHandle->GetOuterObjects(objs);
	for (UObject* obj : objs)
	{
		blueprintObject = obj->GetOuter();
	}

	StructPropertyHandle->GetProperty()->GetOwnerClass();
	PropUtils = StructCustomizationUtils.GetPropertyUtilities();

	FDetailWidgetRow& buttonRow = StructBuilder.AddCustomRow(FText::FromString("ReloadTox"));


	// Add groups to hold input and output variables
	IDetailGroup* InputGroup = &StructBuilder.AddGroup(FName("Inputs"), FText::FromString("Inputs"));
	IDetailGroup* OutputGroup = &StructBuilder.AddGroup(FName("Outputs"), FText::FromString("Outputs"));

	// Add "Reload Tox" button to the details panel 
	buttonRow.ValueContent()
		[
			SNew(SButton)
			.Text(TAttribute<FText>(FText::FromString("Reload Tox")))
		.OnClicked(FOnClicked::CreateSP(this, &TouchEngineDynamicVariableStructDetailsCustomization::OnReloadClicked))
		]
	;

	// handle input variables 
	TSharedPtr<IPropertyHandleArray> inputsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Input))->AsArray();
	uint32 numInputs = 0u;
	auto result = inputsHandle->GetNumElements(numInputs);

	// make sure data is valid
	for (uint32 i = 0; i < numInputs; i++)
	{
		TSharedRef<IPropertyHandle> dynVarHandle = inputsHandle->GetElement(i);
		FTouchEngineDynamicVariable* dynVar;

		TArray<void*> RawData;
		dynVarHandle->AccessRawData(RawData);
		dynVar = static_cast<FTouchEngineDynamicVariable*>(RawData[0]);

		if (dynVar->VarName == "ERROR_NAME")
		{
			DynVars->parent->LoadParameters();
			break;
		}
	}

	if (ToxLoaded_DelegateHandle.IsValid())
	{
		DynVars->Unbind_OnToxLoaded(ToxLoaded_DelegateHandle);
		ToxLoaded_DelegateHandle.Reset();
	}
	ToxLoaded_DelegateHandle = DynVars->CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded));


	for (uint32 i = 0; i < numInputs; i++)
	{
		TSharedRef<IPropertyHandle> dynVarHandle = inputsHandle->GetElement(i);
		FTouchEngineDynamicVariable* dynVar;

		{
			TArray<void*> RawData;
			dynVarHandle->AccessRawData(RawData);
			dynVar = static_cast<FTouchEngineDynamicVariable*>(RawData[0]);
		}

		switch (dynVar->VarType)
		{
		case EVarType::VARTYPE_BOOL:
		{
			FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

			switch (dynVar->VarIntent)
			{
			case EVarIntent::VARINTENT_NOT_SET:
			{
				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, dynVar, dynVarHandle)
					.IsChecked_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, dynVar)
					];
			}
			break;
			case EVarIntent::VARINTENT_MOMENTARY:
			{
				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, dynVar, dynVarHandle)
					.IsChecked_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, dynVar)
					];
			}
			break;
			case EVarIntent::VARINTENT_PULSE:
			{
				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, dynVar, dynVarHandle)
					.IsChecked_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, dynVar)
					];
			}
			break;
			}
		}
		break;
		case EVarType::VARTYPE_INT:
		{
			if (dynVar->count == 1)
			{
				FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

				if (dynVar->VarIntent != EVarIntent::VARINTENT_DROPDOWN)
				{
					newRow.NameContent()
						[
							CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
						]
					.ValueContent()
						[
							SNew(SNumericEntryBox<int>)
							.OnValueCommitted_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<int>, dynVar)
						.AllowSpin(false)
						.Value_Raw(dynVar, &FTouchEngineDynamicVariable::GetValueAsOptionalInt)
						];
				}
				else
				{
					TArray<TSharedPtr<FString>>* dropDownStrings = new TArray<TSharedPtr<FString>>();

					TArray<FString> keys; dynVar->dropDownData.GetKeys(keys);
					for (int j = 0; j < keys.Num(); j++)
					{
						dropDownStrings->Add(TSharedPtr<FString>(new FString(keys[j])));
					}

					newRow.NameContent()
						[
							CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
						]
					.ValueContent()
						[
							SNew(STextComboBox)
							.OptionsSource(dropDownStrings)
						.InitiallySelectedItem((*dropDownStrings)[0])
						.OnSelectionChanged_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleDropDownBoxValueChanged, dynVar)
						]
					;
				}
			}
			else
			{
				for (int j = 0; j < dynVar->count; j++)
				{
					FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

					newRow.NameContent()
						[
							CreateNameWidget((FString(dynVar->VarLabel).Append(FString::FromInt(j + 1))), dynVar->VarName, StructPropertyHandle)
						]
					.ValueContent()
						[
							SNew(SNumericEntryBox<int>)
							.OnValueCommitted(SNumericEntryBox<int>::FOnValueCommitted::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex<int>, j, dynVar))
						.AllowSpin(false)
						.Value(TAttribute<TOptional<int>>::Create(TAttribute<TOptional<int>>::FGetter::CreateRaw(dynVar, &FTouchEngineDynamicVariable::GetIndexedValueAsOptionalInt, j)))
						];
				}
			}
		}
		break;
		case EVarType::VARTYPE_DOUBLE:
		{
			if (dynVar->count == 1)
			{
				FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					[
						SNew(SNumericEntryBox<double>)
						.OnValueCommitted_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<double>, dynVar)
					.AllowSpin(false)
					.Value_Raw(dynVar, &FTouchEngineDynamicVariable::GetValueAsOptionalDouble)
					]
				;
			}
			else
			{
				switch (dynVar->VarIntent)
				{
				case EVarIntent::VARINTENT_NOT_SET:
				{
					for (int j = 0; j < dynVar->count; j++)
					{
						FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

						newRow.NameContent()
							[
								CreateNameWidget((FString(dynVar->VarLabel).Append(FString::FromInt(j + 1))), dynVar->VarName, StructPropertyHandle)
							]
						.ValueContent()
							[
								SNew(SNumericEntryBox<double>)
								.OnValueCommitted(SNumericEntryBox<double>::FOnValueCommitted::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex, j, dynVar))
							.AllowSpin(false)
							.Value(TAttribute<TOptional<double>>::Create(TAttribute<TOptional<double>>::FGetter::CreateRaw(dynVar, &FTouchEngineDynamicVariable::GetIndexedValueAsOptionalDouble, j)))
							]
						;
					}
				}
				break;
				case EVarIntent::VARINTENT_COLOR:
				{
					TSharedPtr<IPropertyHandle> colorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariable, colorProperty));
					colorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleColorChanged, dynVar));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(colorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));
				}
				break;
				case EVarIntent::VARINTENT_POSITION:
				{
					TSharedPtr<IPropertyHandle> vectorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariable, vector4Property));
					vectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleVector4Changed, dynVar));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(vectorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));
				}
				break;
				case EVarIntent::VARINTENT_UVW:
				{
					TSharedPtr<IPropertyHandle> vectorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariable, vectorProperty));
					vectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleVectorChanged, dynVar));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(vectorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));
					// values will be named xyz for now
				}
				break;
				}
			}
		}
		break;
		case EVarType::VARTYPE_FLOAT:
		{
			FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
				]
			.ValueContent()
				[
					SNew(SNumericEntryBox<float>)
					.OnValueCommitted_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<float>, dynVar)
				.AllowSpin(false)
				.Value_Raw(dynVar, &FTouchEngineDynamicVariable::GetValueAsOptionalFloat)
				];
		}
		break;
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			TSharedPtr<IPropertyHandle> floatsHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariable, floatBufferProperty));

			floatsHandle->SetPropertyDisplayName(FText::FromString(dynVar->VarLabel));
			floatsHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChanged, dynVar));
			floatsHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChildChanged, dynVar));
			floatsHandle->SetToolTipText(FText::FromString(dynVar->VarName));

			TSharedPtr<IPropertyHandleArray> floatsArrayHandle = floatsHandle->AsArray();

			TSharedRef<FDetailArrayBuilder> arrayBuilder = MakeShareable(new FDetailArrayBuilder(floatsHandle.ToSharedRef()));
			arrayBuilder->SetDisplayName(FText::FromString(dynVar->VarLabel));
			arrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild));

			//StructBuilder.AddCustomBuilder(arrayBuilder);
			InputGroup->AddPropertyRow(floatsHandle.ToSharedRef());
		}
		break;
		case EVarType::VARTYPE_STRING:
		{
			if (!dynVar->isArray)
			{

				FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					.MaxDesiredWidth(0.0f)
					.MinDesiredWidth(125.0f)
					[
						SNew(SEditableTextBox)
						.ClearKeyboardFocusOnCommit(false)
					.IsEnabled(true)
					.ForegroundColor(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor)
					.OnTextChanged_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextChanged, dynVar)
					.OnTextCommitted_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommited, dynVar)
					.SelectAllTextOnCommit(true)
					.Text_Raw(dynVar, &FTouchEngineDynamicVariable::HandleTextBoxText)
					];
			}
			else
			{
				auto stringHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariable, stringArrayProperty));

				stringHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChanged, dynVar));
				stringHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChildChanged, dynVar));
				stringHandle->SetToolTipText(FText::FromString(dynVar->VarName));

				auto floatsArrayHandle = stringHandle->AsArray();

				TSharedRef<FDetailArrayBuilder> arrayBuilder = MakeShareable(new FDetailArrayBuilder(stringHandle.ToSharedRef()));
				arrayBuilder->SetDisplayName(FText::FromString(dynVar->VarLabel));
				arrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild));

				//StructBuilder.AddCustomBuilder(arrayBuilder);
				InputGroup->AddPropertyRow(stringHandle.ToSharedRef());
			}
		}
		break;
		case EVarType::VARTYPE_TEXTURE:
		{
			FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();
			TSharedPtr<IPropertyHandle> textureHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariable, textureProperty));
			textureHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextureChanged, dynVar));

			// check for strange state world property details panel can be in 
			if (dynVar->textureProperty == nullptr && dynVar->value)
			{
				// value is set but texture property is empty, set texture property from value
				dynVar->textureProperty = dynVar->GetValueAsTexture();
			}

			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
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
		FTouchEngineDynamicVariable* dynVar;

		{
			TArray<void*> RawData;
			PropertyHandle->AccessRawData(RawData);
			dynVarHandle->AccessRawData(RawData);
			dynVar = static_cast<FTouchEngineDynamicVariable*>(RawData[0]);
		}

		FDetailWidgetRow& newRow = OutputGroup->AddWidgetRow();

		switch (dynVar->VarType)
		{
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
				]
			.ValueContent()
				[
					SNew(STextBlock)
					.Text(FText::FromString("Float array will be filled at runtime"))
				]
			;
		}
		break;
		case EVarType::VARTYPE_TEXTURE:
		{
			TSharedPtr<IPropertyHandle> textureHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariable, textureProperty));
			textureHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextureChanged, dynVar));
			auto textureWidget = textureHandle->CreatePropertyValueWidget();
			textureWidget->SetEnabled(false);

			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
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
					CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
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
	return MakeShareable(new TouchEngineDynamicVariableStructDetailsCustomization());
}

void TouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded()
{
	RerenderPanel();
}

void TouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad()
{
	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Purple, FString::Printf(TEXT("Tox Failed Load")));

	//RerenderPanel();
}

void TouchEngineDynamicVariableStructDetailsCustomization::RerenderPanel()
{
	if (PropUtils.IsValid() && !pendingRedraw)
	{
		//if (this->DynVars->parent->IsPendingKill())
		//	return;

		PropUtils->ForceRefresh();

		pendingRedraw = true;
	}

}


FSlateColor TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	return FEditorStyle::GetSlateColor(InvertedForegroundName);
}

void TouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild(TSharedRef<IPropertyHandle> ElementHandle, int32 ChildIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	PropertyHandle->NotifyPreChange();

	FDetailWidgetRow& ElementRow = ChildrenBuilder.AddCustomRow(FText::FromString(FString::Printf(TEXT("Row%i"), ChildIndex)));

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

	PropertyHandle->NotifyPostChange();
}

TSharedRef<SWidget> TouchEngineDynamicVariableStructDetailsCustomization::CreateNameWidget(FString name, FString tooltip, TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	// Simple function, but makes it easier to mass modify / standardize widget names and tooltips
	auto nameContent = StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(name), FText::FromString(tooltip), false);

	return nameContent;
}


FReply TouchEngineDynamicVariableStructDetailsCustomization::OnReloadClicked()
{
	PropertyHandle->NotifyPreChange();

	if (DynVars && DynVars->parent)
		DynVars->parent->ReloadTox();

	PropertyHandle->NotifyPostChange();

	RerenderPanel();

	return FReply::Handled();
}


void TouchEngineDynamicVariableStructDetailsCustomization::HandleChecked(ECheckBoxState InState, FTouchEngineDynamicVariable* dynVar, TSharedRef<IPropertyHandle> dynVarHandle)
{
	dynVarHandle->NotifyPreChange();

	FTouchEngineDynamicVariable oldValue; oldValue.Copy(dynVar);

	//dynVar->HandleChecked(InState, blueprintObject, DynVars->parent);
	dynVar->HandleChecked(InState);

	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

	dynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
}

FText TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxText(FTouchEngineDynamicVariable* dynVar) const
{
	FText returnVal = dynVar->HandleTextBoxText();
	return returnVal;
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextChanged(const FText& NewText, FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();

	FTouchEngineDynamicVariable oldValue; oldValue.Copy(dynVar);
	//dynVar->HandleTextBoxTextChanged(NewText, blueprintObject, DynVars->parent);
	dynVar->HandleTextBoxTextChanged(NewText);
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

	PropertyHandle->NotifyPostChange();
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo, FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();

	FTouchEngineDynamicVariable oldValue; oldValue.Copy(dynVar);
	//dynVar->HandleTextBoxTextCommited(NewText, CommitInfo, blueprintObject, DynVars->parent);
	dynVar->HandleTextBoxTextCommited(NewText);
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

	PropertyHandle->NotifyPostChange();
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleTextureChanged(FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();

	FTouchEngineDynamicVariable oldValue; oldValue.Copy(dynVar);
	//dynVar->HandleTextureChanged(blueprintObject, DynVars->parent);
	dynVar->HandleTextureChanged();
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

	PropertyHandle->NotifyPostChange();
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleColorChanged(FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();

	FTouchEngineDynamicVariable oldValue; oldValue.Copy(dynVar);
	//dynVar->HandleColorChanged(blueprintObject, DynVars->parent);
	dynVar->HandleColorChanged();
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

	PropertyHandle->NotifyPostChange();
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleVector4Changed(FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();

	FTouchEngineDynamicVariable oldValue; oldValue.Copy(dynVar);
	//dynVar->HandleVector4Changed(blueprintObject, DynVars->parent);
	dynVar->HandleVector4Changed();
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

	PropertyHandle->NotifyPostChange();
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleVectorChanged(FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();

	FTouchEngineDynamicVariable oldValue; oldValue.Copy(dynVar);
	//dynVar->HandleVectorChanged(blueprintObject, DynVars->parent);
	dynVar->HandleVectorChanged();
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

	PropertyHandle->NotifyPostChange();
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChanged(FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();

	FTouchEngineDynamicVariable oldValue; oldValue.Copy(dynVar);
	//dynVar->HandleFloatBufferChanged(blueprintObject, DynVars->parent);
	dynVar->HandleFloatBufferChanged();
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

	PropertyHandle->NotifyPostChange();
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChildChanged(FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();

	FTouchEngineDynamicVariable oldValue; oldValue.Copy(dynVar);
	//dynVar->HandleFloatBufferChildChanged(blueprintObject, DynVars->parent);
	dynVar->HandleFloatBufferChildChanged();
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

	PropertyHandle->NotifyPostChange();
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChanged(FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();

	FTouchEngineDynamicVariable oldValue; oldValue.Copy(dynVar);
	//dynVar->HandleStringArrayChanged(blueprintObject, DynVars->parent);
	dynVar->HandleStringArrayChanged();
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

	PropertyHandle->NotifyPostChange();
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChildChanged(FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();

	FTouchEngineDynamicVariable oldValue; oldValue.Copy(dynVar);
	//dynVar->HandleStringArrayChildChanged(blueprintObject, DynVars->parent);
	dynVar->HandleStringArrayChildChanged();
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

	PropertyHandle->NotifyPostChange();
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleDropDownBoxValueChanged(TSharedPtr<FString> arg, ESelectInfo::Type selectType, FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();

	FTouchEngineDynamicVariable oldValue; oldValue.Copy(dynVar);
	dynVar->HandleDropDownBoxValueChanged(arg);
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

	PropertyHandle->NotifyPostChange();
}

ECheckBoxState TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState(FTouchEngineDynamicVariable* dynVar) const
{
	return dynVar->GetValueAsBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}



void TouchEngineDynamicVariableStructDetailsCustomization::UpdateDynVarInstances(UObject* blueprintOwner, UTouchEngineComponentBase* parentComponent, FTouchEngineDynamicVariable oldVar, FTouchEngineDynamicVariable newVar)
{

	/*


	USceneComponent* SceneComp = Cast<USceneComponent>(SelectedNodePtr->FindComponentInstanceInActor(PreviewActor));
	USceneComponent* SelectedTemplate = Cast<USceneComponent>(SelectedNodePtr->GetOrCreateEditableComponentTemplate(BlueprintEditor->GetBlueprintObj()));


bool FInstancedStaticMeshSCSEditorCustomization::HandleViewportDrag(class USceneComponent* InSceneComponent, class USceneComponent* InComponentTemplate, const FVector& InDeltaTranslation, const FRotator& InDeltaRotation, const FVector& InDeltaScale, const FVector& InPivot)
{
	check(InSceneComponent->IsA(UInstancedStaticMeshComponent::StaticClass()));

	UInstancedStaticMeshComponent* InstancedStaticMeshComponentScene = CastChecked<UInstancedStaticMeshComponent>(InSceneComponent);
	UInstancedStaticMeshComponent* InstancedStaticMeshComponentTemplate = CastChecked<UInstancedStaticMeshComponent>(InComponentTemplate);

	// transform pivot into component's space
	const FVector LocalPivot = InstancedStaticMeshComponentScene->GetComponentToWorld().InverseTransformPosition(InPivot);

	// Ensure that selected instances are up-to-date
	ValidateSelectedInstances(InstancedStaticMeshComponentScene);

	bool bMovedInstance = false;
	check(InstancedStaticMeshComponentScene->SelectedInstances.Num() == InstancedStaticMeshComponentScene->PerInstanceSMData.Num());
	for(int32 InstanceIndex = 0; InstanceIndex < InstancedStaticMeshComponentScene->SelectedInstances.Num(); InstanceIndex++)
	{
		if (InstancedStaticMeshComponentScene->SelectedInstances[InstanceIndex] && InstancedStaticMeshComponentTemplate->PerInstanceSMData.IsValidIndex(InstanceIndex))
		{
			const FMatrix& MatrixScene = InstancedStaticMeshComponentScene->PerInstanceSMData[InstanceIndex].Transform;

			FVector Translation = MatrixScene.GetOrigin();
			FRotator Rotation = MatrixScene.Rotator();
			FVector Scale = MatrixScene.GetScaleVector();

			FVector NewTranslation = Translation;
			FRotator NewRotation = Rotation;
			FVector NewScale = Scale;

			if( !InDeltaRotation.IsZero() )
			{
				NewRotation = FRotator( InDeltaRotation.Quaternion() * Rotation.Quaternion() );

				NewTranslation -= LocalPivot;
				NewTranslation = FRotationMatrix( InDeltaRotation ).TransformPosition( NewTranslation );
				NewTranslation += LocalPivot;
			}

			NewTranslation += InDeltaTranslation;

			if( !InDeltaScale.IsNearlyZero() )
			{
				const FScaleMatrix ScaleMatrix( InDeltaScale );

				FVector DeltaScale3D = ScaleMatrix.TransformPosition( Scale );
				NewScale = Scale + DeltaScale3D;

				NewTranslation -= LocalPivot;
				NewTranslation += ScaleMatrix.TransformPosition( NewTranslation );
				NewTranslation += LocalPivot;
			}

			FMatrix& DefaultValue = InstancedStaticMeshComponentTemplate->PerInstanceSMData[InstanceIndex].Transform;
			const FTransform NewTransform(NewRotation, NewTranslation, NewScale);
			InstancedStaticMeshComponentScene->UpdateInstanceTransform(InstanceIndex, NewTransform);

			// Propagate the change to all other instances of the template.
			TArray<UObject*> ArchetypeInstances;
			InstancedStaticMeshComponentTemplate->GetArchetypeInstances(ArchetypeInstances);
			for (UObject* ArchetypeInstance : ArchetypeInstances)
			{
				UInstancedStaticMeshComponent* InstancedStaticMeshComponent = CastChecked<UInstancedStaticMeshComponent>(ArchetypeInstance);
				if (InstancedStaticMeshComponent->PerInstanceSMData.IsValidIndex(InstanceIndex))
				{
					if (InstancedStaticMeshComponent->PerInstanceSMData[InstanceIndex].Transform.Equals(DefaultValue))
					{
						InstancedStaticMeshComponent->UpdateInstanceTransform(InstanceIndex, NewTransform, false, true, true);
					}
				}
			}

			// Update the template.
			InstancedStaticMeshComponentTemplate->Modify();
			DefaultValue = InstancedStaticMeshComponentScene->PerInstanceSMData[InstanceIndex].Transform;

			bMovedInstance = true;
		}
	}

	return bMovedInstance;
}

	*/

	/*
	AActor* ownerArchetype = StaticCast<AActor*>(parentComponent->GetOwner()->GetArchetype());

	if (!ownerArchetype)
	{
		return;
	}

	auto ownerArchetypeComponents = ownerArchetype->GetComponentsByClass(UTouchEngineComponentBase::StaticClass());

	for (int i = 0; i < ownerArchetypeComponents.Num(); i++)
	{
		UTouchEngineComponentBase* archetype = CastChecked<UTouchEngineComponentBase>(ownerArchetypeComponents[i]);

		if (archetype->ToxFilePath == parentComponent->ToxFilePath)
		{

			// find this variable inside the component
			FTouchDynamicVar* dynVar = archetype->dynamicVariables.GetDynamicVariableByIdentifier(newVar.VarIdentifier);

			// didn't find the variable, or had a variable type mismatch.
			// This is an odd case, should probably reload the tox file and try again
			if (!dynVar || dynVar->VarType != newVar.VarType)
			{
				return;
			}
			// check if the value is the default value
			if (oldVar.Identical(dynVar, 0))
			{
				// if it is, replace it
				dynVar->SetValue(&newVar);
			}
		}
	}
	*/


	/*
	// update archetype
	UTouchEngineComponentBase* archetype = CastChecked<UTouchEngineComponentBase>(parentComponent->GetArchetype());

	if (archetype->ToxFilePath == parentComponent->ToxFilePath)
	{
		// find this variable inside the component
		FTouchDynamicVar* dynVar = archetype->dynamicVariables.GetDynamicVariableByIdentifier(newVar.VarIdentifier);

		// didn't find the variable, or had a variable type mismatch.
		// This is an odd case, should probably reload the tox file and try again
		if (!dynVar || dynVar->VarType != newVar.VarType)
		{
			return;
		}
		// check if the value is the default value
		if (oldVar.Identical(dynVar, 0))
		{
			// if it is, replace it
			dynVar->SetValue(&newVar);
		}
	}
	*/


	//TArray<UObject*> testclasses;
	//GetObjectsOfClass(blueprintOwner->GetClass(), testclasses);

	/*
	TArray<UObject*> ArchetypeInstances;
	TArray<AActor*> UpdatedInstances;

	AActor* archetype;
	// update archetype instance
	//if (parentComponent->GetOwner())
		archetype = StaticCast<AActor*>(parentComponent->GetOwner()->GetArchetype());
	else
		archetype = StaticCast<AActor*>(blueprintOwner->GetArchetype());


	auto comps2 = archetype->GetComponentsByClass(UTouchEngineComponentBase::StaticClass());

		if (!parentComponent->HasAnyFlags(RF_ArchetypeObject))
			return;
	archetype->GetArchetypeInstances(ArchetypeInstances);

	for (int32 InstanceIndex = 0; InstanceIndex < ArchetypeInstances.Num(); ++InstanceIndex)
	{
		//UTouchEngineComponentBase* InstancedTEComponent = static_cast<UTouchEngineComponentBase*>(ArchetypeInstances[InstanceIndex]);

		AActor* InstancedBlueprintObject = CastChecked<AActor>(ArchetypeInstances[InstanceIndex]);

		if (InstancedBlueprintObject != nullptr && !UpdatedInstances.Contains(InstancedBlueprintObject))
		{
			auto comps = InstancedBlueprintObject->GetComponentsByClass(UTouchEngineComponentBase::StaticClass());

			for (UActorComponent* InstancedComponent : comps)
			{
				UTouchEngineComponentBase* InstancedTEComponent = static_cast<UTouchEngineComponentBase*>(InstancedComponent);

				if (InstancedTEComponent == parentComponent)
					continue;

				if (InstancedTEComponent->ToxFilePath == parentComponent->ToxFilePath)
				{
					// find this variable inside the component
					FTouchDynamicVar* dynVar = InstancedTEComponent->dynamicVariables.GetDynamicVariableByIdentifier(newVar.VarIdentifier);

					// didn't find the variable, or had a variable type mismatch.
					// This is an odd case, should probably reload the tox file and try again
					if (!dynVar || dynVar->VarType != newVar.VarType)
					{
						continue;
					}
					// check if the value is the default value
					if (oldVar.Identical(dynVar, 0))
					{
						// if it is, replace it
						dynVar->SetValue(&newVar);
					}
				}
			}
		}
	}

	*/
	TArray<UObject*> ArchetypeInstances;
	TArray<UTouchEngineComponentBase*> UpdatedInstances;

	if (parentComponent->HasAnyFlags(RF_ArchetypeObject))
	{
		parentComponent->GetArchetypeInstances(ArchetypeInstances);

		for (int32 InstanceIndex = 0; InstanceIndex < ArchetypeInstances.Num(); ++InstanceIndex)
		{
			UTouchEngineComponentBase* InstancedTEComponent = static_cast<UTouchEngineComponentBase*>(ArchetypeInstances[InstanceIndex]);
			if (InstancedTEComponent != nullptr && !UpdatedInstances.Contains(InstancedTEComponent))
			{
				if (InstancedTEComponent->ToxFilePath == parentComponent->ToxFilePath)
				{
					// find this variable inside the component
					FTouchEngineDynamicVariable* dynVar = InstancedTEComponent->dynamicVariables.GetDynamicVariableByIdentifier(newVar.VarIdentifier);

					// didn't find the variable, or had a variable type mismatch.
					// This is an odd case, should probably reload the tox file and try again
					if (!dynVar || dynVar->VarType != newVar.VarType)
					{
						continue;
					}
					// check if the value is the default value
					if (oldVar.Identical(dynVar, 0))
					{
						// if it is, replace it
						dynVar->SetValue(&newVar);
					}
				}
			}
		}
	}

	/*
	CollisionResponsesHandle->NotifyPreChange();

		if(PrimComponents.Num() && UseDefaultCollisionHandle.IsValid())	//If we have prim components we might be coming from bp editor which needs to propagate all instances
		{
			for(UPrimitiveComponent* PrimComp : PrimComponents)
			{
				if(UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(PrimComp))
				{
					const bool bOldDefault = SMC->bUseDefaultCollision;
					const bool bNewDefault = bUseDefaultCollision;

					TSet<USceneComponent*> UpdatedInstances;
					FComponentEditorUtils::PropagateDefaultValueChange(SMC, UseDefaultCollisionHandle->GetProperty(), bOldDefault, bNewDefault, UpdatedInstances);

					SMC->bUseDefaultCollision = bNewDefault;
				}
			}
		}
		else
		{
			for (const FBodyInstance* BI : BodyInstances)
			{
				if (UStaticMeshComponent* SMC = GetDefaultCollisionProvider(BI))
				{
					SMC->bUseDefaultCollision = bUseDefaultCollision;
				}
			}
		}

		CollisionResponsesHandle->NotifyPostChange();
		*/




}