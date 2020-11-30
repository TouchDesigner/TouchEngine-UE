// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineComponent.h"

FTouchEngineDynamicVariableStruct::FTouchEngineDynamicVariableStruct()
{
}

FTouchEngineDynamicVariableStruct::~FTouchEngineDynamicVariableStruct()
{
}

void FTouchEngineDynamicVariableStruct::AddVar(FString name, EVarType varType, void* data)
{
	DynVars.Add(FTouchEngineDynamicVariable(name, varType, data));
}

void FTouchEngineDynamicVariableStruct::ToxLoaded()
{

}
