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
#include "Engine/TouchVariables.h"
#include "TouchEngineDynamicVariableStruct.generated.h"

class UTexture;
class UTouchEngineComponentBase;
class UTouchEngineInfo;
enum class ECheckBoxState : uint8;
struct FTouchEngineCHOP;

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

// Used to be UTouchEngineCHOP which was replaced by FTouchEngineCHOP and renamed for name clash reasons. It is kept due to Serialization
UCLASS(Deprecated)
class TOUCHENGINE_API UDEPRECATED_TouchEngineCHOPMinimal : public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY()
	int32 NumChannels;
	UPROPERTY()
	int32 NumSamples;
	UPROPERTY()
	TArray<FString> ChannelNames;
	
	TArray<float> GetChannel(int32 Index) const;
	void FillFromCHOP(const FTouchEngineCHOP& CHOP);
	FTouchEngineCHOP ToCHOP() const;
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
	
	// Name of the Channels set by this instance
	UPROPERTY(VisibleAnywhere, Category = "Properties")
	TArray<FString> ChannelNames;

	// Pointer to variable value
	void* Value = nullptr;
	size_t Size = 0; // todo: Is the size necessary? Almost never used

	UPROPERTY() //we need to save if this is an array or not
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
	float* GetValueAsFloatArray() const;
	FString GetValueAsString() const;
	TArray<FString> GetValueAsStringArray() const;
	UTexture* GetValueAsTexture() const;
	UDEPRECATED_TouchEngineCHOPMinimal* GetValueAsCHOP_DEPRECATED() const;
	FTouchEngineCHOP GetValueAsCHOP() const;
	FTouchEngineCHOP GetValueAsCHOP(const UTouchEngineInfo* EngineInfo) const;
	UTouchEngineDAT* GetValueAsDAT() const;

	void SetValue(bool InValue);
	void SetValue(int InValue);
	void SetValue(const TArray<int>& InValue);
	void SetValue(double InValue);
	void SetValue(const TArray<double>& InValue);
	void SetValue(float InValue);
	void SetValue(const TArray<float>& InValue);
	void SetValue(const FTouchEngineCHOP& InValue);
	void SetValueAsCHOP(const TArray<float>& InValue, int NumChannels, int NumSamples);
	void SetValueAsCHOP(const TArray<float>& InValue, const TArray<FString>& InChannelNames);
	void SetValue(const UTouchEngineDAT* InValue);
	void SetValueAsDAT(const TArray<FString>& InValue, int NumRows, int NumColumns);
	void SetValue(const FString& InValue);
	void SetValue(const TArray<FString>& InValue);
	void SetValue(UTexture* InValue);
	void SetValue(const FTouchEngineDynamicVariableStruct* Other);

	/** Function called when serializing this struct to a FArchive */
	bool Serialize(FArchive& Ar);
	/** Function called when copying the object, exporting the Value as string */
	FString ExportValue(const EPropertyPortFlags PortFlags = PPF_Delimited) const;
	/**
	 * Function called when pasting the object, importing the Value from a string.
	 * @returns Buffer pointer advanced by the number of characters consumed when reading the text value, or nullptr if an error occured
	 */
	const TCHAR* ImportValue(const TCHAR* Buffer, const EPropertyPortFlags PortFlags = PPF_Delimited, FOutputDevice* ErrorText = (FOutputDevice*)GWarn);

