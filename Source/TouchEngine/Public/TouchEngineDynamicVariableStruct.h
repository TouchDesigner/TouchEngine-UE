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

UCLASS(BlueprintType)
class TOUCHENGINE_API UTouchEngineCHOP : public UObject
{
	GENERATED_BODY()

public:

	UTouchEngineCHOP() {}
	~UTouchEngineCHOP();

	UPROPERTY(EditAnywhere)
	int channelCount;
	UPROPERTY(EditAnywhere)
	int channelSize;

	UFUNCTION(BlueprintCallable)
	TArray<float> GetChannel(int index);

	void CreateChannels(float** fullChannel, int _channelCount, int _channelSize);

	void CreateChannels(FTouchCHOPFull CHOP);

private:

	TArray<float> channelsAppended;
};


/*
* Dynamic variable - holds a void pointer and functions to cast it correctly
*/
USTRUCT(meta = (NoResetToDefault))
struct TOUCHENGINE_API FTEDynamicVariableStruct
{
	GENERATED_BODY()

	friend class TouchEngineDynamicVariableStructDetailsCustomization;
	friend class UTouchEngine;

public:
	FTEDynamicVariableStruct();
	~FTEDynamicVariableStruct();
	FTEDynamicVariableStruct(FTEDynamicVariableStruct&& Other) { Copy(&Other); }
	FTEDynamicVariableStruct(const FTEDynamicVariableStruct& Other) { Copy(&Other); }
	FTEDynamicVariableStruct& operator=(FTEDynamicVariableStruct&& Other) { Copy(&Other); return *this; }
	FTEDynamicVariableStruct& operator=(const FTEDynamicVariableStruct& Other) { Copy(&Other); return *this; }

	void Copy(const FTEDynamicVariableStruct* other);

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
		FColor colorProperty;
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
		FVector vectorProperty;
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
		FVector4 vector4Property;

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
	// returns value as integer in a TOptional struct
	TOptional<int> GetValueAsOptionalInt() const;
	// returns indexed value as integer
	int GetValueAsIntIndexed(int index) const;
	// returns indexed value as integer in a TOptional struct
	TOptional<int> GetIndexedValueAsOptionalInt(int index) const;
	// returns value as integer array
	int* GetValueAsIntArray() const;
	// returns value as tarray of integers
	TArray<int> GetValueAsIntTArray() const;
	// returns value as double
	double GetValueAsDouble() const;
	// returns indexed value as double
	double GetValueAsDoubleIndexed(int index) const;
	// returns value as double in a TOptional struct
	TOptional<double> GetValueAsOptionalDouble() const;
	// returns indexed value as double in a TOptional struct
	TOptional<double> GetIndexedValueAsOptionalDouble(int index) const;
	// returns value as double array
	double* GetValueAsDoubleArray() const;
	// returns value as tarray of doubles
	TArray<double> GetValueAsDoubleTArray() const;
	// returns value as float
	float GetValueAsFloat() const;
	// returns value as float in a TOptional struct
	TOptional<float> GetValueAsOptionalFloat() const;
	// returns value as fstring
	FString GetValueAsString() const;
	// returns value as fstring array
	TArray<FString> GetValueAsStringArray() const;
	// returns value as texture pointer
	UTexture* GetValueAsTexture() const;
	// returns value as a tarray of floats
	UTouchEngineCHOP* GetValueAsFloatBuffer() const;
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
	// set value as float buffer
	void SetValue(UTouchEngineCHOP* _value);
	// set value as fstring
	void SetValue(FString _value);
	// set value as fstring array
	void SetValue(TArray<FString> _value);
	// set value as texture pointer
	void SetValue(UTexture* _value);
	// set value from other dynamic variable
	void SetValue(const FTEDynamicVariableStruct* other);

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
	/** Handles getting the text to be displayed in the editable text box. */
	FText HandleTextBoxText() const;
	/** Handles changing the value in the editable text box. */
	void HandleTextBoxTextChanged(const FText& NewText);
	/** Handles committing the text in the editable text box. */
	void HandleTextBoxTextCommited(const FText& NewText);
	/** Handles changing the texture value in the render target 2D widget */
	void HandleTextureChanged();
	/** Handles changing the value from the color picker widget */
	void HandleColorChanged();
	/** Handles changing the value from the vector4 widget */
	void HandleVector4Changed();
	/** Handles changing the value from the vector widget */
	void HandleVectorChanged();
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
	bool Identical(const FTEDynamicVariableStruct* Other, uint32 PortFlags) const;

	/** Sends the input value to the engine info */
	void SendInput(UTouchEngineInfo* engineInfo);
	/** Updates the output value from the engine info */
	void GetOutput(UTouchEngineInfo* engineInfo);

};

// Template declaration to tell the serializer to use a custom serializer function. This is done so we can save the void pointer
// data as the correct variable type and read the correct size and type when re-launching the engine
template<>
struct TStructOpsTypeTraits<FTEDynamicVariableStruct> : public TStructOpsTypeTraitsBase2<FTEDynamicVariableStruct>
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
DECLARE_MULTICAST_DELEGATE(FTouchOnLoadFailed);

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
	TArray<FTEDynamicVariableStruct> DynVars_Input;
	// Output variables
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault))
	TArray<FTEDynamicVariableStruct> DynVars_Output;

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
	FDelegateHandle CallOrBind_OnToxFailedLoad(FSimpleMulticastDelegate::FDelegate Delegate);
	// Unbinds the "OnToxFailedLoad" delegate
	void Unbind_OnToxFailedLoad(FDelegateHandle Handle);
	// Callback function attached to parent component's TouchEngine parameters loaded dlegate
	void ToxParametersLoaded(TArray<FTEDynamicVariableStruct> variablesIn, TArray<FTEDynamicVariableStruct> variablesOut);
	// Callback function attached to parent component's TouchEngine tox failed load delegate 
	void ToxFailedLoad();

	// Sends all input variables to the engine info
	void SendInputs(UTouchEngineInfo* engineInfo);
	// Updates all outputs from the engine info
	void GetOutputs(UTouchEngineInfo* engineInfo);
	// Sends input variable at index to the engine info
	void SendInput(UTouchEngineInfo* engineInfo, int index);
	// Updates output variable at index from the engine info
	void GetOutput(UTouchEngineInfo* engineInfo, int index);
	// Returns a dynamic variable with the passed in name if it exists
	FTEDynamicVariableStruct* GetDynamicVariableByName(FString varName);
	// Returns a dynamic variable with the passed in identifier if it exists
	FTEDynamicVariableStruct* GetDynamicVariableByIdentifier(FString varIdentifier);


	/** Function called when serializing this struct to a FArchive */
	bool Serialize(FArchive& Ar);
};

// Templated function definitions

template<typename T>
inline void FTEDynamicVariableStruct::HandleValueChanged(T inValue)
{
	FTEDynamicVariableStruct oldValue; oldValue.Copy(this);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("changed from %d to %d"), oldValue.GetValueAsInt(), (int)inValue));

	SetValue(inValue);
	
	/*
	if (parentComponent->HasAnyFlags(RF_ArchetypeObject))
		UpdateInstances(blueprintOwner, parentComponent, oldValue);
	*/
}

template <typename T>
inline void FTEDynamicVariableStruct::HandleValueChangedWithIndex(T inValue, int index)
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