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
#include "GameFramework/Actor.h"


// pin names copied over from EdGraphSchema_K2.h
namespace FTouchEngineType
{
	const FName PC_Exec(TEXT("exec"));
	const FName PC_Boolean(TEXT("bool"));
	const FName PC_Byte(TEXT("byte"));
	const FName PC_Class(TEXT("class"));
	const FName PC_Int(TEXT("int"));
	const FName PC_Int64(TEXT("int64"));
	const FName PC_Float(TEXT("float"));
	const FName PC_Name(TEXT("name"));
	const FName PC_Delegate(TEXT("delegate"));
	const FName PC_MCDelegate(TEXT("mcdelegate"));
	const FName PC_Object(TEXT("object"));
	const FName PC_Interface(TEXT("interface"));
	const FName PC_String(TEXT("string"));
	const FName PC_Text(TEXT("text"));
	const FName PC_Struct(TEXT("struct"));
	const FName PC_Wildcard(TEXT("wildcard"));
	const FName PC_FieldPath(TEXT("fieldpath"));
	const FName PC_Enum(TEXT("enum"));
	const FName PC_SoftObject(TEXT("softobject"));
	const FName PC_SoftClass(TEXT("softclass"));
	const FName PSC_Self(TEXT("self"));
	const FName PSC_Index(TEXT("index"));
	const FName PSC_Bitmask(TEXT("bitmask"));
	const FName PN_Execute(TEXT("execute"));
	const FName PN_Then(TEXT("then"));
	const FName PN_Completed(TEXT("Completed"));
	const FName PN_DelegateEntry(TEXT("delegate"));
	const FName PN_EntryPoint(TEXT("EntryPoint"));
	const FName PN_Self(TEXT("self"));
	const FName PN_Else(TEXT("else"));
	const FName PN_Loop(TEXT("loop"));
	const FName PN_After(TEXT("after"));
	const FName PN_ReturnValue(TEXT("ReturnValue"));
	const FName PN_ObjectToCast(TEXT("Object"));
	const FName PN_Condition(TEXT("Condition"));
	const FName PN_Start(TEXT("Start"));
	const FName PN_Stop(TEXT("Stop"));
	const FName PN_Index(TEXT("Index"));
	const FName PN_Item(TEXT("Item"));
	const FName PN_CastSucceeded(TEXT("then"));
	const FName PN_CastFailed(TEXT("CastFailed"));
	const FString PN_CastedValuePrefix(TEXT("As"));
	const FName PN_MatineeFinished(TEXT("Finished"));
}


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
	static const FName Texture2DGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetTexture2DByName));
	static const FName StringArrayGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringArrayByName));
	static const FName FloatArrayGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatArrayByName));
	static const FName StringGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringByName));
	static const FName FloatGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatByName));
	static const FName FloatBufferGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatBufferByName));
};

namespace FInputGetterFunctionNames
{
	static const FName FloatInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatInputLatestByName));
	static const FName FloatArrayInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatArrayInputLatestByName));
	static const FName IntInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetIntInputLatestByName));
	static const FName Int64InputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetInt64InputLatestByName));
	static const FName IntArrayInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetIntArrayInputLatestByName));
	static const FName BoolInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetBoolInputLatestByName));
	static const FName NameInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetNameInputLatestByName));
	static const FName ObjectInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetObjectInputLatestByName));
	static const FName Texture2DInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetTexture2DInputLatestByName));
	static const FName ClassInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetClassInputLatestByName));
	static const FName ByteInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetByteInputLatestByName));
	static const FName StringInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringInputLatestByName));
	static const FName StringArrayInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringArrayInputLatestByName));
	static const FName TextInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetTextInputLatestByName));
	static const FName ColorInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetColorInputLatestByName));
	static const FName VectorInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetVectorInputLatestByName));
	static const FName Vector4InputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetVector4InputLatestByName));
	static const FName EnumInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetEnumInputLatestByName));
}


