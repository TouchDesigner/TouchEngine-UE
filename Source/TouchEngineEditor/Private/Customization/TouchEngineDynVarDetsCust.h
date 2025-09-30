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

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateTypes.h"
#include "IPropertyTypeCustomization.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "IPropertyUtilities.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "Blueprint/TouchEngineComponent.h"
#include "Widgets/Input/SNumericEntryBox.h"

class IDetailGroup;
class IPropertyHandle;
class SEditableTextBox;
struct FTouchEngineDynamicVariableStruct;
class UTouchEngineComponentBase;

#define LOCTEXT_NAMESPACE "TouchEngineDynamicVariableDetailsCustomization"

/**
 *
 */
class FTouchEngineDynamicVariableStructDetailsCustomization : public IPropertyTypeCustomization
{
public:
	
	virtual ~FTouchEngineDynamicVariableStructDetailsCustomization() override;

	//~ Begin IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	//~ End IPropertyTypeCustomization Interface

	/** @return A new instance of this class */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

private:

	/** Holds Layout Builder used to create this class so we can use it to rebuild the panel*/
	TWeakPtr<IPropertyUtilities> PropUtils;

	/** Holds a handle to the property being edited. */
	TSharedPtr<IPropertyHandle> DynamicVariablePropertyHandle = nullptr;
	TWeakObjectPtr<UTouchEngineComponentBase> TouchEngineComponent;
	TOptional<FTouchEngineDynamicVariableStruct> PreviousValue;
	struct FDynVarData
	{
		FString Identifier;
		TSharedPtr<IPropertyHandle> DynVarHandle;
	};
	TMap<FString, FDynVarData> DynamicVariablesData;

	FString ErrorMessage;

	FTouchEngineDynamicVariableContainer* GetDynamicVariables() const;
	bool GetDynamicVariableByIdentifier(const FString& Identifier, FTouchEngineDynamicVariableStruct*& DynVar, TSharedPtr<IPropertyHandle>& VarHandle) const;
	static TSharedPtr<FTouchEngineDynamicVariableStructDetailsCustomization> GetDynamicVariableByIdentifierWeak(const TWeakPtr<FTouchEngineDynamicVariableStructDetailsCustomization>& ThisWeak,const FString& Identifier, FTouchEngineDynamicVariableStruct*& DynVar, TSharedPtr<IPropertyHandle>& VarHandle);

	uint32 GenerateInputVariables(IDetailGroup& InputGroup, EVarScope ScopeFilter, const FString& ParentIdentifier = "", uint32 StartIndex = 0);
	uint32 GenerateOutputVariables(IDetailGroup& OutputGroup, const FString& ParentIdentifier = "", uint32 StartIndex = 0);
	void SetPreviousValue(FString Identifier);

	// Handles Filtering incompatible Textures Files from the Selection.
	static bool OnShouldFilterTexture(const FAssetData& AssetData);

