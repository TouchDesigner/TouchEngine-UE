// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineIntVector4StructCust.h"
#include "TouchEngineIntVector4.h"
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
	//StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineIntVector4, vector4));

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
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
