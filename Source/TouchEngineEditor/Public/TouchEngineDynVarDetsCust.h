// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "IPropertyTypeCustomization.h"
#include "DetailLayoutBuilder.h"
#include "TickableEditorObject.h"

class IPropertyHandle;
class SEditableTextBox;
struct FTouchDynamicVar;

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
	void HandleChecked(ECheckBoxState InState, FTouchDynamicVar* dynVar);
	/** Handles value from Numeric Entry box changed */
	template <typename T>
	void HandleValueChanged(T inValue, ETextCommit::Type commitType, FTouchDynamicVar* dynVar);
	/** Handles value from Numeric Entry box changed with array index*/
	template <typename T>
	void HandleValueChangedWithIndex(T inValue, ETextCommit::Type commitType, int index, FTouchDynamicVar* dynVar);
	/** Handles getting the text to be displayed in the editable text box. */
	FText HandleTextBoxText(FTouchDynamicVar* dynVar) const;
	/** Handles changing the value in the editable text box. */
	void HandleTextBoxTextChanged(const FText& NewText, FTouchDynamicVar* dynVar);
	/** Handles committing the text in the editable text box. */
	void HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo, FTouchDynamicVar* dynVar);
	/** Handles changing the texture value in the render target 2D widget */
	void HandleTextureChanged(FTouchDynamicVar* dynVar);
	/** Handles changing the value from the color picker widget */
	void HandleColorChanged(FTouchDynamicVar* dynVar);
	/** Handles changing the value from the vector4 widget */
	void HandleVector4Changed(FTouchDynamicVar* dynVar);
	/** Handles changing the value from the vector widget */
	void HandleVectorChanged(FTouchDynamicVar* dynVar);
	/** Handles adding / removing a child property in the float array widget */
	void HandleFloatBufferChanged(FTouchDynamicVar* dynVar);
	/** Handles changing the value of a child property in the array widget */
	void HandleFloatBufferChildChanged(FTouchDynamicVar* dynVar);
	/** Handles adding / removing a child property in the string array widget */
	void HandleStringArrayChanged(FTouchDynamicVar* dynVar);
	/** Handles changing the value of a child property in the string array widget */
	void HandleStringArrayChildChanged(FTouchDynamicVar* dynVar);
};

template<typename T>
inline void TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged(T inValue, ETextCommit::Type commitType, FTouchDynamicVar* dynVar)
{
	dynVar->HandleValueChanged(inValue, commitType, blueprintObject, DynVars->parent);
}

template<typename T>
inline void TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex(T inValue, ETextCommit::Type commitType, int index, FTouchDynamicVar* dynVar)
{
	dynVar->HandleValueChangedWithIndex(inValue, commitType, index, blueprintObject, DynVars->parent);
}
