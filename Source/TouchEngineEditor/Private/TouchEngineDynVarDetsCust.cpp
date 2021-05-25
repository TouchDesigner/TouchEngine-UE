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
	if (DynVarsDestroyed)
		return;

	if (DynVars && DynVars->parent && !DynVars->parent->IsBeingDestroyed())
	{
		if (DynVars->parent->ToxFilePath.IsEmpty())
		{
			return;
		}

		if (ToxLoaded_DelegateHandle.IsValid())
		{
			DynVars->Unbind_OnToxLoaded(ToxLoaded_DelegateHandle);
		}
		if (ToxFailedLoad_DelegateHandle.IsValid())
		{
			DynVars->Unbind_OnToxFailedLoad(ToxFailedLoad_DelegateHandle);
		}
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

	ToxFailedLoad_DelegateHandle = DynVars->OnToxFailedLoad.Add(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad));//DynVars->CallOrBind_OnToxFailedLoad(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad));

	if (ToxLoaded_DelegateHandle.IsValid())
	{
		DynVars->Unbind_OnToxLoaded(ToxLoaded_DelegateHandle);
		ToxLoaded_DelegateHandle.Reset();
	}

	ToxLoaded_DelegateHandle = DynVars->OnToxLoaded.Add(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded));//DynVars->CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded));

	DynVars->OnDestruction.BindRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::OnDynVarsDestroyed);

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
				.MaxDesiredWidth(250)
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
				.MaxDesiredWidth(250)
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
				.MaxDesiredWidth(250)
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
		FTEDynamicVariableStruct* dynVar;

		TArray<void*> RawData;
		dynVarHandle->AccessRawData(RawData);
		dynVar = static_cast<FTEDynamicVariableStruct*>(RawData[0]);

		if (dynVar->VarName == "ERROR_NAME")
		{
			return;
		}
	}

	for (uint32 i = 0; i < numInputs; i++)
	{
		TSharedRef<IPropertyHandle> dynVarHandle = inputsHandle->GetElement(i);
		FTEDynamicVariableStruct* dynVar;

		TArray<void*> RawData;
		dynVarHandle->AccessRawData(RawData);
		dynVar = static_cast<FTEDynamicVariableStruct*>(RawData[0]);

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
					.MaxDesiredWidth(250)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, dynVar, dynVarHandle)
					.IsChecked_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, dynVar->VarIdentifier)
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
					.MaxDesiredWidth(250)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, dynVar, dynVarHandle)
					.IsChecked_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, dynVar->VarIdentifier)
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
					.MaxDesiredWidth(250)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, dynVar, dynVarHandle)
					.IsChecked_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, dynVar->VarIdentifier)
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
						.MaxDesiredWidth(250)
						[
							SNew(SNumericEntryBox<int>)
							.OnValueCommitted_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<int>, dynVar->VarIdentifier)
						.AllowSpin(false)
						.Value_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalInt, dynVar->VarIdentifier)
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
						.MaxDesiredWidth(250)
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
						.MaxDesiredWidth(250)
						[
							SNew(SNumericEntryBox<int>)
							.OnValueCommitted(SNumericEntryBox<int>::FOnValueCommitted::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex<int>, j, dynVar->VarIdentifier))
						.AllowSpin(false)
						.Value(TAttribute<TOptional<int>>::Create(TAttribute<TOptional<int>>::FGetter::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalInt, j, dynVar->VarIdentifier)))
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
					.MaxDesiredWidth(250)
					[
						SNew(SNumericEntryBox<double>)
						.OnValueCommitted_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<double>, dynVar->VarIdentifier)
					.AllowSpin(false)
					.Value_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalDouble, dynVar->VarIdentifier)
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
							.MaxDesiredWidth(250)
							[
								SNew(SNumericEntryBox<double>)
								.OnValueCommitted(SNumericEntryBox<double>::FOnValueCommitted::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex, j, dynVar->VarIdentifier))
							.AllowSpin(false)
							.Value(TAttribute<TOptional<double>>::Create(TAttribute<TOptional<double>>::FGetter::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalDouble, j, dynVar->VarIdentifier)))
							]
						;
					}
				}
				break;
				case EVarIntent::VARINTENT_COLOR:
				{
					TSharedPtr<IPropertyHandle> colorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTEDynamicVariableStruct, colorProperty));
					colorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleColorChanged, dynVar));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(colorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));
				}
				break;
				case EVarIntent::VARINTENT_POSITION:
				{
					TSharedPtr<IPropertyHandle> vectorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTEDynamicVariableStruct, vector4Property));
					vectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleVector4Changed, dynVar));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(vectorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));
				}
				break;
				case EVarIntent::VARINTENT_UVW:
				{
					TSharedPtr<IPropertyHandle> vectorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTEDynamicVariableStruct, vectorProperty));
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
					.OnValueCommitted_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<float>, dynVar->VarIdentifier)
				.AllowSpin(false)
				.Value_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalFloat, dynVar->VarIdentifier)
				];
		}
		break;
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			TSharedPtr<IPropertyHandle> floatsHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTEDynamicVariableStruct, floatBufferProperty));

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

				if (dynVar->VarIntent != EVarIntent::VARINTENT_DROPDOWN)
				{

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
						.Text_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxText, dynVar->VarIdentifier)
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
				auto stringHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTEDynamicVariableStruct, stringArrayProperty));

				stringHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChanged, dynVar));
				stringHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChildChanged, dynVar));
				stringHandle->SetToolTipText(FText::FromString(dynVar->VarName));
				stringHandle->SetPropertyDisplayName(FText::FromString(dynVar->VarLabel));

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
			TSharedPtr<IPropertyHandle> textureHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTEDynamicVariableStruct, textureProperty));
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
		FTEDynamicVariableStruct* dynVar;

		{
			TArray<void*> RawData;
			PropertyHandle->AccessRawData(RawData);
			dynVarHandle->AccessRawData(RawData);
			dynVar = static_cast<FTEDynamicVariableStruct*>(RawData[0]);
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
				.MaxDesiredWidth(250)
				[
					SNew(STextBlock)
					.Text(FText::FromString("CHOP data will be filled at runtime"))
				]

			;
		}
		break;
		case EVarType::VARTYPE_TEXTURE:
		{
			TSharedPtr<IPropertyHandle> textureHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTEDynamicVariableStruct, textureProperty));
			textureHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleTextureChanged, dynVar));
			auto textureWidget = textureHandle->CreatePropertyValueWidget();
			textureWidget->SetEnabled(false);

			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
				]
			.ValueContent()
				.MaxDesiredWidth(250)
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
				.MaxDesiredWidth(250)
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
	RerenderPanel();
}