private:
	/**
	 * Exports a single value as string by calling FProperty::ExportTextItem_Direct. Used for copying/duplicating
	 * @tparam TProperty An FProperty able to export the given TValue
	 * @tparam TValue The type of the given value
	 * @param OutStr The string to export the property to
	 * @param InValue The value of the property
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 */
	template <typename TProperty, typename TValue>
	static void ExportSingleValue(FString& OutStr, const TValue& InValue, const EPropertyPortFlags PortFlags);
	/**
	 * Exports an array of values as string by calling FArrayProperty::ExportTextInnerItem. Used for copying/duplicating
	 * @tparam TProperty An FProperty able to export a given array item
	 * @tparam TValue The inner type of the given array
	 * @param OutStr The string to export the property to
	 * @param InArray A pointer to the array
	 * @param InNumItems The number of  in the array
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 */
	template <typename TProperty, typename TValue>
	static void ExportArrayValues(FString& OutStr, const TValue* InArray, int32 InNumItems, const EPropertyPortFlags PortFlags);
	/**
	 * Exports a TArray of values as string by calling FProperty::ExportTextItem_Direct. Used for copying/duplicating
	 * @tparam TProperty An FProperty able to export a given array item
	 * @tparam TValue The inner type of the given array
	 * @param OutStr The string to export the property to
	 * @param InArray The TArray
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 */
	template <typename TProperty, typename TValue>
	static void ExportArrayValues(FString& OutStr, const TArray<TValue>& InArray, const EPropertyPortFlags PortFlags);
	/**
	 * Exports a UStruct value as string by calling UStructClass::ExportText. Used for copying/duplicating
	 * @tparam TValue The UStruct type
	 * @param OutStr The string to export the property to
	 * @param InValue The value of the property
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 */
	template <typename TValue>
	static void ExportStruct(FString& OutStr, const TValue& InValue, const EPropertyPortFlags PortFlags);

	/**
	 * Imports a single value from the given buffer by calling FProperty::ImportText_Direct and calls SetValue is successful. Used for pasting/duplicating
	 * @tparam TProperty An FProperty able to import the given TValue
	 * @tparam TValue The type of the imported value
	 * @param Buffer Text representing the property value
	 * @param OutValue The value imported which will be passed to SetValue
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 * @param ErrorText Output device for throwing warnings or errors on import
	 * @returns Buffer pointer advanced by the number of characters consumed when reading the text value. Returns nullptr if unable to read the value
	 */
	template <typename TProperty, typename TValue>
	const TCHAR* ImportAndSetSingleValue(const TCHAR* Buffer, TValue& OutValue, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText = nullptr);
	/**
	 * Imports a TArray of values from the given buffer by calling FProperty::ImportText_Direct and calls SetValue is successful. Used for pasting/duplicating
	 * @tparam TProperty An FProperty able to import a given array item
	 * @tparam TValue The inner type of the given array
	 * @param Buffer Text representing the property value
	 * @param OutArray The value imported which will be passed to SetValue
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 * @param ErrorText Output device for throwing warnings or errors on import
	 * @returns Buffer pointer advanced by the number of characters consumed when reading the text value. Returns nullptr if unable to read the value
	 */
	template <typename TProperty, typename TValue>
	const TCHAR* ImportAndSetArrayValues(const TCHAR* Buffer, TArray<TValue>& OutArray, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText = nullptr);
	/**
	 * Imports a UStruct value from the given buffer by calling UStructClass::ImportText and calls SetValue is successful. Used for pasting/duplicating
	 * @tparam TValue The UStruct type
	 * @param Buffer Text representing the property value
	 * @param OutValue The value imported which will be passed to SetValue
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 * @param ErrorText Output device for throwing warnings or errors on import
	 * @returns Buffer pointer advanced by the number of characters consumed when reading the text value. Returns nullptr if unable to read the value
	 */
	template <typename TValue>
	const TCHAR* ImportAndSetStruct(const TCHAR* Buffer, TValue& OutValue, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText = nullptr);
	/**
	 * Imports a UStruct value from the given buffer by calling UStructClass::ImportText and calls SetValue is successful. Used for pasting/duplicating
	 * @tparam TValue The UClass type
	 * @param Buffer Text representing the property value
	 * @param OutValue The value imported which will be passed to SetValue
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 * @param ErrorText Output device for throwing warnings or errors on import
	 * @returns Buffer pointer advanced by the number of characters consumed when reading the text value. Returns nullptr if unable to read the value
	 */
	template <typename TValue>
	const TCHAR* ImportAndSetUObject(const TCHAR* Buffer, TValue* OutValue, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText = nullptr);

