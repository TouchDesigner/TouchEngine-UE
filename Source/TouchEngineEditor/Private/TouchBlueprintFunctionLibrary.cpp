// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchBlueprintFunctionLibrary.h"


bool UTouchBlueprintFunctionLibrary::TextContainsWhiteSpace(FString InText)
{
	if (InText.IsEmpty())
		return false;

	return InText.Contains(" ", ESearchCase::IgnoreCase, ESearchDir::FromStart);
}
