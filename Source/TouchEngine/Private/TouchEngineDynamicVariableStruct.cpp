// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineComponent.h"

FTouchEngineDynamicVariableStruct::FTouchEngineDynamicVariableStruct()
{
}

FTouchEngineDynamicVariableStruct::FTouchEngineDynamicVariableStruct(UTouchEngineComponentBase* _parent)
{
	//parent = _parent;
	//
	//if (parent->EngineInfo)
	//{
	//	parent->EngineInfo->getOnLoadCompleteDelegate()->AddRaw(this, &FTouchEngineDynamicVariableStruct::ToxLoaded);
	//}
}

FTouchEngineDynamicVariableStruct::~FTouchEngineDynamicVariableStruct()
{
}

void FTouchEngineDynamicVariableStruct::AddVar(FString name, EVarType varType, void* data)
{
	DynVars.Add(FTouchEngineDynamicVariable(name, varType, data));
}

void FTouchEngineDynamicVariableStruct::CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (parent && parent->EngineInfo->isLoaded())
	{
		Delegate.Execute();
	}
	else
	{
		if (!(Delegate.GetUObject() != nullptr ? OnToxLoaded.IsBoundToObject(Delegate.GetUObject()) : false))
		{
			OnToxLoaded.Add(Delegate);
		}
	}
}

void FTouchEngineDynamicVariableStruct::ToxLoaded()
{
	OnToxLoaded.Broadcast();
}