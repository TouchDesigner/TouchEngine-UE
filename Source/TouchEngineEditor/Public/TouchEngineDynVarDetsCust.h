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
#include "TickableEditorObject.h"

class IPropertyHandle;
class SEditableTextBox;
struct FTEDynamicVariableStruct;
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

	bool DynVarsDestroyed = false;
	/** Holds a handle to the property being edited. */
	TSharedPtr<IPropertyHandle> PropertyHandle = nullptr;

	UObject* blueprintObject;

	/** Handle to the delegate we bound so that we can unbind if we need to*/
	FDelegateHandle ToxLoaded_DelegateHandle;

	FDelegateHandle ToxFailedLoad_DelegateHandle;

	bool pendingRedraw = false;

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
	void HandleChecked(ECheckBoxState InState, FString Identifier, TSharedRef<IPropertyHandle> dynVarHandle);
	/** Handles value from Numeric Entry box changed */
	template <typename T>
	void HandleValueChanged(T inValue, ETextCommit::Type commitType, FString Identifier);
	/** Handles value from Numeric Entry box changed with array index*/
	template <typename T>
	void HandleValueChangedWithIndex(T inValue, ETextCommit::Type commitType, int index, FString Identifier);
	/** Handles changing the value in the editable text box. */
	void HandleTextBoxTextChanged(const FText& NewText, FString Identifier);
	/** Handles committing the text in the editable text box. */
	void HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo, FString Identifier);
	/** Handles changing the texture value in the render target 2D widget */
	void HandleTextureChanged(FString Identifier);
	/** Handles changing the value from the color picker widget */
	void HandleColorChanged(FString Identifier);
	/** Handles changing the value from the vector4 widget */
	void HandleVector4Changed(FString Identifier);
	/** Handles changing the value from the vector widget */
	void HandleVectorChanged(FString Identifier);
	/** Handles adding / removing a child property in the float array widget */
	void HandleFloatBufferChanged(FString Identifier);
	/** Handles changing the value of a child property in the array widget */
	void HandleFloatBufferChildChanged(FString Identifier);
	/** Handles adding / removing a child property in the string array widget */
	void HandleStringArrayChanged(FString Identifier);
	/** Handles changing the value of a child property in the string array widget */
	void HandleStringArrayChildChanged(FString Identifier);
	/** Handles changing the value of a drop down box */
	void HandleDropDownBoxValueChanged(TSharedPtr<FString> arg, ESelectInfo::Type selectType, FString Identifier);


	ECheckBoxState GetValueAsCheckState(FString Identifier) const;
	// returns value as integer in a TOptional struct
	TOptional<int> GetValueAsOptionalInt(FString Identifier) const;
	// returns indexed value as integer in a TOptional struct
	TOptional<int> GetIndexedValueAsOptionalInt(int index, FString Identifier) const;
	// returns value as double in a TOptional struct
	TOptional<double> GetValueAsOptionalDouble(FString Identifier) const;
	// returns indexed value as double in a TOptional struct
	TOptional<double> GetIndexedValueAsOptionalDouble(int index, FString Identifier) const;
	// returns value as float in a TOptional struct
	TOptional<float> GetValueAsOptionalFloat(FString Identifier) const;
	/** Handles getting the text to be displayed in the editable text box. */
	FText HandleTextBoxText(FString Identifier) const;



	/** Updates all instances of this type in the world */
	void UpdateDynVarInstances(UObject* blueprintOwner, UTouchEngineComponentBase* parentComponent, FTEDynamicVariableStruct oldVar, FTEDynamicVariableStruct newVar);

	void OnDynVarsDestroyed();
};

template<typename T>
inline void TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged(T inValue, ETextCommit::Type commitType, FString Identifier)
{
	FTEDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	PropertyHandle->NotifyPreChange();
	FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
	dynVar->HandleValueChanged(inValue);
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);
	PropertyHandle->NotifyPostChange();
}

template<typename T>
inline void TouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex(T inValue, ETextCommit::Type commitType, int index, FString Identifier)
{
	FTEDynamicVariableStruct* dynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);

	PropertyHandle->NotifyPreChange();
	FTEDynamicVariableStruct oldValue; oldValue.Copy(dynVar);
	dynVar->HandleValueChangedWithIndex(inValue, index);
	UpdateDynVarInstances(blueprintObject, DynVars->parent, oldValue, *dynVar);
	PropertyHandle->NotifyPostChange();
}
