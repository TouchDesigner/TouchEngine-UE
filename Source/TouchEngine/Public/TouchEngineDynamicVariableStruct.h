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
#include "Math/IntVector.h"
#include "TouchEngineIntVector4.h"
#include "TouchEngineDynamicVariableStruct.generated.h"

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
	VARTYPE_NOT_SET = 0,
	VARTYPE_BOOL,
	VARTYPE_INT,
	VARTYPE_DOUBLE,
	VARTYPE_FLOAT,
	VARTYPE_FLOATBUFFER,
	VARTYPE_STRING,
	VARTYPE_TEXTURE,
	VARTYPE_MAX
};

/*
* possible intents of dynamic variables based on TEParameterIntent
*/
UENUM(meta = (NoResetToDefault))
enum class EVarIntent
{
	VARINTENT_NOT_SET = 0,
	VARINTENT_DROPDOWN,
	VARINTENT_COLOR,
	VARINTENT_POSITION,
	VARINTENT_SIZE,
	VARINTENT_UVW,
	VARINTENT_FILEPATH,
	VARINTENT_DIRECTORYPATH,
	VARINTENT_MOMENTARY,
	VARINTENT_PULSE,
	VARINTENT_MAX
};

UCLASS(BlueprintType, meta = (DisplayName = "TouchEngine CHOP"))
class TOUCHENGINE_API UTouchEngineCHOP : public UObject
{
	GENERATED_BODY()

public:

	UTouchEngineCHOP() {}
	~UTouchEngineCHOP() {}

	UPROPERTY(BlueprintReadOnly)
	int numChannels;
	UPROPERTY(BlueprintReadOnly)
	int numSamples;

	TArray<FString> channelNames;

	UFUNCTION(BlueprintCallable)
		TArray<float> GetChannel(int index);

	UFUNCTION(BlueprintCallable)
		TArray<float> GetChannelByName(FString name);

	void CreateChannels(float** fullChannel, int _channelCount, int _channelSize);

	void CreateChannels(FTouchCHOPFull CHOP);

	UFUNCTION(BlueprintCallable)
	void Clear();

private:

	TArray<float> channelsAppended;
};

UCLASS(BlueprintType, meta = (DisplayName = "TouchEngine DAT"))
class TOUCHENGINE_API UTouchEngineDAT : public UObject
{
	GENERATED_BODY()

public:

	friend struct FTouchEngineDynamicVariableStruct;

	UTouchEngineDAT() {}
	~UTouchEngineDAT() {}

	UPROPERTY(BlueprintReadOnly)
	int numColumns;
	UPROPERTY(BlueprintReadOnly)
	int numRows;

	UFUNCTION(BlueprintCallable)
	TArray<FString> GetRow(int row);
	UFUNCTION(BlueprintCallable)
	TArray<FString> GetRowByName(FString rowName);
	UFUNCTION(BlueprintCallable)
	TArray<FString> GetColumn(int column);
	UFUNCTION(BlueprintCallable)
	TArray<FString> GetColumnByName(FString columnName);
	UFUNCTION(BlueprintCallable)
	FString GetCell(int column, int row);
	UFUNCTION(BlueprintCallable)
	FString GetCellByName(FString columnName, FString rowName);

	void CreateChannels(TArray<FString> appendedArray, int rowCount, int columnCount);

private:

	TArray<FString> valuesAppended;

};


/*
* Dynamic variable - holds a void pointer and functions to cast it correctly
*/
USTRUCT(meta = (NoResetToDefault))
struct TOUCHENGINE_API FTouchEngineDynamicVariableStruct
{
	GENERATED_BODY()

