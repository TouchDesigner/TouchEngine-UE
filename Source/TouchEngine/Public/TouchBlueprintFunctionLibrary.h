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

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TouchBlueprintFunctionLibrary.generated.h"

struct FTouchEngineDynamicVariableStruct;
class UTexture;
class UTexture2D;
class UTouchEngineCHOP;
class UTouchEngineComponentBase;
class UTouchEngineDAT;
class UTouchEngineInfo;

/**
 *
 */
UCLASS()
class TOUCHENGINE_API UTouchBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// Returns a pointer to the appropriate setter UFunction based on type name, struct name, and if the value is an array
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static UFunction* FindSetterByType(FName InType, bool IsArray, FName StructName);
	// Returns a pointer to the appropriate getter UFunction based on type name, struct name, and if the value is an array
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static UFunction* FindGetterByType(FName InType, bool IsArray, FName StructName);
	// Returns a pointer to the appropriate input getter UFunction based on type name, struct name, and if the value is an array
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static UFunction* FindInputGetterByType(FName InType, bool IsArray, FName StructName);


	// Setters for TouchEngine dynamic variables accessed through the TouchEngine Input K2 Node

	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetFloatByName(UTouchEngineComponentBase* Target, FName VarName, float Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float> Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetIntByName(UTouchEngineComponentBase* Target, FName VarName, int Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetInt64ByName(UTouchEngineComponentBase* Target, FName VarName, int64 Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetIntArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<int> Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetBoolByName(UTouchEngineComponentBase* Target, FName VarName, bool Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetNameByName(UTouchEngineComponentBase* Target, FName VarName, FName Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTexture* Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetClassByName(UTouchEngineComponentBase* Target, FName VarName, class UClass* Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetByteByName(UTouchEngineComponentBase* Target, FName VarName, uint8 Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetStringByName(UTouchEngineComponentBase* Target, FName VarName, FString Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetStringArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString> Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetTextByName(UTouchEngineComponentBase* Target, FName VarName, FText Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetColorByName(UTouchEngineComponentBase* Target, FName VarName, FColor Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetVectorByName(UTouchEngineComponentBase* Target, FName VarName, FVector Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetVector4ByName(UTouchEngineComponentBase* Target, FName VarName, FVector4 Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool SetEnumByName(UTouchEngineComponentBase* Target, FName VarName, uint8 Value);

	// Getters for TouchEngine dynamic variables accessed through the TouchEngine Output K2 Node

	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTexture*& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetTexture2DByName(UTouchEngineComponentBase* Target, FName VarName, UTexture2D*& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetStringArrayByName(UTouchEngineComponentBase* Target, FName VarName, UTouchEngineDAT*& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float>& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetStringByName(UTouchEngineComponentBase* Target, FName VarName, FString& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetFloatByName(UTouchEngineComponentBase* Target, FName VarName, float& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetFloatBufferByName(UTouchEngineComponentBase* Target, FName VarName, UTouchEngineCHOP*& Value);



	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetFloatInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, float& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetFloatArrayInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float>& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetIntInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, int& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetInt64InputLatestByName(UTouchEngineComponentBase* Target, FName VarName, int64& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetIntArrayInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, TArray<int>& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetBoolInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, bool& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetNameInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, FName& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetObjectInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, UTexture*& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetTexture2DInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, UTexture2D*& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetClassInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, class UClass*& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetByteInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, uint8& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetStringInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, FString& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetStringArrayInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString>& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetTextInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, FText& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetColorInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, FColor& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetVectorInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, FVector& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetVector4InputLatestByName(UTouchEngineComponentBase* Target, FName VarName, FVector4& Value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static bool GetEnumInputLatestByName(UTouchEngineComponentBase* Target, FName VarName, uint8& Value);



private:

	// returns the dynamic variable with the identifier in the TouchEngineComponent if possible
	static FTouchEngineDynamicVariableStruct* TryGetDynamicVariable(UTouchEngineComponentBase* Target, FName VarName);
	// logs an error in the given TouchEngineInfo struct
	static void LogTouchEngineError(UTouchEngineInfo* Info, FString Error, FString OwnerName, FString InputName, FString FileName);
};
