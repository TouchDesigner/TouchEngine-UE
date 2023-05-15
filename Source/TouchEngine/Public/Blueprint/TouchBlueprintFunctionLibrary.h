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
#include "Engine/TouchVariables.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TouchBlueprintFunctionLibrary.generated.h"

struct FTouchEngineDynamicVariableStruct;
class UTexture;
class UTexture2D;
class UDEPRECATED_TouchEngineCHOPMinimal;
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
	static bool SetFloatByName(UTouchEngineComponentBase* Target, FString VarName, float Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetFloatArrayByName(UTouchEngineComponentBase* Target, FString VarName, TArray<float> Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetIntByName(UTouchEngineComponentBase* Target, FString VarName, int32 Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetInt64ByName(UTouchEngineComponentBase* Target, FString VarName, int64 Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetIntArrayByName(UTouchEngineComponentBase* Target, FString VarName, TArray<int> Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetBoolByName(UTouchEngineComponentBase* Target, FString VarName, bool Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetNameByName(UTouchEngineComponentBase* Target, FString VarName, FName Value, FString Prefix);
	/**
	 * Set a texture by Name 
	 * @param Target The Component holding the input
	 * @param VarName The name of the Texture input
	 * @param Value The Value of the texture input
	 * @param Prefix 
	 * @param bReuseExistingTexture Indicates if we should reuse the last texture we sent (if any) for better performance. This would imply that the content of the Texture (e.g. the pixels) has not changed.
	 */
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetObjectByName(UTouchEngineComponentBase* Target, FString VarName, UTexture* Value, FString Prefix, bool bReuseExistingTexture);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetClassByName(UTouchEngineComponentBase* Target, FString VarName, class UClass* Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetByteByName(UTouchEngineComponentBase* Target, FString VarName, uint8 Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetStringByName(UTouchEngineComponentBase* Target, FString VarName, FString Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetStringArrayByName(UTouchEngineComponentBase* Target, FString VarName, TArray<FString> Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetTextByName(UTouchEngineComponentBase* Target, FString VarName, FText Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetColorByName(UTouchEngineComponentBase* Target, FString VarName, FColor Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetVectorByName(UTouchEngineComponentBase* Target, FString VarName, FVector Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetVector2DByName(UTouchEngineComponentBase* Target, FString VarName, FVector2D Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetVector4ByName(UTouchEngineComponentBase* Target, FString VarName, FVector4 Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetEnumByName(UTouchEngineComponentBase* Target, FString VarName, uint8 Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetChopByName(UTouchEngineComponentBase* Target, FString VarName, const FTouchEngineCHOP& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool SetChopChannelByName(UTouchEngineComponentBase* Target, FString VarName, const FTouchEngineCHOPChannel& Value, FString Prefix);

	// Getters for TouchEngine dynamic variables accessed through the TouchEngine Output K2 Node

	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetObjectByName(UTouchEngineComponentBase* Target, FString VarName, UTexture*& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetTexture2DByName(UTouchEngineComponentBase* Target, FString VarName, UTexture2D*& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetStringArrayByName(UTouchEngineComponentBase* Target, FString VarName, UTouchEngineDAT*& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetFloatArrayByName(UTouchEngineComponentBase* Target, FString VarName, TArray<float>& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetStringByName(UTouchEngineComponentBase* Target, FString VarName, FString& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetFloatByName(UTouchEngineComponentBase* Target, FString VarName, float& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetCHOPByName(UTouchEngineComponentBase* Target, FString VarName, FTouchEngineCHOP& Value, FString Prefix);


	// Get latest //todo: not sure what that means. Clarify names and description

	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetFloatInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, float& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetFloatArrayInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, TArray<float>& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetIntInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, int32& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetInt64InputLatestByName(UTouchEngineComponentBase* Target, FString VarName, int64& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetIntArrayInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, TArray<int>& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetBoolInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, bool& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetNameInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FName& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetObjectInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, UTexture*& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetTexture2DInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, UTexture2D*& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetClassInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, class UClass*& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetByteInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, uint8& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetStringInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FString& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetStringArrayInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, TArray<FString>& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetTextInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FText& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetColorInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FColor& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetVectorInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FVector& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetVector4InputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FVector4& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetVector2DInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, FVector2D& Value, FString Prefix);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "TouchEngine")
	static bool GetEnumInputLatestByName(UTouchEngineComponentBase* Target, FString VarName, uint8& Value, FString Prefix);

	// Converters

	UFUNCTION(BlueprintPure, meta = (DisplayName = "FTouchEngineCHOP To String", CompactNodeTitle = "->", BlueprintAutocast), Category = "TouchEngine")
	static FString Conv_TouchEngineCHOPToString(const FTouchEngineCHOP& InChop);
	UFUNCTION(BlueprintPure, meta = (DisplayName = "FTouchEngineCHOPChannel To String", CompactNodeTitle = "->", BlueprintAutocast), Category = "TouchEngine")
	static FString Conv_TouchEngineCHOPChannelToString(const FTouchEngineCHOPChannel& InChopChannel);

	// FTouchEngineCHOP Functions

	/**
	 * Check if the FTouchEngineCHOP is valid. An FTouchEngineCHOP is valid when all channels have the same number of samples. An FTouchEngineCHOP with no channels or with only empty channels is valid.
	 */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Is Valid", CompactNodeTitle = "Is Valid?"), Category = "TouchEngine")
	static bool IsValidCHOP(const FTouchEngineCHOP& InChop);

	/**
	 * Returns the number of Channels in the FTouchEngineCHOP, equivalent of breaking the FTouchEngineCHOP structure and getting the length of the Channels array.
	 */
	UFUNCTION(BlueprintPure, Category = "TouchEngine")
	static int32 GetNumChannels(const FTouchEngineCHOP& InChop);
	/**
	 * Returns the number of Samples in the FTouchEngineCHOP, equivalent of breaking the FTouchEngineCHOP structure, getting the first Channel and getting the length of the Values array.
	 * It is only guaranteed to be the same for each Channel if the FTouchEngineCHOP is valid.
	 */
	UFUNCTION(BlueprintPure, Category = "TouchEngine")
	static int32 GetNumSamples(const FTouchEngineCHOP& InChop);
	
	/**
	 * Returns the Channel at the given index, equivalent of breaking the FTouchEngineCHOP structure and getting the Channel at the given Index from the Channels array.
	 */
	UFUNCTION(BlueprintPure, Category = "TouchEngine")
	static void GetChannel(UPARAM(Ref) FTouchEngineCHOP& InChop, const int32 InIndex, FTouchEngineCHOPChannel& OutChannel);
	
	/**
	 * Returns the first Channel with the given name if found, equivalent of breaking the FTouchEngineCHOP structure, looping through the Channels array and returning the first one with the matching name.
	 * @return Returns True if an `FTouchEngineCHOPChannel` with the given Channel Name was found, otherwise false
	 */
	UFUNCTION(BlueprintPure, Category = "TouchEngine")
	static bool GetChannelByName(UPARAM(Ref) FTouchEngineCHOP& InChop, const FString& InChannelName, FTouchEngineCHOPChannel& OutChannel);

	/**
	 * Remove all the Channels and Values from the `FTouchEngineCHOP`
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Clear"), Category = "TouchEngine")
	static void ClearCHOP(UPARAM(Ref) FTouchEngineCHOP& InChop);


private:
	// returns the dynamic variable with the identifier in the TouchEngineComponent if possible
	static FTouchEngineDynamicVariableStruct* TryGetDynamicVariable(UTouchEngineComponentBase* Target, FString VarName, FString Prefix);
	// logs an error in the given TouchEngineInfo struct
	static void LogTouchEngineError(const UTouchEngineInfo* Info, const FString& Error, const FString& OwnerName, const FString& InputName, const FString& FileName);
};