		friend class TouchEngineDynamicVariableStructDetailsCustomization;
	friend class UTouchEngine;

public:
	FTouchEngineDynamicVariableStruct();
	~FTouchEngineDynamicVariableStruct();
	FTouchEngineDynamicVariableStruct(FTouchEngineDynamicVariableStruct&& Other) { Copy(&Other); }
	FTouchEngineDynamicVariableStruct(const FTouchEngineDynamicVariableStruct& Other) { Copy(&Other); }
	FTouchEngineDynamicVariableStruct& operator=(FTouchEngineDynamicVariableStruct&& Other) { Copy(&Other); return *this; }
	FTouchEngineDynamicVariableStruct& operator=(const FTouchEngineDynamicVariableStruct& Other) { Copy(&Other); return *this; }

	void Copy(const FTouchEngineDynamicVariableStruct* other);

	// Display name of variable
	UPROPERTY(EditAnywhere)
		FString VarLabel = "ERROR_LABEL";
	// Name used to get / set variable by user 
	UPROPERTY(EditAnywhere)
		FString VarName = "ERROR_NAME";
	// random characters used to identify the variable in TouchEngine
	UPROPERTY(EditAnywhere)
		FString VarIdentifier = "ERROR_IDENTIFIER";
	// Variable data type
	UPROPERTY(EditAnywhere)
		EVarType VarType = EVarType::VARTYPE_NOT_SET;
	// Variable intent
	UPROPERTY(EditAnywhere)
		EVarIntent VarIntent = EVarIntent::VARINTENT_NOT_SET;
	// Number of variables (if array)
	UPROPERTY(EditAnywhere)
		int count = 0;
	// Pointer to variable value
	void* value = nullptr;
	// Byte size of variable
	size_t size = 0;
	// If the value is an array value
	bool isArray = false;

private:

#if WITH_EDITORONLY_DATA

	// these properties exist to generate the property handles and to be a go between between the editor functions and the void pointer value

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
		TArray<float> floatBufferProperty;
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
		TArray<FString> stringArrayProperty;
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
		UTexture* textureProperty = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
		FVector2D vector2DProperty;
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
		FVector vectorProperty;
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault, NoSpinbox))
		FVector4 vector4Property;
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
		FColor colorProperty;

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
		FIntPoint intPointProperty;
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
		FIntVector intVectorProperty;
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
		FTouchEngineIntVector4 intVector4Property = FTouchEngineIntVector4();


	UPROPERTY(EditAnywhere, Category = "Menu Data", meta = (NoResetToDefault))
		TMap<FString, int> dropDownData;

#endif

public:

	void Clear();

	// Get / Set Values

	// returns value as bool 
	bool GetValueAsBool() const;
	// returns value as integer
	int GetValueAsInt() const;
	// returns indexed value as integer
	int GetValueAsIntIndexed(int index) const;
	// returns value as integer array
	int* GetValueAsIntArray() const;
	// returns value as tarray of integers
	TArray<int> GetValueAsIntTArray() const;
	// returns value as double
	double GetValueAsDouble() const;
	// returns indexed value as double
	double GetValueAsDoubleIndexed(int index) const;
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
	UTouchEngineCHOP* GetValueAsCHOP(UTouchEngineInfo* engineInfo) const;
	// returns value as a DAT value
	UTouchEngineDAT* GetValueAsDAT() const;

	// get void pointer directly
	void* GetValue() const { return value; }

	// set value as boolean
	void SetValue(bool _value);
	// set value as integer
	void SetValue(int _value);
	// sets value as integer array
	void SetValue(TArray<int> _value);
	// set value as double
	void SetValue(double _value);
	// set value as double array
	void SetValue(TArray<double> _value);
	// set value as float
	void SetValue(float _value);
	// set value as float array
	void SetValue(TArray<float> _value);
	// set value as chop data
	void SetValue(UTouchEngineCHOP* _value);
	// set value as dat data
	void SetValue(UTouchEngineDAT* _value);
	// set value as fstring
	void SetValue(FString _value);
	// set value as fstring array
	void SetValue(TArray<FString> _value);
	// set value as texture pointer
	void SetValue(UTexture* _value);
	// set value from other dynamic variable
	void SetValue(const FTouchEngineDynamicVariableStruct* other);

