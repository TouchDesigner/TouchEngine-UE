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
#include "TouchEngineDynamicVariableStruct.h"
#include "Blueprint/TouchEngineComponent.h"
#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "Engine/TouchEngineInfo.h"

#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "Rendering/Texture2DResource.h"

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
namespace FSetterFunctionNames // Sets the DynamicVariable value from the given values
{
	static const FName FloatSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetFloatByName));
	static const FName FloatArraySetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetFloatArrayByName));
	static const FName IntSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetIntByName));
	static const FName Int64SetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetInt64ByName));
	static const FName IntArraySetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetIntArrayByName));
	static const FName BoolSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetBoolByName));
	static const FName NameSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetNameByName));
	static const FName ObjectSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetTextureByName));
	static const FName ClassSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetClassByName));
	static const FName ByteSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetByteByName));
	static const FName StringSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetStringByName));
	static const FName StringArraySetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetStringArrayByName));
	static const FName TextSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetTextByName));
	static const FName ColorSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetColorByName));
	static const FName LinearColorSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetLinearColorByName));
	static const FName VectorSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetVectorByName));
	static const FName Vector2DSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetVector2DByName));
	static const FName Vector4SetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetVector4ByName));
	static const FName EnumSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetEnumByName));
	static const FName ChopSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetChopByName));
	static const FName ChopChannelSetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetChopChannelByName));
};

// names of the UFunctions that correspond to the correct getter type
namespace FGetterFunctionNames //Get the DynamicVariable output values from TouchEngine
{
	static const FName TextureGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetTextureByName));
	static const FName Texture2DGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetTexture2DByName));
	static const FName StringArrayGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringArrayByName));
	static const FName FloatArrayGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatArrayByName));
	static const FName StringGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringByName));
	static const FName FloatGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatByName));
	static const FName FloatCHOPGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetCHOPByName));
};

namespace FInputGetterFunctionNames //Get the last value passed to the given DynamicVariable input
{
	static const FName FloatInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatInputLatestByName));
	static const FName FloatArrayInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatArrayInputLatestByName));
	static const FName IntInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetIntInputLatestByName));
	static const FName Int64InputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetInt64InputLatestByName));
	static const FName IntArrayInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetIntArrayInputLatestByName));
	static const FName BoolInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetBoolInputLatestByName));
	static const FName NameInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetNameInputLatestByName));
	static const FName TextureInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetTextureInputLatestByName));
	static const FName Texture2DInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetTexture2DInputLatestByName));
	static const FName ClassInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetClassInputLatestByName));
	static const FName ByteInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetByteInputLatestByName));
	static const FName StringInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringInputLatestByName));
	static const FName StringArrayInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringArrayInputLatestByName));
	static const FName TextInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetTextInputLatestByName));
	static const FName ColorInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetColorInputLatestByName));
	static const FName LinearColorInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetLinearColorInputLatestByName));
	static const FName VectorInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetVectorInputLatestByName));
	static const FName Vector2DInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetVector2DInputLatestByName));
	static const FName Vector4InputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetVector4InputLatestByName));
	static const FName EnumInputGetterName(GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetEnumInputLatestByName));
}


