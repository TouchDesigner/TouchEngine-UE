// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TouchEngineDynamicVariableStruct.generated.h"

class UTouchEngineComponentBase;
class UTouchEngineInfo;
enum class ECheckBoxState : uint8;

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


/*
* Dynamic variable - holds a void pointer and functions to cast it correctly
*/
USTRUCT(meta = (NoResetToDefault))
struct TOUCHENGINE_API FTouchEngineDynamicVariable
{
	GENERATED_BODY()

	friend class TouchEngineDynamicVariableStructDetailsCustomization;
	friend class UTouchEngine;

public:
	FTouchEngineDynamicVariable();
	~FTouchEngineDynamicVariable();
	FTouchEngineDynamicVariable(FTouchEngineDynamicVariable&& Other) { Copy(&Other); }
	FTouchEngineDynamicVariable(const FTouchEngineDynamicVariable& Other) { Copy(&Other); }
	FTouchEngineDynamicVariable& operator=(FTouchEngineDynamicVariable&& Other) { Copy(&Other); return *this; }
	FTouchEngineDynamicVariable& operator=(const FTouchEngineDynamicVariable& Other) { Copy(&Other); return *this; }

	void Copy(const FTouchEngineDynamicVariable* other);

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
	TArray<float> GetValueAsFloatBuffer() const;
	// get void pointer directly
	void* GetValue() const { return value; }
	// override for unimplemented types
	template <typename T>
	T GetValueAs() const { return (T)(*value); }

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
	// set value as fstring
	void SetValue(FString _value);
	// set value as fstring array
	void SetValue(TArray<FString> _value);
	// set value as texture pointer
	void SetValue(UTexture* _value);
	// set value from other dynamic variable
	void SetValue(const FTouchEngineDynamicVariable* other);

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
	bool Identical(const FTouchEngineDynamicVariable* Other, uint32 PortFlags) const;

	//bool ExportTextItem(FString& ValueStr, FTouchDynamicVar const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;

	//bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);


	/** Sends the input value to the engine info */
	void SendInput(UTouchEngineInfo* engineInfo);
	/** Updates the output value from the engine info */
	void GetOutput(UTouchEngineInfo* engineInfo);

};

// Template declaration to tell the serializer to use a custom serializer function. This is done so we can save the void pointer
// data as the correct variable type and read the correct size and type when re-launching the engine
template<>
struct TStructOpsTypeTraits<FTouchEngineDynamicVariable> : public TStructOpsTypeTraitsBase2<FTouchEngineDynamicVariable>
{
	enum
	{
		WithCopy = true,			// struct can be copied via its copy assignment operator.
		WithIdentical = true,		// struct can be compared via an Identical(const T* Other, uint32 PortFlags) function.  This should be mutually exclusive with WithIdenticalViaEquality.
		WithSerializer = true,		// struct has a Serialize function for serializing its state to an FArchive.
		//WithExportTextItem = true,	// struct has an ExportTextItem function used to serialize its state into a string.
		//WithImportTextItem = true,	// struct has an ImportTextItem function used to deserialize a string into an object of that class.
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
	TArray<FTouchEngineDynamicVariable> DynVars_Input;
	// Output variables
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault))
	TArray<FTouchEngineDynamicVariable> DynVars_Output;

	// Parent TouchEngine Component
	UPROPERTY(EditAnywhere)
	UTouchEngineComponentBase* parent = nullptr;
	// Delegate for when tox is loaded in TouchEngine instance
	FTouchOnLoadComplete OnToxLoaded;
	// Delegate for when tox fails to load in TouchEngine instance
	FTouchOnLoadFailed OnToxFailedLoad;

	// Calls or binds "OnToxLoaded" delegate based on whether it is already bound or not
	FDelegateHandle CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate Delegate);
	// Unbinds the "OnToxLoaded" delegate
	void Unbind_OnToxLoaded(FDelegateHandle Handle);
	// Calls or binds "OnToxFailedLoad" delegate based on whether it is already bound or not
	FDelegateHandle CallOrBind_OnToxFailedLoad(FSimpleMulticastDelegate::FDelegate Delegate);
	// Unbinds the "OnToxFailedLoad" delegate
	void Unbind_OnToxFailedLoad(FDelegateHandle Handle);
	// Callback function attached to parent component's TouchEngine tox loaded delegate 
	void ToxLoaded();
	// Callback function attached to parent component's TouchEngine parameters loaded dlegate
	void ToxParametersLoaded(TArray<FTouchEngineDynamicVariable> variablesIn, TArray<FTouchEngineDynamicVariable> variablesOut);
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
	FTouchEngineDynamicVariable* GetDynamicVariableByName(FString varName);
	// Returns a dynamic variable with the passed in identifier if it exists
	FTouchEngineDynamicVariable* GetDynamicVariableByIdentifier(FString varIdentifier);


	/** Function called when serializing this struct to a FArchive */
	bool Serialize(FArchive& Ar);
};

template<>
struct TStructOpsTypeTraits<FTouchEngineDynamicVariableContainer> : public TStructOpsTypeTraitsBase2<FTouchEngineDynamicVariableContainer>
{
	enum
	{
		//WithCopy = true,			// struct can be copied via its copy assignment operator.
		//WithIdentical = true,		// struct can be compared via an Identical(const T* Other, uint32 PortFlags) function.  This should be mutually exclusive with WithIdenticalViaEquality.
		//WithSerializer = true,		// struct has a Serialize function for serializing its state to an FArchive.
		//WithExportTextItem = true,	// struct has an ExportTextItem function used to serialize its state into a string.
		//WithImportTextItem = true,	// struct has an ImportTextItem function used to deserialize a string into an object of that class.
	};
};

// Templated function definitions

template<typename T>
inline void FTouchEngineDynamicVariable::HandleValueChanged(T inValue)
{
	FTouchEngineDynamicVariable oldValue; oldValue.Copy(this);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("changed from %d to %d"), oldValue.GetValueAsInt(), (int)inValue));

	SetValue(inValue);
	
	/*
	if (parentComponent->HasAnyFlags(RF_ArchetypeObject))
		UpdateInstances(blueprintOwner, parentComponent, oldValue);
	*/
}

template <typename T>
inline void FTouchEngineDynamicVariable::HandleValueChangedWithIndex(T inValue, int index)
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