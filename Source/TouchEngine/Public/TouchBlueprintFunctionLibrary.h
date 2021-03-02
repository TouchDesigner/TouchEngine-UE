// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TouchEngineComponent.h"
#include "TouchBlueprintFunctionLibrary.generated.h"

/**
 *
 */
UCLASS()
class TOUCHENGINE_API UTouchBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static UFunction* FindSetterByType(FName InType, bool IsArray, FName structName);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
		static UFunction* FindGetterByType(FName InType, bool IsArray, FName structName);


	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetFloatByName(UTouchEngineComponentBase* Target, FName VarName, float value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float> value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetIntByName(UTouchEngineComponentBase* Target, FName VarName, int value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetInt64ByName(UTouchEngineComponentBase* Target, FName VarName, int64 value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetBoolByName(UTouchEngineComponentBase* Target, FName VarName, bool value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetNameByName(UTouchEngineComponentBase* Target, FName VarName, FName value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTextureRenderTarget2D* value);
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
	//UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	//static bool SetStructByName(UTouchEngineComponentBase* Target, FName VarName, UScriptStruct* value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetColorByName(UTouchEngineComponentBase* Target, FName VarName, FColor value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetVectorByName(UTouchEngineComponentBase* Target, FName VarName, FVector value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetVector4ByName(UTouchEngineComponentBase* Target, FName VarName, FVector4 value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetEnumByName(UTouchEngineComponentBase* Target, FName VarName, uint8 value);

	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetObjectByName(UTouchEngineComponentBase* Target, FName VarName, UTexture*& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetStringArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<FString>& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetFloatArrayByName(UTouchEngineComponentBase* Target, FName VarName, TArray<float>& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetStringByName(UTouchEngineComponentBase* Target, FName VarName, FString& value);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetFloatByName(UTouchEngineComponentBase* Target, FName VarName, float& value);


private: 

	static FTouchEngineDynamicVariableStruct* TryGetDynamicVariable(UTouchEngineComponentBase* Target, FName VarIdentifier);
};
