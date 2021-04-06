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

	for (uint32 i = 0; i < numInputs; i++)
	{
		auto dynVarHandle = inputsHandle->GetElement(i);
		FTouchDynamicVar* dynVar;

		{
			TArray<void*> RawData;
			dynVarHandle->AccessRawData(RawData);
			dynVar = static_cast<FTouchDynamicVar*>(RawData[0]);
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
						.OnCheckStateChanged_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, dynVar)
						.IsChecked_Raw(dynVar, &FTouchDynamicVar::GetValueAsCheckState)
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
						.OnCheckStateChanged_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, dynVar)
						.IsChecked_Raw(dynVar, &FTouchDynamicVar::GetValueAsCheckState)
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
						.OnCheckStateChanged_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, dynVar)
						.IsChecked_Raw(dynVar, &FTouchDynamicVar::GetValueAsCheckState)
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

				newRow.NameContent()
					[
						CreateNameWidget(dynVar->VarLabel, dynVar->VarName, StructPropertyHandle)
					]
				.ValueContent()
					[
						SNew(SNumericEntryBox<int>)
						.OnValueCommitted_Raw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<int>, dynVar)
						.AllowSpin(false)
						.Value_Raw(dynVar, &FTouchDynamicVar::GetValueAsOptionalInt)
					];
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
							.Value(TAttribute<TOptional<int>>::Create(TAttribute<TOptional<int>>::FGetter::CreateRaw(dynVar, &FTouchDynamicVar::GetIndexedValueAsOptionalInt, j)))
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
						.Value_Raw(dynVar, &FTouchDynamicVar::GetValueAsOptionalDouble)
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
								.Value(TAttribute<TOptional<double>>::Create(TAttribute<TOptional<double>>::FGetter::CreateRaw(dynVar, &FTouchDynamicVar::GetIndexedValueAsOptionalDouble, j)))
							]
						;
					}
				}
				break;
				case EVarIntent::VARINTENT_COLOR:
				{
					TSharedPtr<IPropertyHandle> colorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchDynamicVar, colorProperty));
					colorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleColorChanged, dynVar));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(colorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));
				}
				break;
				case EVarIntent::VARINTENT_POSITION:
				{
					TSharedPtr<IPropertyHandle> vectorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchDynamicVar, vector4Property));
					vectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &TouchEngineDynamicVariableStructDetailsCustomization::HandleVector4Changed, dynVar));
					IDetailPropertyRow* property = &InputGroup->AddPropertyRow(vectorHandle.ToSharedRef());
					property->ToolTip(FText::FromString(dynVar->VarName));
					property->DisplayName(FText::FromString(dynVar->VarLabel));
				}
				break;
				case EVarIntent::VARINTENT_UVW:
				{
					TSharedPtr<IPropertyHandle> vectorHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchDynamicVar, vectorProperty));
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
					.Value_Raw(dynVar, &FTouchDynamicVar::GetValueAsOptionalFloat)
				];
		}
		break;
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			TSharedPtr<IPropertyHandle> floatsHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchDynamicVar, floatBufferProperty));

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
						.Text_Raw(dynVar, &FTouchDynamicVar::HandleTextBoxText)
					];
			}
			else
			{
				auto stringHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchDynamicVar, stringArrayProperty));

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
			TSharedPtr<IPropertyHandle> textureHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchDynamicVar, textureProperty));
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
		FTouchDynamicVar* dynVar;

		{
			TArray<void*> RawData;
			PropertyHandle->AccessRawData(RawData);
			dynVarHandle->AccessRawData(RawData);
			dynVar = static_cast<FTouchDynamicVar*>(RawData[0]);
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
			TSharedPtr<IPropertyHandle> textureHandle = dynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchDynamicVar, textureProperty));
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

	RerenderPanel();
}

void TouchEngineDynamicVariableStructDetailsCustomization::RerenderPanel()
{
	if (PropUtils.IsValid())
	{
		//if (this->DynVars->parent->IsPendingKill())
		//	return;

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


void TouchEngineDynamicVariableStructDetailsCustomization::HandleChecked(ECheckBoxState InState, FTouchDynamicVar* dynVar)
{
	dynVar->HandleChecked(InState, blueprintObject, DynVars->parent);
}

FText TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxText(FTouchDynamicVar* dynVar) const
{
	FText returnVal = dynVar->HandleTextBoxText();
	return returnVal;
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextChanged(const FText& NewText, FTouchDynamicVar* dynVar)
{
	dynVar->HandleTextBoxTextChanged(NewText, blueprintObject, DynVars->parent);
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo, FTouchDynamicVar* dynVar)
{
	dynVar->HandleTextBoxTextCommited(NewText, CommitInfo, blueprintObject, DynVars->parent);
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleTextureChanged(FTouchDynamicVar* dynVar)
{
	dynVar->HandleTextureChanged(blueprintObject, DynVars->parent);
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleColorChanged(FTouchDynamicVar* dynVar)
{
	dynVar->HandleColorChanged(blueprintObject, DynVars->parent);
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleVector4Changed(FTouchDynamicVar* dynVar)
{
	dynVar->HandleVector4Changed(blueprintObject, DynVars->parent);
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleVectorChanged(FTouchDynamicVar* dynVar)
{
	dynVar->HandleVectorChanged(blueprintObject, DynVars->parent);
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChanged(FTouchDynamicVar* dynVar)
{
	dynVar->HandleFloatBufferChanged(blueprintObject, DynVars->parent);
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleFloatBufferChildChanged(FTouchDynamicVar* dynVar)
{
	dynVar->HandleFloatBufferChildChanged(blueprintObject, DynVars->parent);
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChanged(FTouchDynamicVar* dynVar)
{
	dynVar->HandleStringArrayChanged(blueprintObject, DynVars->parent);
}

void TouchEngineDynamicVariableStructDetailsCustomization::HandleStringArrayChildChanged(FTouchDynamicVar* dynVar)
{
	dynVar->HandleStringArrayChildChanged(blueprintObject, DynVars->parent);
} 