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
#include "Blueprint/TouchEngineComponent.h"
#include "TouchEngineDynamicVariableStruct.h"

#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "IPropertyUtilities.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/STextComboBox.h"

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
		return;
	}

	DynVars = static_cast<FTouchEngineDynamicVariableContainer*>(RawData[0]);
	if (!ensure(DynVars))
	{
		return;
	}

	if (!DynVars->Parent)
	{
		TArray<UObject*> Outers;
		PropertyHandle->GetOuterObjects(Outers);

		for (int32 i = 0; i < Outers.Num(); i++)
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

	ToxLoaded_DelegateHandle = DynVars->OnToxLoaded.Add(FTouchOnLoadComplete::FDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded));

	DynVars->OnDestruction.BindRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnDynVarsDestroyed);

	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget(LOCTEXT("ToxParameters", "Tox Parameters"), LOCTEXT("InputOutput", "Input and output variables as read from the TOX file"), false)
		]
		.ValueContent()
		.MaxDesiredWidth(250)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			BuildHeaderValueWidget()
		];
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

	PropUtils = StructCustomizationUtils.GetPropertyUtilities();

	FDetailWidgetRow& ButtonRow = StructBuilder.AddCustomRow(LOCTEXT("ReloadTox", "ReloadTox"));


	// Add groups to hold input and output variables

	// Add "Reload Tox" button to the details panel
	ButtonRow.ValueContent()
		[
			SNew(SButton)
			.Text(TAttribute<FText>(LOCTEXT("ReloadTox", "Reload Tox")))
		.OnClicked(FOnClicked::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnReloadClicked))
		]
	;

	GenerateInputVariables(StructPropertyHandle, StructBuilder);
	GenerateOutputVariables(StructPropertyHandle, StructBuilder);
}

TSharedRef<SWidget> FTouchEngineDynamicVariableStructDetailsCustomization::BuildHeaderValueWidget()
{
	if (DynVars->Parent->ToxFilePath.IsEmpty())
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("EmptyFilePath", "Empty file path."));
	}

	if (DynVars->Parent->HasFailedLoad())
	{
		// we have failed to load the tox file
		if (ErrorMessage.IsEmpty() && !DynVars->Parent->ErrorMessage.IsEmpty())
		{
			ErrorMessage = DynVars->Parent->ErrorMessage;
		}

		return SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("ToxLoadFailed", "Failed to load TOX file: {0}"), FText::FromString(ErrorMessage)));
	}
	
	const bool bIsLoading = !DynVars->Parent->IsLoaded(); 
	if (bIsLoading)
	{
		return SNew(SThrobber)
			.Animate(SThrobber::VerticalAndOpacity)
			.NumPieces(5);
	}

	// It's loaded
	return SNullWidget::NullWidget;
}

