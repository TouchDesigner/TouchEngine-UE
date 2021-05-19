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

#include "TouchBlueprintFunctionLibrary.h"
#include "TouchEngineComponent.h"
#include "TouchEngineInfo.h"
#include "TouchEngineDynamicVariableStruct.h"

// names of the UFunctions that correspond to the correct setter type
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
	static const FName ColorSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetColorByName));
	static const FName VectorSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetVectorByName));
	static const FName Vector4SetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetVector4ByName));
	static const FName EnumSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetEnumByName));
};

// names of the UFunctions that correspond to the correct getter type
namespace FGetterFunctionNames
{
	static const FName ObjectGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetObjectByName));
	static const FName StringArrayGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringArrayByName));
	static const FName FloatArrayGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatArrayByName));
	static const FName StringGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringByName));
	static const FName FloatGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatByName));
	static const FName FloatBufferGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatBufferByName));
};


UFunction* UTouchBlueprintFunctionLibrary::FindSetterByType(FName InType, bool IsArray, FName structName)
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
		if (structName == TEXT("Color"))
		{
			FunctionName = FSetterFunctionNames::ColorSetterName;
		}
		else if (structName == TEXT("Vector"))
		{
			FunctionName = FSetterFunctionNames::VectorSetterName;
		}
		else if (structName == TEXT("Vector4"))
		{
			FunctionName = FSetterFunctionNames::Vector4SetterName;
		}
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

UFunction* UTouchBlueprintFunctionLibrary::FindGetterByType(FName InType, bool IsArray, FName structName)
{
	if (InType.ToString().IsEmpty())
		return nullptr;

	FName FunctionName = "";

	if (InType == TEXT("object"))
	{
		if (structName == TEXT("TouchEngineCHOP"))
		{
			FunctionName = FGetterFunctionNames::FloatBufferGetterName;
		}
		else
		{
			FunctionName = FGetterFunctionNames::ObjectGetterName;
		}
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
		if (Target->EngineInfo)
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
		if (Target->EngineInfo)
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
			if (Target->EngineInfo)
				Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not an array property."), *VarName.ToString()));
			return false;
		}
	}

	if (Target->EngineInfo)
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a float array property."), *VarName.ToString()));
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetIntByName(UTouchEngineComponentBase* Target, FName VarName, int value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		if (Target->EngineInfo)
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
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		if (Target->EngineInfo)
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
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_BOOL)
	{
		if (Target->EngineInfo)
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
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a string property."), *VarName.ToString()));
		return false;
	}

	dynVar->SetValue(value.ToString());
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTexture* value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_TEXTURE)
	{
		if (Target->EngineInfo)
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
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		if (Target->EngineInfo)
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
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
	{
		if (Target->EngineInfo)
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
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType == EVarType::VARTYPE_STRING || dynVar->VarType == EVarType::VARTYPE_FLOATBUFFER)
	{
		dynVar->SetValue(value);
		return true;
	}

	if (Target->EngineInfo)
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a string array property."), *VarName.ToString()));
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetTextByName(UTouchEngineComponentBase* Target, FName VarName, FText value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a string property."), *VarName.ToString()));
		return false;
	}

	dynVar->SetValue(value.ToString());
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetColorByName(UTouchEngineComponentBase* Target, FName VarName, FColor value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_DOUBLE)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a double property."), *VarName.ToString()));
		return false;
	}

	if (dynVar->VarIntent != EVarIntent::VARINTENT_COLOR)
	{
		// intent is not color, should log warning but not stop setting since you can set a vector of size 4 with a color

	}
	TArray<double> buffer;
	buffer.Add((double)value.R);
	buffer.Add((double)value.G);
	buffer.Add((double)value.B);
	buffer.Add((double)value.A);
	dynVar->SetValue(buffer);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetVectorByName(UTouchEngineComponentBase* Target, FName VarName, FVector value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_DOUBLE)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a double property."), *VarName.ToString()));
		return false;
	}

	if (dynVar->VarIntent != EVarIntent::VARINTENT_UVW)
	{
		// intent is not uvw, maybe should not log warning

	}

	TArray<double> buffer;
	buffer.Add(value.X);
	buffer.Add(value.Y);
	buffer.Add(value.Z);
	dynVar->SetValue(buffer);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetVector4ByName(UTouchEngineComponentBase* Target, FName VarName, FVector4 value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_DOUBLE)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not a double property."), *VarName.ToString()));
		return false;
	}

	if (dynVar->VarIntent != EVarIntent::VARINTENT_POSITION)
	{
		// intent is not position, maybe should not log warning

	}

	TArray<double> buffer;
	buffer.Add(value.X);
	buffer.Add(value.Y);
	buffer.Add(value.Z);
	buffer.Add(value.W);
	dynVar->SetValue(buffer);
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetEnumByName(UTouchEngineComponentBase* Target, FName VarName, uint8 value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		if (Target->EngineInfo)
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
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_TEXTURE)
	{
		if (Target->EngineInfo)
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
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING || dynVar->isArray == false)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is not a table property."), *VarName.ToString()));
		return false;
	}

	if (dynVar->value)
	{
		value = dynVar->GetValueAsStringArray();
		return true;
	}

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float>& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}
	else if (dynVar->isArray == false)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is not an array variable"), *VarName.ToString()));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_DOUBLE)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is not a table property."), *VarName.ToString()));
		return false;
	}

	if (dynVar->value)
	{
		auto doubleArray = dynVar->GetValueAsDoubleTArray();

		if (doubleArray.Num() != 0)
		{
			for (int i = 0; i < doubleArray.Num(); i++)
			{
				value.Add(doubleArray[i]);
			}
		}

		return true;
	}

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
	TArray<float> tempValue = TArray<float>();
	if (GetFloatArrayByName(Target, VarName, tempValue))
	{
		if (tempValue.IsValidIndex(0))
		{
			value = tempValue[0];
			return true;
		}
		value = 0.f;
		return true;
	}
	value = 0.f;
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetFloatBufferByName(UTouchEngineComponentBase* Target, FName VarName, UTouchEngineCHOP*& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s not found in file %s"), *VarName.ToString(), *Target->ToxFilePath));
		return false;
	}
	else if (dynVar->isArray == false)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is not an array variable"), *VarName.ToString()));
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_FLOATBUFFER)
	{
		if (Target->EngineInfo)
			Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is not a table property."), *VarName.ToString()));
		return false;
	}

	if (dynVar->value)
	{
		if (auto floatBuffer = dynVar->GetValueAsFloatBuffer())
		{
			value = floatBuffer;
			return true;
		}
	}

	return true;
}


FTEDynamicVariableStruct* UTouchBlueprintFunctionLibrary::TryGetDynamicVariable(UTouchEngineComponentBase* Target, FName VarName)
{
	// try to find by name
	FTEDynamicVariableStruct* dynVar = Target->dynamicVariables.GetDynamicVariableByIdentifier(VarName.ToString());

	if (!dynVar)
	{
		// failed to find by name, try to find by visible name
		dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());
	}

	return dynVar;
}
