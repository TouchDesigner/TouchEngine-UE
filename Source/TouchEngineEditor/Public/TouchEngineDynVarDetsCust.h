// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "IPropertyTypeCustomization.h"
#include "DetailLayoutBuilder.h"
#include "TickableEditorObject.h"

class IPropertyHandle;
class SEditableTextBox;
struct FTouchEngineDynamicVariable;
class UTouchEngineComponentBase;

/**
 *
 */
class TouchEngineDynamicVariableStructDetailsCustomization : public IPropertyTypeCustomization
{
public:
	TouchEngineDynamicVariableStructDetailsCustomization();
	~TouchEngineDynamicVariableStructDetailsCustomization();

	// IPropertyTypeCustomization interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	/** @return A new instance of this class */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

private:

	/** Holds the text box for editing the Guid. */
	TSharedPtr<SEditableTextBox> TextBox = nullptr;
	/** Holds Layout Builder used to create this class so we can use it to rebuild the panel*/
	TSharedPtr<class IPropertyUtilities> PropUtils;
	/** Holds all input and output variables*/
	struct FTouchEngineDynamicVariableContainer* DynVars;
	/** Holds a handle to the property being edited. */
	TSharedPtr<IPropertyHandle> PropertyHandle = nullptr;

	UObject* blueprintObject;

	/** Handle to the delegate we bound so that we can unbind if we need to*/
	FDelegateHandle ToxLoaded_DelegateHandle;

	FDelegateHandle ToxFailedLoad_DelegateHandle;

	/** Callback when struct is filled out*/
	void ToxLoaded();
	/** Callback when struct fails to load tox file*/
	void ToxFailedLoad();

	/** Redraws the details panel*/
	void RerenderPanel();
	/** Handles getting the text color of the editable text box. */
	FSlateColor HandleTextBoxForegroundColor() const;
	/** Handles the creation of a new array element widget from the details customization panel*/
	void OnGenerateArrayChild(TSharedRef<IPropertyHandle> ElementHandle, int32 ChildIndex, IDetailChildrenBuilder& ChildrenBuilder);
	/** Creates a default name widget */
	TSharedRef<SWidget> CreateNameWidget(FString name, FString tooltip, TSharedRef<IPropertyHandle> StructPropertyHandle);

	FReply OnReloadClicked();



	/** Handles check box state changed */
	void HandleChecked(ECheckBoxState InState, FTouchEngineDynamicVariable* dynVar, TSharedRef<IPropertyHandle> dynVarHandle);
	/** Handles value from Numeric Entry box changed */
	template <typename T>
	void HandleValueChanged(T inValue, ETextCommit::Type commitType, FTouchEngineDynamicVariable* dynVar);
	/** Handles value from Numeric Entry box changed with array index*/
	template <typename T>
	void HandleValueChangedWithIndex(T inValue, ETextCommit::Type commitType, int index, FTouchEngineDynamicVariable* dynVar);
	/** Handles getting the text to be displayed in the editable text box. */
	FText HandleTextBoxText(FTouchEngineDynamicVariable* dynVar) const;
	/** Handles changing the value in the editable text box. */
	void HandleTextBoxTextChanged(const FText& NewText, FTouchEngineDynamicVariable* dynVar);
	/** Handles committing the text in the editable text box. */
	void HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo, FTouchEngineDynamicVariable* dynVar);
	/** Handles changing the texture value in the render target 2D widget */
	void HandleTextureChanged(FTouchEngineDynamicVariable* dynVar);
	/** Handles changing the value from the color picker widget */
	void HandleColorChanged(FTouchEngineDynamicVariable* dynVar);
	/** Handles changing the value from the vector4 widget */
	void HandleVector4Changed(FTouchEngineDynamicVariable* dynVar);
	/** Handles changing the value from the vector widget */
	void HandleVectorChanged(FTouchEngineDynamicVariable* dynVar);
	/** Handles adding / removing a child property in the float array widget */
	void HandleFloatBufferChanged(FTouchEngineDynamicVariable* dynVar);
	/** Handles changing the value of a child property in the array widget */
	void HandleFloatBufferChildChanged(FTouchEngineDynamicVariable* dynVar);
	/** Handles adding / removing a child property in the string array widget */
	void HandleStringArrayChanged(FTouchEngineDynamicVariable* dynVar);
	/** Handles changing the value of a child property in the string array widget */
	void HandleStringArrayChildChanged(FTouchEngineDynamicVariable* dynVar);
	/** Handles changing the value of a drop down box */
	void HandleDropDownBoxValueChanged(TSharedPtr<FString> arg, ESelectInfo::Type selectType, FTouchEngineDynamicVariable* dynVar);

	/** Updates all instances of this type in the world */
	void UpdateDynVarInstances(UObject* blueprintOwner, UTouchEngineComponentBase* parentComponent, FTouchEngineDynamicVariable oldVar, FTouchEngineDynamicVariable newVar);
};

template<typename T>
inline void TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged(T inValue, ETextCommit::Type commitType, FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();
	dynVar->HandleValueChanged(inValue, commitType);
	PropertyHandle->NotifyPostChange();
}

template<typename T>
inline void TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex(T inValue, ETextCommit::Type commitType, int index, FTouchEngineDynamicVariable* dynVar)
{
	PropertyHandle->NotifyPreChange();
	dynVar->HandleValueChangedWithIndex(inValue, commitType, index);
	PropertyHandle->NotifyPostChange();
}
