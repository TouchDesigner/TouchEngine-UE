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


#define LOCTEXT_NAMESPACE "TouchEngineDynamicVariableDetailsCustomization"

FTouchEngineDynamicVariableStructDetailsCustomization::FTouchEngineDynamicVariableStructDetailsCustomization()
{

}

FTouchEngineDynamicVariableStructDetailsCustomization::~FTouchEngineDynamicVariableStructDetailsCustomization()
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

void FTouchEngineDynamicVariableStructDetailsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	PropertyHandle = StructPropertyHandle;

	PendingRedraw = false;

	TArray<UObject*> objs;
	StructPropertyHandle->GetOuterObjects(objs);
	for (UObject* obj : objs)
	{
		BlueprintObject = obj->GetOuter();
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

	DynVars->parent->ValidateParameters();

	//DynVars->parent->CreateEngineInfo();
	if (ToxFailedLoad_DelegateHandle.IsValid())
	{
		DynVars->Unbind_OnToxFailedLoad(ToxFailedLoad_DelegateHandle);
		ToxFailedLoad_DelegateHandle.Reset();
	}

	ToxFailedLoad_DelegateHandle = DynVars->OnToxFailedLoad.AddRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad);//DynVars->CallOrBind_OnToxFailedLoad(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad));

	if (ToxLoaded_DelegateHandle.IsValid())
	{
		DynVars->Unbind_OnToxLoaded(ToxLoaded_DelegateHandle);
		ToxLoaded_DelegateHandle.Reset();
	}

	ToxLoaded_DelegateHandle = DynVars->OnToxLoaded.Add(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded));//DynVars->CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded));

	DynVars->OnDestruction.BindRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnDynVarsDestroyed);

	// check tox file load state
	if (!DynVars->parent->IsLoaded())
	{
		// tox file is not loaded yet
		if (!DynVars->parent->HasFailedLoad())
		{

			// file still loading, run throbber
			HeaderRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(LOCTEXT("ToxParameters", "Tox Parameters"), LOCTEXT("Input and output variables as read from the TOX file", "InputOutput"), false)
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
			if (ErrorMessage.IsEmpty() && !DynVars->parent->ErrorMessage.IsEmpty())
			{
				ErrorMessage = DynVars->parent->ErrorMessage;
			}

			HeaderRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(LOCTEXT("ToxParameters", "Tox Parameters"), LOCTEXT("Input and output variables as read from the TOX file", "InputOutput"), false)
				]
			.ValueContent()
				.MaxDesiredWidth(250)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("ToxLoadFailed", "Failed to load TOX file: {ErrorMessage}"), FText::FromString(ErrorMessage)))
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
					StructPropertyHandle->CreatePropertyNameWidget(LOCTEXT("ToxParameters", "Tox Parameters"), LOCTEXT("InputOutput", "Input and output variables as read from the TOX file"), false)
				]
			;
		}
		else
		{
			// this should not be hit, just here to ensure throbber does not run forever in unexpected state
			// we have failed to load the tox file
			HeaderRow.NameContent()
				[
					StructPropertyHandle->CreatePropertyNameWidget(LOCTEXT("ToxParameters", "Tox Parameters"), LOCTEXT("InputOutput", "Input and output variables as read from the TOX file"), false)
				]
			.ValueContent()
				.MaxDesiredWidth(250)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ToxLoadFailed", "Failed to load TOX file"))
				]
			;
		}
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	PropertyHandle = StructPropertyHandle;

	TArray<UObject*> objs;
	StructPropertyHandle->GetOuterObjects(objs);
	for (UObject* obj : objs)
	{
		BlueprintObject = obj->GetOuter();
	}

	StructPropertyHandle->GetProperty()->GetOwnerClass();
	PropUtils = StructCustomizationUtils.GetPropertyUtilities();

	FDetailWidgetRow& buttonRow = StructBuilder.AddCustomRow(LOCTEXT("ReloadTox", "ReloadTox"));


	// Add groups to hold input and output variables
	IDetailGroup* InputGroup = &StructBuilder.AddGroup(FName("Inputs"), LOCTEXT("Inputs", "Inputs"));
	IDetailGroup* OutputGroup = &StructBuilder.AddGroup(FName("Outputs"), LOCTEXT("Outputs", "Outputs"));

	// Add "Reload Tox" button to the details panel 
	buttonRow.ValueContent()
		[
			SNew(SButton)
			.Text(TAttribute<FText>(LOCTEXT("ReloadTox", "Reload Tox")))
		.OnClicked(FOnClicked::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnReloadClicked))
		]
	;

	// handle input variables 
	TSharedPtr<IPropertyHandleArray> inputsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Input))->AsArray();
	uint32 numInputs = 0u;
	FPropertyAccess::Result result = inputsHandle->GetNumElements(numInputs);

	// make sure data is valid
	for (uint32 i = 0; i < numInputs; i++)
	{
		TSharedRef<IPropertyHandle> dynVarHandle = inputsHandle->GetElement(i);
		FTouchEngineDynamicVariableStruct* dynVar;

		TArray<void*> RawData;
		dynVarHandle->AccessRawData(RawData);

		if (RawData.Num() == 0 || !RawData[0])
		{
			return;
		}
		dynVar = static_cast<FTouchEngineDynamicVariableStruct*>(RawData[0]);

		if (dynVar->VarName == TEXT("ERROR_NAME"))
		{
			return;
		}
	}

	for (uint32 i = 0; i < numInputs; i++)
	{
		TSharedRef<IPropertyHandle> dynVarHandle = inputsHandle->GetElement(i);
		FTouchEngineDynamicVariableStruct* dynVar;

		TArray<void*> RawData;
		dynVarHandle->AccessRawData(RawData);
		dynVar = static_cast<FTouchEngineDynamicVariableStruct*>(RawData[0]);

		switch (dynVar->VarType)
		{
		case EVarType::Bool:
		{
			FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

			switch (dynVar->VarIntent)
			{
			case EVarIntent::NotSet:
			{
				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					.MaxDesiredWidth(250)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, dynVar->VarIdentifier, dynVarHandle)
					.IsChecked_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, dynVar->VarIdentifier)
					];
				break;
			}
			case EVarIntent::Momentary:
			{
				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					.MaxDesiredWidth(250)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, dynVar->VarIdentifier, dynVarHandle)
					.IsChecked_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, dynVar->VarIdentifier)
					];
				break;
			}
			case EVarIntent::Pulse:
			{
				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					.MaxDesiredWidth(250)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, dynVar->VarIdentifier, dynVarHandle)
					.IsChecked_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, dynVar->VarIdentifier)
					];
				break;
			}
			}
			break;
		}
		case EVarType::Int:
		{
			if (dynVar->Count == 1)
			{
				FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

				if (dynVar->VarIntent != EVarIntent::DropDown)
				{
					newRow.NameContent()
						[
							CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
						]
					.ValueContent()
						.MaxDesiredWidth(250)
						[
							SNew(SNumericEntryBox<int>)
							.OnValueCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<int>, dynVar->VarIdentifier)
						.AllowSpin(false)
						.Value_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalInt, dynVar->VarIdentifier)
						];
				}
				else
				{
					TArray<TSharedPtr<FString>>* dropDownStrings = new TArray<TSharedPtr<FString>>();

					TArray<FString> keys; dynVar->DropDownData.GetKeys(keys);
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
						.OnSelectionChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleDropDownBoxValueChanged, dynVar->VarIdentifier)
						]
					;
				}
			}
			else
			{
				switch (dynVar->Count)
				{
				case 2:
				{
					TSharedPtr<IPropertyHandle> intVector2DHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, IntPointProperty));
					intVector2DHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVector2Changed, dynVar->VarIdentifier));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(intVector2DHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));

					break;
				}
				case 3:
				{
					TSharedPtr<IPropertyHandle> intVectorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, IntVectorProperty));
					intVectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVectorChanged, dynVar->VarIdentifier));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(intVectorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));

					break;
				}
				case 4:
				{
					TSharedPtr<IPropertyHandle> intVector4Handle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, IntVector4Property));
					intVector4Handle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVector4Changed, dynVar->VarIdentifier));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(intVector4Handle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));

					break;
				}
				default:
				{
					for (int j = 0; j < dynVar->Count; j++)
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
								.OnValueCommitted(SNumericEntryBox<int>::FOnValueCommitted::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex<int>, j, dynVar->VarIdentifier))
							.AllowSpin(false)
							.Value(TAttribute<TOptional<int>>::Create(TAttribute<TOptional<int>>::FGetter::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalInt, j, dynVar->VarIdentifier)))
							];
						break;
					}
				}
				}
			}
			break;
		}
		case EVarType::Double:
		{
			if (dynVar->Count == 1)
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
						.OnValueCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<double>, dynVar->VarIdentifier)
					.AllowSpin(false)
					.Value_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalDouble, dynVar->VarIdentifier)
					]
				;
			}
			else
			{
				switch (dynVar->VarIntent)
				{
				case EVarIntent::NotSet:
				{

					switch (dynVar->Count)
					{
					case 2:
					{
						TSharedPtr<IPropertyHandle> vector2DHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, Vector2DProperty));
						vector2DHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector2Changed, dynVar->VarIdentifier));
						IDetailPropertyRow* property = &InputGroup->AddPropertyRow(vector2DHandle.ToSharedRef());
						property->ToolTip(FText::FromString(dynVar->VarName));
						property->DisplayName(FText::FromString(dynVar->VarLabel));

						break;
					}
					case 3:
					{
						TSharedPtr<IPropertyHandle> vectorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, VectorProperty));
						vectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleVectorChanged, dynVar->VarIdentifier));
						IDetailPropertyRow* property = &InputGroup->AddPropertyRow(vectorHandle.ToSharedRef());
						property->ToolTip(FText::FromString(dynVar->VarName));
						property->DisplayName(FText::FromString(dynVar->VarLabel));

						break;
					}
					case 4:
					{
						TSharedPtr<IPropertyHandle> vector4Handle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, Vector4Property));
						vector4Handle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector4Changed, dynVar->VarIdentifier));
						vector4Handle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector4ChildChanged, dynVar->VarIdentifier));
						IDetailPropertyRow* property = &InputGroup->AddPropertyRow(vector4Handle.ToSharedRef());
						property->ToolTip(FText::FromString(dynVar->VarName));
						property->DisplayName(FText::FromString(dynVar->VarLabel));

						break;
					}

					default:
					{
						for (int j = 0; j < dynVar->Count; j++)
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
									.OnValueCommitted(SNumericEntryBox<double>::FOnValueCommitted::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex, j, dynVar->VarIdentifier))
								.AllowSpin(false)
								.Value(TAttribute<TOptional<double>>::Create(TAttribute<TOptional<double>>::FGetter::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalDouble, j, dynVar->VarIdentifier)))
								]
							;
						}
						break;
					}
					}
					break;
				}
				case EVarIntent::Color:
				{
					TSharedPtr<IPropertyHandle> colorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, ColorProperty));
					colorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleColorChanged, dynVar->VarIdentifier));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(colorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));
					break;
				}
				case EVarIntent::Position:
				{
					TSharedPtr<IPropertyHandle> vectorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, Vector4Property));
					vectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector4Changed, dynVar->VarIdentifier));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(vectorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));
					break;
				}
				case EVarIntent::UVW:
				{
					TSharedPtr<IPropertyHandle> vectorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, VectorProperty));
					vectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleVectorChanged, dynVar->VarIdentifier));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(vectorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));
					// values will be named xyz for now
					break;
				}
				}
			}
			break;
		}
		case EVarType::Float:
		{
			FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
				]
			.ValueContent()
				[
					SNew(SNumericEntryBox<float>)
					.OnValueCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<float>, dynVar->VarIdentifier)
				.AllowSpin(false)
				.Value_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalFloat, dynVar->VarIdentifier)
				];
			break;
		}
		case EVarType::CHOP:
		{
			TSharedPtr<IPropertyHandle> floatsHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, FloatBufferProperty));

			floatsHandle->SetPropertyDisplayName(FText::FromString(dynVar->VarLabel));
			floatsHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChanged, dynVar->VarIdentifier));
			floatsHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChildChanged, dynVar->VarIdentifier));
			floatsHandle->SetToolTipText(FText::FromString(dynVar->VarName));

			TSharedPtr<IPropertyHandleArray> floatsArrayHandle = floatsHandle->AsArray();

			TSharedRef<FDetailArrayBuilder> arrayBuilder = MakeShareable(new FDetailArrayBuilder(floatsHandle.ToSharedRef()));
			arrayBuilder->SetDisplayName(FText::FromString(dynVar->VarLabel));
			arrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild));

			InputGroup->AddPropertyRow(floatsHandle.ToSharedRef());
			break;
		}
		case EVarType::String:
		{
			if (!dynVar->IsArray)
			{
				FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

				if (dynVar->VarIntent != EVarIntent::DropDown)
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
						.ForegroundColor(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor)
						.OnTextChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextChanged, dynVar->VarIdentifier)
						.OnTextCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommited, dynVar->VarIdentifier)
						.SelectAllTextOnCommit(true)
						.Text_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxText, dynVar->VarIdentifier)
						];
				}
				else
				{
					TArray<TSharedPtr<FString>>* dropDownStrings = new TArray<TSharedPtr<FString>>();

					TArray<FString> keys; dynVar->DropDownData.GetKeys(keys);
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
						.OnSelectionChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleDropDownBoxValueChanged, dynVar->VarIdentifier)
						]
					;
				}
			}
			else
			{
				TSharedPtr<IPropertyHandle> stringHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, StringArrayProperty));

				stringHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChanged, dynVar->VarIdentifier));
				stringHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChildChanged, dynVar->VarIdentifier));
				stringHandle->SetToolTipText(FText::FromString(dynVar->VarName));
				stringHandle->SetPropertyDisplayName(FText::FromString(dynVar->VarLabel));

				TSharedPtr<IPropertyHandleArray> floatsArrayHandle = stringHandle->AsArray();

				TSharedRef<FDetailArrayBuilder> arrayBuilder = MakeShareable(new FDetailArrayBuilder(stringHandle.ToSharedRef()));
				arrayBuilder->SetDisplayName(FText::FromString(dynVar->VarLabel));
				arrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild));

				//StructBuilder.AddCustomBuilder(arrayBuilder);
				InputGroup->AddPropertyRow(stringHandle.ToSharedRef());
			}
		}
		break;
		case EVarType::Texture:
		{
			FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();
			TSharedPtr<IPropertyHandle> textureHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, TextureProperty));
			textureHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextureChanged, dynVar->VarIdentifier));

			// check for strange state world property details panel can be in 
			if (dynVar->TextureProperty == nullptr && dynVar->Value)
			{
				// value is set but texture property is empty, set texture property from value
				dynVar->TextureProperty = dynVar->GetValueAsTexture();
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
			break;
		}
		default:
		{
			break;
		}
		}
	}

	// handle output variables
	TSharedPtr<IPropertyHandleArray> outputsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Output))->AsArray();
	uint32 numOutputs = 0u;
	outputsHandle->GetNumElements(numOutputs);

	for (uint32 i = 0; i < numOutputs; i++)
	{
		TSharedRef<IPropertyHandle> dynVarHandle = outputsHandle->GetElement(i);
		FTouchEngineDynamicVariableStruct* dynVar;

		{
			TArray<void*> RawData;
			PropertyHandle->AccessRawData(RawData);
			dynVarHandle->AccessRawData(RawData);
			dynVar = static_cast<FTouchEngineDynamicVariableStruct*>(RawData[0]);
		}

		FDetailWidgetRow& newRow = OutputGroup->AddWidgetRow();

		switch (dynVar->VarType)
		{
		case EVarType::CHOP:
		{
			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
				]
			.ValueContent()
				.MaxDesiredWidth(250)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CHOPAtRunTime", "CHOP data will be filled at runtime"))
				]

			;
			break;
		}
		case EVarType::Texture:
		{
			TSharedPtr<IPropertyHandle> textureHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, TextureProperty));
			textureHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextureChanged, dynVar->VarIdentifier));
			TSharedRef<SWidget> textureWidget = textureHandle->CreatePropertyValueWidget();
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
			break;
		}
		// string data
		case EVarType::String:
		{
			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
				]
			.ValueContent()
				.MaxDesiredWidth(250)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DATAtRunTime", "DAT data will be filled at runtime"))
				]
			;
			break;
		}
		default:
		{
			// VARTYPE_NOT_SET or VARTYPE_MAX

			break;
		}
		}
	}
}