	/** Handles check box state changed */
	void HandleChecked(ECheckBoxState InState, FString Identifier);
	/** Handles value from Numeric Entry box changed */
	template <typename T>
	void HandleValueChanged(T InValue, FString Identifier);
	/** Handles value from Numeric Entry box committed */
	template <typename T>
	void HandleValueCommitted(T InValue, ETextCommit::Type CommitType, FString Identifier);
	/** Handles value from Numeric Entry box changed with array Index*/
	template <typename T>
	void HandleValueChangedWithIndex(T InValue, int Index, FString Identifier);
	/** Handles value from Numeric Entry box committed with array Index*/
	template <typename T>
	void HandleValueCommittedWithIndex(T InValue, ETextCommit::Type CommitType, int Index, FString Identifier);
	/** Handles committing the text in the editable text box. */
	void HandleTextBoxTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo, FString Identifier);

	DECLARE_DELEGATE_TwoParams(FValueChangedCallback, FTouchEngineDynamicVariableStruct&, const UTouchEngineInfo*);
	void HandleValueChanged(FString Identifier, FValueChangedCallback UpdateValueFunc);
	
	/** Callback when struct is filled out */
	void ToxLoaded();
	/** Callback when struct has its data reset */
	void ToxReset();
	/** Callback when struct fails to load tox file */
	void ToxFailedLoad(const FString& Error);

	/** Creates a default name widget */
	static TSharedRef<SWidget> CreateNameWidget(const FString& Name, const FText& Tooltip, const TSharedRef<IPropertyHandle>& StructPropertyHandle);
	
	ECheckBoxState GetValueAsCheckState(FString Identifier) const;
	template <typename T>
	TOptional<T> GetValueAsOptionalT(FString Identifier) const;
	template <typename T>
	TOptional<T> GetIndexedValueAsOptionalT(int Index, FString Identifier, FString From) const;
	FText GetValueAsFText(FString Identifier) const;
	
	/** Updates all instances of this type in the world */
	void UpdateDynVarInstances(UTouchEngineComponentBase* ParentComponent, const FTouchEngineDynamicVariableStruct& OldVar, const FTouchEngineDynamicVariableStruct& NewVar);

	void ForceRefresh();
	
	bool IsResetToDefaultVisible(const FString Identifier) const;
	bool IsResetToDefaultVisible(const FString Identifier, int Index) const;
	void ResetToDefaultHandler(const FString Identifier);
	void ResetToDefaultHandler(const FString Identifier, int Index);

	template <typename T>
	FDetailWidgetRow& GenerateNumericInputForProperty(IDetailGroup& DetailGroup, TSharedRef<IPropertyHandle>& VarHandle, FTouchEngineDynamicVariableStruct* DynVar, const FResetToDefaultOverride& ResetToDefault);

	FDetailWidgetRow& GenerateDropDownInputForProperty(IDetailGroup& DetailGroup, TSharedRef<IPropertyHandle>& VarHandle, FTouchEngineDynamicVariableStruct* DynVar, const FResetToDefaultOverride& ResetToDefault);
	FDetailWidgetRow& GenerateSuggestionDropDownInputForProperty(IDetailGroup& DetailGroup, TSharedRef<IPropertyHandle>& VarHandle, FTouchEngineDynamicVariableStruct* DynVar, const FResetToDefaultOverride& ResetToDefault);
	template <typename T>
	IDetailGroup& GenerateNumericArrayInputForProperty(IDetailGroup& DetailGroup, TSharedRef<IPropertyHandle>& VarHandle, FTouchEngineDynamicVariableStruct* DynVar, const FResetToDefaultOverride& ResetToDefault);
	IDetailGroup& GenerateColorInputForProperty(IDetailGroup& DetailGroup, TSharedRef<IPropertyHandle>& VarHandle, FTouchEngineDynamicVariableStruct* DynVar, const FResetToDefaultOverride& ResetToDefault);
};

template<typename T>
void FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged(T InValue, FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		InValue = DynVar->GetClampedValue(InValue);
		if (!DynVar->HasSameValueT(InValue))
		{
			DynVar->HandleValueChanged(DynVar->GetClampedValue(InValue), TouchEngineComponent->EngineInfo);
		}
	}
}

template <typename T>
void FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueCommitted(T InValue, ETextCommit::Type CommitType, FString Identifier)
{
	ON_SCOPE_EXIT
	{
		PreviousValue.Reset();
	};
	
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (!GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		return;
	}
	
	InValue = DynVar->GetClampedValue(InValue);
	const bool ValueChanged = !(PreviousValue ? PreviousValue->HasSameValueT(InValue) : DynVar->HasSameValueT(InValue));
	if (!ValueChanged)
	{
		return;
	}

	if (!PreviousValue) // If we don't have a previous value, the slider was not used so the current value of the DynVar will change to InValue
	{
		PreviousValue = *DynVar;
	}
	else
	{
		// If we do have a previous value, the slider was used and the current value of the DynVar has already changed from when the slider started to be used.
		// we reset the value of the DynVar before calling all the events
		DynVar->SetValue(PreviousValue.GetPtrOrNull());
	}
	
	GEditor->BeginTransaction(FText::FromString(FString::Printf(TEXT("Edit %s"), *Identifier)));
	DynVarHandle->NotifyPreChange();
	
	DynVar->HandleValueChanged(InValue, TouchEngineComponent->EngineInfo);
	UpdateDynVarInstances(TouchEngineComponent.Get(), PreviousValue.GetValue(), *DynVar);
	
	DynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	GEditor->EndTransaction();
}

