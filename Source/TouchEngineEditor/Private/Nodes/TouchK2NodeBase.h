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
#include "K2Node.h"
#include "TouchK2NodeBase.generated.h"

/**
 * Base Touch K2 blueprint node
 */
UCLASS()
class TOUCHENGINEEDITOR_API UTouchK2NodeBase : public UK2Node
{
	GENERATED_BODY()

public:
	//~ Begin UEdGraphNode implementation
	/**
	 * Make this node look like a standard function node with the 'f' icon at
	 * the corner and same color for it and the background
	 */
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	/** Implement our own node tooltip text */
	virtual FText GetTooltipText() const override;
	//~ End UEdGraphNode implementation

	//~ Begin K2Node implementation
	// Implement our own node category
	virtual FText GetMenuCategory() const override;
	//~ End K2Node implementation

	/** Returns whether or not the pin type is valid for a TouchEngine Output */
	virtual bool IsPinCategoryValid(UEdGraphPin* Pin) const;

	/** Return the name of the category based on input pin */
	static FName GetCategoryNameChecked(const UEdGraphPin* InPin);

	void ValidateLegacyVariableNames(const FName InSourceVar, const FKismetCompilerContext& InCompilerContext, const FString& InNodeTypePrefix) const;

protected:
	/** Common pin names used among the TouchEngine nodes */
	struct FPinNames
	{
		static const FName ParameterName; // used for TouchEngine Parameters
		static const FName InputName; // used for TouchEngine Input Variables
		static const FName OutputName; // used for TouchEngine Output Variables
		// static const FName OutputValue;
		static const FName Prefix; // Internal, part of Getter/Setter functions
		static const FName Result;
		static const FName TouchEngineComponent;
		static const FName Value;
		static const FName FrameLastUpdated;
		/** The default input Pins of a UTouchInputK2Node without exec pins. Currently FPinNames::TouchEngineComponent, FPinNames::InputName, FPinNames::Value */
		static const TArray<FName> DefaultInputs;
	};
	/** Common function parameters names used in the UTouchBlueprintFunctionLibrary functions */
	struct FFunctionParametersNames
	{
		/** Target parameter */
		static const FName TouchEngineComponent;
		/** VarName parameter */
		static const FName ParameterName;
		/** Value parameter */
		static const FName Value;
		/** Last Updated Frame parameter of type int64 */
		static const FName OutputFrameLastUpdated;
		/** Prefix parameter */
		static const FName Prefix; // Internal, part of Getter/Setter functions
		/** The default parameters of every UTouchBlueprintFunctionLibrary function. Currently all the values of FFunctionParametersNames */
		static const TArray<FName> DefaultParameters;
	};

	UEdGraphPin* CreateTouchComponentPin(const FText& Tooltip);

private:

	/**
	 * Check if the pin category is allowed to be used with the current node wildcard pin
	 *
	 * @param InPin Graph pin input
	 * @param InPinCategory Pin category string (ex. double, int, string)
	 * @return
	 */
	static bool IsPinCategoryValidInternal(const UEdGraphPin* InPin, const FName& InPinCategory);
};
