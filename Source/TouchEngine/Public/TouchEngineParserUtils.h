// Copyright © Derivative Inc. 2022

#pragma once

#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TEResult.h"

struct FTouchEngineDynamicVariableStruct;

class FTouchEngineParserUtils
{
public:
	static TEResult	ParseGroup(TEInstance* Instance, const char* Identifier, TArray<FTouchEngineDynamicVariableStruct>& VariableList);
	static TEResult	ParseInfo(TEInstance* Instance, const char* Identifier, TArray<FTouchEngineDynamicVariableStruct>& VariableList);
};
