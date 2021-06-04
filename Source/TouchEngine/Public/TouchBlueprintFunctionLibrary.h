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
class UTouchEngineCHOP;
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
	static UFunction* FindSetterByType(FName InType, bool IsArray, FName structName);
	// Returns a pointer to the appropriate getter UFunction based on type name, struct name, and if the value is an array
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static UFunction* FindGetterByType(FName InType, bool IsArray, FName structName);
	// Returns a pointer to the appropriate input getter UFunction based on type name, struct name, and if the value is an array
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static UFunction* FindInputGetterByType(FName InType, bool IsArray, FName structName);


	// Setters for TouchEngine dynamic variables accessed through the TouchEngine Input K2 Node

	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetFloatByName(UTouchEngineComponentBase* Target, FName VarName, float value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float> value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetIntByName(UTouchEngineComponentBase* Target, FName VarName, int value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetInt64ByName(UTouchEngineComponentBase* Target, FName VarName, int64 value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetIntArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<int> value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetBoolByName(UTouchEngineComponentBase* Target, FName VarName, bool value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetNameByName(UTouchEngineComponentBase* Target, FName VarName, FName value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTexture* value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetClassByName(UTouchEngineComponentBase* Target, FName VarName, class UClass* value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetByteByName(UTouchEngineComponentBase* Target, FName VarName, uint8 value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetStringByName(UTouchEngineComponentBase* Target, FName VarName, FString value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetStringArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString> value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetTextByName(UTouchEngineComponentBase* Target, FName VarName, FText value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetColorByName(UTouchEngineComponentBase* Target, FName VarName, FColor value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetVectorByName(UTouchEngineComponentBase* Target, FName VarName, FVector value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetVector4ByName(UTouchEngineComponentBase* Target, FName VarName, FVector4 value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetEnumByName(UTouchEngineComponentBase* Target, FName VarName, uint8 value);

	// Getters for TouchEngine dynamic variables accessed through the TouchEngine Output K2 Node

	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTexture*& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetStringArrayByName(UTouchEngineComponentBase* Target, FName VarName, UTouchEngineDAT*& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float>& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetStringByName(UTouchEngineComponentBase* Target, FName VarName, FString& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetFloatByName(UTouchEngineComponentBase* Target, FName VarName, float& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetFloatBufferByName(UTouchEngineComponentBase* Target, FName VarName, UTouchEngineCHOP*& value);



	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetFloatInputByName(UTouchEngineComponentBase* Target, FName VarName, float& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetFloatArrayInputByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float>& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetIntInputByName(UTouchEngineComponentBase* Target, FName VarName, int& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetInt64InputByName(UTouchEngineComponentBase* Target, FName VarName, int64& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetIntArrayInputByName(UTouchEngineComponentBase* Target, FName VarName, TArray<int>& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetBoolInputByName(UTouchEngineComponentBase* Target, FName VarName, bool& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetNameInputByName(UTouchEngineComponentBase* Target, FName VarName, FName& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetObjectInputByName(UTouchEngineComponentBase* Target, FName VarName, UTexture*& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetClassInputByName(UTouchEngineComponentBase* Target, FName VarName, class UClass*& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetByteInputByName(UTouchEngineComponentBase* Target, FName VarName, uint8& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetStringInputByName(UTouchEngineComponentBase* Target, FName VarName, FString& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetStringArrayInputByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString>& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetTextInputByName(UTouchEngineComponentBase* Target, FName VarName, FText& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetColorInputByName(UTouchEngineComponentBase* Target, FName VarName, FColor& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetVectorInputByName(UTouchEngineComponentBase* Target, FName VarName, FVector& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetVector4InputByName(UTouchEngineComponentBase* Target, FName VarName, FVector4& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetEnumInputByName(UTouchEngineComponentBase* Target, FName VarName, uint8& value);



private: 

	// returns the dynamic variable with the identifier in the TouchEngineComponent if possible
	static FTouchEngineDynamicVariableStruct* TryGetDynamicVariable(UTouchEngineComponentBase* Target, FName VarName);

	static void LogTouchEngineError(UTouchEngineInfo* info, FString error, FString ownerName, FString inputName, FString fileName);
};