private:

	// sets void pointer to UObject pointer, does not copy memory
	void SetValue(UObject* newValue, size_t _size);


	// Callbacks

	/** Handles check box state changed */
	void HandleChecked(ECheckBoxState InState);
	/** Handles value from Numeric Entry box changed */
	template <typename T>
	void HandleValueChanged(T inValue);
	/** Handles value from Numeric Entry box changed with array index*/
	template <typename T>
	void HandleValueChangedWithIndex(T inValue, int index);
	/** Handles changing the value in the editable text box. */
	void HandleTextBoxTextChanged(const FText& NewText);
	/** Handles committing the text in the editable text box. */
	void HandleTextBoxTextCommited(const FText& NewText);
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
	void HandleDropDownBoxValueChanged(TSharedPtr<FString> arg);

public:

	/** Function called when serializing this struct to a FArchive */
	bool Serialize(FArchive& Ar);
	/** Comparer function for two Dynamic Variables */
	bool Identical(const FTouchEngineDynamicVariableStruct* Other, uint32 PortFlags) const;

	/** Sends the input value to the engine info */
	void SendInput(UTouchEngineInfo* engineInfo);
	/** Updates the output value from the engine info */
	void GetOutput(UTouchEngineInfo* engineInfo);

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
DECLARE_MULTICAST_DELEGATE_OneParam(FTouchOnLoadFailed, FString);

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
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault))
		TArray<FTouchEngineDynamicVariableStruct> DynVars_Input;
	// Output variables
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault))
		TArray<FTouchEngineDynamicVariableStruct> DynVars_Output;

	// Parent TouchEngine Component
	UPROPERTY(EditAnywhere)
		UTouchEngineComponentBase* parent = nullptr;
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
	// Callback function attached to parent component's TouchEngine parameters loaded dlegate
	void ToxParametersLoaded(TArray<FTouchEngineDynamicVariableStruct> variablesIn, TArray<FTouchEngineDynamicVariableStruct> variablesOut);

	void ValidateParameters(TArray<FTouchEngineDynamicVariableStruct> variablesIn, TArray<FTouchEngineDynamicVariableStruct> variablesOut);
	// Callback function attached to parent component's TouchEngine tox failed load delegate 
	void ToxFailedLoad(FString error);

	// Sends all input variables to the engine info
	void SendInputs(UTouchEngineInfo* engineInfo);
	// Updates all outputs from the engine info
	void GetOutputs(UTouchEngineInfo* engineInfo);
	// Sends input variable at index to the engine info
	void SendInput(UTouchEngineInfo* engineInfo, int index);
	// Updates output variable at index from the engine info
	void GetOutput(UTouchEngineInfo* engineInfo, int index);
	// Returns a dynamic variable with the passed in name if it exists
	FTouchEngineDynamicVariableStruct* GetDynamicVariableByName(FString varName);
	// Returns a dynamic variable with the passed in identifier if it exists
	FTouchEngineDynamicVariableStruct* GetDynamicVariableByIdentifier(FString varIdentifier);


	/** Function called when serializing this struct to a FArchive */
	bool Serialize(FArchive& Ar);
};

// Templated function definitions

template<typename T>
inline void FTouchEngineDynamicVariableStruct::HandleValueChanged(T inValue)
{
	FTouchEngineDynamicVariableStruct oldValue; oldValue.Copy(this);

	SetValue(inValue);

	/*
	if (parentComponent->HasAnyFlags(RF_ArchetypeObject))
		UpdateInstances(blueprintOwner, parentComponent, oldValue);
	*/
}

template <typename T>
inline void FTouchEngineDynamicVariableStruct::HandleValueChangedWithIndex(T inValue, int index)
{
	if (!value)
	{
		// if the value doesn't exist, 
		value = new T[count];
		size = sizeof(T) * count;

		for (int i = 0; i < count; i++)
		{
			//value[i] = (T)0;
		}
	}

	//FTouchDynamicVar oldValue; oldValue.Copy(this);

	((T*)value)[index] = inValue;

	//UpdateInstances(blueprintOwner, parentComponent, oldValue);
}