public:	
	/** Comparer function for two Dynamic Variables */
	bool Identical(const FTouchEngineDynamicVariableStruct* Other, uint32 PortFlags) const;

	/** Sends the input value to the engine info */
	void SendInput(UTouchEngineInfo* EngineInfo);
	/** Updates the output value from the engine info */
	void GetOutput(UTouchEngineInfo* EngineInfo);

	/** Tooltip text for this parameter / input / output when shown in the display panel */
	FText GetTooltip() const;

private:
#if WITH_EDITORONLY_DATA

	// these properties exist to generate the property handles and to be a go between for the editor functions and the void pointer value

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault), Transient)
	FTouchEngineCHOP CHOPProperty;
	
	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault), Transient)
	TArray<float> FloatBufferProperty = TArray<float>();

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault), Transient)
	TArray<FString> StringArrayProperty = TArray<FString>();

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault), Transient)
	TObjectPtr<UTexture> TextureProperty = nullptr;


	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault), Transient)
	FVector2D Vector2DProperty = FVector2D::Zero();

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault), Transient)
	FVector VectorProperty = FVector::Zero();

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault, NoSpinbox), Transient)
	FVector4 Vector4Property = FVector4::Zero();

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault), Transient)
	FColor ColorProperty = FColor::Black;


	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault), Transient)
	FIntPoint IntPointProperty = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault), Transient)
	FIntVector IntVectorProperty = FIntVector::ZeroValue;

	UPROPERTY(EditAnywhere, Category = "Handle Creators", meta = (NoResetToDefault), Transient)
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
	void HandleStringArrayChanged();
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
	
	// void SendInput(UTouchEngineInfo* EngineInfo, int32 Index);
	// void GetOutput(UTouchEngineInfo* EngineInfo, int32 Index);
	
	FTouchEngineDynamicVariableStruct* GetDynamicVariableByName(const FString& VarName);
	FTouchEngineDynamicVariableStruct* GetDynamicVariableByIdentifier(const FString& VarIdentifier);
};

// Templated function definitions

template <typename TProperty, typename TValue>
void FTouchEngineDynamicVariableStruct::ExportSingleValue(FString& OutStr, const TValue& InValue, const EPropertyPortFlags PortFlags)
{
	const TProperty* PropertyCDO = GetDefault<TProperty>();
	check(PropertyCDO);
	PropertyCDO->ExportTextItem_Direct(OutStr, &InValue, nullptr /*DefaultValue*/, nullptr /*Parent*/, PortFlags, nullptr /*ExportRootScope*/);
}

template <typename TProperty, typename TValue>
void FTouchEngineDynamicVariableStruct::ExportArrayValues(FString& OutStr, const TValue* InArray, int32 InNumItems, const EPropertyPortFlags PortFlags)
{
	if (InArray)
	{
		const TProperty* InnerPropertyCDO = GetDefault<TProperty>();
		check(InnerPropertyCDO);

		const FArrayProperty* ArrayPropertyCDO = GetDefault<FArrayProperty>();
		ArrayPropertyCDO->ExportTextInnerItem(OutStr, InnerPropertyCDO, InArray, InNumItems, nullptr, 0, nullptr, PortFlags, nullptr);
	}
}

template <typename TProperty, typename TValue>
void FTouchEngineDynamicVariableStruct::ExportArrayValues(FString& OutStr, const TArray<TValue>& InArray, const EPropertyPortFlags PortFlags)
{
	const TProperty* InnerPropertyCDO = GetDefault<TProperty>();
	const FArrayProperty* ArrayPropertyCDO = GetDefault<FArrayProperty>();
	check(InnerPropertyCDO);

	// we cannot use the CDO as we need to set the inner property
	FArrayProperty ArrayProperty(nullptr, ArrayPropertyCDO->NamePrivate, ArrayPropertyCDO->GetFlags());
	// The InnerProperty must be created on the heap as this is deleted in the destructor FArrayProperty::~FArrayProperty
	TProperty* InnerProperty = new TProperty(nullptr, InnerPropertyCDO->NamePrivate, InnerPropertyCDO->GetFlags());
	ArrayProperty.AddCppProperty(InnerProperty); // needed for FScriptArrayHelper
	FScriptArrayHelper ArrayHelper(&ArrayProperty, &InArray);

	ArrayProperty.ExportTextInnerItem(OutStr, InnerProperty, ArrayHelper.GetRawPtr(0), ArrayHelper.Num(), nullptr, 0, nullptr, PortFlags, nullptr);
}

