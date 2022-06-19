﻿/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
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
	/** Returns whether or not the pin type is valid for a TouchEngine Output */
	virtual bool CheckPinCategory(UEdGraphPin* Pin) const;
	
	/** Return the name of the category based on input pin */
	static FName GetCategoryNameChecked(const UEdGraphPin* InPin);

private:

	/**
	 * Checks the pin category, if that allowed to use with current node pin wildcard pin
	 * 
	 * @param InPin Graphic pin input
	 * @param InPinCategory Pin category string (ex. double, int, string)
	 * @return 
	 */
	bool CheckPinCategoryInternal(const UEdGraphPin* InPin, const FName& InPinCategory) const;
};