TSharedRef<IPropertyTypeCustomization> FTouchEngineDynamicVariableStructDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FTouchEngineDynamicVariableStructDetailsCustomization());
}

void FTouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded()
{
	RerenderPanel();
}

void FTouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad(FString Error)
{
	ErrorMessage = Error;
	DynVars->parent->ErrorMessage = Error;

	RerenderPanel();
}

void FTouchEngineDynamicVariableStructDetailsCustomization::RerenderPanel()
{
	if (PropUtils.IsValid() && !PendingRedraw)
	{
		if (!DynVars || !DynVars->parent || DynVars->parent->IsPendingKill() || DynVars->parent->EngineInfo)
		{
			return;
		}

		PropUtils->ForceRefresh();

		PendingRedraw = true;

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


FSlateColor FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	return FEditorStyle::GetSlateColor(InvertedForegroundName);
}

void FTouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild(TSharedRef<IPropertyHandle> ElementHandle, int32 ChildIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	PropertyHandle->NotifyPreChange();

	FDetailWidgetRow& ElementRow = ChildrenBuilder.AddCustomRow(FText::Format(LOCTEXT("RowNum", "Row{i}"), ChildIndex));

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

TSharedRef<SWidget> FTouchEngineDynamicVariableStructDetailsCustomization::CreateNameWidget(FString Name, FString Tooltip, TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	// Simple function, but makes it easier to mass modify / standardize widget names and tooltips
	TSharedRef<SWidget> nameContent = StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(Name), FText::FromString(Tooltip), false);

	return nameContent;
}


FReply FTouchEngineDynamicVariableStructDetailsCustomization::OnReloadClicked()
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


void FTouchEngineDynamicVariableStructDetailsCustomization::HandleChecked(ECheckBoxState InState, FString Identifier, TSharedRef<IPropertyHandle> dynVarHandle)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		dynVarHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);

		dynVar->HandleChecked(InState);

		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		dynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextChanged(const FText& NewText, FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleTextBoxTextChanged(NewText);
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo, FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleTextBoxTextCommited(NewText);
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextureChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleTextureChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleColorChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleColorChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector2Changed(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleVector2Changed();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleVectorChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleVectorChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector4Changed(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("%f, %f, %f, %f"), dynVar->Vector4Property.X, dynVar->Vector4Property.Y, dynVar->Vector4Property.Z, dynVar->Vector4Property.W));

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleVector4Changed();
		//UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector4ChildChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleVector4Changed();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVector2Changed(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleIntVector2Changed();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVectorChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleIntVectorChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVector4Changed(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleIntVector4Changed();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleFloatBufferChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChildChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleFloatBufferChildChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleStringArrayChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChildChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleStringArrayChildChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleDropDownBoxValueChanged(TSharedPtr<FString> Arg, ESelectInfo::Type SelectType, FString Identifier)
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
		dynVar->HandleDropDownBoxValueChanged(Arg);
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->parent, oldValue, *dynVar);

		PropertyHandle->NotifyPostChange();
	}
}


ECheckBoxState FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->Value && dynVar->VarType == EVarType::Bool)
	{
		return dynVar->GetValueAsBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

TOptional<int> FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalInt(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->Value && dynVar->VarType == EVarType::Int)
	{
		return TOptional<int>(dynVar->GetValueAsInt());
	}
	else
	{
		return TOptional<int>(0);
	}
}

TOptional<int> FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalInt(int Index, FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->Value && dynVar->VarType == EVarType::Int)
	{
		return TOptional<int>(dynVar->GetValueAsIntIndexed(Index));
	}
	else
	{
		return TOptional<int>(0);
	}
}

TOptional<double> FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalDouble(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->Value && dynVar->VarType == EVarType::Double)
	{
		return TOptional<double>(dynVar->GetValueAsDouble());
	}
	else
	{
		return TOptional<double>(0);
	}
}

TOptional<double> FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalDouble(int Index, FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->Value && dynVar->VarType == EVarType::Double)
	{
		return TOptional<double>(dynVar->GetValueAsDoubleIndexed(Index));
	}
	else
	{
		return TOptional<double>(0);
	}
}

TOptional<float> FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalFloat(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->Value && dynVar->VarType == EVarType::Float)
	{
		return TOptional<float>(dynVar->GetValueAsFloat());
	}
	else
	{
		return  TOptional<float>(0);
	}
}

