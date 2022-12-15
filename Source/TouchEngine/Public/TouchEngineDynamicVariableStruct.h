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
#include "TouchEngineIntVector4.h"
#include "TouchEngineDynamicVariableStruct.generated.h"

class UTexture;
class UTouchEngineComponentBase;
class UTouchEngineInfo;
enum class ECheckBoxState : uint8;
struct FTouchCHOPFull;

/*
* possible variable types of dynamic variables based on TEParameterType
*/
UENUM(meta = (NoResetToDefault))
enum class EVarType
{
	NotSet = 0,
	Bool,
	Int,
	Double,
	Float,
	CHOP,
	String,
	Texture,
	Max				UMETA(Hidden)
};

/*
* possible intents of dynamic variables based on TEParameterIntent
*/
UENUM(meta = (NoResetToDefault))
enum class EVarIntent
{
	NotSet = 0,
	DropDown,
	Color,
	Position,
	Size,
	UVW,
	FilePath,
	DirectoryPath,
	Momentary,
	Pulse,
	Max				UMETA(Hidden)
};

UCLASS(BlueprintType, meta = (DisplayName = "TouchEngine CHOP"))
class TOUCHENGINE_API UTouchEngineCHOP : public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadOnly, Category = "Properties")
	int32 NumChannels;
	UPROPERTY(BlueprintReadOnly, Category = "Properties")
	int32 NumSamples;

	TArray<FString> ChannelNames;

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|Properties")
	TArray<float> GetChannel(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|Properties")
	TArray<float> GetChannelByName(const FString& Name);

	void CreateChannels(float** FullChannel, int InChannelCount, int InChannelSize);
	void CreateChannels(FTouchCHOPFull CHOP);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|Properties")
	void Clear();

private:
	TArray<float> ChannelsAppended;
};

UCLASS(BlueprintType, meta = (DisplayName = "TouchEngine DAT"))
class TOUCHENGINE_API UTouchEngineDAT : public UObject
{
	GENERATED_BODY()
	friend struct FTouchEngineDynamicVariableStruct;

public:

	UPROPERTY(BlueprintReadOnly, Category = "Properties")
	int32 NumColumns;
	UPROPERTY(BlueprintReadOnly, Category = "Properties")
	int32 NumRows;

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|Properties")
	TArray<FString> GetRow(int32 Row);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|Properties")
	TArray<FString> GetRowByName(const FString& RowName);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|Properties")
	TArray<FString> GetColumn(int32 Column);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|Properties")
	TArray<FString> GetColumnByName(const FString& ColumnName);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|Properties")
	FString GetCell(int32 Column, int32 Row);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|Properties")
	FString GetCellByName(const FString& ColumnName, const FString& RowName);

	void CreateChannels(const TArray<FString>& AppendedArray, int32 RowCount, int32 ColumnCount);

private:
	TArray<FString> ValuesAppended;
};

template <typename T>
struct TTouchVar
{
	T Data;
};

/*
* Dynamic variable - holds a void pointer and functions to cast it correctly
*/
USTRUCT(meta = (NoResetToDefault))
struct TOUCHENGINE_API FTouchEngineDynamicVariableStruct
{
	GENERATED_BODY()
	friend class FTouchEngineDynamicVariableStructDetailsCustomization;
	friend class FTouchEngineParserUtils;

	FTouchEngineDynamicVariableStruct() = default;
	~FTouchEngineDynamicVariableStruct();
	FTouchEngineDynamicVariableStruct(FTouchEngineDynamicVariableStruct&& Other) { Copy(&Other); }
	FTouchEngineDynamicVariableStruct(const FTouchEngineDynamicVariableStruct& Other) { Copy(&Other); }
	FTouchEngineDynamicVariableStruct& operator=(FTouchEngineDynamicVariableStruct&& Other) { Copy(&Other); return *this; }
	FTouchEngineDynamicVariableStruct& operator=(const FTouchEngineDynamicVariableStruct& Other) { Copy(&Other); return *this; }

	void Copy(const FTouchEngineDynamicVariableStruct* Other);

	// Display name of variable
	UPROPERTY(EditAnywhere, Category = "Properties")
	FString VarLabel = "ERROR_LABEL";

