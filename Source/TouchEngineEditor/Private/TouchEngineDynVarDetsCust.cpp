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

	if (DynVars && DynVars->Parent && !DynVars->Parent->IsBeingDestroyed())
	{
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

	TArray<UObject*> Objs;
	StructPropertyHandle->GetOuterObjects(Objs);
	for (UObject* Obj : Objs)
	{
		BlueprintObject = Obj->GetOuter();
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

	if (!DynVars->Parent)
	{
		TArray<UObject*> Outers;
		PropertyHandle->GetOuterObjects(Outers);

		for (int i = 0; i < Outers.Num(); i++)
		{
			UTouchEngineComponentBase* Parent = static_cast<UTouchEngineComponentBase*>(Outers[i]);
			if (Parent)
			{
				DynVars->Parent = Parent;

				break;
			}
		}
	}

	DynVars->Parent->ValidateParameters();

	if (ToxFailedLoad_DelegateHandle.IsValid())
	{
		DynVars->Unbind_OnToxFailedLoad(ToxFailedLoad_DelegateHandle);
		ToxFailedLoad_DelegateHandle.Reset();
	}

	ToxFailedLoad_DelegateHandle = DynVars->OnToxFailedLoad.AddRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad);

	if (ToxLoaded_DelegateHandle.IsValid())
	{
		DynVars->Unbind_OnToxLoaded(ToxLoaded_DelegateHandle);
		ToxLoaded_DelegateHandle.Reset();
	}

	ToxLoaded_DelegateHandle = DynVars->OnToxLoaded.Add(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded));

	DynVars->OnDestruction.BindRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnDynVarsDestroyed);

	// check tox file load state
	if (!DynVars->Parent->IsLoaded())
	{
		// tox file is not loaded yet
		if (!DynVars->Parent->HasFailedLoad())
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
			if (ErrorMessage.IsEmpty() && !DynVars->Parent->ErrorMessage.IsEmpty())
			{
				ErrorMessage = DynVars->Parent->ErrorMessage;
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
					.Text(FText::Format(LOCTEXT("ToxLoadFailed", "Failed to load TOX file: {0}"), FText::FromString(ErrorMessage)))
				]
			;
		}
	}
	else
	{
		// tox file has finished loading
		if (!DynVars->Parent->HasFailedLoad())
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

	TArray<UObject*> Objs;
	StructPropertyHandle->GetOuterObjects(Objs);
	for (UObject* Obj : Objs)
	{
		BlueprintObject = Obj->GetOuter();
	}

	StructPropertyHandle->GetProperty()->GetOwnerClass();
	PropUtils = StructCustomizationUtils.GetPropertyUtilities();

	FDetailWidgetRow& ButtonRow = StructBuilder.AddCustomRow(LOCTEXT("ReloadTox", "ReloadTox"));


	// Add groups to hold input and output variables
	IDetailGroup* InputGroup = &StructBuilder.AddGroup(FName("Inputs"), LOCTEXT("Inputs", "Inputs"));
	IDetailGroup* OutputGroup = &StructBuilder.AddGroup(FName("Outputs"), LOCTEXT("Outputs", "Outputs"));

	// Add "Reload Tox" button to the details panel 
	ButtonRow.ValueContent()
		[
			SNew(SButton)
			.Text(TAttribute<FText>(LOCTEXT("ReloadTox", "Reload Tox")))
		.OnClicked(FOnClicked::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnReloadClicked))
		]
	;

	// handle input variables 
	TSharedPtr<IPropertyHandleArray> InputsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Input))->AsArray();
	uint32 NumInputs = 0u;
	FPropertyAccess::Result result = InputsHandle->GetNumElements(NumInputs);

	// make sure data is valid
	for (uint32 i = 0; i < NumInputs; i++)
	{
		TSharedRef<IPropertyHandle> DynVarHandle = InputsHandle->GetElement(i);
		FTouchEngineDynamicVariableStruct* DynVar;

		TArray<void*> RawData;
		DynVarHandle->AccessRawData(RawData);

		if (RawData.Num() == 0 || !RawData[0])
		{
			return;
		}
		DynVar = static_cast<FTouchEngineDynamicVariableStruct*>(RawData[0]);

		if (DynVar->VarName == TEXT("ERROR_NAME"))
		{
			return;
		}
	}

	for (uint32 i = 0; i < NumInputs; i++)
	{
		TSharedRef<IPropertyHandle> DynVarHandle = InputsHandle->GetElement(i);
		FTouchEngineDynamicVariableStruct* DynVar;

		TArray<void*> RawData;
		DynVarHandle->AccessRawData(RawData);
		DynVar = static_cast<FTouchEngineDynamicVariableStruct*>(RawData[0]);

		switch (DynVar->VarType)
		{
		case EVarType::Bool:
		{
			FDetailWidgetRow& NewRow = InputGroup->AddWidgetRow();

			switch (DynVar->VarIntent)
			{
			case EVarIntent::NotSet:
			{
				NewRow.NameContent()
					[
						CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					.MaxDesiredWidth(250)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, DynVar->VarIdentifier, DynVarHandle)
					.IsChecked_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, DynVar->VarIdentifier)
					];
				break;
			}
			case EVarIntent::Momentary:
			{
				NewRow.NameContent()
					[
						CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					.MaxDesiredWidth(250)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, DynVar->VarIdentifier, DynVarHandle)
					.IsChecked_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, DynVar->VarIdentifier)
					];
				break;
			}
			case EVarIntent::Pulse:
			{
				NewRow.NameContent()
					[
						CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					.MaxDesiredWidth(250)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, DynVar->VarIdentifier, DynVarHandle)
					.IsChecked_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, DynVar->VarIdentifier)
					];
				break;
			}
			}
			break;
		}
		case EVarType::Int:
		{
			if (DynVar->Count == 1)
			{
				FDetailWidgetRow& NewRow = InputGroup->AddWidgetRow();

				if (DynVar->VarIntent != EVarIntent::DropDown)
				{
					NewRow.NameContent()
						[
							CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
						]
					.ValueContent()
						.MaxDesiredWidth(250)
						[
							SNew(SNumericEntryBox<int>)
							.OnValueCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<int>, DynVar->VarIdentifier)
						.AllowSpin(false)
						.Value_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalInt, DynVar->VarIdentifier)
						];
				}
				else
				{
					TArray<TSharedPtr<FString>>* DropDownStrings = new TArray<TSharedPtr<FString>>();

					TArray<FString> Keys; DynVar->DropDownData.GetKeys(Keys);
					for (int j = 0; j < Keys.Num(); j++)
					{
						DropDownStrings->Add(TSharedPtr<FString>(new FString(Keys[j])));
					}

					NewRow.NameContent()
						[
							CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
						]
					.ValueContent()
						.MaxDesiredWidth(250)
						[
							SNew(STextComboBox)
							.OptionsSource(DropDownStrings)
						.InitiallySelectedItem((*DropDownStrings)[0])
						.OnSelectionChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleDropDownBoxValueChanged, DynVar->VarIdentifier)
						]
					;
				}
			}
			else
			{
				switch (DynVar->Count)
				{
				case 2:
				{
					TSharedPtr<IPropertyHandle> IntVector2DHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, IntPointProperty));
					IntVector2DHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVector2Changed, DynVar->VarIdentifier));
					IntVector2DHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVector2Changed, DynVar->VarIdentifier));
					IDetailPropertyRow* Property = &InputGroup->AddPropertyRow(IntVector2DHandle.ToSharedRef());
					Property->ToolTip(FText::FromString(DynVar->VarName));
					Property->DisplayName(FText::FromString(DynVar->VarLabel));

					break;
				}
				case 3:
				{
					TSharedPtr<IPropertyHandle> IntVectorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, IntVectorProperty));
					IntVectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVectorChanged, DynVar->VarIdentifier));
					IDetailPropertyRow* Property = &InputGroup->AddPropertyRow(IntVectorHandle.ToSharedRef());
					Property->ToolTip(FText::FromString(DynVar->VarName));
					Property->DisplayName(FText::FromString(DynVar->VarLabel));

					break;
				}
				case 4:
				{
					TSharedPtr<IPropertyHandle> IntVector4Handle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, IntVector4Property));
					IntVector4Handle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVector4Changed, DynVar->VarIdentifier));
					IDetailPropertyRow* Property = &InputGroup->AddPropertyRow(IntVector4Handle.ToSharedRef());
					Property->ToolTip(FText::FromString(DynVar->VarName));
					Property->DisplayName(FText::FromString(DynVar->VarLabel));

					break;
				}
				default:
				{
					for (int j = 0; j < DynVar->Count; j++)
					{
						FDetailWidgetRow& NewRow = InputGroup->AddWidgetRow();

						NewRow.NameContent()
							[
								CreateNameWidget((FString(DynVar->VarLabel).Append(FString::FromInt(j + 1))), DynVar->VarName, StructPropertyHandle)
							]
						.ValueContent()
							.MaxDesiredWidth(250)
							[
								SNew(SNumericEntryBox<int>)
								.OnValueCommitted(SNumericEntryBox<int>::FOnValueCommitted::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex<int>, j, DynVar->VarIdentifier))
							.AllowSpin(false)
							.Value(TAttribute<TOptional<int>>::Create(TAttribute<TOptional<int>>::FGetter::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalInt, j, DynVar->VarIdentifier)))
							];
					}
					break;
				}
				}
			}
			break;
		}
		case EVarType::Double:
		{
			if (DynVar->Count == 1)
			{
				FDetailWidgetRow& NewRow = InputGroup->AddWidgetRow();

				NewRow.NameContent()
					[
						CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					.MaxDesiredWidth(250)
					[
						SNew(SNumericEntryBox<double>)
						.OnValueCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<double>, DynVar->VarIdentifier)
					.AllowSpin(false)
					.Value_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalDouble, DynVar->VarIdentifier)
					]
				;
			}
			else
			{
				switch (DynVar->VarIntent)
				{
				case EVarIntent::NotSet:
				{

					switch (DynVar->Count)
					{
					case 2:
					{
						TSharedPtr<IPropertyHandle> Vector2DHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, Vector2DProperty));
						Vector2DHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector2Changed, DynVar->VarIdentifier));
						IDetailPropertyRow* Property = &InputGroup->AddPropertyRow(Vector2DHandle.ToSharedRef());
						Property->ToolTip(FText::FromString(DynVar->VarName));
						Property->DisplayName(FText::FromString(DynVar->VarLabel));

						break;
					}
					case 3:
					{
						TSharedPtr<IPropertyHandle> VectorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, VectorProperty));
						VectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleVectorChanged, DynVar->VarIdentifier));
						IDetailPropertyRow* Property = &InputGroup->AddPropertyRow(VectorHandle.ToSharedRef());
						Property->ToolTip(FText::FromString(DynVar->VarName));
						Property->DisplayName(FText::FromString(DynVar->VarLabel));

						break;
					}
					case 4:
					{
						TSharedPtr<IPropertyHandle> Vector4Handle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, Vector4Property));
						Vector4Handle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector4Changed, DynVar->VarIdentifier));
						Vector4Handle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector4ChildChanged, DynVar->VarIdentifier));
						IDetailPropertyRow* Property = &InputGroup->AddPropertyRow(Vector4Handle.ToSharedRef());
						Property->ToolTip(FText::FromString(DynVar->VarName));
						Property->DisplayName(FText::FromString(DynVar->VarLabel));

						break;
					}

					default:
					{
						for (int j = 0; j < DynVar->Count; j++)
						{
							FDetailWidgetRow& NewRow = InputGroup->AddWidgetRow();

							NewRow.NameContent()
								[
									CreateNameWidget((FString(DynVar->VarLabel).Append(FString::FromInt(j + 1))), DynVar->VarName, StructPropertyHandle)
								]
							.ValueContent()
								.MaxDesiredWidth(250)
								[
									SNew(SNumericEntryBox<double>)
									.OnValueCommitted(SNumericEntryBox<double>::FOnValueCommitted::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex, j, DynVar->VarIdentifier))
								.AllowSpin(false)
								.Value(TAttribute<TOptional<double>>::Create(TAttribute<TOptional<double>>::FGetter::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalDouble, j, DynVar->VarIdentifier)))
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
					TSharedPtr<IPropertyHandle> ColorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, ColorProperty));
					ColorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleColorChanged, DynVar->VarIdentifier));
					ColorHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleColorChanged, DynVar->VarIdentifier));
					IDetailPropertyRow* Property = &InputGroup->AddPropertyRow(ColorHandle.ToSharedRef());
					Property->ToolTip(FText::FromString(DynVar->VarName));
					Property->DisplayName(FText::FromString(DynVar->VarLabel));
					break;
				}
				case EVarIntent::Position:
				{
					TSharedPtr<IPropertyHandle> VectorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, Vector4Property));
					VectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector4Changed, DynVar->VarIdentifier));
					IDetailPropertyRow* Property = &InputGroup->AddPropertyRow(VectorHandle.ToSharedRef());
					Property->ToolTip(FText::FromString(DynVar->VarName));
					Property->DisplayName(FText::FromString(DynVar->VarLabel));
					break;
				}
				case EVarIntent::UVW:
				{
					TSharedPtr<IPropertyHandle> VectorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, VectorProperty));
					VectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleVectorChanged, DynVar->VarIdentifier));
					IDetailPropertyRow* Property = &InputGroup->AddPropertyRow(VectorHandle.ToSharedRef());
					Property->ToolTip(FText::FromString(DynVar->VarName));
					Property->DisplayName(FText::FromString(DynVar->VarLabel));
					// values will be named xyz for now
					break;
				}
				}
			}
			break;
		}
		case EVarType::Float:
		{
			FDetailWidgetRow& NewRow = InputGroup->AddWidgetRow();

			NewRow.NameContent()
				[
					CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
				]
			.ValueContent()
				[
					SNew(SNumericEntryBox<float>)
					.OnValueCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<float>, DynVar->VarIdentifier)
				.AllowSpin(false)
				.Value_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalFloat, DynVar->VarIdentifier)
				];
			break;
		}
		case EVarType::CHOP:
		{
			TSharedPtr<IPropertyHandle> FloatsHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, FloatBufferProperty));

			FloatsHandle->SetPropertyDisplayName(FText::FromString(DynVar->VarLabel));
			FloatsHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChanged, DynVar->VarIdentifier));
			FloatsHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChildChanged, DynVar->VarIdentifier));
			FloatsHandle->SetToolTipText(FText::FromString(DynVar->VarName));

			TSharedPtr<IPropertyHandleArray> FloatsArrayHandle = FloatsHandle->AsArray();

			TSharedRef<FDetailArrayBuilder> ArrayBuilder = MakeShareable(new FDetailArrayBuilder(FloatsHandle.ToSharedRef()));
			ArrayBuilder->SetDisplayName(FText::FromString(DynVar->VarLabel));
			ArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild));

			InputGroup->AddPropertyRow(FloatsHandle.ToSharedRef());
			break;
		}
		case EVarType::String:
		{
			if (!DynVar->IsArray)
			{
				FDetailWidgetRow& NewRow = InputGroup->AddWidgetRow();

				if (DynVar->VarIntent != EVarIntent::DropDown)
				{

					NewRow.NameContent()
						[
							CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
						]
					.ValueContent()
						.MaxDesiredWidth(0.0f)
						.MinDesiredWidth(125.0f)
						[
							SNew(SEditableTextBox)
							.ClearKeyboardFocusOnCommit(false)
						.IsEnabled(true)
						.ForegroundColor(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor)
						.OnTextChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextChanged, DynVar->VarIdentifier)
						.OnTextCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommited, DynVar->VarIdentifier)
						.SelectAllTextOnCommit(true)
						.Text_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxText, DynVar->VarIdentifier)
						];
				}
				else
				{
					TArray<TSharedPtr<FString>>* DropDownStrings = new TArray<TSharedPtr<FString>>();

					TArray<FString> Keys; DynVar->DropDownData.GetKeys(Keys);
					for (int j = 0; j < Keys.Num(); j++)
					{
						DropDownStrings->Add(TSharedPtr<FString>(new FString(Keys[j])));
					}

					NewRow.NameContent()
						[
							CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
						]
					.ValueContent()
						[
							SNew(STextComboBox)
							.OptionsSource(DropDownStrings)
						.InitiallySelectedItem((*DropDownStrings)[0])
						.OnSelectionChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleDropDownBoxValueChanged, DynVar->VarIdentifier)
						]
					;
				}
			}
			else
			{
				TSharedPtr<IPropertyHandle> stringHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, StringArrayProperty));

				stringHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChanged, DynVar->VarIdentifier));
				stringHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChildChanged, DynVar->VarIdentifier));
				stringHandle->SetToolTipText(FText::FromString(DynVar->VarName));
				stringHandle->SetPropertyDisplayName(FText::FromString(DynVar->VarLabel));

				TSharedPtr<IPropertyHandleArray> FloatsArrayHandle = stringHandle->AsArray();

				TSharedRef<FDetailArrayBuilder> ArrayBuilder = MakeShareable(new FDetailArrayBuilder(stringHandle.ToSharedRef()));
				ArrayBuilder->SetDisplayName(FText::FromString(DynVar->VarLabel));
				ArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild));

				//StructBuilder.AddCustomBuilder(ArrayBuilder);
				InputGroup->AddPropertyRow(stringHandle.ToSharedRef());
			}
		}
		break;
		case EVarType::Texture:
		{
			FDetailWidgetRow& NewRow = InputGroup->AddWidgetRow();
			TSharedPtr<IPropertyHandle> TextureHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, TextureProperty));
			TextureHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextureChanged, DynVar->VarIdentifier));

			// check for strange state world Property details panel can be in 
			if (DynVar->TextureProperty == nullptr && DynVar->Value)
			{
				// value is set but texture Property is empty, set texture Property from value
				DynVar->TextureProperty = DynVar->GetValueAsTexture();
			}

			NewRow.NameContent()
				[
					CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
				]
			.ValueContent()
				[
					TextureHandle->CreatePropertyValueWidget()
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
	TSharedPtr<IPropertyHandleArray> OutputsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Output))->AsArray();
	uint32 NumOutputs = 0u;
	OutputsHandle->GetNumElements(NumOutputs);

	for (uint32 i = 0; i < NumOutputs; i++)
	{
		TSharedRef<IPropertyHandle> DynVarHandle = OutputsHandle->GetElement(i);
		FTouchEngineDynamicVariableStruct* DynVar;

		{
			TArray<void*> RawData;
			PropertyHandle->AccessRawData(RawData);
			DynVarHandle->AccessRawData(RawData);
			DynVar = static_cast<FTouchEngineDynamicVariableStruct*>(RawData[0]);
		}

		FDetailWidgetRow& NewRow = OutputGroup->AddWidgetRow();

		switch (DynVar->VarType)
		{
		case EVarType::CHOP:
		{
			NewRow.NameContent()
				[
					CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
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
			TSharedPtr<IPropertyHandle> TextureHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, TextureProperty));
			TextureHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextureChanged, DynVar->VarIdentifier));
			TSharedRef<SWidget> TextureWidget = TextureHandle->CreatePropertyValueWidget();
			TextureWidget->SetEnabled(false);

			NewRow.NameContent()
				[
					CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
				]
			.ValueContent()
				.MaxDesiredWidth(250)
				[
					TextureWidget
				]
			;
			break;
		}
		// string data
		case EVarType::String:
		{
			NewRow.NameContent()
				[
					CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
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

	if (DynVars && IsValid(DynVars->Parent))
	{
		DynVars->Parent->ErrorMessage = Error;
	}

	RerenderPanel();
}

void FTouchEngineDynamicVariableStructDetailsCustomization::RerenderPanel()
{
	if (PropUtils.IsValid() && !PendingRedraw)
	{
		if (!DynVars || !DynVars->Parent || DynVars->Parent->IsPendingKill() || DynVars->Parent->EngineInfo)
		{
			return;
		}

		PropUtils->ForceRefresh();

		PendingRedraw = true;

		if (ToxLoaded_DelegateHandle.IsValid())
		{
			if (DynVars && DynVars->Parent)
				DynVars->Unbind_OnToxLoaded(ToxLoaded_DelegateHandle);
		}
		if (ToxFailedLoad_DelegateHandle.IsValid())
		{
			if (DynVars && DynVars->Parent)
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
	TSharedRef<SWidget> NameContent = StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(Name), FText::FromString(Tooltip), false);

	return NameContent;
}


FReply FTouchEngineDynamicVariableStructDetailsCustomization::OnReloadClicked()
{
	if (DynVars->Parent->EngineInfo)
	{
		// this will hit if we try to reload a tox file while during begin play, which can cause many errors
		return FReply::Handled();;
	}

	PropertyHandle->NotifyPreChange();

	if (DynVars && DynVars->Parent)
		DynVars->Parent->ReloadTox();

	PropertyHandle->NotifyPostChange();

	RerenderPanel();

	return FReply::Handled();
}


void FTouchEngineDynamicVariableStructDetailsCustomization::HandleChecked(ECheckBoxState InState, FString Identifier, TSharedRef<IPropertyHandle> DynVarHandle)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		DynVarHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);

		DynVar->HandleChecked(InState);

		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		DynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextChanged(const FText& NewText, FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleTextBoxTextChanged(NewText);
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();

		OldValue.Clear();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo, FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleTextBoxTextCommited(NewText);
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextureChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleTextureChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		PropertyHandle->NotifyPostChange();

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleColorChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleColorChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector2Changed(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleVector2Changed();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleVectorChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleVectorChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector4Changed(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("%f, %f, %f, %f"), DynVar->Vector4Property.X, DynVar->Vector4Property.Y, DynVar->Vector4Property.Z, DynVar->Vector4Property.W));

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleVector4Changed();

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleVector4ChildChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleVector4Changed();

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVector2Changed(FString Identifier)
{
	GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, "Modifying Int Point Value");


	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleIntVector2Changed();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVectorChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleIntVectorChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleIntVector4Changed(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleIntVector4Changed();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleFloatBufferChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChildChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleFloatBufferChildChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleStringArrayChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChildChanged(FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleStringArrayChildChanged();
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleDropDownBoxValueChanged(TSharedPtr<FString> Arg, ESelectInfo::Type SelectType, FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleDropDownBoxValueChanged(Arg);
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange();
	}
}


ECheckBoxState FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::Bool)
	{
		return DynVar->GetValueAsBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

TOptional<int> FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalInt(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::Int)
	{
		return TOptional<int>(DynVar->GetValueAsInt());
	}
	else
	{
		return TOptional<int>(0);
	}
}

TOptional<int> FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalInt(int Index, FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::Int)
	{
		return TOptional<int>(DynVar->GetValueAsIntIndexed(Index));
	}
	else
	{
		return TOptional<int>(0);
	}
}

TOptional<double> FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalDouble(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::Double)
	{
		return TOptional<double>(DynVar->GetValueAsDouble());
	}
	else
	{
		return TOptional<double>(0);
	}
}

TOptional<double> FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalDouble(int Index, FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::Double)
	{
		return TOptional<double>(DynVar->GetValueAsDoubleIndexed(Index));
	}
	else
	{
		return TOptional<double>(0);
	}
}

TOptional<float> FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalFloat(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::Float)
	{
		return TOptional<float>(DynVar->GetValueAsFloat());
	}
	else
	{
		return  TOptional<float>(0);
	}
}

FText FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxText(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::String)
	{
		return FText::FromString(DynVar->GetValueAsString());
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
					FTouchEngineDynamicVariableStruct* DynVar = InstancedTEComponent->DynamicVariables.GetDynamicVariableByIdentifier(NewVar.VarIdentifier);

					// didn't find the variable, or had a variable type mismatch.
					// This is an odd case, should probably reload the tox file and try again
					if (!DynVar || DynVar->VarType != NewVar.VarType)
					{
						continue;
					}
					// check if the value is the default value
					if (OldVar.Identical(DynVar, 0))
					{
						// if it is, replace it
						DynVar->SetValue(&NewVar);
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