// Copyright © Derivative Inc. 2021

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "TouchK2NodeBase.generated.h"

/**
 * 
 */
UCLASS()
class TOUCHENGINEEDITOR_API UTouchK2NodeBase : public UK2Node
{
	GENERATED_BODY()

public:
	/** Returns whether or not the pin type is valid for a TouchEngine Output */
	virtual bool CheckPinCategory(UEdGraphPin* Pin) const;
	
	/** */
	static FName GetCategoryNameChecked(const UEdGraphPin* InPin);

private:
	/** */
	bool CheckPinCategoryInternal(const UEdGraphPin* InPin, const FName& InPinCategory) const;
};
