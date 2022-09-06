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
	UTouchEngineCHOP() {}
	virtual ~UTouchEngineCHOP() override {}

	UPROPERTY(BlueprintReadOnly, Category = "Properties")
	int32 NumChannels;

	UPROPERTY(BlueprintReadOnly, Category = "Properties")
	int32 NumSamples;

	TArray<FString> ChannelNames;

	UFUNCTION(BlueprintCallable, Category = "Properties")
	TArray<float> GetChannel(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Properties")
	TArray<float> GetChannelByName(const FString& Name);

	void CreateChannels(float** FullChannel, int InChannelCount, int InChannelSize);

	void CreateChannels(FTouchCHOPFull CHOP);

	UFUNCTION(BlueprintCallable, Category = "Properties")
	void Clear();

private:
	TArray<float> ChannelsAppended;
};

UCLASS(BlueprintType, meta = (DisplayName = "TouchEngine DAT"))
class TOUCHENGINE_API UTouchEngineDAT : public UObject
{
	GENERATED_BODY()

public:
	friend struct FTouchEngineDynamicVariableStruct;

	UTouchEngineDAT() {}
	virtual ~UTouchEngineDAT() override {}

	UPROPERTY(BlueprintReadOnly, Category = "Properties")
	int32 NumColumns;

	UPROPERTY(BlueprintReadOnly, Category = "Properties")
	int32 NumRows;

	UFUNCTION(BlueprintCallable, Category = "Properties")
	TArray<FString> GetRow(int32 Row);

	UFUNCTION(BlueprintCallable, Category = "Properties")
	TArray<FString> GetRowByName(const FString& RowName);

	UFUNCTION(BlueprintCallable, Category = "Properties")
	TArray<FString> GetColumn(int32 Column);

	UFUNCTION(BlueprintCallable, Category = "Properties")
	TArray<FString> GetColumnByName(const FString& ColumnName);

	UFUNCTION(BlueprintCallable, Category = "Properties")
	FString GetCell(int32 Column, int32 Row);

	UFUNCTION(BlueprintCallable, Category = "Properties")
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

public:
	FTouchEngineDynamicVariableStruct();
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

	// Byte size of variable
	size_t Size = 0;

	// If the value is an array value
	bool IsArray = false;

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

public:

	void Clear();

	// Get / Set Values

	// returns value as bool
	bool GetValueAsBool() const;
	// returns value as integer
	int GetValueAsInt() const;
	// returns indexed value as integer
	int GetValueAsIntIndexed(int Index) const;
	// returns value as integer array
	int* GetValueAsIntArray() const;
	// returns value as tarray of integers
	TArray<int> GetValueAsIntTArray() const;
	// returns value as double
	double GetValueAsDouble() const;
	// returns indexed value as double
	double GetValueAsDoubleIndexed(int Index) const;
	// returns value as double array
	double* GetValueAsDoubleArray() const;
	// returns value as tarray of doubles
	TArray<double> GetValueAsDoubleTArray() const;
	// returns value as float
	float GetValueAsFloat() const;
	// returns value as fstring
	FString GetValueAsString() const;
	// returns value as fstring array
	TArray<FString> GetValueAsStringArray() const;
	// returns value as texture pointer
	UTexture* GetValueAsTexture() const;
	// returns value as a CHOP value
	UTouchEngineCHOP* GetValueAsCHOP() const;
	// returns value as a CHOP value and fills out the names array
	UTouchEngineCHOP* GetValueAsCHOP(UTouchEngineInfo* EngineInfo) const;
	// returns value as a DAT value
	UTouchEngineDAT* GetValueAsDAT() const;

	// get void pointer directly
	void* GetValue() const { return Value; }

	// set value as boolean
	void SetValue(bool InValue);
	// set value as integer
	void SetValue(int InValue);
	// sets value as integer array
	void SetValue(const TArray<int>& InValue);
	// set value as double
	void SetValue(double InValue);
	// set value as double array
	void SetValue(const TArray<double>& InValue);
	// set value as float
	void SetValue(float InValue);
	// set value as float array
	void SetValue(const TArray<float>& InValue);
	// set value as chop data
	void SetValue(UTouchEngineCHOP* InValue);
	// set value as chop data from float array
	void SetValueAsCHOP(const TArray<float>& InValue, int NumChannels, int NumSamples);
	// set value as dat data
	void SetValue(UTouchEngineDAT* InValue);
	// set value as dat data from fstring array
	void SetValueAsDAT(const TArray<FString>& InValue, int NumRows, int NumColumns);
	// set value as fstring
	void SetValue(const FString& InValue);
	// set value as fstring array
	void SetValue(const TArray<FString>& InValue);
	// set value as texture pointer
	void SetValue(UTexture* InValue);
	// set value from other dynamic variable
	void SetValue(const FTouchEngineDynamicVariableStruct* Other);

private:

	// sets void pointer to UObject pointer, does not copy memory
	void SetValue(UObject* InValue, size_t InSize);


	// Callbacks

