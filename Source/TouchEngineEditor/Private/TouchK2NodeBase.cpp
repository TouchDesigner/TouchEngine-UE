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

FName UTouchK2NodeBase::GetCategoryNameChecked(const UEdGraphPin* InPin)
{
	check(InPin);
	return InPin->PinType.PinSubCategory.IsNone() ? InPin->PinType.PinCategory : InPin->PinType.PinSubCategory;
}

bool UTouchK2NodeBase::CheckPinCategory(UEdGraphPin* Pin) const
{
	const FName PinCategory = Pin->PinType.PinCategory;
	
	if (PinCategory == UEdGraphSchema_K2::PC_Real)
	{
		// In case of call we need to call the function one more time but with subcategory
		// It supports all containers EPinContainerType	Array, Set, Map
		if (const FName PinSubCategory = Pin->PinType.PinSubCategory; !PinSubCategory.IsNone())
		{
			return CheckPinCategoryInternal(Pin, PinSubCategory);
		}
	}

	return CheckPinCategoryInternal(Pin, PinCategory);
}

bool UTouchK2NodeBase::CheckPinCategoryInternal(const UEdGraphPin* InPin, const FName& InPinCategory) const
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
		if (InPin->PinType.PinSubCategoryObject.Get()->GetFName() == TBaseStructure<FVector>::Get()->GetFName())
		{
			return true;
		}
		if (InPin->PinType.PinSubCategoryObject.Get()->GetFName() == TBaseStructure<FVector4>::Get()->GetFName())
		{
			return true;
		}
		if (InPin->PinType.PinSubCategoryObject.Get()->GetFName() == TBaseStructure<FColor>::Get()->GetFName())
		{
			return true;
		}
	}
	
	return false;
}