template <typename T>
void FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex(T InValue, int Index, FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		InValue = DynVar->GetClampedValue(InValue, Index);
		if (!DynVar->HasSameValueT(InValue, Index))
		{
			DynVar->HandleValueChangedWithIndex(InValue, Index, TouchEngineComponent->EngineInfo);
		}
	}
}

template<typename T>
void FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueCommittedWithIndex(T InValue, ETextCommit::Type CommitType, int Index, FString Identifier)
{
	ON_SCOPE_EXIT
	{
		PreviousValue.Reset();
	};
	
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (!GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		return;
	}
	
	InValue = DynVar->GetClampedValue(InValue, Index);
	const bool ValueChanged = !(PreviousValue ? PreviousValue->HasSameValueT(InValue, Index) : DynVar->HasSameValueT(InValue, Index));
	if (!ValueChanged)
	{
		return;
	}

	if (!PreviousValue) // If we don't have a previous value, the slider was not used so the current value of the DynVar will change to InValue
	{
		PreviousValue = *DynVar;
	}
	else
	{
		// If we do have a previous value, the slider was used and the current value of the DynVar has already changed from when the slider started to be used.
		// we reset the value of the DynVar before calling all the events
		DynVar->SetValue(PreviousValue.GetPtrOrNull());
	}
	
	GEditor->BeginTransaction(FText::FromString(FString::Printf(TEXT("Edit %s"), *Identifier)));
	DynVarHandle->NotifyPreChange();
	
	DynVar->HandleValueChangedWithIndex(InValue, Index, TouchEngineComponent->EngineInfo);
	UpdateDynVarInstances(TouchEngineComponent.Get(), PreviousValue.GetValue(), *DynVar);
	
	DynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	GEditor->EndTransaction();
}

template <typename T>
TOptional<T> FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalT(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		return TOptional<T>(DynVar->GetValue<T>());
	}
	else if (DynVar)
	{
		return TOptional<T>(T {});
	}
	return {};
}

template <typename T>
TOptional<T> FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalT(int Index, FString Identifier, FString From) const
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		T Value = DynVar->GetValue<T>(Index);
		return TOptional<T>(Value);
	}
	else if (DynVar)
	{
		return TOptional<T>(T {});
	}
	return {};
}

template <typename T>
FDetailWidgetRow& FTouchEngineDynamicVariableStructDetailsCustomization::GenerateNumericInputForProperty(IDetailGroup& DetailGroup, TSharedRef<IPropertyHandle>& VarHandle, FTouchEngineDynamicVariableStruct* DynVar, const FResetToDefaultOverride& ResetToDefault)
{
	TOptional<T> ClampMinValue = DynVar->ClampMin.IsEmpty() ? TOptional<T>{} : DynVar->ClampMin.GetValue<T>();
	TOptional<T> ClampMaxValue = DynVar->ClampMax.IsEmpty() ? TOptional<T>{} : DynVar->ClampMax.GetValue<T>();
	TOptional<T> UIMinValue = DynVar->UIMin.IsEmpty() ? TOptional<T>{} : DynVar->UIMin.GetValue<T>();
	TOptional<T> UIMaxValue = DynVar->UIMax.IsEmpty() ? TOptional<T>{} : DynVar->UIMax.GetValue<T>();
	
	FDetailWidgetRow& Row = DetailGroup.AddPropertyRow(VarHandle) // we need a property handle to allow display name copy
		.ShowPropertyButtons(false)
		.CustomWidget()
		.NameContent()
		[
			VarHandle->CreatePropertyNameWidget(FText::FromString(DynVar->VarLabel), DynVar->GetTooltip())
		]
		.ValueContent()
		[
			SNew(SNumericEntryBox<T>)
			.OnValueCommitted(SNumericEntryBox<T>::FOnValueCommitted::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueCommitted<T>, DynVar->VarIdentifier))
			.OnValueChanged(SNumericEntryBox<T>::FOnValueChanged::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<T>, DynVar->VarIdentifier))
			.OnBeginSliderMovement(FSimpleDelegate::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::SetPreviousValue, DynVar->VarIdentifier))
			.AllowSpin(UIMinValue.IsSet() && UIMaxValue.IsSet())
			.MinValue(ClampMinValue)
			.MinSliderValue(UIMinValue)
			.MaxValue(ClampMaxValue)
			.MaxSliderValue(UIMaxValue)
			.Value(TAttribute<TOptional<T>>::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalT<T>, DynVar->VarIdentifier))
		]
		.OverrideResetToDefault(ResetToDefault);

	return Row;
}

template <typename T>
IDetailGroup& FTouchEngineDynamicVariableStructDetailsCustomization::GenerateNumericArrayInputForProperty(IDetailGroup& DetailGroup, TSharedRef<IPropertyHandle>& VarHandle, FTouchEngineDynamicVariableStruct* DynVar, const FResetToDefaultOverride& ResetToDefault)
{
	// First we setup the metadata and variables. part of the below inspired from SPropertyEditorNumeric.
	check(DynVar->bIsArray && (DynVar->VarType == EVarType::Double || DynVar->VarType == EVarType::Float || DynVar->VarType == EVarType::Int))

	TArray<TOptional<T>> ClampMinValues = DynVar->ClampMin.IsEmpty() ? TArray<TOptional<T>>{} : DynVar->ClampMin.GetValue<TArray<TOptional<T>>>();
	TArray<TOptional<T>> ClampMaxValues = DynVar->ClampMax.IsEmpty() ? TArray<TOptional<T>>{} : DynVar->ClampMax.GetValue<TArray<TOptional<T>>>();
	TArray<TOptional<T>> UIMinValues = DynVar->UIMin.IsEmpty() ? TArray<TOptional<T>>{} : DynVar->UIMin.GetValue<TArray<TOptional<T>>>();
	TArray<TOptional<T>> UIMaxValues = DynVar->UIMax.IsEmpty() ? TArray<TOptional<T>>{} : DynVar->UIMax.GetValue<TArray<TOptional<T>>>();

	TArray<FString> Labels;
	if (DynVar->VarIntent == EVarIntent::Position)
	{
		Labels = {TEXT("X"), TEXT("Y"), TEXT("Z"), TEXT("W")};
	}
	else if (DynVar->VarIntent == EVarIntent::Size)
	{
		Labels = {TEXT("W"), TEXT("H")};
	}
	else if (DynVar->VarIntent == EVarIntent::UVW)
	{
		Labels = {TEXT("U"), TEXT("V"), TEXT("W")};
	}
	else if (DynVar->VarIntent == EVarIntent::Color)
	{
		Labels = {TEXT("R"), TEXT("G"), TEXT("B"), TEXT("A")};
	}
	while (Labels.Num() < DynVar->Count)
	{
		Labels.Add(FString::Printf(TEXT("[%d]"), Labels.Num()));
	}

	
	IDetailGroup& Group = DetailGroup.AddGroup(FName(DynVar->VarIdentifier), FText::FromString(DynVar->VarLabel));
	TSharedPtr<SHorizontalBox> HorizontalBox;
	Group.HeaderProperty(VarHandle) // we need a property handle to allow display name copy
		.ShowPropertyButtons(false)
		.CustomWidget()
		.NameContent()
		[
			VarHandle->CreatePropertyNameWidget(FText::FromString(DynVar->VarLabel), DynVar->GetTooltip())
		]
		.ValueContent()
		.HAlign(HAlign_Fill)
		[
			SAssignNew(HorizontalBox, SHorizontalBox)
		]
		.OverrideResetToDefault(ResetToDefault);
	
	for (int i = 0; i < DynVar->Count; ++i)
	{
		TOptional<T> ClampMinValue = ClampMinValues.IsValidIndex(i) ? ClampMinValues[i] : TOptional<T>{};
		TOptional<T> ClampMaxValue = ClampMaxValues.IsValidIndex(i) ? ClampMaxValues[i] : TOptional<T>{};
		TOptional<T> UIMinValue = UIMinValues.IsValidIndex(i) ? UIMinValues[i] : TOptional<T>{};
		TOptional<T> UIMaxValue = UIMaxValues.IsValidIndex(i) ? UIMaxValues[i] : TOptional<T>{};
		if (DynVar->VarIntent == EVarIntent::Color && i == 3) // ensuring Alpha values have a slider too. This is due to how the Dynamic Var currently handles colors and RGB colors sometimes having an Alpha value
		{
			if (!UIMinValue.IsSet())
			{
				UIMinValue = 0;
			}
			if (!UIMaxValue.IsSet())
			{
				UIMaxValue = 1;
			}
		}

		// we cannot use the FResetToDefaultOverride Create(FIsResetToDefaultVisible, FResetToDefaultHandler) constructor as the click delegate never gets called if the property is null
		FResetToDefaultOverride ResetToDefaultIndex = FResetToDefaultOverride::Create(
			TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::IsResetToDefaultVisible, DynVar->VarIdentifier, i)),
			FSimpleDelegate::CreateSP(SharedThis(this), &FTouchEngineDynamicVariableStructDetailsCustomization::ResetToDefaultHandler, DynVar->VarIdentifier, i)
		);

		Group.AddWidgetRow()
		   .NameContent()
			[
				VarHandle->CreatePropertyNameWidget(FText::FromString(Labels[i]), FText::FromString(Labels[i]))
			]
			.ValueContent()
			[
				SNew(SNumericEntryBox<T>)
				.OnValueCommitted(SNumericEntryBox<T>::FOnValueCommitted::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueCommittedWithIndex<T>, i, DynVar->VarIdentifier))
				.OnValueChanged(SNumericEntryBox<T>::FOnValueChanged::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex<T>, i, DynVar->VarIdentifier))
				.OnBeginSliderMovement(FSimpleDelegate::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::SetPreviousValue, DynVar->VarIdentifier))
				.AllowSpin(true)
				// We actually NEED this to have this to true. There is an issue in SNumericEntryBox<>::GetCachedString which might lead to the internal data and
				// displayed data to get disconnected on undo. This issue does not appear with AllowSpin(true) because the value is then re-cached on Tick
				.MinValue(ClampMinValue)
				.MinSliderValue(UIMinValue.Get(0)) // As we need the slider, default to 0
				.MaxValue(ClampMaxValue)
				.MaxSliderValue(UIMaxValue.Get(1)) // As we need the slider, default to 1
				.Value(TAttribute<TOptional<T>>::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalT<T>, i, DynVar->VarIdentifier, TEXT("Slider")))
			]
			.OverrideResetToDefault(ResetToDefaultIndex);

		if (DynVar->Count <= 4)
		{
			HorizontalBox->AddSlot()
			.Padding( FMargin( 0, 1, 0, 1 ) )
			.FillWidth(1)
			[
				SNew(SNumericEntryBox<T>)
				.OnValueCommitted(SNumericEntryBox<T>::FOnValueCommitted::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueCommittedWithIndex<T>, i, DynVar->VarIdentifier))
				.OnValueChanged(SNumericEntryBox<T>::FOnValueChanged::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex<T>, i, DynVar->VarIdentifier))
				.OnBeginSliderMovement(FSimpleDelegate::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::SetPreviousValue, DynVar->VarIdentifier))
				.AllowSpin(true)
				// We actually NEED this to have this to true. There is an issue in SNumericEntryBox<>::GetCachedString which might lead to the internal data and
				// displayed data to get disconnected on undo. This issue does not appear with AllowSpin(true) because the value is then re-cached on Tick
				.MinValue(ClampMinValue)
				.MinSliderValue(UIMinValue.Get(0)) // As we need the slider, default to 0
				.MaxValue(ClampMaxValue)
				.MaxSliderValue(UIMaxValue.Get(1)) // As we need the slider, default to 1
				.Value(TAttribute<TOptional<T>>::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalT<T>, i, DynVar->VarIdentifier, TEXT("Header")))
			];
		}
	}
	return Group;

}

#undef LOCTEXT_NAMESPACE
