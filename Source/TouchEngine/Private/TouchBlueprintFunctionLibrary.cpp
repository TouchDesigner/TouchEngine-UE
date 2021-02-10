// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchBlueprintFunctionLibrary.h"

namespace FSetterFunctionNames
{
	static const FName FloatSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetFloatByName));
	static const FName FloatArraySetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetFloatArrayByName));
	static const FName IntSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetIntByName));
	static const FName Int64SetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetInt64ByName));
	static const FName BoolSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetBoolByName));
	static const FName NameSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetNameByName));
	static const FName ObjectSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetObjectByName));
	static const FName ClassSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetClassByName));
	static const FName ByteSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetByteByName));
	static const FName StringSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetStringByName));
	static const FName StringArraySetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetStringArrayByName));
	static const FName TextSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetTextByName));
	static const FName StructSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetStructByName));
	static const FName EnumSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetEnumByName));
};

namespace FGetterFunctionNames
{
	static const FName ObjectGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetObjectByName));
	static const FName StringArrayGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringArrayByName));
	static const FName FloatArrayGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatArrayByName));
	static const FName StringGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringByName));
	static const FName FloatGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatByName));
};


UFunction* UTouchBlueprintFunctionLibrary::FindSetterByType(FName InType, bool IsArray)
{
	if (InType.ToString().IsEmpty())
		return nullptr;

	FName FunctionName = "";

	if (InType == TEXT("float"))
	{
		if (!IsArray)
		{
			FunctionName = FSetterFunctionNames::FloatSetterName;
		}
		else
		{
			FunctionName = FSetterFunctionNames::FloatArraySetterName;
		}
	}
	else if (InType == TEXT("int"))
	{
		FunctionName = FSetterFunctionNames::IntSetterName;
	}
	else if (InType == TEXT("int64"))
	{
		FunctionName = FSetterFunctionNames::Int64SetterName;
	}
	else if (InType == TEXT("bool"))
	{
		FunctionName = FSetterFunctionNames::BoolSetterName;
	}
	else if (InType == TEXT("name"))
	{
		FunctionName = FSetterFunctionNames::NameSetterName;
	}
	else if (InType == TEXT("object"))
	{
		FunctionName = FSetterFunctionNames::ObjectSetterName;
	}
	else if (InType == TEXT("class"))
	{
		FunctionName = FSetterFunctionNames::ClassSetterName;
	}
	else if (InType == TEXT("byte"))
	{
		FunctionName = FSetterFunctionNames::ByteSetterName;
	}
	else if (InType == TEXT("string"))
	{
		if (!IsArray)
		{
			FunctionName = FSetterFunctionNames::StringSetterName;
		}
		else
		{
			FunctionName = FSetterFunctionNames::StringArraySetterName;
		}
	}
	else if (InType == TEXT("text"))
	{
		FunctionName = FSetterFunctionNames::TextSetterName;
	}
	else if (InType == TEXT("struct"))
	{
		FunctionName = FSetterFunctionNames::StructSetterName;
	}
	else if (InType == TEXT("enum"))
	{
		FunctionName = FSetterFunctionNames::EnumSetterName;
	}
	else
	{
		return nullptr;
	}

	return UTouchBlueprintFunctionLibrary::StaticClass()->FindFunctionByName(FunctionName);
}

UFunction* UTouchBlueprintFunctionLibrary::FindGetterByType(FName InType, bool IsArray)
{
	if (InType.ToString().IsEmpty())
		return nullptr;

	FName FunctionName = "";

	if (InType == TEXT("object"))
	{
		FunctionName = FGetterFunctionNames::ObjectGetterName;
	}
	else if (InType == TEXT("string"))
	{
		if (IsArray)
			FunctionName = FGetterFunctionNames::StringArrayGetterName;
		else
			FunctionName = FGetterFunctionNames::StringGetterName;
	}
	else if (InType == TEXT("float"))
	{
		if (IsArray)
			FunctionName = FGetterFunctionNames::FloatArrayGetterName;
		else
			FunctionName = FGetterFunctionNames::FloatGetterName;
	}
	else
	{
		return nullptr;
	}

	return UTouchBlueprintFunctionLibrary::StaticClass()->FindFunctionByName(FunctionName);
}