template <typename TValue>
void FTouchEngineDynamicVariableStruct::ExportStruct(FString& OutStr, const TValue& InValue, const EPropertyPortFlags PortFlags)
{
	const UScriptStruct* StructClass = InValue.StaticStruct();
	StructClass->ExportText(OutStr, &InValue, nullptr, nullptr, PortFlags, nullptr);
}


template <typename TProperty, typename TValue>
const TCHAR* FTouchEngineDynamicVariableStruct::ImportAndSetSingleValue(const TCHAR* Buffer, TValue& OutValue, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText)
{
	const TProperty* PropertyCDO = GetDefault<TProperty>();
	Buffer = PropertyCDO->ImportText_Direct(Buffer, &OutValue, nullptr, PortFlags, ErrorText);
	if (Buffer)
	{
		SetValue(OutValue);
	}
	return Buffer;
}

template <typename TProperty, typename TValue>
const TCHAR* FTouchEngineDynamicVariableStruct::ImportAndSetArrayValues(const TCHAR* Buffer, TArray<TValue>& OutArray, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText)
{
	const TProperty* InnerPropertyCDO = GetDefault<TProperty>();
	const FArrayProperty* ArrayPropertyCDO = GetDefault<FArrayProperty>();
	check(InnerPropertyCDO);

	// we cannot use the CDO as we need to set the inner property
	FArrayProperty ArrayProperty(nullptr, ArrayPropertyCDO->NamePrivate, ArrayPropertyCDO->GetFlags());
	// The InnerProperty must be created on the heap as this is deleted in the destructor FArrayProperty::~FArrayProperty
	TProperty* InnerProperty = new TProperty(nullptr, InnerPropertyCDO->NamePrivate, InnerPropertyCDO->GetFlags());
	ArrayProperty.AddCppProperty(InnerProperty); // needed for FScriptArrayHelper
	FScriptArrayHelper ArrayHelper(&ArrayProperty, &OutArray);
	Buffer = ArrayProperty.ImportTextInnerItem(Buffer, InnerProperty, ArrayHelper.GetRawPtr(0), PortFlags, nullptr, &ArrayHelper, ErrorText);
	if (Buffer)
	{
		SetValue(OutArray);
	}
	return Buffer;
}

template <typename TValue>
const TCHAR* FTouchEngineDynamicVariableStruct::ImportAndSetStruct(const TCHAR* Buffer, TValue& OutValue, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText)
{
	UScriptStruct* StructClass = OutValue.StaticStruct();
	Buffer = StructClass->ImportText(Buffer, &OutValue, nullptr, PortFlags, ErrorText, StructClass->GetName());
	if (Buffer)
	{
		SetValue(OutValue);
	}
	return Buffer;
}

template <typename TValue>
const TCHAR* FTouchEngineDynamicVariableStruct::ImportAndSetUObject(const TCHAR* Buffer, TValue* OutValue, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText)
{
	const FObjectPtrProperty* PropertyCDO = GetDefault<FObjectPtrProperty>();

	FObjectPtrProperty PtrProperty(nullptr, PropertyCDO->NamePrivate, PropertyCDO->GetFlags());
	PtrProperty.SetPropertyClass(TValue::StaticClass());
	Buffer = PtrProperty.ImportText_Direct(Buffer, &OutValue, nullptr, PortFlags, ErrorText);
	if (Buffer)
	{
		SetValue(OutValue);
	}
	return Buffer;
}

template<typename T>
void FTouchEngineDynamicVariableStruct::HandleValueChanged(T InValue)
{
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