void TouchEngineDynamicVariableStructDetailsCustomization::RerenderPanel()
{
	if (PropUtils.IsValid() && !pendingRedraw)
	{
		if (!DynVars || !DynVars->parent || DynVars->parent->IsPendingKill() || DynVars->parent->EngineInfo)
		{
			return;
		}

		PropUtils->ForceRefresh();

		pendingRedraw = true;

		if (ToxLoaded_DelegateHandle.IsValid())
		{
			if (DynVars && DynVars->parent)
				DynVars->Unbind_OnToxLoaded(ToxLoaded_DelegateHandle);
		}
		if (ToxFailedLoad_DelegateHandle.IsValid())
		{
			if (DynVars && DynVars->parent)
				DynVars->Unbind_OnToxFailedLoad(ToxFailedLoad_DelegateHandle);
		}
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
	if (DynVars->parent->EngineInfo)
	{
		// this will hit if we try to reload a tox file while during begin play, which can cause many errors
		return FReply::Handled();;
	}

	PropertyHandle->NotifyPreChange();

	if (DynVars && DynVars->parent)
		DynVars->parent->ReloadTox();

	PropertyHandle->NotifyPostChange();

	RerenderPanel();

	return FReply::Handled();
}


void TouchEngineDynamicVariableStructDetailsCustomization::HandleChecked(ECheckBoxState InState, FTEDynamicVariableStruct* dynVar, TSharedRef<IPropertyHandle> dynVarHandle)
{
	if (dynVar)
	{
		dynVarHandle->NotifyPreChange();

		FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);

		dynVar->HandleChecked(InState);

		UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		dynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextChanged(const FText& NewText, FTEDynamicVariableStruct* dynVar)
{
	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleTextBoxTextChanged(NewText);
		UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo, FTEDynamicVariableStruct* dynVar)
{
	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleTextBoxTextCommited(NewText);
		UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleTextureChanged(FTEDynamicVariableStruct* dynVar)
{
	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleTextureChanged();
		UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleColorChanged(FTEDynamicVariableStruct* dynVar)
{
	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleColorChanged();
		UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleVector4Changed(FTEDynamicVariableStruct* dynVar)
{
	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleVector4Changed();
		UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleVectorChanged(FTEDynamicVariableStruct* dynVar)
{
	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleVectorChanged();
		UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChanged(FTEDynamicVariableStruct* dynVar)
{
	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleFloatBufferChanged();
		UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChildChanged(FTEDynamicVariableStruct* dynVar)
{
	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleFloatBufferChildChanged();
		UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChanged(FTEDynamicVariableStruct* dynVar)
{
	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleStringArrayChanged();
		UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChildChanged(FTEDynamicVariableStruct* dynVar)
{
	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleStringArrayChildChanged();
		UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleDropDownBoxValueChanged(TSharedPtr<FString> arg, ESelectInfo::Type selectType, FTEDynamicVariableStruct* dynVar)
{
	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleDropDownBoxValueChanged(arg);
		UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}


ECheckBoxState TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState(FString Identifier) const
{
	FTEDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->value && dynVar->VarType == EVarType::VARTYPE_BOOL)
	{
		return dynVar->GetValueAsBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

TOptional<int> TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalInt(FString Identifier) const
{
	FTEDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->value && dynVar->VarType == EVarType::VARTYPE_INT)
	{
		return TOptional<int>(dynVar->GetValueAsInt());
	}
	else
	{
		return TOptional<int>(0);
	}
}

TOptional<int> TouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalInt(int index, FString Identifier) const
{
	FTEDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->value && dynVar->VarType == EVarType::VARTYPE_INT)
	{
		return TOptional<int>(dynVar->GetValueAsIntIndexed(index));
	}
	else
	{
		return TOptional<int>(0);
	}
}

TOptional<double> TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalDouble(FString Identifier) const
{
	FTEDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->value && dynVar->VarType == EVarType::VARTYPE_DOUBLE)
	{
		return TOptional<double>(dynVar->GetValueAsDouble());
	}
	else
	{
		return TOptional<double>(0);
	}
}

TOptional<double> TouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalDouble(int index, FString Identifier) const
{
	FTEDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->value && dynVar->VarType == EVarType::VARTYPE_DOUBLE)
	{
		return TOptional<double>(dynVar->GetValueAsDoubleIndexed(index));
	}
	else
	{
		return TOptional<double>(0);
	}
}

TOptional<float> TouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalFloat(FString Identifier) const
{
	FTEDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->value && dynVar->VarType == EVarType::VARTYPE_FLOAT)
	{
		return TOptional<float>(dynVar->GetValueAsFloat());
	}
	else
	{
		return  TOptional<float>(0);
	}
}

FText TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxText(FString Identifier) const
{
	FTEDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->value && dynVar->VarType == EVarType::VARTYPE_STRING)
	{
		return FText::FromString(dynVar->GetValueAsString());
	}
	else
	{
		return FText();
	}
}


void TouchEngineDynamicVariableStructDetailsCustomization::UpdateDynVarInstances(UObject* blueprintOwner, UTouchEngineComponentBase* parentComponent, FTEDynamicVariableStruct oldVar, FTEDynamicVariableStruct newVar)
{
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
					FTEDynamicVariableStruct* dynVar = InstancedTEComponent->dynamicVariables.GetDynamicVariableByIdentifier(newVar.VarIdentifier);

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
}

void TouchEngineDynamicVariableStructDetailsCustomization::OnDynVarsDestroyed()
{ 
	DynVarsDestroyed = true;
}