UFunction* UTouchBlueprintFunctionLibrary::FindSetterByType(const FName InType, const bool IsArray, const FName StructName)
{
	if (InType.ToString().IsEmpty())
	{
		return nullptr;
	}

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
		else if (StructName == TBaseStructure<FLinearColor>::Get()->GetFName())
		{
			FunctionName = FSetterFunctionNames::LinearColorSetterName;
		}
		else if (StructName == TBaseStructure<FVector>::Get()->GetFName())
		{
			FunctionName = FSetterFunctionNames::VectorSetterName;
		}
		else if (StructName == TBaseStructure<FVector2D>::Get()->GetFName())
		{
			FunctionName = FSetterFunctionNames::Vector2DSetterName;
		}
		else if (StructName == TBaseStructure<FVector4>::Get()->GetFName())
		{
			FunctionName = FSetterFunctionNames::Vector4SetterName;
		}
		else if (StructName == TBaseStructure<FTouchEngineCHOP>::Get()->GetFName())
		{
			FunctionName = FSetterFunctionNames::ChopSetterName;
		}
		else if (StructName == TBaseStructure<FTouchEngineCHOPChannel>::Get()->GetFName())
		{
			FunctionName = FSetterFunctionNames::ChopChannelSetterName;
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

UFunction* UTouchBlueprintFunctionLibrary::FindGetterByType(const FName InType, const bool IsArray, const FName StructName)
{
	if (InType.ToString().IsEmpty())
	{
		return nullptr;
	}

	FName FunctionName = "";

	if (InType == FTouchEngineType::PC_Object)
	{
		if (StructName == UTouchEngineDAT::StaticClass()->GetFName())
		{
			FunctionName = FGetterFunctionNames::StringArrayGetterName;
		}
		else if (StructName == UTexture2D::StaticClass()->GetFName())
		{
			FunctionName = FGetterFunctionNames::Texture2DGetterName;
		}
		else
		{
			FunctionName = FGetterFunctionNames::TextureGetterName;
		}
	}
	else if (InType == FTouchEngineType::PC_Struct)
	{
		if (StructName == FTouchEngineCHOP::StaticStruct()->GetFName())
		{
			FunctionName = FGetterFunctionNames::FloatCHOPGetterName;
		}
	}
	else if (InType == FTouchEngineType::PC_String)
	{
		if (IsArray)
		{
			FunctionName = FGetterFunctionNames::StringArrayGetterName;
		}
		else
		{
			FunctionName = FGetterFunctionNames::StringGetterName;
		}
	}
	else if (InType == FTouchEngineType::PC_Float || InType == FTouchEngineType::PC_Double)
	{
		if (IsArray)
		{
			FunctionName = FGetterFunctionNames::FloatArrayGetterName;
		}
		else
		{
			FunctionName = FGetterFunctionNames::FloatGetterName;
		}
	}
	else
	{
		return nullptr;
	}

	return StaticClass()->FindFunctionByName(FunctionName);
}

UFunction* UTouchBlueprintFunctionLibrary::FindInputGetterByType(const FName InType, const bool IsArray, const FName StructName)
{
	if (InType.ToString().IsEmpty())
	{
		return nullptr;
	}

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
			FunctionName = FInputGetterFunctionNames::TextureInputGetterName;
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
		else if (StructName == TBaseStructure<FLinearColor>::Get()->GetFName())
		{
			FunctionName = FInputGetterFunctionNames::LinearColorInputGetterName;
		}
		else if (StructName == TBaseStructure<FVector>::Get()->GetFName())
		{
			FunctionName = FInputGetterFunctionNames::VectorInputGetterName;
		}
		else if (StructName == TBaseStructure<FVector2D>::Get()->GetFName())
		{
			FunctionName = FInputGetterFunctionNames::Vector2DInputGetterName;
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


bool UTouchBlueprintFunctionLibrary::SetFloatByName(UTouchEngineComponentBase* Target, const FString VarName, const float Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Float)
		{
			DynVar->SetValue(Value);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		else if (DynVar->VarType == EVarType::Double)
		{
			DynVar->SetValue(static_cast<double>(Value));
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetFloatByName), TEXT("Input is not a float property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetFloatArrayByName(UTouchEngineComponentBase* Target, const FString VarName, const TArray<float> Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Float || DynVar->VarType == EVarType::Double)
		{
			if (DynVar->bIsArray)
			{
				DynVar->SetValue(Value);
				DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
				return true;
			}
		}
		else if (DynVar->VarType == EVarType::CHOP)
		{
			DynVar->SetValue(Value);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetFloatArrayByName), TEXT("Input is not a float array or CHOP property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetIntByName(UTouchEngineComponentBase* Target, const FString VarName, const int32 Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Int)
		{
			DynVar->SetValue(Value);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetIntByName), TEXT("Input is not an integer property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetInt64ByName(UTouchEngineComponentBase* Target, const FString VarName, const int64 Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Int)
		{
			DynVar->SetValue(static_cast<int>(Value)); //todo: possible overflow issue?
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetInt64ByName), TEXT("Input is not an integer property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetIntArrayByName(UTouchEngineComponentBase* Target, const FString VarName, const TArray<int> Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Int && DynVar->bIsArray)
		{
			DynVar->SetValue(Value);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetIntArrayByName), TEXT("Input is not an integer array property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetBoolByName(UTouchEngineComponentBase* Target, const FString VarName, const bool Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Bool)
		{
			DynVar->SetValue(Value);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetBoolByName), TEXT("Input is not an boolean property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetNameByName(UTouchEngineComponentBase* Target, const FString VarName, const FName Value, const FString Prefix)
{
	return SetStringByName(Target, VarName, Value.ToString(), Prefix);
}

bool UTouchBlueprintFunctionLibrary::SetTextureByName(UTouchEngineComponentBase* Target, const FString VarName, UTexture* Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Texture)
		{
			DynVar->SetValue(Value);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetTextureByName), TEXT("Input is not a texture property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetClassByName(UTouchEngineComponentBase* Target, const FString VarName, UClass* Value, FString Prefix)
{
	LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
		GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetClassByName), TEXT("Unsupported dynamic variable type."));
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetByteByName(UTouchEngineComponentBase* Target, const FString VarName, const uint8 Value, const FString Prefix)
{
	return SetIntByName(Target, VarName, static_cast<int>(Value), Prefix);
}

bool UTouchBlueprintFunctionLibrary::SetStringByName(UTouchEngineComponentBase* Target, const FString VarName, const FString Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::String)
		{
			DynVar->SetValue(Value);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetStringByName), TEXT("Input is not a string property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetStringArrayByName(UTouchEngineComponentBase* Target, const FString VarName, const TArray<FString> Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::String || DynVar->VarType == EVarType::CHOP) //todo: double check if CHOP is acceptable
		{
			DynVar->SetValueAsDAT(Value, Value.Num(), 1);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetStringArrayByName), TEXT("Input is not a string array property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetTextByName(UTouchEngineComponentBase* Target, const FString VarName, const FText Value, const FString Prefix)
{
	return SetStringByName(Target, VarName, Value.ToString(), Prefix);
}

bool UTouchBlueprintFunctionLibrary::SetColorByName(UTouchEngineComponentBase* Target, const FString VarName, const FColor Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if ((DynVar->VarType == EVarType::Double || DynVar->VarType == EVarType::Float) && DynVar->bIsArray)
		{
			if (DynVar->VarIntent != EVarIntent::Color)
			{
				// todo: intent is not color, should log warning but not stop setting since you can set a vector of size 4 with a color
			}
			DynVar->SetValue(Value);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetColorByName), TEXT("Input is not a color property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetLinearColorByName(UTouchEngineComponentBase* Target, FString VarName, FLinearColor Value, FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if ((DynVar->VarType == EVarType::Double || DynVar->VarType == EVarType::Float) && DynVar->bIsArray)
		{
			if (DynVar->VarIntent != EVarIntent::Color)
			{
				// todo: intent is not color, should log warning but not stop setting since you can set a vector of size 4 with a color
			}
			DynVar->SetValue(Value);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetLinearColorByName), TEXT("Input is not a color property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetVectorByName(UTouchEngineComponentBase* Target, const FString VarName, const FVector Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Double && DynVar->bIsArray)
		{
			if (DynVar->VarIntent != EVarIntent::UVW)
			{
				// intent is not uvw, maybe should log warning
			}
			const TArray<double> Buffer{Value.X, Value.Y, Value.Z};
			DynVar->SetValue(Buffer);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetVectorByName), TEXT("Input is not a double array property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetVector2DByName(UTouchEngineComponentBase* Target, FString VarName, FVector2D Value, FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Double && DynVar->bIsArray)
		{
			const TArray<double> Buffer{Value.X, Value.Y};
			DynVar->SetValue(Buffer);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetVector2DByName), TEXT("Input is not a double array property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetVector4ByName(UTouchEngineComponentBase* Target, const FString VarName, const FVector4 Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Double && DynVar->bIsArray)
		{
			const TArray<double> Buffer{Value.X, Value.Y, Value.Z, Value.W};
			DynVar->SetValue(Buffer);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetVector4ByName), TEXT("Input is not a vector 4 property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetEnumByName(UTouchEngineComponentBase* Target, const FString VarName, const uint8 Value, const FString Prefix)
{
	return SetIntByName(Target, VarName, static_cast<int>(Value), Prefix);
}

bool UTouchBlueprintFunctionLibrary::SetChopByName(UTouchEngineComponentBase* Target, const FString VarName, const FTouchEngineCHOP& Value, const FString Prefix)
{
	FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::CHOP)
		{
			if (!Value.IsValid()) // todo: there should not be the need to check if the value is valid as this is checked later on, but we need to find a way to return false.
			{
				LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
					GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetChopByName), TEXT("Value given is not a valid CHOP."));
				return false;
			}
			DynVar->SetValue(Value);
			DynVar->SetFrameLastUpdatedFromNextCookFrame(Target->EngineInfo);
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, SetChopByName), TEXT("Input is not a CHOP property."));
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::SetChopChannelByName(UTouchEngineComponentBase* Target, const FString VarName, const FTouchEngineCHOPChannel& Value, const FString Prefix)
{
	FTouchEngineCHOP Chop;
	Chop.Channels.Emplace(Value);
	return SetChopByName(Target, VarName, Chop, Prefix);
}



bool UTouchBlueprintFunctionLibrary::GetTextureByName(UTouchEngineComponentBase* Target, const FString VarName, UTexture*& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = nullptr;
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Texture)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->GetValueAsTexture();
				return IsValid(Value);
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetTextureByName), TEXT("Output is not a texture property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetTexture2DByName(UTouchEngineComponentBase* Target, const FString VarName, UTexture2D*& Value, int64& FrameLastUpdated, const FString Prefix)
{
	UTexture* Texture;
	if (GetTextureByName(Target, VarName, Texture, FrameLastUpdated, Prefix))
	{
		Value = Cast<UTexture2D>(Texture);
		return IsValid(Value);
	}

	Value = nullptr;
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetStringArrayByName(UTouchEngineComponentBase* Target, const FString VarName, UTouchEngineDAT*& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = nullptr;
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::String && DynVar->bIsArray)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->GetValueAsDAT();
				return IsValid(Value);
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringArrayByName), TEXT("Output is not a DAT property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetFloatArrayByName(UTouchEngineComponentBase* Target, const FString VarName, TArray<float>& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Double && DynVar->bIsArray) //todo: should this accept float and CHOP?
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				const TArray<double> DoubleArray = DynVar->GetValueAsDoubleTArray();
				if (DoubleArray.Num() != 0)
				{
					Value.Append(DoubleArray);
				}
				return true;
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatArrayByName), TEXT("Output is not a double array property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetStringByName(UTouchEngineComponentBase* Target, const FString VarName, FString& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::String && !DynVar->bIsArray)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->GetValueAsString();
				return true;
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringByName), TEXT("Output is not a string property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetFloatByName(UTouchEngineComponentBase* Target, const FString VarName, float& Value, int64& FrameLastUpdated, const FString Prefix)
{
	TArray<float> FloatArray;
	if (GetFloatArrayByName(Target, VarName, FloatArray, FrameLastUpdated, Prefix)) //todo If the current variable is not an array, this would return false. Is that what we want?
	{
		if (FloatArray.IsValidIndex(0))
		{
			Value = FloatArray[0];
			return true;
		}
	}
	
	Value = {};
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetCHOPByName(UTouchEngineComponentBase* Target, const FString VarName, FTouchEngineCHOP& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::CHOP && DynVar->bIsArray)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = Target->EngineInfo ? DynVar->GetValueAsCHOP(Target->EngineInfo) : DynVar->GetValueAsCHOP(); // todo: why do we need the EngineInfo here?
				return Value.IsValid();
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetCHOPByName), TEXT("Output is not a CHOP property."));
		}
	}
	return false;
}


bool UTouchBlueprintFunctionLibrary::GetFloatInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, float& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Float || DynVar->VarType == EVarType::Double)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->VarType == EVarType::Float ? DynVar->GetValueAsFloat() : static_cast<float>(DynVar->GetValueAsDouble()); //todo: possible overflow issue?
				return true;
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatInputLatestByName), TEXT("Input is not a float property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetFloatArrayInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, TArray<float>& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if ((DynVar->VarType == EVarType::Float || DynVar->VarType == EVarType::Double) && DynVar->bIsArray)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				const TArray<double> BufferDoubleArray = DynVar->GetValueAsDoubleTArray();
				Value.Append(BufferDoubleArray); //todo: possible overflow issue?
				return true;
			}
		}
		else if (DynVar->VarType == EVarType::CHOP)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				const FTouchEngineCHOP Chop = DynVar->GetValueAsCHOP();
				return Chop.GetCombinedValues(Value); //todo: check when this is called and what value should be returned
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetFloatArrayInputLatestByName), TEXT("Input is not a float array property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetDoubleArrayInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, TArray<double>& Value, int64& FrameLastUpdated, FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Double && DynVar->bIsArray)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->GetValueAsDoubleTArray();
				return true;
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetDoubleArrayInputLatestByName), TEXT("Input is not a double array property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetIntInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, int32& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Int)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->GetValueAsInt();
				return true;
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetIntInputLatestByName), TEXT("Input is not an integer property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetInt64InputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, int64& Value, int64& FrameLastUpdated, const FString Prefix)
{
	int32 Int;
	if (GetIntInputLatestByName(Target, VarName, Int, FrameLastUpdated, Prefix))
	{
		Value = static_cast<int64>(Int);
		return true;
	}
	
	Value = {};
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetIntArrayInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, TArray<int>& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Int && DynVar->bIsArray)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->GetValueAsIntTArray();
				return true;
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetIntArrayInputLatestByName), TEXT("Input is not an integer array property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetBoolInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, bool& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Bool)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->GetValueAsBool();
				return true;
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
	GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetBoolInputLatestByName), TEXT("Input is not a boolean property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetNameInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, FName& Value, int64& FrameLastUpdated, const FString Prefix)
{
	FString Str;
	if (GetStringInputLatestByName(Target, VarName, Str, FrameLastUpdated, Prefix))
	{
		Value = FName(Str);
		return true;
	}
	
	Value = {};
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetTextureInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, UTexture*& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = nullptr;
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::Bool)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->GetValueAsTexture();
				return true;
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetTextureInputLatestByName), TEXT("Input is not a texture property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetTexture2DInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, UTexture2D*& Value, int64& FrameLastUpdated, const FString Prefix)
{
	UTexture* Texture;
	if (GetTextureInputLatestByName(Target, VarName, Texture, FrameLastUpdated, Prefix))
	{
		Value = Cast<UTexture2D>(Texture);
		return IsValid(Value);
	}

	Value = nullptr;
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetClassInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, class UClass*& Value, int64& FrameLastUpdated, FString Prefix)
{
	LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
		GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetClassInputLatestByName), TEXT("Input type is not supported."));
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetByteInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, uint8& Value, int64& FrameLastUpdated, const FString Prefix)
{
	int32 Int;
	if (GetIntInputLatestByName(Target, VarName, Int, FrameLastUpdated, Prefix))
	{
		Value = static_cast<uint8>(Int);//todo: possible overflow issue?
		return true;
	}
	
	Value = {};
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetStringInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, FString& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::String)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->GetValueAsString();
				return true;
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringInputLatestByName), TEXT("Input is not a string property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetStringArrayInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, TArray<FString>& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if (DynVar->VarType == EVarType::String || DynVar->VarType == EVarType::CHOP)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->GetValueAsStringArray();
				return true;
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetStringArrayInputLatestByName), TEXT("Input is not a string array property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetTextInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, FText& Value, int64& FrameLastUpdated, const FString Prefix)
{
	FString Str;
	if (GetStringInputLatestByName(Target, VarName, Str, FrameLastUpdated, Prefix))
	{
		Value = FText::FromString(Str);
		return true;
	}
	
	Value = {};
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetColorInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, FColor& Value, int64& FrameLastUpdated, const FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if ((DynVar->VarType == EVarType::Float || DynVar->VarType == EVarType::Double) && DynVar->bIsArray)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->GetValueAsColor();
				return true;
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetColorInputLatestByName), TEXT("Input is not a double array property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetLinearColorInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FLinearColor& Value, int64& FrameLastUpdated, FString Prefix)
{
	Value = {};
	FrameLastUpdated = -1;

	const FTouchEngineDynamicVariableStruct* DynVar = TryGetDynamicVariable(Target, VarName, Prefix);
	if (DynVar)
	{
		if ((DynVar->VarType == EVarType::Float || DynVar->VarType == EVarType::Double) && DynVar->bIsArray)
		{
			FrameLastUpdated = DynVar->FrameLastUpdated;
			if (FrameLastUpdated > 0 && DynVar->Value)
			{
				Value = DynVar->GetValueAsLinearColor();
				return true;
			}
		}
		else
		{
			LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
				GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetLinearColorInputLatestByName), TEXT("Input is not a double array property."));
		}
	}
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetVectorInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, FVector& Value, int64& FrameLastUpdated, const FString Prefix)
{
	TArray<double> DoubleArray;
	if (GetDoubleArrayInputLatestByName(Target, VarName, DoubleArray, FrameLastUpdated, Prefix))
	{
		if (DoubleArray.Num() == 3)
		{
			Value = {DoubleArray[0], DoubleArray[1], DoubleArray[2]};
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetVectorInputLatestByName), TEXT("Input is not a FVector property."));
	}
	
	Value = {};
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetVector4InputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, FVector4& Value, int64& FrameLastUpdated, const FString Prefix)
{
	TArray<double> DoubleArray;
	if (GetDoubleArrayInputLatestByName(Target, VarName, DoubleArray, FrameLastUpdated, Prefix))
	{
		if (DoubleArray.Num() == 4)
		{
			Value = FVector4{DoubleArray[0], DoubleArray[1], DoubleArray[2], DoubleArray[3]};
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetVector4InputLatestByName), TEXT("Input is not a FVector4 property."));
	}
	
	Value = FVector4{};
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetVector2DInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FVector2D& Value, int64& FrameLastUpdated, FString Prefix)
{
	TArray<double> DoubleArray;
	if (GetDoubleArrayInputLatestByName(Target, VarName, DoubleArray, FrameLastUpdated, Prefix))
	{
		if (DoubleArray.Num() == 2)
		{
			Value = {DoubleArray[0], DoubleArray[1]};
			return true;
		}
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Prefix + VarName,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, GetVector2DInputLatestByName), TEXT("Input is not a FVector2D property."));
	}
	
	Value = {};
	return false;
}

bool UTouchBlueprintFunctionLibrary::GetEnumInputLatestByName(UTouchEngineComponentBase* Target, const FString VarName, uint8& Value, int64& FrameLastUpdated, const FString Prefix)
{
	return GetByteInputLatestByName(Target, VarName, Value, FrameLastUpdated, Prefix);
}



bool UTouchBlueprintFunctionLibrary::RefreshTextureSampler(UTexture* Texture)
{
	if (!IsValid(Texture) || !Texture->GetResource())
	{
		return false;
	}

	// Texture->RefreshSamplerStates(); // that should do the trick by itself but does not fully work at the moment because the Filter value is not updated.
	if (FTextureResource* Resource = Texture->GetResource())
	{
		// the issue here is that we cannot override the Resource->Filter as this is a protected property. It seems like it should be updated in FStreamableTextureResource::CacheSamplerStateInitializer, but it isn't
		// copied from FStreamableTextureResource::FStreamableTextureResource, FStreamableTextureResource::CacheSamplerStateInitializer
		const ESamplerFilter SamplerFilter = static_cast<ESamplerFilter>(UDeviceProfileManager::Get().GetActiveProfile()->GetTextureLODSettings()->GetSamplerFilter(Texture));
		const ESamplerAddressMode AddressU = Texture->GetTextureAddressX() == TA_Wrap ? AM_Wrap : (Texture->GetTextureAddressX() == TA_Clamp ? AM_Clamp : AM_Mirror);
		const ESamplerAddressMode AddressV = Texture->GetTextureAddressY() == TA_Wrap ? AM_Wrap : (Texture->GetTextureAddressY() == TA_Clamp ? AM_Clamp : AM_Mirror);
		const ESamplerAddressMode AddressW = Texture->GetTextureAddressZ() == TA_Wrap ? AM_Wrap : (Texture->GetTextureAddressZ() == TA_Clamp ? AM_Clamp : AM_Mirror);
		float MipBias = 0;
		if (UTexture2D* Texture2D = Cast<UTexture2D>(Texture))
		{
			float DefaultMipBias = 0;
			const FTexturePlatformData* PlatformData = Texture2D->GetPlatformData();
			if (PlatformData && Texture->LODGroup == TEXTUREGROUP_UI)
			{
				DefaultMipBias = -PlatformData->Mips.Num();
			}
			MipBias = UTexture2D::GetGlobalMipMapLODBias() + DefaultMipBias;
		}
	
		// -- below copied from FStreamableTextureResource::RefreshSamplerStates
		const FSamplerStateInitializerRHI SamplerStateInitializer
		(
			SamplerFilter,
			AddressU,
			AddressV,
			AddressW,
			MipBias
		);
		// -- We do not have access to FTexture::GetOrCreateSamplerState or GTextureSamplerStateCache, so we go straight to the RHI function
		Resource->SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer); // GetOrCreateSamplerState(SamplerStateInitializer);
	
		// Create a custom sampler state for using this texture in a deferred pass, where ddx / ddy are discontinuous
		const FSamplerStateInitializerRHI DeferredPassSamplerStateInitializer
		(
			SamplerFilter,
			AddressU,
			AddressV,
			AddressW,
			MipBias,
			// Disable anisotropic filtering, since aniso doesn't respect MaxLOD
			1,
			0,
			// Prevent the less detailed mip levels from being used, which hides artifacts on silhouettes due to ddx / ddy being very large
			// This has the side effect that it increases minification aliasing on light functions
			2
		);
		// -- We do not have access to FTexture::GetOrCreateSamplerState or GTextureSamplerStateCache, so we go straight to the RHI function
		Resource->DeferredPassSamplerStateRHI = RHICreateSamplerState(DeferredPassSamplerStateInitializer); // GetOrCreateSamplerState(DeferredPassSamplerStateInitializer);
		
		return true;
	}
	return false;
}

FString UTouchBlueprintFunctionLibrary::Conv_TouchEngineCHOPToString(const FTouchEngineCHOP& InChop)
{
	return InChop.ToString();
}

FString UTouchBlueprintFunctionLibrary::Conv_TouchEngineCHOPChannelToString(const FTouchEngineCHOPChannel& InChopChannel)
{
	return InChopChannel.ToString();
}

bool UTouchBlueprintFunctionLibrary::IsValidCHOP(const FTouchEngineCHOP& InChop)
{
	return InChop.IsValid();
}

int32 UTouchBlueprintFunctionLibrary::GetNumChannels(const FTouchEngineCHOP& InChop)
{
	return InChop.GetNumChannels();
}

int32 UTouchBlueprintFunctionLibrary::GetNumSamples(const FTouchEngineCHOP& InChop)
{
	return InChop.GetNumSamples();
}

void UTouchBlueprintFunctionLibrary::GetChannel(FTouchEngineCHOP& InChop, const int32 InIndex, FTouchEngineCHOPChannel& OutChannel)
{
	OutChannel = InChop.Channels.IsValidIndex(InIndex) ? InChop.Channels[InIndex] : FTouchEngineCHOPChannel();
}

void UTouchBlueprintFunctionLibrary::ClearCHOP(FTouchEngineCHOP& InChop)
{
	InChop.Clear();
}

bool UTouchBlueprintFunctionLibrary::GetChannelByName(FTouchEngineCHOP& InChop, const FString& InChannelName, FTouchEngineCHOPChannel& OutChannel)
{
	return InChop.GetChannelByName(InChannelName, OutChannel);
}


FTouchEngineDynamicVariableStruct* UTouchBlueprintFunctionLibrary::TryGetDynamicVariable(UTouchEngineComponentBase* Target, const FString& VarName, const FString& Prefix)
{
	if (!Target)
	{
		return nullptr;
	}

	if (!Target->IsLoaded())
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("Attempted to get or set the variable '%s' while TouchEngine was not ready. Skipping."), *VarName);
		return nullptr;
	}

	FString VarNameWithPrefix = VarName;
	if (VarName.StartsWith("p/") || VarName.StartsWith("i/") || VarName.StartsWith("o/"))
	{
		// Legacy names. The user was previously required to explicitly supply the prefix in Blueprint
	}
	else
	{
		VarNameWithPrefix = Prefix + VarName;
	}

	// try to find by name
	FTouchEngineDynamicVariableStruct* DynVar = Target->DynamicVariables.GetDynamicVariableByIdentifier(VarNameWithPrefix);

	if (!DynVar)
	{
		// failed to find by name, try to find by visible name
		DynVar = Target->DynamicVariables.GetDynamicVariableByName(VarNameWithPrefix);
	}
	
	if (!DynVar)
	{
		LogTouchEngineError(Target, UE::TouchEngine::FTouchErrorLog::EErrorType::VariableNameNotFound, VarNameWithPrefix,
			GET_FUNCTION_NAME_CHECKED(UTouchBlueprintFunctionLibrary, TryGetDynamicVariable));
	}

	return DynVar;
}

void UTouchBlueprintFunctionLibrary::LogTouchEngineError(const UTouchEngineComponentBase* Target, UE::TouchEngine::FTouchErrorLog::EErrorType ErrorType, const FString& VarName, const FName& FunctionName, const FString& AdditionalDescription)
{
	if (Target && Target->EngineInfo)
	{
		Target->EngineInfo->LogTouchEngineError(ErrorType, VarName, FunctionName, AdditionalDescription);
	}
}
