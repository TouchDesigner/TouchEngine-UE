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

#include "Blueprint/TouchBlueprintFunctionLibrary.h"

#include "Logging.h"
#include "Blueprint/TouchEngineComponent.h"
#include "Engine/TouchEngineInfo.h"
#include "TouchEngineDynamicVariableStruct.h"

#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
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
	const FName PC_Double(TEXT("double"));
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

	if (InType == FTouchEngineType::PC_Float || InType == FTouchEngineType::PC_Double)
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

	return StaticClass()->FindFunctionByName(FunctionName);
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
	else if (InType == FTouchEngineType::PC_Float || InType == FTouchEngineType::PC_Double)
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

	return StaticClass()->FindFunctionByName(FunctionName);
}

UFunction* UTouchBlueprintFunctionLibrary::FindInputGetterByType(FName InType, bool IsArray, FName StructName)
{
	if (InType.ToString().IsEmpty())
		return nullptr;

	FName FunctionName = "";

	if (InType == FTouchEngineType::PC_Float || InType == FTouchEngineType::PC_Double)
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

	return StaticClass()->FindFunctionByName(FunctionName);
}


bool UTouchBlueprintFunctionLibrary::SetFloatByName(UTouchEngineComponentBase* Target, FString VarName, float Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType == EVarType::Float)
	{
		DynVar->SetValue(Value);
		if (Target->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(Target->EngineInfo);
		}
		return true;
	}
	else if (DynVar->VarType == EVarType::Double)
	{
		DynVar->SetValue((double)Value);
		if (Target->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(Target->EngineInfo);
		}
		return true;
	}
	LogTouchEngineError(Target->EngineInfo, "Input is not a float property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetFloatArrayByName(UTouchEngineComponentBase* Target, FString VarName, TArray<float> Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType == EVarType::Float || DynVar->VarType == EVarType::Double)
	{
		if (DynVar->bIsArray)
		{
			DynVar->SetValue(Value);
			if (Target->SendMode == ETouchEngineSendMode::OnAccess)
			{
				DynVar->SendInput(Target->EngineInfo);
			}
			return true;
		}
		else
		{
			LogTouchEngineError(Target->EngineInfo, "Input is not an array property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
			return false;
		}
	}
	else if (DynVar->VarType == EVarType::CHOP)
	{
		DynVar->SetValueAsCHOP(Value, 1, Value.Num());

		if (Target->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(Target->EngineInfo);
		}
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not a float array property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetIntByName(UTouchEngineComponentBase* Target, FString VarName, int32 Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	DynVar->SetValue(Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetInt64ByName(UTouchEngineComponentBase* Target, FString VarName, int64 Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	DynVar->SetValue((int)Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetIntArrayByName(UTouchEngineComponentBase* Target, FString VarName, TArray<int> Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType == EVarType::Int)
	{
		if (DynVar->bIsArray)
		{
			DynVar->SetValue(Value);
			if (Target->SendMode == ETouchEngineSendMode::OnAccess)
			{
				DynVar->SendInput(Target->EngineInfo);
			}
			return true;
		}
		else
		{
			LogTouchEngineError(Target->EngineInfo, "Input is not an array property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
			return false;
		}
	}

	if (Target->EngineInfo)
		Target->EngineInfo->LogTouchEngineError(FString::Printf(TEXT("Input %s is not an integer array property in file %s."), *VarName, *Target->GetFilePath()));
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetBoolByName(UTouchEngineComponentBase* Target, FString VarName, bool Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Bool)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a boolean property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	DynVar->SetValue(Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetNameByName(UTouchEngineComponentBase* Target, FString VarName, FName Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::String)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	DynVar->SetValue(Value.ToString());
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetObjectByName(UTouchEngineComponentBase* Target, FString VarName, UTexture* Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Texture)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a texture property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	DynVar->SetValue(Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetClassByName(UTouchEngineComponentBase* Target, FString VarName, UClass* Value)
{
	LogTouchEngineError(Target->EngineInfo, "Unsupported dynamic variable type.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetByteByName(UTouchEngineComponentBase* Target, FString VarName, uint8 Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	DynVar->SetValue((int)Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetStringByName(UTouchEngineComponentBase* Target, FString VarName, FString Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::String)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	DynVar->SetValue(Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetStringArrayByName(UTouchEngineComponentBase* Target, FString VarName, TArray<FString> Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType == EVarType::String || DynVar->VarType == EVarType::CHOP)
	{
		DynVar->SetValueAsDAT(Value, Value.Num(), 1);


		if (Target->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(Target->EngineInfo);
		}
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not a string array property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());

	return false;
}

bool UTouchBlueprintFunctionLibrary::SetTextByName(UTouchEngineComponentBase* Target, FString VarName, FText Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::String)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	DynVar->SetValue(Value.ToString());
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetColorByName(UTouchEngineComponentBase* Target, FString VarName, FColor Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a color property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarIntent != EVarIntent::Color)
	{
		// intent is not color, should log warning but not stop setting since you can set a vector of size 4 with a color

	}
	TArray<double> Buffer;
	Buffer.Add((double)Value.R);
	Buffer.Add((double)Value.G);
	Buffer.Add((double)Value.B);
	Buffer.Add((double)Value.A);
	DynVar->SetValue(Buffer);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetVectorByName(UTouchEngineComponentBase* Target, FString VarName, FVector Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a double property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarIntent != EVarIntent::UVW)
	{
		// intent is not uvw, maybe should not log warning

	}

	TArray<double> Buffer;
	Buffer.Add(Value.X);
	Buffer.Add(Value.Y);
	Buffer.Add(Value.Z);
	DynVar->SetValue(Buffer);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetVector4ByName(UTouchEngineComponentBase* Target, FString VarName, FVector4 value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector 4 property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	TArray<double> Buffer;
	Buffer.Add(value.X);
	Buffer.Add(value.Y);
	Buffer.Add(value.Z);
	Buffer.Add(value.W);
	DynVar->SetValue(Buffer);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(Target->EngineInfo);
	}
	return true;
}

bool UTouchBlueprintFunctionLibrary::SetEnumByName(UTouchEngineComponentBase* Target, FString VarName, uint8 Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to set variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	DynVar->SetValue((int)Value);
	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->SendInput(Target->EngineInfo);
	}
	return true;
}


bool UTouchBlueprintFunctionLibrary::GetObjectByName(UTouchEngineComponentBase* Target, FString VarName, UTexture*& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Texture)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a texture property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->GetOutput(Target->EngineInfo);
	}

	if (DynVar->Value)
	{
		Value = DynVar->GetValueAsTexture();
		return true;
	}

	Value = nullptr;
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetTexture2DByName(UTouchEngineComponentBase* Target, FString VarName, UTexture2D*& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	UTexture* TexVal;
	bool RetVal = GetObjectByName(Target, VarName, TexVal);

	if (IsValid(reinterpret_cast<UObject*>(TexVal)))
	{
		Value = reinterpret_cast<UTexture2D*>(TexVal);
	}

	return RetVal;
}

bool UTouchBlueprintFunctionLibrary::GetStringArrayByName(UTouchEngineComponentBase* Target, FString VarName, UTouchEngineDAT*& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::String || DynVar->bIsArray == false)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a DAT property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->Value)
	{
		if (Target->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->GetOutput(Target->EngineInfo);
		}

		Value = DynVar->GetValueAsDAT();

		if (!Value)
		{
			Value = NewObject<UTouchEngineDAT>();
		}
		return true;
	}

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetFloatArrayByName(UTouchEngineComponentBase* Target, FString VarName, TArray<float>& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}
	else if (DynVar->bIsArray == false)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a CHOP property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a CHOP property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->GetOutput(Target->EngineInfo);
	}

	if (DynVar->Value)
	{
		TArray<double> DoubleArray = DynVar->GetValueAsDoubleTArray();

		if (DoubleArray.Num() != 0)
		{
			for (int i = 0; i < DoubleArray.Num(); i++)
			{
				Value.Add(DoubleArray[i]);
			}
		}

		return true;
	}

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetStringByName(UTouchEngineComponentBase* Target, FString VarName, FString& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::String || DynVar->bIsArray == true)
	{

		LogTouchEngineError(Target->EngineInfo, "Output is not a string property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->GetOutput(Target->EngineInfo);
	}

	if (DynVar->Value)
	{
		Value = DynVar->GetValueAsString();
		return true;
	}

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetFloatByName(UTouchEngineComponentBase* Target, FString VarName, float& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
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

bool UTouchBlueprintFunctionLibrary::GetFloatBufferByName(UTouchEngineComponentBase* Target, FString VarName, UTouchEngineCHOP*& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Output not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}
	else if (DynVar->bIsArray == false)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a CHOP property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::CHOP)
	{
		LogTouchEngineError(Target->EngineInfo, "Output is not a CHOP property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (Target->SendMode == ETouchEngineSendMode::OnAccess)
	{
		DynVar->GetOutput(Target->EngineInfo);
	}

	if (DynVar->Value)
	{
		if (Target->EngineInfo)
		{
			if (UTouchEngineCHOP* FloatBuffer = DynVar->GetValueAsCHOP(Target->EngineInfo))
			{
				Value = FloatBuffer;
				return true;
			}
		}

		if (UTouchEngineCHOP* FloatBuffer = DynVar->GetValueAsCHOP())
		{
			Value = FloatBuffer;
			return true;
		}
	}

	return true;
}


bool UTouchBlueprintFunctionLibrary::GetFloatInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, float& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType == EVarType::Float)
	{
		Value = DynVar->GetValueAsFloat();
		return true;
	}
	else if (DynVar->VarType == EVarType::Double)
	{
		Value = DynVar->GetValueAsDouble();
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Output is not a float property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetFloatArrayInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, TArray<float>& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType == EVarType::Float || DynVar->VarType == EVarType::Double)
	{
		if (DynVar->bIsArray)
		{
			TArray<float> BufferFloatArray;
			TArray<double> BufferDoubleArray = DynVar->GetValueAsDoubleTArray();

			for (int i = 0; i < BufferDoubleArray.Num(); i++)
			{
				BufferFloatArray.Add(BufferDoubleArray[i]);
			}

			Value = BufferFloatArray;
			return true;
		}
		else
		{
			LogTouchEngineError(Target->EngineInfo, "Input is not an array property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
			return false;
		}
	}
	else if (DynVar->VarType == EVarType::CHOP)
	{
		UTouchEngineCHOP* Buffer = DynVar->GetValueAsCHOP();

		if (Buffer)
		{
			Value = Buffer->GetChannel(0);
		}
		else
		{
			Value = TArray<float>();
		}
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not a float array property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetIntInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, int32& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	Value = DynVar->GetValueAsInt();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetInt64InputLatestByName(UTouchEngineComponentBase* Target, FString VarName, int64& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	Value = (int64)(DynVar->GetValueAsInt());
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetIntArrayInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, TArray<int>& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType == EVarType::Int)
	{
		if (DynVar->bIsArray)
		{
			Value = DynVar->GetValueAsIntTArray();
			return true;
		}
		else
		{
			LogTouchEngineError(Target->EngineInfo, "Input is not an array property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
			return false;
		}
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not an integer array property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetBoolInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, bool& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Bool)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a boolean property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	Value = DynVar->GetValueAsBool();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetNameInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FName& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::String)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	Value = FName(DynVar->GetValueAsString());
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetObjectInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, UTexture*& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Texture)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a texture property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	Value = DynVar->GetValueAsTexture();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetTexture2DInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, UTexture2D*& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	UTexture* TexVal;
	bool retVal = GetObjectInputLatestByName(Target, VarName, TexVal);

	if (TexVal)
	{
		Value = Cast<UTexture2D>(TexVal);
	}

	return retVal;
}

bool UTouchBlueprintFunctionLibrary::GetClassInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, class UClass*& Value)
{
	LogTouchEngineError(Target->EngineInfo, "Unsupported dynamic variable type.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetByteInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, uint8& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	Value = (uint8)(DynVar->GetValueAsInt());
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetStringInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FString& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::String)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	Value = DynVar->GetValueAsString();
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetStringArrayInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, TArray<FString>& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType == EVarType::String || DynVar->VarType == EVarType::CHOP)
	{
		Value = DynVar->GetValueAsStringArray();
		return true;
	}

	LogTouchEngineError(Target->EngineInfo, "Input is not a string array property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetTextInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FText& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::String)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a string property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	Value = FText::FromString(DynVar->GetValueAsString());
	return true;
}

bool UTouchBlueprintFunctionLibrary::GetColorInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FColor& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a color property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	TArray<double> Buffer = DynVar->GetValueAsDoubleTArray();

	if (Buffer.Num() != 4)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a color property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	Value.R = Buffer[0];
	Value.G = Buffer[1];
	Value.B = Buffer[2];
	Value.A = Buffer[3];

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetVectorInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FVector& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}
	TArray<double> Buffer = DynVar->GetValueAsDoubleTArray();

	if (Buffer.Num() != 3)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	Value.X = Buffer[0];
	Value.Y = Buffer[1];
	Value.Z = Buffer[2];

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetVector4InputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FVector4& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Double)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector4 property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	TArray<double> Buffer = DynVar->GetValueAsDoubleTArray();

	if (Buffer.Num() != 4)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not a vector4 property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	Value.X = Buffer[0];
	Value.Y = Buffer[1];
	Value.Z = Buffer[2];
	Value.W = Buffer[3];

	return true;
}

bool UTouchBlueprintFunctionLibrary::GetEnumInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, uint8& Value)
{
	if (!Target)
	{
		return false;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return false;
	}
	
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName);
	if (!DynVar)
	{
		LogTouchEngineError(Target->EngineInfo, "Input not found.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	if (DynVar->VarType != EVarType::Int)
	{
		LogTouchEngineError(Target->EngineInfo, "Input is not an integer property.", Target->GetOwner()->GetName(), VarName, Target->GetFilePath());
		return false;
	}

	Value = (uint8)DynVar->GetValueAsInt();
	return true;
}

FTouchEngineDynamicVariableStruct* UTouchBlueprintFunctionLibrary::TryGetDynamicVariable(UTouchEngineComponentBase* Target, FString VarName)
{
	if (!Target)
	{
		return nullptr;
	}
	
	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get variable while TouchEngine was not ready. Skipping."));
		return nullptr;
	}
	
	// try to find by name
	FTouchEngineDynamicVariableStruct* DynVar = Target->DynamicVariables.GetDynamicVariableByIdentifier(VarName);

	if (!DynVar)
	{
		// failed to find by name, try to find by visible name
		DynVar = Target->DynamicVariables.GetDynamicVariableByName(VarName);
	}

	return DynVar;
}

void UTouchBlueprintFunctionLibrary::LogTouchEngineError(UTouchEngineInfo* Info, const FString& Error, const FString& OwnerName, const FString& InputName, const FString& FileName)
{
	if (Info)
	{
		Info->LogTouchEngineError(FString::Printf(TEXT("Blueprint %s: File %s: Param %s: %s"), *OwnerName, *FileName, *InputName, *Error));
	}
}
