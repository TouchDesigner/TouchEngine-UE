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
#include "IDetailGroup.h"

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

	//DynVars->parent->CreateEngineInfo();
	if (ToxLoaded_DelegateHandle.IsValid())
	{
		DynVars->Unbind_OnToxLoaded(ToxLoaded_DelegateHandle);
		ToxLoaded_DelegateHandle.Reset();
	}
	if (ToxFailedLoad_DelegateHandle.IsValid())
	{
		DynVars->Unbind_OnToxFailedLoad(ToxFailedLoad_DelegateHandle);
		ToxFailedLoad_DelegateHandle.Reset();
	}

	ToxLoaded_DelegateHandle = DynVars->CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded));
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
	PropUtils = StructCustomizationUtils.GetPropertyUtilities();

	//StructBuilder.AddChildProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, test)).ToSharedRef());
	IDetailGroup* InputGroup = &StructBuilder.AddGroup(FName("Inputs"), FText::FromString("Inputs"));
	IDetailGroup* OutputGroup = &StructBuilder.AddGroup(FName("Outputs"), FText::FromString("Outputs"));

	//InputGroup->AddPropertyRow(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, test)).ToSharedRef());

	// handle input variables 
	TSharedPtr<IPropertyHandleArray> inputsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Input))->AsArray();
	uint32 numInputs = 0u;
	auto result = inputsHandle->GetNumElements(numInputs);

	for (uint32 i = 0; i < numInputs; i++)
	{
		auto dynVarHandle = inputsHandle->GetElement(i);
		FTouchEngineDynamicVariable* dynVar;

		{
			TArray<void*> RawData;
			PropertyHandle->AccessRawData(RawData);
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
						CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
					]
				.ValueContent()
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(dynVar, &FTouchEngineDynamicVariable::HandleChecked)
					.IsChecked_Raw(dynVar, &FTouchEngineDynamicVariable::GetValueAsCheckState)
					];
			}
			break;
			case EVarIntent::VARINTENT_MOMENTARY:
			{
				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
					]
				.ValueContent()
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(dynVar, &FTouchEngineDynamicVariable::HandleChecked)
					.IsChecked_Raw(dynVar, &FTouchEngineDynamicVariable::GetValueAsCheckState)
					];
			}
			break;
			case EVarIntent::VARINTENT_PULSE:
			{
				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
					]
				.ValueContent()
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(dynVar, &FTouchEngineDynamicVariable::HandleChecked)
					.IsChecked_Raw(dynVar, &FTouchEngineDynamicVariable::GetValueAsCheckState)
					];
			}
			break;
			}
		}
		break;
		case EVarType::VARTYPE_INT:
		{
			FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

			newRow.NameContent()
				[
					CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
				]
			.ValueContent()
				[
					SNew(SNumericEntryBox<int>)
					.OnValueCommitted_Raw(dynVar, &FTouchEngineDynamicVariable::HandleValueChanged<int>)
				.AllowSpin(false)
				.Value_Raw(dynVar, &FTouchEngineDynamicVariable::GetValueAsOptionalInt)
				];
		}
		break;
		case EVarType::VARTYPE_DOUBLE:
		{
			if (dynVar->count == 1)
			{
				FDetailWidgetRow& newRow = InputGroup->AddWidgetRow();

				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
					]
				.ValueContent()
					[
						SNew(SNumericEntryBox<double>)
						.OnValueCommitted_Raw(dynVar, &FTouchEngineDynamicVariable::HandleValueChanged<double>)
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
								CreateNameWidget((FString(dynVar->VarName).Append(FString::FromInt(j + 1))), dynVar->VarIdentifier, StructPropertyHandle)
							]
						.ValueContent()
							[
								SNew(SNumericEntryBox<double>)
								.OnValueCommitted(SNumericEntryBox<double>::FOnValueCommitted::CreateRaw(dynVar, &FTouchEngineDynamicVariable::HandleValueChangedWithIndex, j))
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
					colorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTouchEngineDynamicVariable::HandleColorChanged));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(colorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarIdentifier));
					property->DisplayName(FText::FromString(dynVar->VarName));
				}
				break;
				case EVarIntent::VARINTENT_POSITION:
				{
					TSharedPtr<IPropertyHandle> vectorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariable, vector4Property));
					vectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTouchEngineDynamicVariable::HandleVector4Changed));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(vectorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarIdentifier));
					property->DisplayName(FText::FromString(dynVar->VarName));
				}
				break;
				case EVarIntent::VARINTENT_UVW:
				{
					TSharedPtr<IPropertyHandle> vectorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariable, vectorProperty));
					vectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTouchEngineDynamicVariable::HandleVectorChanged));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(vectorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarIdentifier));
					property->DisplayName(FText::FromString(dynVar->VarName));
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
					CreateNameWidget(dynVar->VarName, dynVar->VarIdentifier, StructPropertyHandle)
				]
			.ValueContent()
				[
					SNew(SNumericEntryBox<float>)
					.OnValueCommitted_Raw(dynVar, &FTouchEngineDynamicVariable::HandleValueChanged<float>)
				.AllowSpin(false)
				.Value_Raw(dynVar, &FTouchEngineDynamicVariable::GetValueAsOptionalFloat)
				];
		}
		break;
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			TSharedPtr<IPropertyHandle> floatsHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariable, floatBufferProperty));

			floatsHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTouchEngineDynamicVariable::HandleFloatBufferChanged));
			floatsHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTouchEngineDynamicVariable::HandleFloatBufferChildChanged));
			floatsHandle->SetToolTipText(FText::FromString(dynVar->VarIdentifier));

			TSharedPtr<IPropertyHandleArray> floatsArrayHandle = floatsHandle->AsArray();

			TSharedRef<FDetailArrayBuilder> arrayBuilder = MakeShareable(new FDetailArrayBuilder(floatsHandle.ToSharedRef()));
			arrayBuilder->SetDisplayName(FText::FromString(dynVar->VarName));
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
					.OnTextChanged_Raw(dynVar, &FTouchEngineDynamicVariable::HandleTextBoxTextChanged)
					.OnTextCommitted_Raw(dynVar, &FTouchEngineDynamicVariable::HandleTextBoxTextCommited)
					.SelectAllTextOnCommit(true)
					.Text_Raw(dynVar, &FTouchEngineDynamicVariable::HandleTextBoxText)
					];
			}
			else
			{
				auto stringHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariable, stringArrayProperty));

				stringHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTouchEngineDynamicVariable::HandleStringArrayChanged));
				stringHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTouchEngineDynamicVariable::HandleStringArrayChildChanged));
				stringHandle->SetToolTipText(FText::FromString(dynVar->VarIdentifier));

				auto floatsArrayHandle = stringHandle->AsArray();

				TSharedRef<FDetailArrayBuilder> arrayBuilder = MakeShareable(new FDetailArrayBuilder(stringHandle.ToSharedRef()));
				arrayBuilder->SetDisplayName(FText::FromString(dynVar->VarName));
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
			textureHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTouchEngineDynamicVariable::HandleTextureChanged));

			// check for strange state world property details panel can be in 
			if (dynVar->textureProperty == nullptr && dynVar->value)
			{
				// value is set but texture property is empty, set texture property from value
				dynVar->textureProperty = dynVar->GetValueAsTextureRenderTarget();
			}

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
		case EVarType::VARTYPE_TEXTURE:
		{
			TSharedPtr<IPropertyHandle> textureHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariable, textureProperty));
			textureHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(dynVar, &FTouchEngineDynamicVariable::HandleTextureChanged));
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
	static const FName InvertedForegroundName("InvertedForeground");
	return FEditorStyle::GetSlateColor(InvertedForegroundName);
}

void TouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild(TSharedRef<IPropertyHandle> ElementHandle, int32 ChildIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
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
}

TSharedRef<SWidget> TouchEngineDynamicVariableStructDetailsCustomization::CreateNameWidget(FString name, FString tooltip, TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	// Simple function, but makes it easier to mass modify / standardize widget names and tooltips
	auto nameContent = StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(name), FText::FromString(tooltip), false);

	return nameContent;
}