bool UTouchBlueprintFunctionLibrary::SetFloatByName(UTouchEngineComponentBase* Target, FName VarName, float value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType == EVarType::VARTYPE_FLOAT)
	{
		return true;
	}
	else if (dynVar->VarType == EVarType::VARTYPE_DOUBLE)
	{
		dynVar->SetValue((double)value);
		return true;
	}

	Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a float property."), *VarName.ToString()));
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float> value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType == EVarType::VARTYPE_FLOAT || dynVar->VarType == EVarType::VARTYPE_FLOATBUFFER || dynVar->VarType == EVarType::VARTYPE_DOUBLE)
	{
		if (dynVar->isArray)
		{
			dynVar->SetValue(value);
			return true;
		}
		else
		{
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not an array property."), *VarName.ToString()));
			return false;
		}
	}

	Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a float array property."), *VarName.ToString()));
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetIntByName(UTouchEngineComponentBase* Target, FName VarName, int value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not an integer property."), *VarName.ToString()));
		return false;
	}

	dynVar->SetValue(value);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetInt64ByName(UTouchEngineComponentBase* Target, FName VarName, int64 value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not an integer property."), *VarName.ToString()));
		return false;
	}

	dynVar->SetValue((int)value);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetBoolByName(UTouchEngineComponentBase* Target, FName VarName, bool value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_BOOL)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a boolean property."), *VarName.ToString()));
		return false;
	}

	dynVar->SetValue(value);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetNameByName(UTouchEngineComponentBase* Target, FName VarName, FName value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a string property."), *VarName.ToString()));
		return false;
	}

	dynVar->SetValue(value.ToString());
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTextureRenderTarget2D* value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_TEXTURE)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a texture property."), *VarName.ToString()));
		return false;
	}

	dynVar->SetValue(value);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetClassByName(UTouchEngineComponentBase* Target, FName VarName, UClass* value)
{
	UE_LOG(LogTemp, Error, TEXT("Unsupported dynamic variable type."));
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetByteByName(UTouchEngineComponentBase* Target, FName VarName, uint8 value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not an integer property."), *VarName.ToString()));
		return false;
	}

	dynVar->SetValue((int)value);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetStringByName(UTouchEngineComponentBase* Target, FName VarName, FString value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a string property."), *VarName.ToString()));
		return false;
	}

	dynVar->SetValue(value);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetStringArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString> value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType == EVarType::VARTYPE_STRING || dynVar->VarType == EVarType::VARTYPE_FLOATBUFFER)
	{
		dynVar->SetValue(value);
		return true;
	}

	Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a string array property."), *VarName.ToString()));
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetTextByName(UTouchEngineComponentBase* Target, FName VarName, FText value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a string property."), *VarName.ToString()));
		return false;
	}

	dynVar->SetValue(value.ToString());
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetStructByName(UTouchEngineComponentBase* Target, FName VarName, UScriptStruct* value)
{
	UE_LOG(LogTemp, Error, TEXT("Unsupported dynamic variable type."));
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetEnumByName(UTouchEngineComponentBase* Target, FName VarName, uint8 value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not an integer property."), *VarName.ToString()));
		return false;
	}

	dynVar->SetValue((int)value);
	return true;
}


bool UTouchBlueprintFunctionLibrary::GetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTexture*& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_TEXTURE)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is not a texture property."), *VarName.ToString()));
		return false;
	}

	if (dynVar->value)
	{
		value = dynVar->GetValueAsTexture();
		return true;
	}

	//Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is null."), *VarName.ToString()));
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetStringArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString>& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING || dynVar->isArray == false)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is not a table property."), *VarName.ToString()));
		return false;
	}

	if (dynVar->value)
	{
		value = dynVar->GetValueAsStringArray();
		return true;
	}

	//Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is null."), *VarName.ToString()));
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float>& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}
	else if (dynVar->isArray == false)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is not an array variable"), *VarName.ToString()));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING && dynVar->VarType != EVarType::VARTYPE_FLOATBUFFER)
	{
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is not a table property."), *VarName.ToString()));
		return false;
	}

	if (dynVar->value)
	{
		value = dynVar->GetValueAsFloatBuffer();
		return true;
	}

	//Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is null."), *VarName.ToString()));
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetStringByName(UTouchEngineComponentBase* Target, FName VarName, FString& value)
{
	TArray<FString> tempValue;
	GetStringArrayByName(Target, VarName, tempValue);

	if (tempValue.IsValidIndex(0))
	{
		value = tempValue[0];
		return true;
	}
	value = FString();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetFloatByName(UTouchEngineComponentBase* Target, FName VarName, float& value)
{
	TArray<float> tempValue;
	GetFloatArrayByName(Target, VarName, tempValue);

	if (tempValue.IsValidIndex(0))
	{
		value = tempValue[0];
		return true;
	}
	value = 0.f;
	return true;
}


FTouchEngineDynamicVariableStruct* UTouchBlueprintFunctionLibrary::TryGetDynamicVariable(UTouchEngineComponentBase* Target, FName VarIdentifier)
{
	// try to find by identifier
	FTouchEngineDynamicVariableStruct* dynVar = Target->dynamicVariables.GetDynamicVariableByIdentifier(VarIdentifier.ToString());

	if (!dynVar)
	{
		// failed to find by identifier, try to find by visible name
		dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarIdentifier.ToString());
	}

	return dynVar;
}