FText FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxText(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (dynVar && dynVar->Value && dynVar->VarType == EVarType::String)
	{
		return FText::FromString(dynVar->GetValueAsString());
	}
	else
	{
		return FText();
	}
}


void FTouchEngineDynamicVariableStructDetailsCustomization::UpdateDynVarInstances(UObject* BlueprintOwner, UTouchEngineComponentBase* ParentComponent, FTouchEngineDynamicVariableStruct OldVar, FTouchEngineDynamicVariableStruct NewVar)
{
	TArray<UObject*> ArchetypeInstances;
	TArray<UTouchEngineComponentBase*> UpdatedInstances;

	if (ParentComponent->HasAnyFlags(RF_ArchetypeObject))
	{
		ParentComponent->GetArchetypeInstances(ArchetypeInstances);

		for (int32 InstanceIndex = 0; InstanceIndex < ArchetypeInstances.Num(); ++InstanceIndex)
		{
			UTouchEngineComponentBase* InstancedTEComponent = Cast<UTouchEngineComponentBase>(ArchetypeInstances[InstanceIndex]);
			if (InstancedTEComponent != nullptr && !UpdatedInstances.Contains(InstancedTEComponent))
			{
				if (InstancedTEComponent->ToxFilePath == ParentComponent->ToxFilePath)
				{
					// find this variable inside the component
					FTouchEngineDynamicVariableStruct* dynVar = InstancedTEComponent->DynamicVariables.GetDynamicVariableByIdentifier(NewVar.VarIdentifier);

					// didn't find the variable, or had a variable type mismatch.
					// This is an odd case, should probably reload the tox file and try again
					if (!dynVar || dynVar->VarType != NewVar.VarType)
					{
						continue;
					}
					// check if the value is the default value
					if (OldVar.Identical(dynVar, 0))
					{
						// if it is, replace it
						dynVar->SetValue(&NewVar);
					}
				}
			}
		}
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::OnDynVarsDestroyed()
{
	DynVarsDestroyed = true;
}


#undef LOCTEXT_NAMESPACE