	/** Handles check box state changed */
	void HandleChecked(ECheckBoxState InState);
	/** Handles value from Numeric Entry box changed */
	template <typename T>
	void HandleValueChanged(T InValue);
	/** Handles value from Numeric Entry box changed with array index*/
	template <typename T>
	void HandleValueChangedWithIndex(T InValue, int32 Index);
	/** Handles changing the value in the editable text box. */
	void HandleTextBoxTextChanged(const FText& NewText);
	/** Handles committing the text in the editable text box. */
	void HandleTextBoxTextCommitted(const FText& NewText);
	/** Handles changing the texture value in the render target 2D widget */
	void HandleTextureChanged();
	/** Handles changing the value from the color picker widget */
	void HandleColorChanged();
	/** Handles changing the value from the vector2 widget */
	void HandleVector2Changed();
	/** Handles changing the value from the vector widget */
	void HandleVectorChanged();
	/** Handles changing the value from the vector4 widget */
	void HandleVector4Changed();
	/** Handles changing the value from the int vector2 widget */
	void HandleIntVector2Changed();
	/** Handles changing the value from the int vector widget */
	void HandleIntVectorChanged();
	/** Handles changing the value from the int vector4 widget */
	void HandleIntVector4Changed();
	/** Handles adding / removing a child property in the float array widget */
	void HandleFloatBufferChanged();
	/** Handles changing the value of a child property in the array widget */
	void HandleFloatBufferChildChanged();
	/** Handles adding / removing a child property in the string array widget */
	void HandleStringArrayChanged();
	/** Handles changing the value of a child property in the string array widget */
	void HandleStringArrayChildChanged();
	/** Handles changing the value of a property through a drop down menu */
	void HandleDropDownBoxValueChanged(TSharedPtr<FString> Arg);

public:

	/** Function called when serializing this struct to a FArchive */
	bool Serialize(FArchive& Ar);
	/** Comparer function for two Dynamic Variables */
	bool Identical(const FTouchEngineDynamicVariableStruct* Other, uint32 PortFlags) const;

	/** Sends the input value to the engine info */
	void SendInput(UTouchEngineInfo* EngineInfo);
	/** Updates the output value from the engine info */
	void GetOutput(UTouchEngineInfo* EngineInfo);

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

public:
	FTouchEngineDynamicVariableContainer();
	~FTouchEngineDynamicVariableContainer();

	// Input variables
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault), Category = "Properties")
	TArray<FTouchEngineDynamicVariableStruct> DynVars_Input;

	// Output variables
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault), Category = "Properties")
	TArray<FTouchEngineDynamicVariableStruct> DynVars_Output;

	// Parent TouchEngine Component
	UPROPERTY(EditAnywhere, Category = "Properties")
	TObjectPtr<UTouchEngineComponentBase> Parent = nullptr;

	// Delegate for when tox is loaded in TouchEngine instance
	FTouchOnLoadComplete OnToxLoaded;
	// Delegate for when tox fails to load in TouchEngine instance
	FTouchOnLoadFailed OnToxFailedLoad;
	// Delegate for when this is destroyed (used to ensure we don't access this after garbage collection)
	FSimpleDelegate OnDestruction;

	// Calls or binds "OnToxLoaded" delegate based on whether it is already bound or not
	FDelegateHandle CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate Delegate);
	// Unbinds the "OnToxLoaded" delegate
	void Unbind_OnToxLoaded(FDelegateHandle Handle);
	// Calls or binds "OnToxFailedLoad" delegate based on whether it is already bound or not
	FDelegateHandle CallOrBind_OnToxFailedLoad(FTouchOnLoadFailed::FDelegate Delegate);
	// Unbinds the "OnToxFailedLoad" delegate
	void Unbind_OnToxFailedLoad(FDelegateHandle Handle);
	// Callback function attached to parent component's TouchEngine parameters loaded delegate
	void ToxParametersLoaded(const TArray<FTouchEngineDynamicVariableStruct>& VariablesIn, const TArray<FTouchEngineDynamicVariableStruct>& VariablesOut);

	void ValidateParameters(const TArray<FTouchEngineDynamicVariableStruct>& VariablesIn, const TArray<FTouchEngineDynamicVariableStruct>& VariablesOut);
	// Callback function attached to parent component's TouchEngine tox failed load delegate
	void ToxFailedLoad(const FString& Error);

	// Sends all input variables to the engine info
	void SendInputs(UTouchEngineInfo* EngineInfo);
	// Updates all outputs from the engine info
	void GetOutputs(UTouchEngineInfo* EngineInfo);
	// Sends input variable at index to the engine info
	void SendInput(UTouchEngineInfo* EngineInfo, int32 Index);
	// Updates output variable at index from the engine info
	void GetOutput(UTouchEngineInfo* EngineInfo, int32 Index);
	// Returns a dynamic variable with the passed in name if it exists
	FTouchEngineDynamicVariableStruct* GetDynamicVariableByName(FString VarName);
	// Returns a dynamic variable with the passed in identifier if it exists
	FTouchEngineDynamicVariableStruct* GetDynamicVariableByIdentifier(FString VarIdentifier);


	/** Function called when serializing this struct to a FArchive */
	bool Serialize(FArchive& Ar);
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
