// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TouchEngineDynamicVariableStruct.generated.h"


class UTouchEngineComponentBase;

enum class EVarType
{
	VARTYPE_NOT_SET = 0,
	VARTYPE_INT,
	VARTYPE_FLOAT,
	VARTYPE_STRING,
	VARTYPE_MAX
};

USTRUCT()
struct TOUCHENGINE_API FTouchEngineDynamicVariable
{
	GENERATED_BODY()

public:
	FTouchEngineDynamicVariable() {}
	~FTouchEngineDynamicVariable() {}

	FTouchEngineDynamicVariable(FString name, EVarType type, void* data)
	{
		VarName = name; VarType = type; Data = data;
	}

	FString VarName = "Hello";
	EVarType VarType;
	void* Data;
};


/**
 * 
 */
USTRUCT(BlueprintType)
struct TOUCHENGINE_API FTouchEngineDynamicVariableStruct
{
	GENERATED_BODY()

public:
	FTouchEngineDynamicVariableStruct();
	~FTouchEngineDynamicVariableStruct();

	void AddVar(FString name, EVarType varType, void* data);


	TArray<FTouchEngineDynamicVariable> DynVars;


	UTouchEngineComponentBase* parent;



	void ToxLoaded();
};
