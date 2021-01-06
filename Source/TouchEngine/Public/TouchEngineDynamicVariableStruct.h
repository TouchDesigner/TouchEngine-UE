// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UTouchEngine.h"
#include "TouchEngineDynamicVariableStruct.generated.h"

class UTouchEngineComponentBase;

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
* Dynamic variable - holds a void pointer and functions to cast it correctly
*/
USTRUCT(meta = (NoResetToDefault))
struct TOUCHENGINE_API FTouchEngineDynamicVariableStruct
{
	GENERATED_BODY()

	friend class TouchEngineDynamicVariableStructDetailsCustomization;


public:
	FTouchEngineDynamicVariableStruct() {}
	~FTouchEngineDynamicVariableStruct() {}

	// Display name of variable
	UPROPERTY(EditAnywhere)
	FString VarName = "ERROR_NAME";
	// Variable data type
	UPROPERTY(EditAnywhere)
	EVarType VarType = EVarType::VARTYPE_NOT_SET;
	// Number of variables (if array)
	UPROPERTY(EditAnywhere)
	int count = 0;
	// Pointer to variable value
	void* value = nullptr;
	// Byte size of variable
	size_t size = 0;

private:

#if WITH_EDITORONLY_DATA

	// these properties exist to generate the property handles and to be a go between between the editor functions and the void pointer value

	// Float Array Property
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
	TArray<float> floatBufferProperty;
	// Texture Property
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault))
	UTextureRenderTarget2D* textureProperty = nullptr;

#endif

	// Get / Set Values

public:

	// returns value as bool 
	bool GetValueAsBool() const										{ return *(bool*)value; }
	// returns value as integer
	int GetValueAsInt() const										{ return *(int*)value; }
	// returns value as double
	double GetValueAsDouble() const									{ return *(double*)value; }
	// returns value as float
	float GetValueAsFloat() const									{ return *(double*)value; }
	// returns value as fstring
	FString GetValueAsString() const								{ return *(FString*)value; }
	// returns value as render target 2D pointer
	UTextureRenderTarget2D* GetValueAsTextureRenderTarget() const	{ return (UTextureRenderTarget2D*)value; }
	// returns value as texture 2D pointer
	UTexture2D* GetValueAsTexture() const							{ return (UTexture2D*)value; }
	// returns value as a tarray of floats
	TArray<float> GetValueAsFloatBuffer() const;

	// get void pointer directly
	void* GetValue() { return value; }
	// override for unimplemented types
	template <typename T>
	T GetValueAs() { return (T)(*value); }

private:

	// sets void pointer to UObject pointer, does not copy memory
	void SetValue(UObject* newValue, size_t _size);
	// sets void pointer value via memcopy internally
	void SetValue(void* newValue, size_t _size);
	// Typeless call to auto set size of value
	template<typename T> 
	void SetValue(T _value) { SetValue(_value, sizeof(_value)); }

public:

	// set value as boolean
	void SetValue(bool _value)						{ if (VarType == EVarType::VARTYPE_BOOL)		SetValue((void*)&_value); }
	// set value as integer
	void SetValue(int _value)						{ if (VarType == EVarType::VARTYPE_INT)			SetValue((void*)&_value); }
	// set value as double
	void SetValue(double _value)					{ if (VarType == EVarType::VARTYPE_DOUBLE)		SetValue((void*)&_value); }
	// set value as float
	void SetValue(float _value)						{ if (VarType == EVarType::VARTYPE_FLOAT)		SetValue((void*)&_value); }
	// set value as float array
	void SetValue(float* _value)					{ if (VarType == EVarType::VARTYPE_FLOATBUFFER)	SetValue((void*)_value); }
	// set value as float array
	void SetValue(TArray<float> _value);
	// set value as fstring
	void SetValue(FString _value)					{ if (VarType == EVarType::VARTYPE_STRING)		SetValue((void*)&_value); }
	// set value as render target 2D pointer
	void SetValue(UTextureRenderTarget2D* _value);
	// set value as texture 2D pointer
	void SetValue(UTexture2D* _value);

	// Callbacks

	/** Handles check box state changed */
	void HandleChecked(ECheckBoxState InState);
	/** Handles value from Numeric Entry box changed */
	template <typename T>
	void HandleValueChanged(T inValue, ETextCommit::Type commitType) { SetValue(inValue); }
	/** Handles getting the text to be displayed in the editable text box. */
	FText HandleTextBoxText() const;
	/** Handles changing the value in the editable text box. */
	void HandleTextBoxTextChanged(const FText& NewText);
	/** Handles committing the text in the editable text box. */
	void HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo);
	/** Handles changing the texture value in the render target 2D widget */
	void HandleTextureChanged();
	/** Handles adding / removing a child property in the array widget */
	void HandleFloatBufferChanged();
	/** Handles changing the value of a child property in the array widget */
	void HandleFloatBufferChildChanged();

	/** Function called when serializing this struct to a FArchive */
	bool Serialize(FArchive& Ar);

	/** Sends the input value to the engine info */
	void SetInput(UTouchEngineInfo* engineInfo);
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
		WithSerializer = true,
	};
};




/**
 * Holds all input and output variables for an instance of the "UTouchEngineComponentBase" component class. 
 * Also holds callbacks from the touch engine to get info about when parameters are loaded
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

	// Parent Touch Engine Component
	UTouchEngineComponentBase* parent = nullptr;
	// Delegate for when tox is loaded in TouchEngine class
	FTouchOnLoadComplete OnToxLoaded;

	// Calls or binds "OnToxLoaded" delegate based on whether it is already bound or not
	FDelegateHandle CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate Delegate);
	void Unbind_OnToxLoaded(FDelegateHandle Handle);
	// Callback function attached to parent component's TouchEngine tox loaded delegate 
	void ToxLoaded();
	// Callback function attached to parent component's TouchEngine parameters loaded dlegate
	void ToxParametersLoaded(TArray<FTouchEngineDynamicVariableStruct> variablesIn, TArray<FTouchEngineDynamicVariableStruct> variablesOut);

	// Sends all input variables to the engine info
	void SetInputs(UTouchEngineInfo* engineInfo);
	// Updates all outputs from the engine info
	void GetOutputs(UTouchEngineInfo* engineInfo);
	// Sends input variable at index to the engine info
	void SetInput(UTouchEngineInfo* engineInfo, int index);
	// Updates output variable at index from the engine info
	void GetOutput(UTouchEngineInfo* engineInfo, int index);
};