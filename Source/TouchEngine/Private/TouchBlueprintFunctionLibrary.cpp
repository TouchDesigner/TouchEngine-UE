// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchBlueprintFunctionLibrary.h"

namespace FSetterFunctionNames
{
	static const FName FloatSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetFloatByName));
	static const FName IntSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetIntByName));
	static const FName Int64SetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetInt64ByName));
	static const FName BoolSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetBoolByName));
	static const FName NameSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetNameByName));
	static const FName ObjectSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetObjectByName));
	static const FName ClassSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetClassByName));
	static const FName ByteSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetByteByName));
	static const FName StringSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetStringByName));
	static const FName TextSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetTextByName));
	static const FName StructSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetStructByName));
	static const FName EnumSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetEnumByName));
};

namespace FGetterFunctionNames
{
	static const FName ObjectGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetObjectByName));
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
		FunctionName = FSetterFunctionNames::StringSetterName;
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

UFunction* UTouchBlueprintFunctionLibrary::FindGetterByType(FName InType)
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
		FunctionName = FGetterFunctionNames::StringGetterName;
	}
	else if (InType == TEXT("float"))
	{
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
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType == EVarType::VARTYPE_FLOAT)
	{
		dynVar->SetValue(value);
		return true;
	}
	else if (dynVar->VarType != EVarType::VARTYPE_DOUBLE)
	{
		dynVar->SetValue((double)value);
		return true;
	}

	return false;
}

bool UTouchBlueprintFunctionLibrary::SetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float> value)
{
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_FLOAT && dynVar->VarType != EVarType::VARTYPE_FLOATBUFFER)
		return false;

	dynVar->SetValue(value);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetIntByName(UTouchEngineComponentBase* Target, FName VarName, int value)
{
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_INT)
		return false;

	dynVar->SetValue(value);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetInt64ByName(UTouchEngineComponentBase* Target, FName VarName, int64 value)
{
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_INT)
		return false;

	dynVar->SetValue((int)value);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetBoolByName(UTouchEngineComponentBase* Target, FName VarName, bool value)
{
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_BOOL)
		return false;

	dynVar->SetValue(value);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetNameByName(UTouchEngineComponentBase* Target, FName VarName, FName value)
{
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
		return false;

	dynVar->SetValue(value.ToString());
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTextureRenderTarget2D* value)
{
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_TEXTURE)
		return false;

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
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_INT)
		return false;

	dynVar->SetValue((int)value);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetStringByName(UTouchEngineComponentBase* Target, FName VarName, FString value)
{
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
		return false;

	dynVar->SetValue(value);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetStringArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString> value)
{
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetTextByName(UTouchEngineComponentBase* Target, FName VarName, FText value)
{
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
		return false;

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
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_INT)
		return false;

	dynVar->SetValue((int)value);
	return true;
}


bool UTouchBlueprintFunctionLibrary::GetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTexture*& value)
{
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_TEXTURE)
		return false;

	value = dynVar->GetValueAsTexture();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetStringByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString>& value)
{
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
		return false;

	value = dynVar->GetValueAsStringArray();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetFloatByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float>& value)
{
	auto dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());

	if (!dynVar)
		return false;

	if (dynVar->VarType != EVarType::VARTYPE_STRING && dynVar->VarType != EVarType::VARTYPE_FLOATBUFFER)
		return false;

	value = dynVar->GetValueAsFloatBuffer();
	return true;
}
