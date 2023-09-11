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

#include "TouchK2NodeBase.h"

#include "Blueprint/TouchBlueprintFunctionLibrary.h"
#include "Blueprint/TouchEngineComponent.h"

#include "EdGraphSchema_K2.h"
#include "Engine/Texture.h"
#include "GraphEditorSettings.h"
#include "KismetCompiler.h"

#define LOCTEXT_NAMESPACE "TouchK2NodeBase"


const FName UTouchK2NodeBase::FPinNames::ParameterName          { TEXT("ParameterName") };
const FName UTouchK2NodeBase::FPinNames::InputName				{ TEXT("InputName") };
const FName UTouchK2NodeBase::FPinNames::OutputName				{ TEXT("OutputName") };
// const FName UTouchK2NodeBase::FPinNames::OutputValue			{ TEXT("OutputValue") };
const FName UTouchK2NodeBase::FPinNames::Prefix                 { TEXT("Prefix") };
const FName UTouchK2NodeBase::FPinNames::Result					{ TEXT("Result") };
const FName UTouchK2NodeBase::FPinNames::TouchEngineComponent	{ TEXT("TouchEngineComponent") };
const FName UTouchK2NodeBase::FPinNames::Value					{ TEXT("Value") };
const FName UTouchK2NodeBase::FPinNames::FrameLastUpdated	{ TEXT("FrameLastUpdated") };
const TArray<FName> UTouchK2NodeBase::FPinNames::DefaultInputs {TouchEngineComponent, InputName, Value};

const FName UTouchK2NodeBase::FFunctionParametersNames::TouchEngineComponent	{ TEXT("Target") };
const FName UTouchK2NodeBase::FFunctionParametersNames::ParameterName			{ TEXT("VarName") };
const FName UTouchK2NodeBase::FFunctionParametersNames::Value					{ TEXT("Value") };
const FName UTouchK2NodeBase::FFunctionParametersNames::Prefix					{ TEXT("Prefix") };
const FName UTouchK2NodeBase::FFunctionParametersNames::OutputFrameLastUpdated	{ TEXT("FrameLastUpdated") };
const TArray<FName> UTouchK2NodeBase::FFunctionParametersNames::DefaultParameters {TouchEngineComponent, ParameterName, Value, Prefix};

FSlateIcon UTouchK2NodeBase::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;

	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UTouchK2NodeBase::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
}

FText UTouchK2NodeBase::GetTooltipText() const
{
	const FText ClassName{ UTouchBlueprintFunctionLibrary::StaticClass()->GetDisplayNameText() };
	FFormatNamedArguments ContextArgs;
	ContextArgs.Add(TEXT("Class"), ClassName);
	const FText Context { FText::Format(LOCTEXT("NodeTitle_Context", "Target is {Class}"), ContextArgs) };

	FFormatNamedArguments FullTitleArgs;
	FullTitleArgs.Add(TEXT("Title"), GetNodeTitle(ENodeTitleType::MenuTitle));
	FullTitleArgs.Add(TEXT("Context"), Context);

	return FText::Format(LOCTEXT("NodeTitle_FullTitle", "{Title}\n\n{Context}"), FullTitleArgs);
}

FText UTouchK2NodeBase::GetMenuCategory() const
{
	return LOCTEXT("TouchEngine_MenuCategory", "TouchEngine|Properties");
}

FName UTouchK2NodeBase::GetCategoryNameChecked(const UEdGraphPin* InPin)
{
	check(InPin);
	return InPin->PinType.PinSubCategory.IsNone() ? InPin->PinType.PinCategory : InPin->PinType.PinSubCategory;
}

void UTouchK2NodeBase::ValidateLegacyVariableNames(const FName InSourceVar, const FKismetCompilerContext& InCompilerContext, const FString& InNodeTypePrefix) const
{
	const FString VarName = *FindPinChecked(InSourceVar)->DefaultValue;
	if (VarName.StartsWith("p/") || VarName.StartsWith("i/") || VarName.StartsWith("o/"))
	{
		InCompilerContext.MessageLog.Warning("LegacyParamNames", *FString::Printf(TEXT("%s - Prefix is no longer required for Touch Variables.\nReplace with a matching Parameter/Input/Output node"), *VarName));
	}
}

