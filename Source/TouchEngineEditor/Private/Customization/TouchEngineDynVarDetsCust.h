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
#include "DetailLayoutBuilder.h"
#include "IDetailCustomization.h"
#include "IPropertyUtilities.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "Blueprint/TouchEngineComponent.h"

class IDetailGroup;
class IPropertyHandle;
class SEditableTextBox;
struct FTouchEngineDynamicVariableStruct;
class UTouchEngineComponentBase;

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

	TSharedPtr<SBox> HeaderValueWidget;
	
	FString ErrorMessage;

	FTouchEngineDynamicVariableContainer* GetDynamicVariables() const;
	
	void RebuildHeaderValueWidgetContent();

	void GenerateInputVariables(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder);
	void GenerateOutputVariables(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder);

	// Handles Filtering incompatible Textures Files from the Selection.
	bool OnShouldFilterTexture(const FAssetData& AssetData) const;

	/** Handles check box state changed */
	void HandleChecked(ECheckBoxState InState, FString Identifier, TSharedRef<IPropertyHandle> DynVarHandle);
	/** Handles value from Numeric Entry box changed */
	template <typename T>
	void HandleValueChanged(T InValue, ETextCommit::Type CommitType, FString Identifier);
	/** Handles value from Numeric Entry box changed with array Index*/
	template <typename T>
	void HandleValueChangedWithIndex(T InValue, ETextCommit::Type CommitType, int Index, FString Identifier);
	/** Handles committing the text in the editable text box. */
	void HandleTextBoxTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo, FString Identifier);

	DECLARE_DELEGATE_OneParam(FValueChangedCallback, FTouchEngineDynamicVariableStruct&);
	void HandleValueChanged(FString Identifier, FValueChangedCallback UpdateValueFunc);
	
	/** Handles changing the value of a drop down box */
	void HandleDropDownBoxValueChanged(TSharedPtr<FString> Arg, ESelectInfo::Type SelectType, FString Identifier);
	
	/** Callback when struct is filled out */
	void ToxLoaded();
	/** Callback when struct has its data reset */
	void ToxReset();
	/** Callback when struct fails to load tox file */
	void ToxFailedLoad(const FString& Error);

	/** Handles getting the text color of the editable text box. */
	FSlateColor HandleTextBoxForegroundColor() const;
	/** Handles the creation of a new array element widget from the details customization panel*/
	void OnGenerateArrayChild(TSharedRef<IPropertyHandle> ElementHandle, int32 ChildIndex, IDetailChildrenBuilder& ChildrenBuilder);
	/** Creates a default name widget */
	TSharedRef<SWidget> CreateNameWidget(const FString& Name, const FString& Tooltip, TSharedRef<IPropertyHandle> StructPropertyHandle);

	FReply OnReloadClicked();
	
	ECheckBoxState GetValueAsCheckState(FString Identifier) const;
	TOptional<int> GetValueAsOptionalInt(FString Identifier) const;
	TOptional<int> GetIndexedValueAsOptionalInt(int Index, FString Identifier) const;
	TOptional<double> GetValueAsOptionalDouble(FString Identifier) const;
	TOptional<double> GetIndexedValueAsOptionalDouble(int Index, FString Identifier) const;
	TOptional<float> GetValueAsOptionalFloat(FString Identifier) const;
	FText HandleTextBoxText(FString Identifier) const;
	
	/** Updates all instances of this type in the world */
	void UpdateDynVarInstances(UTouchEngineComponentBase* ParentComponent, FTouchEngineDynamicVariableStruct OldVar, FTouchEngineDynamicVariableStruct NewVar);

	void ForceRefresh();
};

template<typename T>
void FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged(T InValue, ETextCommit::Type CommitType, FString Identifier)
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!DynVars || !TouchEngineComponent.IsValid())
	{
		return;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);
	DynamicVariablePropertyHandle->NotifyPreChange();
	FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
	DynVar->HandleValueChanged(InValue);
	UpdateDynVarInstances(TouchEngineComponent.Get(), OldValue, *DynVar);

	if (TouchEngineComponent->EngineInfo && TouchEngineComponent->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(TouchEngineComponent->EngineInfo);
	}

	DynamicVariablePropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
}

template<typename T>
void FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex(T InValue, ETextCommit::Type CommitType, int Index, FString Identifier)
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!DynVars || !TouchEngineComponent.IsValid())
	{
		return;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);
	DynamicVariablePropertyHandle->NotifyPreChange();
	FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
	DynVar->HandleValueChangedWithIndex(InValue, Index);
	UpdateDynVarInstances(TouchEngineComponent.Get(), OldValue, *DynVar);

	if (TouchEngineComponent->EngineInfo && TouchEngineComponent->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(TouchEngineComponent->EngineInfo);
	}

	DynamicVariablePropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
}
