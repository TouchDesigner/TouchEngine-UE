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


#include "TouchEngineIntVector4StructCust.h"
#include "UObject/UnrealType.h"

TSharedRef<IPropertyTypeCustomization> FTouchEngineIntVector4StructCust::MakeInstance()
{
	return MakeShareable(new FTouchEngineIntVector4StructCust);
}

void FTouchEngineIntVector4StructCust::GetSortedChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, TArray<TSharedRef<IPropertyHandle>>& OutChildren)
{
	static const FName X("X");
	static const FName Y("Y");
	static const FName Z("Z");
	static const FName W("W");


	TSharedPtr<IPropertyHandle> VectorChildren[4];

	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren(NumChildren);

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		const TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		if (PropertyName == X)
		{
			VectorChildren[0] = ChildHandle;
		}
		else if (PropertyName == Y)
		{
			VectorChildren[1] = ChildHandle;
		}
		else if (PropertyName == Z)
		{
			VectorChildren[2] = ChildHandle;
		}
		else
		{
			check(PropertyName == W);
			VectorChildren[3] = ChildHandle;
		}
	}

	OutChildren.Add(VectorChildren[0].ToSharedRef());
	OutChildren.Add(VectorChildren[1].ToSharedRef());
	OutChildren.Add(VectorChildren[2].ToSharedRef());
	OutChildren.Add(VectorChildren[3].ToSharedRef());
}