	// Name used to get / set variable by user
	UPROPERTY(EditAnywhere, Category = "Properties")
	FString VarName = "ERROR_NAME";

	// random characters used to identify the variable in TouchEngine
	UPROPERTY(EditAnywhere, Category = "Properties")
	FString VarIdentifier = "ERROR_IDENTIFIER";

	// Variable data type
	UPROPERTY(EditAnywhere, Category = "Properties")
	EVarType VarType = EVarType::NotSet;

	// Variable intent
	UPROPERTY(EditAnywhere, Category = "Properties")
	EVarIntent VarIntent = EVarIntent::NotSet;

	// Number of variables (if array)
	UPROPERTY(EditAnywhere, Category = "Properties")
	int32 Count = 0;

	// Pointer to variable value
	void* Value = nullptr;
	size_t Size = 0;
	bool bIsArray = false;

	bool GetValueAsBool() const;
	int GetValueAsInt() const;
	int GetValueAsIntIndexed(int Index) const;
	int* GetValueAsIntArray() const;
	TArray<int> GetValueAsIntTArray() const;
	double GetValueAsDouble() const;
	double GetValueAsDoubleIndexed(int Index) const;
	double* GetValueAsDoubleArray() const;
	TArray<double> GetValueAsDoubleTArray() const;
	float GetValueAsFloat() const;
	FString GetValueAsString() const;
	TArray<FString> GetValueAsStringArray() const;
	UTexture* GetValueAsTexture() const;
	UTouchEngineCHOP* GetValueAsCHOP() const;
	UTouchEngineCHOP* GetValueAsCHOP(UTouchEngineInfo* EngineInfo) const;
	UTouchEngineDAT* GetValueAsDAT() const;

	void SetValue(bool InValue);
	void SetValue(int InValue);
	void SetValue(const TArray<int>& InValue);
	void SetValue(double InValue);
	void SetValue(const TArray<double>& InValue);
	void SetValue(float InValue);
	void SetValue(const TArray<float>& InValue);
	void SetValue(UTouchEngineCHOP* InValue);
	void SetValueAsCHOP(const TArray<float>& InValue, int NumChannels, int NumSamples);
	void SetValue(UTouchEngineDAT* InValue);
	void SetValueAsDAT(const TArray<FString>& InValue, int NumRows, int NumColumns);
	void SetValue(const FString& InValue);
	void SetValue(const TArray<FString>& InValue);
	void SetValue(UTexture* InValue);
	void SetValue(const FTouchEngineDynamicVariableStruct* Other);

	/** Function called when serializing this struct to a FArchive */
	bool Serialize(FArchive& Ar);
	/** Comparer function for two Dynamic Variables */
	bool Identical(const FTouchEngineDynamicVariableStruct* Other, uint32 PortFlags) const;

	/** Sends the input value to the engine info */
	void SendInput(UTouchEngineInfo* EngineInfo);
	/** Updates the output value from the engine info */
	void GetOutput(UTouchEngineInfo* EngineInfo);

private:

#if WITH_EDITORONLY_DATA

	// these properties exist to generate the property handles and to be a go between for the editor functions and the void pointer value

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
	TArray<float> FloatBufferProperty = TArray<float>();

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
	TArray<FString> StringArrayProperty = TArray<FString>();

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
	TObjectPtr<UTexture> TextureProperty = nullptr;


	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
	FVector2D Vector2DProperty = FVector2D::Zero();

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
	FVector VectorProperty = FVector::Zero();

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault, NoSpinbox))
	FVector4 Vector4Property = FVector4::Zero();

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
	FColor ColorProperty = FColor::Black;


	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
	FIntPoint IntPointProperty = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
	FIntVector IntVectorProperty = FIntVector::ZeroValue;

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
	FTouchEngineIntVector4 IntVector4Property = FTouchEngineIntVector4();


	UPROPERTY(EditAnywhere, Category = "Menu Data", meta = (NoResetToDefault))
	TMap<FString, int32> DropDownData = TMap<FString, int32>();