bool UTouchK2NodeBase::IsPinCategoryValid(UEdGraphPin* Pin) const
{
	const FName PinCategory = Pin->PinType.PinCategory;

	if (PinCategory == UEdGraphSchema_K2::PC_Real)
	{
		// In case of call we need to call the function one more time but with subcategory
		// It supports all containers EPinContainerType	Array, Set, Map
		if (const FName PinSubCategory = Pin->PinType.PinSubCategory; !PinSubCategory.IsNone())
		{
			return IsPinCategoryValidInternal(Pin, PinSubCategory);
		}
	}

	return IsPinCategoryValidInternal(Pin, PinCategory);
}

UEdGraphPin* UTouchK2NodeBase::CreateTouchComponentPin(const FText& Tooltip)
{
	UEdGraphPin* InObjectPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UTouchEngineComponentBase::StaticClass(), FPinNames::TouchEngineComponent);
	if (ensure(InObjectPin))
	{
		// Format tooltip to add the type of object the pin expects
		const static FText PinTypeDescription{ FText::FormatOrdered(
				LOCTEXT("ComponentPinTypeDesc", "{0} Object Reference"),
				UTouchEngineComponentBase::StaticClass()->GetDisplayNameText()
			)};
		FFormatNamedArguments Args;
		Args.Add(TEXT("Tooltip"), Tooltip);
		Args.Add(TEXT("PinDescription"), PinTypeDescription);
		InObjectPin->PinToolTip = FText::Format(LOCTEXT("ObjPinTooltip", "{Tooltip}\n{PinDescription}"), Args).ToString();

		// Workaround to prevent 'TouchEngine' from becoming 'Touch Engine'.
		// We override the GetPinNameOverride function and use the PinFriendlyName there.
		// We have to set bAllowFriendlyName to false to prevent UEdGraphSchema_K2::GetPinDisplayName
		// from overriding the overridden name (yeah, it's stupid that the override gets overridden there)
		InObjectPin->PinFriendlyName = UTouchEngineComponentBase::StaticClass()->GetDisplayNameText();
		InObjectPin->bAllowFriendlyName = false;
	}
	return InObjectPin;
}

bool UTouchK2NodeBase::IsPinCategoryValidInternal(const UEdGraphPin* InPin, const FName& InPinCategory)
{
	if (InPinCategory == UEdGraphSchema_K2::PC_Float ||
		InPinCategory == UEdGraphSchema_K2::PC_Double ||
		InPinCategory == UEdGraphSchema_K2::PC_Int ||
		InPinCategory == UEdGraphSchema_K2::PC_Int64 ||
		InPinCategory == UEdGraphSchema_K2::PC_Boolean ||
		InPinCategory == UEdGraphSchema_K2::PC_Name ||
		InPinCategory == UEdGraphSchema_K2::PC_Int ||
		InPinCategory == UEdGraphSchema_K2::PC_Byte ||
		InPinCategory == UEdGraphSchema_K2::PC_String ||
		InPinCategory == UEdGraphSchema_K2::PC_Text ||
		InPinCategory == UEdGraphSchema_K2::PC_Enum
		)
	{
		return true;
	}
	if (InPinCategory == UEdGraphSchema_K2::PC_Object)
	{
		if (Cast<UClass>(InPin->PinType.PinSubCategoryObject.Get())->IsChildOf<UTexture>() || UTexture::StaticClass()->IsChildOf(Cast<UClass>(InPin->PinType.PinSubCategoryObject.Get())))
		{
			return true;
		}
	}
	else if (InPinCategory == UEdGraphSchema_K2::PC_Struct)
	{
		if (const UObject* PinSubCategoryObject = InPin->PinType.PinSubCategoryObject.Get())
		{
			if (PinSubCategoryObject->GetFName() == TBaseStructure<FVector>::Get()->GetFName())
			{
				return true;
			}
			if (PinSubCategoryObject->GetFName() == TBaseStructure<FVector4>::Get()->GetFName())
			{
				return true;
			}
			if (PinSubCategoryObject->GetFName() == TBaseStructure<FVector2D>::Get()->GetFName())
			{
				return true;
			}
			if (PinSubCategoryObject->GetFName() == TBaseStructure<FColor>::Get()->GetFName())
			{
				return true;
			}
			if (PinSubCategoryObject->GetFName() == TBaseStructure<FLinearColor>::Get()->GetFName())
			{
				return true;
			}
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE