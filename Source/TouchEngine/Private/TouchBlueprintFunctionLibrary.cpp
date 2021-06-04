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
	static const FName IntArraySetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetIntArrayByName));
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

namespace FInputGetterFunctionNames
{
	static const FName FloatInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatInputByName));
	static const FName FloatArrayInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatArrayInputByName));
	static const FName IntInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetIntInputByName));
	static const FName Int64InputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetInt64InputByName));
	static const FName IntArrayInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetIntArrayInputByName));
	static const FName BoolInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetBoolInputByName));
	static const FName NameInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetNameInputByName));
	static const FName ObjectInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetObjectInputByName));
	static const FName ClassInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetClassInputByName));
	static const FName ByteInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetByteInputByName));
	static const FName StringInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringInputByName));
	static const FName StringArrayInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringArrayInputByName));
	static const FName TextInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetTextInputByName));
	static const FName ColorInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetColorInputByName));
	static const FName VectorInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetVectorInputByName));
	static const FName Vector4InputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetVector4InputByName));
	static const FName EnumInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetEnumInputByName));
}


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
		if (!IsArray)
		{
			FunctionName = FSetterFunctionNames::IntSetterName;
		}
		else
		{
			FunctionName = FSetterFunctionNames::IntArraySetterName;
		}
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
		else if (structName == TEXT("TouchEngineDAT"))
		{

			FunctionName = FGetterFunctionNames::StringArrayGetterName;
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

UFunction* UTouchBlueprintFunctionLibrary::FindInputGetterByType(FName InType, bool IsArray, FName structName)
{
	if (InType.ToString().IsEmpty())
		return nullptr;

	FName FunctionName = "";

	if (InType == TEXT("float"))
	{
		if (!IsArray)
		{
			FunctionName = FInputGetterFunctionNames::FloatInputGetterName;
		}
		else
		{
			FunctionName = FInputGetterFunctionNames::FloatArrayInputGetterName;
		}
	}
	else if (InType == TEXT("int"))
	{
		if (!IsArray)
		{
			FunctionName = FInputGetterFunctionNames::IntInputGetterName;
		}
		else
		{
			FunctionName = FInputGetterFunctionNames::IntArrayInputGetterName;
		}
	}
	else if (InType == TEXT("int64"))
	{
		FunctionName = FInputGetterFunctionNames::Int64InputGetterName;
	}
	else if (InType == TEXT("bool"))
	{
		FunctionName = FInputGetterFunctionNames::BoolInputGetterName;
	}
	else if (InType == TEXT("name"))
	{
		FunctionName = FInputGetterFunctionNames::NameInputGetterName;
	}
	else if (InType == TEXT("object"))
	{
		FunctionName = FInputGetterFunctionNames::ObjectInputGetterName;
	}
	else if (InType == TEXT("class"))
	{
		FunctionName = FInputGetterFunctionNames::ClassInputGetterName;
	}
	else if (InType == TEXT("byte"))
	{
		FunctionName = FInputGetterFunctionNames::ByteInputGetterName;
	}
	else if (InType == TEXT("string"))
	{
		if (!IsArray)
		{
			FunctionName = FInputGetterFunctionNames::StringInputGetterName;
		}
		else
		{
			FunctionName = FInputGetterFunctionNames::StringArrayInputGetterName;
		}
	}
	else if (InType == TEXT("text"))
	{
		FunctionName = FInputGetterFunctionNames::TextInputGetterName;
	}
	else if (InType == TEXT("struct"))
	{
		if (structName == TEXT("Color"))
		{
			FunctionName = FInputGetterFunctionNames::ColorInputGetterName;
		}
		else if (structName == TEXT("Vector"))
		{
			FunctionName = FInputGetterFunctionNames::VectorInputGetterName;
		}
		else if (structName == TEXT("Vector4"))
		{
			FunctionName = FInputGetterFunctionNames::Vector4InputGetterName;
		}
	}
	else if (InType == TEXT("enum"))
	{
		FunctionName = FInputGetterFunctionNames::EnumInputGetterName;
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
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);

		return false;
	}

	if (dynVar->VarType == EVarType::VARTYPE_FLOAT)
	{
		dynVar->SetValue(value);
		if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
		{
			dynVar->SendInput(Target->EngineInfo);
		}
		return true;
	}
	else if (dynVar->VarType == EVarType::VARTYPE_DOUBLE)
	{
		dynVar->SetValue((double)value);
		if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
		{
			dynVar->SendInput(Target->EngineInfo);
		}
		return true;
	}
	LogTouchEngineError(Target->EngineInfo, "Input is not a float property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float> value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::VARTYPE_FLOAT || dynVar->VarType == EVarType::VARTYPE_DOUBLE)
	{
		if (dynVar->isArray)
		{
			dynVar->SetValue(value);
			if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
			{
				dynVar->SendInput(Target->EngineInfo);
			}
			return true;
		}
		else
		{
			LogTouchEngineError(Target->EngineInfo, "Input is not an array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
			return false;
		}
	}
	else if (dynVar->VarType == EVarType::VARTYPE_FLOATBUFFER)
	{
		UTouchEngineCHOP* buffer = NewObject<UTouchEngineCHOP>();
		float* valueData = value.GetData();
		buffer->CreateChannels(&valueData, 1, value.Num());

		dynVar->SetValue(buffer);
		if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
		{
			dynVar->SendInput(Target->EngineInfo);
		}
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not a float array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetIntByName(UTouchEngineComponentBase* Target, FName VarName, int value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue(value);
	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetInt64ByName(UTouchEngineComponentBase* Target, FName VarName, int64 value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue((int)value);
	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetIntArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<int> value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::VARTYPE_INT)
	{
		if (dynVar->isArray)
		{
			dynVar->SetValue(value);
			if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
			{
				dynVar->SendInput(Target->EngineInfo);
			}
			return true;
		}
		else
		{
			LogTouchEngineError(Target->EngineInfo, "Input is not an array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
			return false;
		}
	}

	if (Target->EngineInfo)
		Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Input %s is not an integer array property in file %s."), *VarName.ToString(), *Target->ToxFilePath));
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetBoolByName(UTouchEngineComponentBase* Target, FName VarName, bool value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_BOOL)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a boolean property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue(value);
	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetNameByName(UTouchEngineComponentBase* Target, FName VarName, FName value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue(value.ToString());
	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTexture* value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_TEXTURE)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a texture property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue(value);
	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
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
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue((int)value);
	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetStringByName(UTouchEngineComponentBase* Target, FName VarName, FString value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue(value);
	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetStringArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString> value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::VARTYPE_STRING || dynVar->VarType == EVarType::VARTYPE_FLOATBUFFER)
	{
		UTouchEngineDAT* tempValue = NewObject<UTouchEngineDAT>();
		tempValue->CreateChannels(value, 1, value.Num());

		dynVar->SetValue(tempValue);


		if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
		{
			dynVar->SendInput(Target->EngineInfo);
		}
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not a string array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);

	return false;
}

bool UTouchBlueprintFunctionLibrary::SetTextByName(UTouchEngineComponentBase* Target, FName VarName, FText value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue(value.ToString());
	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetColorByName(UTouchEngineComponentBase* Target, FName VarName, FColor value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_DOUBLE)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a color property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
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
	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetVectorByName(UTouchEngineComponentBase* Target, FName VarName, FVector value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_DOUBLE)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a double property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
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
	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetVector4ByName(UTouchEngineComponentBase* Target, FName VarName, FVector4 value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_DOUBLE)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector 4 property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	TArray<double> buffer;
	buffer.Add(value.X);
	buffer.Add(value.Y);
	buffer.Add(value.Z);
	buffer.Add(value.W);
	dynVar->SetValue(buffer);
	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetEnumByName(UTouchEngineComponentBase* Target, FName VarName, uint8 value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue((int)value);
	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}


bool UTouchBlueprintFunctionLibrary::GetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTexture*& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_TEXTURE)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a texture property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->GetOutput(Target->EngineInfo);
	}

	if (dynVar->value)
	{
		value = dynVar->GetValueAsTexture();
		return true;
	}

	//Target->EngineInfo->logTouchEngineError(FString::Printf(TEXT("Output %s is null."), *VarName.ToString()));
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetStringArrayByName(UTouchEngineComponentBase* Target, FName VarName, UTouchEngineDAT*& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING || dynVar->isArray == false)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a DAT property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->value)
	{
		if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
		{
			dynVar->GetOutput(Target->EngineInfo);
		}
		
		value = dynVar->GetValueAsDAT();
		
		if (value)
		{
			value = NewObject<UTouchEngineDAT>();
		}
		return true;
	}

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float>& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}
	else if (dynVar->isArray == false)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a CHOP property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_DOUBLE)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a CHOP property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->GetOutput(Target->EngineInfo);
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
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING || dynVar->isArray == true)
	{

		LogTouchEngineError(Target->EngineInfo, "Output is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->GetOutput(Target->EngineInfo);
	}

	if (dynVar->value)
	{
		value = dynVar->GetValueAsString();
		return true;
	}

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
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}
	else if (dynVar->isArray == false)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a CHOP property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_FLOATBUFFER)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a CHOP property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (Target->sendMode == ETouchEngineSendMode::SENDMODE_ONACCESS)
	{
		dynVar->GetOutput(Target->EngineInfo);
	}

	if (dynVar->value)
	{
		if (auto floatBuffer = dynVar->GetValueAsCHOP())
		{
			value = floatBuffer;
			return true;
		}
	}

	return true;
}


bool UTouchBlueprintFunctionLibrary::GetFloatInputByName(UTouchEngineComponentBase* Target, FName VarName, float& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::VARTYPE_FLOAT)
	{
		value = dynVar->GetValueAsFloat();
		return true;
	}
	else if (dynVar->VarType == EVarType::VARTYPE_DOUBLE)
	{
		value = dynVar->GetValueAsDouble();
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Output is not a float property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetFloatArrayInputByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float>& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::VARTYPE_FLOAT || dynVar->VarType == EVarType::VARTYPE_DOUBLE)
	{
		if (dynVar->isArray)
		{
			TArray<float> bufferFloatArray;
			TArray<double> bufferDoubleArray = dynVar->GetValueAsDoubleTArray();

			for (int i = 0; i < bufferDoubleArray.Num(); i++)
			{
				bufferFloatArray.Add(bufferDoubleArray[i]);
			}

			value = bufferFloatArray;
			return true;
		}
		else
		{
			LogTouchEngineError(Target->EngineInfo, "Input is not an array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
			return false;
		}
	}
	else if (dynVar->VarType == EVarType::VARTYPE_FLOATBUFFER)
	{
		UTouchEngineCHOP* buffer = dynVar->GetValueAsCHOP();
		value = buffer->GetChannel(0);
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not a float array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetIntInputByName(UTouchEngineComponentBase* Target, FName VarName, int& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	value = dynVar->GetValueAsInt();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetInt64InputByName(UTouchEngineComponentBase* Target, FName VarName, int64& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	value = (int64)(dynVar->GetValueAsInt());
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetIntArrayInputByName(UTouchEngineComponentBase* Target, FName VarName, TArray<int>& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::VARTYPE_INT)
	{
		if (dynVar->isArray)
		{
			value = dynVar->GetValueAsIntTArray();
			return true;
		}
		else
		{
			LogTouchEngineError(Target->EngineInfo, "Input is not an array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
			return false;
		}
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not an integer array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetBoolInputByName(UTouchEngineComponentBase* Target, FName VarName, bool& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_BOOL)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a boolean property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	value = dynVar->GetValueAsBool();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetNameInputByName(UTouchEngineComponentBase* Target, FName VarName, FName& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	value = FName(dynVar->GetValueAsString());
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetObjectInputByName(UTouchEngineComponentBase* Target, FName VarName, UTexture*& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_TEXTURE)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a texture property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	value = dynVar->GetValueAsTexture();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetClassInputByName(UTouchEngineComponentBase* Target, FName VarName, class UClass*& value)
{
	UE_LOG(LogTemp, Error, TEXT("Unsupported dynamic variable type."));
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetByteInputByName(UTouchEngineComponentBase* Target, FName VarName, uint8& value)
{

	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	value = (uint8)(dynVar->GetValueAsInt());
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetStringInputByName(UTouchEngineComponentBase* Target, FName VarName, FString& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	value = dynVar->GetValueAsString();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetStringArrayInputByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString>& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::VARTYPE_STRING || dynVar->VarType == EVarType::VARTYPE_FLOATBUFFER)
	{
		value = dynVar->GetValueAsStringArray();
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not a string array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetTextInputByName(UTouchEngineComponentBase* Target, FName VarName, FText& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_STRING)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	value = FText::FromString(dynVar->GetValueAsString());
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetColorInputByName(UTouchEngineComponentBase* Target, FName VarName, FColor& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_DOUBLE)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a color property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarIntent != EVarIntent::VARINTENT_COLOR)
	{
		// intent is not color, should log warning but not stop setting since you can set a vector of size 4 with a color

	}
	TArray<double> buffer = dynVar->GetValueAsDoubleTArray();

	if (buffer.Num() != 4)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a color property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	value.R = buffer[0];
	value.G = buffer[1];
	value.B = buffer[2];
	value.A = buffer[3];

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetVectorInputByName(UTouchEngineComponentBase* Target, FName VarName, FVector& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_DOUBLE)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarIntent != EVarIntent::VARINTENT_UVW)
	{
		// intent is not uvw, maybe should not log warning

	}

	TArray<double> buffer = dynVar->GetValueAsDoubleTArray();

	if (buffer.Num() != 3)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	value.X = buffer[0];
	value.Y = buffer[1];
	value.Z = buffer[2];

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetVector4InputByName(UTouchEngineComponentBase* Target, FName VarName, FVector4& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_DOUBLE)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector4 property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarIntent != EVarIntent::VARINTENT_UVW)
	{
		// intent is not uvw, maybe should not log warning

	}

	TArray<double> buffer = dynVar->GetValueAsDoubleTArray();

	if (buffer.Num() != 4)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector4 property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	value.X = buffer[0];
	value.Y = buffer[1];
	value.Z = buffer[2];
	value.W = buffer[3];

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetEnumInputByName(UTouchEngineComponentBase* Target, FName VarName, uint8& value)
{
	auto dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::VARTYPE_INT)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	value = (uint8)dynVar->GetValueAsInt();
	return true;
}



FTouchEngineDynamicVariableStruct* UTouchBlueprintFunctionLibrary::TryGetDynamicVariable(UTouchEngineComponentBase* Target, FName VarName)
{
	// try to find by name
	FTouchEngineDynamicVariableStruct* dynVar = Target->dynamicVariables.GetDynamicVariableByIdentifier(VarName.ToString());

	if (!dynVar)
	{
		// failed to find by name, try to find by visible name
		dynVar = Target->dynamicVariables.GetDynamicVariableByName(VarName.ToString());
	}

	return dynVar;
}

void UTouchBlueprintFunctionLibrary::LogTouchEngineError(UTouchEngineInfo* info, FString error, FString ownerName, FString inputName, FString fileName)
{
	if (info)
	{
		info->logTouchEngineError(FString::Printf(TEXT("Blueprint %s: File %s: Param %s: %s"), *ownerName, *fileName, *inputName, *error));
	}
}