void FTouchEngineDynamicVariableStructDetailsCustomization::GenerateInputVariables(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder)
{
	IDetailGroup& InputGroup = StructBuilder.AddGroup(FName("Inputs"), LOCTEXT("Inputs", "Inputs"));
	
	// handle input variables
	TSharedPtr<IPropertyHandleArray> InputsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Input))->AsArray();
	uint32 NumInputs = 0u;
	InputsHandle->GetNumElements(NumInputs);

	for (uint32 i = 0; i < NumInputs; i++)
	{
		TSharedRef<IPropertyHandle> DynVarHandle = InputsHandle->GetElement(i);
		FTouchEngineDynamicVariableStruct* DynVar;

		TArray<void*> RawData;
		DynVarHandle->AccessRawData(RawData);

		if (RawData.Num() == 0 || !RawData[0])
		{
			continue;
		}
		DynVar = static_cast<FTouchEngineDynamicVariableStruct*>(RawData[0]);
		if (DynVar->VarName == TEXT("ERROR_NAME"))
		{
			continue;
		}

		switch (DynVar->VarType)
		{
		case EVarType::Bool:
			{
				InputGroup.AddWidgetRow()
					.NameContent()
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
		case EVarType::Int:
			{
				if (DynVar->Count == 1)
				{
					FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();

					if (DynVar->VarIntent != EVarIntent::DropDown)
					{
						NewRow.NameContent()
							[
								CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
							]
							.ValueContent()
							.MaxDesiredWidth(250)
							[
								SNew(SNumericEntryBox<int32>)
								.OnValueCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<int32>, DynVar->VarIdentifier)
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
							DropDownStrings->Add(MakeShared<FString>(Keys[j]));
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
							];
					}
				}
				else
				{
					switch (DynVar->Count)
					{
					case 2:
						{
							TSharedPtr<IPropertyHandle> IntVector2DHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, IntPointProperty));
							const FSimpleDelegate OnChangedDelegate = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
									DynVar->VarIdentifier,
									FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleIntVector2Changed(); })
							);
							IntVector2DHandle->SetOnPropertyValueChanged(OnChangedDelegate);
							IntVector2DHandle->SetOnChildPropertyValueChanged(OnChangedDelegate);
							
							InputGroup.AddPropertyRow(IntVector2DHandle.ToSharedRef())
								.ToolTip(FText::FromString(DynVar->VarName))
								.DisplayName(FText::FromString(DynVar->VarLabel));
							break;
						}
					case 3:
						{
							TSharedPtr<IPropertyHandle> IntVectorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, IntVectorProperty));
							IntVectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
									DynVar->VarIdentifier,
									FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleIntVectorChanged(); })
								)
							);
							InputGroup.AddPropertyRow(IntVectorHandle.ToSharedRef())
								.ToolTip(FText::FromString(DynVar->VarName))
								.DisplayName(FText::FromString(DynVar->VarLabel));
							break;
						}
					case 4:
						{
							TSharedPtr<IPropertyHandle> IntVector4Handle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, IntVector4Property));
							IntVector4Handle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
									DynVar->VarIdentifier,
									FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleIntVector4Changed(); })
								));
							IDetailPropertyRow& Property = InputGroup.AddPropertyRow(IntVector4Handle.ToSharedRef());
							Property.ToolTip(FText::FromString(DynVar->VarName));
							Property.DisplayName(FText::FromString(DynVar->VarLabel));

							break;
						}
					default:
						{
							for (int j = 0; j < DynVar->Count; j++)
							{
								FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();

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
					FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();

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
						];
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
									Vector2DHandle->SetOnPropertyValueChanged(
										FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
											DynVar->VarIdentifier,
											FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleVector2Changed(); })
										)
									);
									IDetailPropertyRow& Property = InputGroup.AddPropertyRow(Vector2DHandle.ToSharedRef());
									Property.ToolTip(FText::FromString(DynVar->VarName));
									Property.DisplayName(FText::FromString(DynVar->VarLabel));
									break;
								}
							case 3:
								{
									TSharedPtr<IPropertyHandle> VectorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, VectorProperty));
									VectorHandle->SetOnPropertyValueChanged(
										FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
											DynVar->VarIdentifier,
											FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleVectorChanged(); })
										)
									);
									IDetailPropertyRow& Property = InputGroup.AddPropertyRow(VectorHandle.ToSharedRef());
									Property.ToolTip(FText::FromString(DynVar->VarName));
									Property.DisplayName(FText::FromString(DynVar->VarLabel));

									break;
								}
							case 4:
								{
									TSharedPtr<IPropertyHandle> Vector4Handle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, Vector4Property));
									const FSimpleDelegate OnValueChangedDelegate = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
											DynVar->VarIdentifier,
											FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleVector4Changed(); })
										);
									Vector4Handle->SetOnPropertyValueChanged(OnValueChangedDelegate);
									Vector4Handle->SetOnChildPropertyValueChanged(OnValueChangedDelegate);
									IDetailPropertyRow& Property = InputGroup.AddPropertyRow(Vector4Handle.ToSharedRef());
									Property.ToolTip(FText::FromString(DynVar->VarName));
									Property.DisplayName(FText::FromString(DynVar->VarLabel));

									break;
								}

							default:
								{
									for (int j = 0; j < DynVar->Count; j++)
									{
										FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();

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
											];
									}
									break;
								}
							}
							break;
						}
					case EVarIntent::Color:
						{
							TSharedPtr<IPropertyHandle> ColorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, ColorProperty));
							const FSimpleDelegate OnValueChanged = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
									DynVar->VarIdentifier,
									FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleColorChanged(); })
								);
							ColorHandle->SetOnPropertyValueChanged(OnValueChanged);
							ColorHandle->SetOnChildPropertyValueChanged(OnValueChanged);
							
							InputGroup.AddPropertyRow(ColorHandle.ToSharedRef())
								.ToolTip(FText::FromString(DynVar->VarName))
								.DisplayName(FText::FromString(DynVar->VarLabel));
							break;
						}
					case EVarIntent::Position:
						{
							TSharedPtr<IPropertyHandle> VectorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, Vector4Property));
							VectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
									DynVar->VarIdentifier,
									FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleVector4Changed(); })
								));
							InputGroup.AddPropertyRow(VectorHandle.ToSharedRef())
								.ToolTip(FText::FromString(DynVar->VarName))
								.DisplayName(FText::FromString(DynVar->VarLabel));
							break;
						}
					case EVarIntent::UVW:
						{
							TSharedPtr<IPropertyHandle> VectorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, VectorProperty));
							VectorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
									DynVar->VarIdentifier,
									FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleVectorChanged(); })
								));
							InputGroup.AddPropertyRow(VectorHandle.ToSharedRef())
								.ToolTip(FText::FromString(DynVar->VarName))
								.DisplayName(FText::FromString(DynVar->VarLabel));
							break;
						}
					}
				}
				break;
			}
		case EVarType::Float:
			InputGroup.AddWidgetRow()
				.NameContent()
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
		case EVarType::CHOP:
			{
				const TSharedPtr<IPropertyHandle> FloatsHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, FloatBufferProperty));
				FloatsHandle->SetPropertyDisplayName(FText::FromString(DynVar->VarLabel));

				FloatsHandle->SetOnPropertyValueChanged(
					FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
						DynVar->VarIdentifier,
						FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleFloatBufferChanged(); })
					)
				);
				FloatsHandle->SetOnChildPropertyValueChanged(
					FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
						DynVar->VarIdentifier,
						FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleFloatBufferChildChanged(); })
					)
				);
				FloatsHandle->SetToolTipText(FText::FromString(DynVar->VarName));

				TSharedRef<FDetailArrayBuilder> ArrayBuilder = MakeShareable(new FDetailArrayBuilder(FloatsHandle.ToSharedRef()));
				ArrayBuilder->SetDisplayName(FText::FromString(DynVar->VarLabel));
				ArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild));

				InputGroup.AddPropertyRow(FloatsHandle.ToSharedRef());
				break;
			}
		case EVarType::String:
			{
				if (!DynVar->IsArray)
				{
					FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();

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
								.OnTextCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommitted, DynVar->VarIdentifier)
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
							DropDownStrings->Add(MakeShared<FString>(Keys[j]));
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
							];
					}
				}
				else
				{
					TSharedPtr<IPropertyHandle> StringHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, StringArrayProperty));

					StringHandle->SetOnPropertyValueChanged(
						FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
							DynVar->VarIdentifier,
							FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleStringArrayChanged(); })
						)
					);
					StringHandle->SetOnChildPropertyValueChanged(
						FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
							DynVar->VarIdentifier,
							FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleStringArrayChildChanged(); })
						)
					);
					StringHandle->SetToolTipText(FText::FromString(DynVar->VarName));
					StringHandle->SetPropertyDisplayName(FText::FromString(DynVar->VarLabel));

					TSharedPtr<IPropertyHandleArray> FloatsArrayHandle = StringHandle->AsArray();

					TSharedRef<FDetailArrayBuilder> ArrayBuilder = MakeShareable(new FDetailArrayBuilder(StringHandle.ToSharedRef()));
					ArrayBuilder->SetDisplayName(FText::FromString(DynVar->VarLabel));
					ArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild));

					InputGroup.AddPropertyRow(StringHandle.ToSharedRef());
				}
			}
			break;
		case EVarType::Texture:
			{
				FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();
				TSharedPtr<IPropertyHandle> TextureHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, TextureProperty));
				TextureHandle->SetOnPropertyValueChanged(
					FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
						DynVar->VarIdentifier,
						FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleTextureChanged(); })
					)
				);

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
					];
				break;
			}
		default:
			{
				break;
			}
		}
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::GenerateOutputVariables(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder)
{
	IDetailGroup& OutputGroup = StructBuilder.AddGroup(FName("Outputs"), LOCTEXT("Outputs", "Outputs"));
	
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
			if (!DynVar)
			{
				// TODO DP: Add warning
				continue;
			}
		}

		FDetailWidgetRow& NewRow = OutputGroup.AddWidgetRow();

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
				const TSharedPtr<IPropertyHandle> TextureHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, TextureProperty));
				TextureHandle->SetOnPropertyValueChanged(
					FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
						DynVar->VarIdentifier,
						FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleTextureChanged(); })
					)
				);
				
				const TSharedRef<SWidget> TextureWidget = TextureHandle->CreatePropertyValueWidget();
				TextureWidget->SetEnabled(false);
				NewRow.NameContent()
					[
						CreateNameWidget(DynVar->VarLabel, DynVar->VarName, StructPropertyHandle)
					]
					.ValueContent()
					.MaxDesiredWidth(250)
					[
						TextureWidget
					];
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
			checkNoEntry();
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

void FTouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad(const FString& Error)
{
	if (!Error.IsEmpty())
	{
		ErrorMessage = Error;

		if (DynVars && IsValid(DynVars->Parent))
		{
			DynVars->Parent->ErrorMessage = Error;
		}
	}

	RerenderPanel();
}

void FTouchEngineDynamicVariableStructDetailsCustomization::RerenderPanel()
{
	if (PropUtils.IsValid() && !PendingRedraw)
	{
		if (!DynVars || !IsValid(DynVars->Parent) || DynVars->Parent->EngineInfo)
		{
			return;
		}

		PropUtils->ForceRefresh();
		PendingRedraw = true;

		if (ToxLoaded_DelegateHandle.IsValid() && DynVars && DynVars->Parent)
		{
			DynVars->Unbind_OnToxLoaded(ToxLoaded_DelegateHandle);
		}
		if (ToxFailedLoad_DelegateHandle.IsValid() && DynVars && DynVars->Parent)
		{
			DynVars->Unbind_OnToxFailedLoad(ToxFailedLoad_DelegateHandle);
		}
	}

}


FSlateColor FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor() const
{
	const FName InvertedForegroundName("InvertedForeground");
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

	PropertyHandle->NotifyPostChange(EPropertyChangeType::ArrayAdd);
}

TSharedRef<SWidget> FTouchEngineDynamicVariableStructDetailsCustomization::CreateNameWidget(const FString& Name, const FString& Tooltip, TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	return StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(Name), FText::FromString(Tooltip), false);
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
	{
		DynVars->Parent->ReloadTox();
	}

	PropertyHandle->NotifyPostChange(EPropertyChangeType::Unspecified);

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

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo, FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleTextBoxTextCommitted(NewText);
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}

		PropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged(FString Identifier, FValueChangedCallback UpdateValueFunc)
{
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	if (DynVar)
	{
		PropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue;
		OldValue.Copy(DynVar);
		UpdateValueFunc.Execute(*DynVar);
		UpdateDynVarInstances(BlueprintObject.Get(), DynVars->Parent, OldValue, *DynVar);

		PropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);

		if (DynVars->Parent->EngineInfo && DynVars->Parent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(DynVars->Parent->EngineInfo);
		}
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

		PropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
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