UFunction* UTouchBlueprintFunctionLibrary::FindSetterByType(FName InType, bool IsArray, FName StructName)
{
	if (InType.ToString().IsEmpty())
		return nullptr;

	FName FunctionName = "";

	if (InType == FTouchEngineType::PC_Float)
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
	else if (InType == FTouchEngineType::PC_Int)
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
	else if (InType == FTouchEngineType::PC_Int64)
	{
		FunctionName = FSetterFunctionNames::Int64SetterName;
	}
	else if (InType == FTouchEngineType::PC_Boolean)
	{
		FunctionName = FSetterFunctionNames::BoolSetterName;
	}
	else if (InType == FTouchEngineType::PC_Name)
	{
		FunctionName = FSetterFunctionNames::NameSetterName;
	}
	else if (InType == FTouchEngineType::PC_Object)
	{
		FunctionName = FSetterFunctionNames::ObjectSetterName;
	}
	else if (InType == FTouchEngineType::PC_Class)
	{
		FunctionName = FSetterFunctionNames::ClassSetterName;
	}
	else if (InType == FTouchEngineType::PC_Byte)
	{
		FunctionName = FSetterFunctionNames::ByteSetterName;
	}
	else if (InType == FTouchEngineType::PC_String)
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
	else if (InType == FTouchEngineType::PC_Text)
	{
		FunctionName = FSetterFunctionNames::TextSetterName;
	}
	else if (InType == FTouchEngineType::PC_Struct)
	{
		if (StructName == TBaseStructure<FColor>::Get()->GetFName())
		{
			FunctionName = FSetterFunctionNames::ColorSetterName;
		}
		else if (StructName == TBaseStructure<FVector>::Get()->GetFName())
		{
			FunctionName = FSetterFunctionNames::VectorSetterName;
		}
		else if (StructName == TBaseStructure<FVector4>::Get()->GetFName())
		{
			FunctionName = FSetterFunctionNames::Vector4SetterName;
		}
	}
	else if (InType == FTouchEngineType::PC_Enum)
	{
		FunctionName = FSetterFunctionNames::EnumSetterName;
	}
	else
	{
		return nullptr;
	}

	return UTouchBlueprintFunctionLibrary::StaticClass()->FindFunctionByName(FunctionName);
}