#endif

	// sets void pointer to UObject pointer, does not copy memory
	void SetValue(UObject* InValue, size_t InSize);
	void Clear();


	// Callbacks

	void HandleChecked(ECheckBoxState InState);

	template <typename T>
	void HandleValueChanged(T InValue);
	template <typename T>
	void HandleValueChangedWithIndex(T InValue, int32 Index);

	void HandleTextBoxTextCommitted(const FText& NewText);
	void HandleTextureChanged();
	void HandleColorChanged();
	void HandleVector2Changed();
	void HandleVectorChanged();
	void HandleVector4Changed();
	void HandleIntVector2Changed();
	void HandleIntVectorChanged();
	void HandleIntVector4Changed();
	void HandleFloatBufferChanged();
	void HandleFloatBufferChildChanged();
	void HandleStringArrayChanged();
	void HandleStringArrayChildChanged();
	void HandleDropDownBoxValueChanged(TSharedPtr<FString> Arg);
};

// Template declaration to tell the serializer to use a custom serializer function. This is done so we can save the void pointer
// data as the correct variable type and read the correct size and type when re-launching the engine
template<>
struct TStructOpsTypeTraits<FTouchEngineDynamicVariableStruct> : public TStructOpsTypeTraitsBase2<FTouchEngineDynamicVariableStruct>
{
	enum
	{
		WithCopy = true,			// struct can be copied via its copy assignment operator.
		WithIdentical = true,		// struct can be compared via an Identical(const T* Other, uint32 PortFlags) function.  This should be mutually exclusive with WithIdenticalViaEquality.
		WithSerializer = true,		// struct has a Serialize function for serializing its state to an FArchive.
	};
};


// Callback for when the TouchEngine instance loads a tox file
DECLARE_MULTICAST_DELEGATE(FTouchOnLoadComplete);
// Callback for when the TouchEngine instance resets its state
DECLARE_MULTICAST_DELEGATE(FTouchOnReset);
// Callback for when the TouchEngine instance fails to load a tox file
DECLARE_MULTICAST_DELEGATE_OneParam(FTouchOnLoadFailed, const FString&);

/**
 * Holds all input and output variables for an instance of the "UTouchEngineComponentBase" component class.
 * Also holds callbacks from the TouchEngine to get info about when parameters are loaded
 */
USTRUCT(BlueprintType, meta = (NoResetToDefault))
struct TOUCHENGINE_API FTouchEngineDynamicVariableContainer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (NoResetToDefault, DisplayName = "Input"), Category = "Properties")
	TArray<FTouchEngineDynamicVariableStruct> DynVars_Input;

	UPROPERTY(EditAnywhere, meta = (NoResetToDefault, DisplayName = "Output"), Category = "Properties")
	TArray<FTouchEngineDynamicVariableStruct> DynVars_Output;

	// Callback function attached to parent component's TouchEngine parameters loaded delegate
	void ToxParametersLoaded(const TArray<FTouchEngineDynamicVariableStruct>& VariablesIn, const TArray<FTouchEngineDynamicVariableStruct>& VariablesOut);
	void Reset();

	void SendInputs(UTouchEngineInfo* EngineInfo);
	void GetOutputs(UTouchEngineInfo* EngineInfo);
	
	void SendInput(UTouchEngineInfo* EngineInfo, int32 Index);
	void GetOutput(UTouchEngineInfo* EngineInfo, int32 Index);
	
	FTouchEngineDynamicVariableStruct* GetDynamicVariableByName(const FString& VarName);
	FTouchEngineDynamicVariableStruct* GetDynamicVariableByIdentifier(const FString& VarIdentifier);
};

// Templated function definitions

template<typename T>
void FTouchEngineDynamicVariableStruct::HandleValueChanged(T InValue)
{
	FTouchEngineDynamicVariableStruct OldValue = *this;
	SetValue(InValue);
}

template <typename T>
void FTouchEngineDynamicVariableStruct::HandleValueChangedWithIndex(T InValue, int32 Index)
{
	if (!Value)
	{
		// if the value doesn't exist,
		Value = new T[Count];
		Size = sizeof(T) * Count;
	}

	((T*)Value)[Index] = InValue;
}