UFunction* UTouchBlueprintFunctionLibrary::FindGetterByType(FName InType, bool IsArray, FName StructName)
{
	if (InType.ToString().IsEmpty())
		return nullptr;

	FName FunctionName = "";

	if (InType == FTouchEngineType::PC_Object)
	{
		if (StructName == UTouchEngineCHOP::StaticClass()->GetFName())
		{
			FunctionName = FGetterFunctionNames::FloatBufferGetterName;
		}
		else if (StructName == UTouchEngineDAT::StaticClass()->GetFName())
		{
			FunctionName = FGetterFunctionNames::StringArrayGetterName;
		}
		else if (StructName == UTexture2D::StaticClass()->GetFName())
		{
			FunctionName = FGetterFunctionNames::Texture2DGetterName;
		}
		else
		{
			FunctionName = FGetterFunctionNames::ObjectGetterName;
		}
	}
	else if (InType == FTouchEngineType::PC_String)
	{
		if (IsArray)
			FunctionName = FGetterFunctionNames::StringArrayGetterName;
		else
			FunctionName = FGetterFunctionNames::StringGetterName;
	}
	else if (InType == FTouchEngineType::PC_Float)
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

UFunction* UTouchBlueprintFunctionLibrary::FindInputGetterByType(FName InType, bool IsArray, FName StructName)
{
	if (InType.ToString().IsEmpty())
		return nullptr;

	FName FunctionName = "";

	if (InType == FTouchEngineType::PC_Float)
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
	else if (InType == FTouchEngineType::PC_Int)
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
	else if (InType == FTouchEngineType::PC_Int64)
	{
		FunctionName = FInputGetterFunctionNames::Int64InputGetterName;
	}
	else if (InType == FTouchEngineType::PC_Boolean)
	{
		FunctionName = FInputGetterFunctionNames::BoolInputGetterName;
	}
	else if (InType == FTouchEngineType::PC_Name)
	{
		FunctionName = FInputGetterFunctionNames::NameInputGetterName;
	}
	else if (InType == FTouchEngineType::PC_Object)
	{
		if (StructName == UTexture2D::StaticClass()->GetFName())
		{
			FunctionName = FInputGetterFunctionNames::Texture2DInputGetterName;
		}
		else
		{
			FunctionName = FInputGetterFunctionNames::ObjectInputGetterName;
		}
	}
	else if (InType == FTouchEngineType::PC_Class)
	{
		FunctionName = FInputGetterFunctionNames::ClassInputGetterName;
	}
	else if (InType == FTouchEngineType::PC_Byte)
	{
		FunctionName = FInputGetterFunctionNames::ByteInputGetterName;
	}
	else if (InType == FTouchEngineType::PC_String)
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
	else if (InType == FTouchEngineType::PC_Text)
	{
		FunctionName = FInputGetterFunctionNames::TextInputGetterName;
	}
	else if (InType == FTouchEngineType::PC_Struct)
	{
		if (StructName == TBaseStructure<FColor>::Get()->GetFName())
		{
			FunctionName = FInputGetterFunctionNames::ColorInputGetterName;
		}
		else if (StructName == TBaseStructure<FVector>::Get()->GetFName())
		{
			FunctionName = FInputGetterFunctionNames::VectorInputGetterName;
		}
		else if (StructName == TBaseStructure<FVector4>::Get()->GetFName())
		{
			FunctionName = FInputGetterFunctionNames::Vector4InputGetterName;
		}
	}
	else if (InType == FTouchEngineType::PC_Enum)
	{
		FunctionName = FInputGetterFunctionNames::EnumInputGetterName;
	}
	else
	{
		return nullptr;
	}

	return UTouchBlueprintFunctionLibrary::StaticClass()->FindFunctionByName(FunctionName);
}


bool UTouchBlueprintFunctionLibrary::SetFloatByName(UTouchEngineComponentBase* Target, FName VarName, float Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);

		return false;
	}

	if (dynVar->VarType == EVarType::Float)
	{
		dynVar->SetValue(Value);
		if (Target->SendMode == ETouchEngineSendMode::OnAccess)
		{
			dynVar->SendInput(Target->EngineInfo);
		}
		return true;
	}
	else if (dynVar->VarType == EVarType::Double)
	{
		dynVar->SetValue((double)Value);
		if (Target->SendMode == ETouchEngineSendMode::OnAccess)
		{
			dynVar->SendInput(Target->EngineInfo);
		}
		return true;
	}
	LogTouchEngineError(Target->EngineInfo, "Input is not a float property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float> Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::Float || dynVar->VarType == EVarType::Double)
	{
		if (dynVar->IsArray)
		{
			dynVar->SetValue(Value);
			if (Target->SendMode == ETouchEngineSendMode::OnAccess)
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
	else if (dynVar->VarType == EVarType::CHOP)
	{
		UTouchEngineCHOP* buffer = NewObject<UTouchEngineCHOP>();
		float* valueData = Value.GetData();
		buffer->CreateChannels(&valueData, 1, Value.Num());

		dynVar->SetValue(buffer);
		if (Target->SendMode == ETouchEngineSendMode::OnAccess)
		{
			dynVar->SendInput(Target->EngineInfo);
		}
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not a float array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetIntByName(UTouchEngineComponentBase* Target, FName VarName, int Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue(Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetInt64ByName(UTouchEngineComponentBase* Target, FName VarName, int64 Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue((int)Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetIntArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<int> Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::Int)
	{
		if (dynVar->IsArray)
		{
			dynVar->SetValue(Value);
			if (Target->SendMode == ETouchEngineSendMode::OnAccess)
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
		Target->EngineInfo->LogTouchEngineError(FString::Printf(TEXT("Input %s is not an integer array property in file %s."), *VarName.ToString(), *Target->ToxFilePath));
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetBoolByName(UTouchEngineComponentBase* Target, FName VarName, bool Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Bool)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a boolean property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue(Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetNameByName(UTouchEngineComponentBase* Target, FName VarName, FName Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::String)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue(Value.ToString());
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTexture* Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Texture)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a texture property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue(Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetClassByName(UTouchEngineComponentBase* Target, FName VarName, UClass* Value)
{
	LogTouchEngineError(Target->EngineInfo, "Unsupported dynamic variable type.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetByteByName(UTouchEngineComponentBase* Target, FName VarName, uint8 Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue((int)Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetStringByName(UTouchEngineComponentBase* Target, FName VarName, FString Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::String)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue(Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetStringArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString> Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::String || dynVar->VarType == EVarType::CHOP)
	{
		UTouchEngineDAT* tempValue = NewObject<UTouchEngineDAT>();
		tempValue->CreateChannels(Value, 1, Value.Num());

		dynVar->SetValue(tempValue);


		if (Target->SendMode == ETouchEngineSendMode::OnAccess)
		{
			dynVar->SendInput(Target->EngineInfo);
		}
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not a string array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);

	return false;
}

bool UTouchBlueprintFunctionLibrary::SetTextByName(UTouchEngineComponentBase* Target, FName VarName, FText Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::String)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue(Value.ToString());
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetColorByName(UTouchEngineComponentBase* Target, FName VarName, FColor Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a color property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarIntent != EVarIntent::Color)
	{
		// intent is not color, should log warning but not stop setting since you can set a vector of size 4 with a color

	}
	TArray<double> buffer;
	buffer.Add((double)Value.R);
	buffer.Add((double)Value.G);
	buffer.Add((double)Value.B);
	buffer.Add((double)Value.A);
	dynVar->SetValue(buffer);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetVectorByName(UTouchEngineComponentBase* Target, FName VarName, FVector Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a double property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarIntent != EVarIntent::UVW)
	{
		// intent is not uvw, maybe should not log warning

	}

	TArray<double> buffer;
	buffer.Add(Value.X);
	buffer.Add(Value.Y);
	buffer.Add(Value.Z);
	dynVar->SetValue(buffer);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetVector4ByName(UTouchEngineComponentBase* Target, FName VarName, FVector4 value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Double)
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
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetEnumByName(UTouchEngineComponentBase* Target, FName VarName, uint8 Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	dynVar->SetValue((int)Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->SendInput(Target->EngineInfo);
	}
	return true;
}


bool UTouchBlueprintFunctionLibrary::GetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTexture*& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Texture)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a texture property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->GetOutput(Target->EngineInfo);
	}

	if (dynVar->Value)
	{
		Value = dynVar->GetValueAsTexture();
		return true;
	}

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetTexture2DByName(UTouchEngineComponentBase* Target, FName VarName, UTexture2D*& Value)
{
	UTexture* texVal;
	bool retVal = GetObjectByName(Target, VarName, texVal);

	if (texVal)
	{
		Value = Cast<UTexture2D>(texVal);
	}

	return retVal;
}

bool UTouchBlueprintFunctionLibrary::GetStringArrayByName(UTouchEngineComponentBase* Target, FName VarName, UTouchEngineDAT*& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::String || dynVar->IsArray == false)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a DAT property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->Value)
	{
		if (Target->SendMode == ETouchEngineSendMode::OnAccess)
		{
			dynVar->GetOutput(Target->EngineInfo);
		}

		Value = dynVar->GetValueAsDAT();

		if (!Value)
		{
			Value = NewObject<UTouchEngineDAT>();
		}
		return true;
	}

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float>& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}
	else if (dynVar->IsArray == false)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a CHOP property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a CHOP property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->GetOutput(Target->EngineInfo);
	}

	if (dynVar->Value)
	{
		TArray<double> doubleArray = dynVar->GetValueAsDoubleTArray();

		if (doubleArray.Num() != 0)
		{
			for (int i = 0; i < doubleArray.Num(); i++)
			{
				Value.Add(doubleArray[i]);
			}
		}

		return true;
	}

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetStringByName(UTouchEngineComponentBase* Target, FName VarName, FString& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::String || dynVar->IsArray == true)
	{

		LogTouchEngineError(Target->EngineInfo, "Output is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->GetOutput(Target->EngineInfo);
	}

	if (dynVar->Value)
	{
		Value = dynVar->GetValueAsString();
		return true;
	}

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetFloatByName(UTouchEngineComponentBase* Target, FName VarName, float& Value)
{
	TArray<float> tempValue = TArray<float>();
	if (GetFloatArrayByName(Target, VarName, tempValue))
	{
		if (tempValue.IsValidIndex(0))
		{
			Value = tempValue[0];
			return true;
		}
		Value = 0.f;
		return true;
	}
	Value = 0.f;
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetFloatBufferByName(UTouchEngineComponentBase* Target, FName VarName, UTouchEngineCHOP*& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}
	else if (dynVar->IsArray == false)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a CHOP property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::CHOP)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a CHOP property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		dynVar->GetOutput(Target->EngineInfo);
	}

	if (dynVar->Value)
	{
		if (Target->EngineInfo)
		{
			if (UTouchEngineCHOP* floatBuffer = dynVar->GetValueAsCHOP(Target->EngineInfo))
			{
				Value = floatBuffer;
				return true;
			}
		}

		if (UTouchEngineCHOP* floatBuffer = dynVar->GetValueAsCHOP())
		{
			Value = floatBuffer;
			return true;
		}
	}

	return true;
}


bool UTouchBlueprintFunctionLibrary::GetFloatInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, float& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::Float)
	{
		Value = dynVar->GetValueAsFloat();
		return true;
	}
	else if (dynVar->VarType == EVarType::Double)
	{
		Value = dynVar->GetValueAsDouble();
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Output is not a float property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetFloatArrayInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float>& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::Float || dynVar->VarType == EVarType::Double)
	{
		if (dynVar->IsArray)
		{
			TArray<float> bufferFloatArray;
			TArray<double> bufferDoubleArray = dynVar->GetValueAsDoubleTArray();

			for (int i = 0; i < bufferDoubleArray.Num(); i++)
			{
				bufferFloatArray.Add(bufferDoubleArray[i]);
			}

			Value = bufferFloatArray;
			return true;
		}
		else
		{
			LogTouchEngineError(Target->EngineInfo, "Input is not an array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
			return false;
		}
	}
	else if (dynVar->VarType == EVarType::CHOP)
	{
		UTouchEngineCHOP* buffer = dynVar->GetValueAsCHOP();

		if (buffer)
		{
			Value = buffer->GetChannel(0);
		}
		else
		{
			Value = TArray<float>();
		}
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not a float array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetIntInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, int& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	Value = dynVar->GetValueAsInt();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetInt64InputLatestByName(UTouchEngineComponentBase* Target, FName VarName, int64& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	Value = (int64)(dynVar->GetValueAsInt());
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetIntArrayInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, TArray<int>& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::Int)
	{
		if (dynVar->IsArray)
		{
			Value = dynVar->GetValueAsIntTArray();
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

bool UTouchBlueprintFunctionLibrary::GetBoolInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, bool& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Bool)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a boolean property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	Value = dynVar->GetValueAsBool();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetNameInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, FName& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::String)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	Value  = FName(dynVar->GetValueAsString());
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetObjectInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, UTexture*& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Texture)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a texture property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	Value = dynVar->GetValueAsTexture();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetTexture2DInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, UTexture2D*& Value)
{
	UTexture* texVal;
	bool retVal = GetObjectInputLatestByName(Target, VarName, texVal);

	if (texVal)
	{
		Value = Cast<UTexture2D>(texVal);
	}

	return retVal;
}

bool UTouchBlueprintFunctionLibrary::GetClassInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, class UClass*& Value)
{
	LogTouchEngineError(Target->EngineInfo, "Unsupported dynamic variable type.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetByteInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, uint8& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	Value = (uint8)(dynVar->GetValueAsInt());
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetStringInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, FString& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::String)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	Value = dynVar->GetValueAsString();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetStringArrayInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString>& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType == EVarType::String || dynVar->VarType == EVarType::CHOP)
	{
		Value = dynVar->GetValueAsStringArray();
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not a string array property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetTextInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, FText& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::String)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	Value = FText::FromString(dynVar->GetValueAsString());
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetColorInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, FColor& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a color property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	TArray<double> buffer = dynVar->GetValueAsDoubleTArray();

	if (buffer.Num() != 4)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a color property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	Value.R = buffer[0];
	Value.G = buffer[1];
	Value.B = buffer[2];
	Value.A = buffer[3];

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetVectorInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, FVector& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}
	TArray<double> buffer = dynVar->GetValueAsDoubleTArray();

	if (buffer.Num() != 3)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	Value.X = buffer[0];
	Value.Y = buffer[1];
	Value.Z = buffer[2];

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetVector4InputLatestByName(UTouchEngineComponentBase* Target, FName VarName, FVector4& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector4 property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	TArray<double> buffer = dynVar->GetValueAsDoubleTArray();

	if (buffer.Num() != 4)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector4 property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	Value.X = buffer[0];
	Value.Y = buffer[1];
	Value.Z = buffer[2];
	Value.W = buffer[3];

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetEnumInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, uint8& Value)
{
	FTouchEngineDynamicVariableStruct* dynVar = TryGetDynamicVariable(Target, VarName);

	if (!dynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	if (dynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName.ToString(), Target->ToxFilePath);
		return false;
	}

	Value = (uint8)dynVar->GetValueAsInt();
	return true;
}



FTouchEngineDynamicVariableStruct* UTouchBlueprintFunctionLibrary::TryGetDynamicVariable(UTouchEngineComponentBase* Target, FName VarName)
{
	// try to find by name
	FTouchEngineDynamicVariableStruct* dynVar = Target->DynamicVariables.GetDynamicVariableByIdentifier(VarName.ToString());

	if (!dynVar)
	{
		// failed to find by name, try to find by visible name
		dynVar = Target->DynamicVariables.GetDynamicVariableByName(VarName.ToString());
	}

	return dynVar;
}

void UTouchBlueprintFunctionLibrary::LogTouchEngineError(UTouchEngineInfo* Info, FString Error, FString OwnerName, FString InputName, FString FileName)
{
	if (Info)
	{
		Info->LogTouchEngineError(FString::Printf(TEXT("Blueprint %s: File %s: Param %s: %s"), *OwnerName, *FileName, *InputName, *Error));
	}
}



/*
bool UTouchBlueprintFunctionLibrary::GetRGBofPixel(UTexture2D* texture, int32 U, int32 V, FColor& RGB)
{
	if (!texture)
	{
		return false;
	}

	FTexture2DMipMap* mipMap = &texture->PlatformData->Mips[0];
	FByteBulkData* rawImageData = &mipMap->BulkData;

	FColor* formattedImageData = static_cast<FColor*>(rawImageData->Lock(LOCK_READ_ONLY));
	int32 width = mipMap->SizeX, height = mipMap->SizeY;

	if (U >= 0 && U < width && V >= 0 && V < height)
	{
		RGB = formattedImageData[V * width + U];
	}

	rawImageData->Unlock();

	return true;
}

bool UTouchBlueprintFunctionLibrary::TextureToVectorArray(UTexture2D* texture, TArray<FVector>& outVectors, int32& width, int32& height)
{
	if (!texture)
	{
		return false;
	}



	FTexture2DMipMap* MyMipMap = &texture->PlatformData->Mips[0];
	FColor* FormatedImageData = NULL;
	MyMipMap->BulkData.GetCopy((void**)&FormatedImageData);
	uint32 TextureWidth = MyMipMap->SizeX, TextureHeight = MyMipMap->SizeY;
	FColor PixelColor;

	for (uint8 PixelY = 0; PixelY < TextureHeight; PixelY++)
	{
		for (uint8 PixelX = 0; PixelX < TextureWidth; PixelX++)
		{
			FColor pixelColor = FormatedImageData[PixelY * TextureWidth + PixelX];
			outVectors.Add(FVector((float)pixelColor.R / 255.f - .5f, (float)pixelColor.G / 255.f - .5f, (float)pixelColor.B / 255.f - .5f));
		}
	}

	return true;
}
*/
