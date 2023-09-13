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
#include "Misc/Variant.h"
#include "TouchEngineDynamicVariableStruct.generated.h"

struct FTouchEngineInputFrameData;

namespace UE
{
	namespace TouchEngine
	{
		class FTouchVariableManager;
	}
}

class UTexture;
class UTouchEngineComponentBase;
class UTouchEngineInfo;
enum class ECheckBoxState : uint8;
struct FTouchEngineCHOP;

/*
* possible intents of dynamic variables based on TEScope
*/
UENUM(meta = (NoResetToDefault))
enum class EVarScope
{
	Input,
	Output,
	Parameter,
	Max				UMETA(Hidden)
};

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

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|DAT")
	TArray<FString> GetRow(int32 Row);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|DAT")
	TArray<FString> GetRowByName(const FString& RowName);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|DAT")
	TArray<FString> GetColumn(int32 Column);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|DAT")
	TArray<FString> GetColumnByName(const FString& ColumnName);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|DAT")
	FString GetCell(int32 Column, int32 Row);

	UFUNCTION(BlueprintCallable, Category = "TouchEngine|DAT")
	FString GetCellByName(const FString& ColumnName, const FString& RowName);

	void CreateChannels(const TArray<FString>& AppendedArray, int32 RowCount, int32 ColumnCount);

private:
	TArray<FString> ValuesAppended;
};

template<> struct TVariantTraits<TArray<double>>
{
	/** Only returns EVariantTypes::Custom. Be mindful that other TVariantTraits might use it */
	static CONSTEXPR EVariantTypes GetType() { return EVariantTypes::Custom; }
};
template<> struct TVariantTraits<TArray<float>>
{
	/** Only returns EVariantTypes::Custom. Be mindful that other TVariantTraits might use it */
	static CONSTEXPR EVariantTypes GetType() { return EVariantTypes::Custom; }
};
template<> struct TVariantTraits<TArray<int>>
{
	/** Only returns EVariantTypes::Custom. Be mindful that other TVariantTraits might use it */
	static CONSTEXPR EVariantTypes GetType() { return EVariantTypes::Custom; }
};

namespace UE
{
	namespace TouchEngine
	{
		namespace DynamicVariable
		{
			template <typename T>
			T GetValueFromDynamicVariable(const struct FTouchEngineDynamicVariableStruct& DynVar)
			{
				check(0); // Undefined implementation
				return T(0);
			}
			template <>
			bool GetValueFromDynamicVariable(const struct FTouchEngineDynamicVariableStruct& DynVar);
			template <>
			int GetValueFromDynamicVariable(const struct FTouchEngineDynamicVariableStruct& DynVar);
			template <>
			float GetValueFromDynamicVariable(const struct FTouchEngineDynamicVariableStruct& DynVar);
			template <>
			double GetValueFromDynamicVariable(const struct FTouchEngineDynamicVariableStruct& DynVar);
			template <>
			TArray<int> GetValueFromDynamicVariable(const struct FTouchEngineDynamicVariableStruct& DynVar);
			template <>
			TArray<float> GetValueFromDynamicVariable(const struct FTouchEngineDynamicVariableStruct& DynVar);
			template <>
			TArray<double> GetValueFromDynamicVariable(const struct FTouchEngineDynamicVariableStruct& DynVar);
			template <>
			FLinearColor GetValueFromDynamicVariable(const struct FTouchEngineDynamicVariableStruct& DynVar);
			template <>
			FColor GetValueFromDynamicVariable(const struct FTouchEngineDynamicVariableStruct& DynVar);
			template <>
			FString GetValueFromDynamicVariable(const struct FTouchEngineDynamicVariableStruct& DynVar);

			template <typename T>
			T GetClampedValue(const T& InValue, const struct FTouchEngineDynamicVariableStruct& DynVar);
			template <>
			FLinearColor GetClampedValue(const FLinearColor& InValue, const struct FTouchEngineDynamicVariableStruct& DynVar);
		}
	}
}

/*
* Dynamic variable - holds a void pointer and functions to cast it correctly
*/
USTRUCT(BlueprintType, meta = (NoResetToDefault))
struct TOUCHENGINE_API FTouchEngineDynamicVariableStruct
{
	GENERATED_BODY()
	friend class FTouchEngineDynamicVariableStructDetailsCustomization;
	friend class FTouchEngineParserUtils;
	friend struct FTouchEngineDynamicVariableContainer;

	FTouchEngineDynamicVariableStruct() = default;
	~FTouchEngineDynamicVariableStruct();
	FTouchEngineDynamicVariableStruct(FTouchEngineDynamicVariableStruct&& Other) noexcept { Copy(&Other); }
	FTouchEngineDynamicVariableStruct(const FTouchEngineDynamicVariableStruct& Other) { Copy(&Other); }
	FTouchEngineDynamicVariableStruct& operator=(FTouchEngineDynamicVariableStruct&& Other) noexcept { Copy(&Other); return *this; }
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
	EVarType VarType = EVarType::NotSet; //todo: this should be a const variable, a DynamicVariable should not be able to change type

	// Variable intent
	UPROPERTY(EditAnywhere, Category = "Properties")
	EVarIntent VarIntent = EVarIntent::NotSet;

	// Number of variables (if array)
	UPROPERTY(EditAnywhere, Category = "Properties")
	int32 Count = 0;
	
	// Name of the Channels set by this instance
	UPROPERTY(VisibleAnywhere, Category = "Properties")
	TArray<FString> ChannelNames;

	/** Indicates if we should reuse the last texture we sent (if any) for better performance. This would imply that the content of the Texture (e.g. the pixels) has not changed. */
	UPROPERTY(Transient)
	bool bReuseExistingTexture_DEPRECATED = false;

	// Pointer to variable value
	void* Value = nullptr;
	size_t Size = 0; // todo: Is the size necessary? Almost never used

	FVariant MinValue;
	FVariant MaxValue;
	FVariant DefaultValue;
	
	UPROPERTY() //we need to save if this is an array or not
	bool bIsArray = false;

	/** Used for Pulse type of inputs, will be set to true if the current variable need to be reset to false after cooking it. */
	UPROPERTY(Transient)
	bool bNeedBoolReset = false;
	
	/** Used to keep track when the variable was last updated. The value should be -1 if it was never updated, and is only updated in GetOutput / SetFrameLastUpdatedFromNextCookFrame */
	UPROPERTY(Transient)
	int64 FrameLastUpdated = -1;

	bool IsInputVariable() const { return VarName.StartsWith("i/"); }
	bool IsOutputVariable() const { return VarName.StartsWith("o/"); }
	bool IsParameterVariable() const { return VarName.StartsWith("p/"); }

	template <typename T>
	T GetValue() const
	{
		return UE::TouchEngine::DynamicVariable::GetValueFromDynamicVariable<T>(*this);
	}
	template <typename T>
	T GetValue(int Index) const
	{
		const TArray<T> Values = UE::TouchEngine::DynamicVariable::GetValueFromDynamicVariable<TArray<T>>(*this);
		return Values.IsValidIndex(Index) ? Values[Index] : T {};
	}
	
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
	double GetValueAsFloatIndexed(int Index) const;
	float* GetValueAsFloatArray() const;
	TArray<float> GetValueAsFloatTArray() const;
	FColor GetValueAsColor() const { return GetValueAsLinearColor().QuantizeRound(); }
	FLinearColor GetValueAsLinearColor() const;

	FString GetValueAsString() const;
	TArray<FString> GetValueAsStringArray() const;
	UTexture* GetValueAsTexture() const;
	UDEPRECATED_TouchEngineCHOPMinimal* GetValueAsCHOP_DEPRECATED() const;
	FTouchEngineCHOP GetValueAsCHOP() const;
	FTouchEngineCHOP GetValueAsCHOP(const UTouchEngineInfo* EngineInfo) const;
	UTouchEngineDAT* GetValueAsDAT() const;

	/** Return a value clamped by Min and Max if available*/
	template <typename T>
	inline T GetClampedValue(const T& InValue) const
	{
		return UE::TouchEngine::DynamicVariable::GetClampedValue<T>(InValue, *this);
	}
	template <typename T>
	inline T GetClampedValue(const T& InValue, int Index) const
	{
		T ClampedValue = InValue;
		if (!MinValue.IsEmpty() && MinValue.GetType() == TVariantTraits<TArray<T>>::GetType())
		{
			TArray<T> MinValues = MinValue.GetValue<TArray<T>>();
			if (MinValues.IsValidIndex(Index))
			{
				ClampedValue = FMath::Max(ClampedValue, MinValues[Index]);
			}
		}
		if (!MaxValue.IsEmpty() && MaxValue.GetType() == TVariantTraits<TArray<T>>::GetType())
		{
			TArray<T> MaxValues = MaxValue.GetValue<TArray<T>>();
			if (MaxValues.IsValidIndex(Index))
			{
				ClampedValue = FMath::Min(ClampedValue, MaxValues[Index]);
			}
		}
		return ClampedValue;
	}
	/**
	 * Returns the Default Value Array by calling DefaultValue.GetValue<TArray<T>>, and ensure that the returned array is the same size as the current Count.
	 * If there are more values than Count, the values will be removed from the back
	 * If there are less values than Count, the current values will be added
	 * If there are no default values for this variable, it will be equal to the current values
	*/
	template <typename T>
	inline TArray<T> GetDefaultValueArrayMatchingCount() const
	{
		TArray<T> DefaultValues = DefaultValue.IsEmpty() ? TArray<T>() : DefaultValue.GetValue<TArray<T>>();
		while (DefaultValues.Num() < Count)
		{
			DefaultValues.Add(GetValue<T>(DefaultValues.Num()));
		}
		while (DefaultValues.Num() > Count)
		{
			DefaultValues.RemoveAt(DefaultValues.Num() - 1);
		}
		return DefaultValues;
	}


	void SetValue(bool InValue);
	void SetValue(int InValue);
	void SetValue(const TArray<int>& InValue);
	void SetValue(double InValue);
	void SetValue(const TArray<double>& InValue);
	void SetValue(float InValue);
	void SetValue(const TArray<float>& InValue);
	void SetValue(const FColor& InValue) { SetValue(InValue.ReinterpretAsLinear()); }
	void SetValue(const FLinearColor& InValue) { SetValue(TArray<float>{InValue.R, InValue.G, InValue.B, InValue.A});}
	void SetValue(const FVector& InValue) { SetValue(TArray<double>{InValue.X, InValue.Y, InValue.Z});}
	void SetValue(const FTouchEngineCHOP& InValue);
	void SetValueAsCHOP(const TArray<float>& InValue, int NumChannels, int NumSamples);
	void SetValueAsCHOP(const TArray<float>& InValue, const TArray<FString>& InChannelNames);
	void SetValue(const UTouchEngineDAT* InValue);
	void SetValueAsDAT(const TArray<FString>& InValue, int NumRows, int NumColumns);
	void SetValue(const FString& InValue);
	void SetValue(const TArray<FString>& InValue);
	void SetValue(UTexture* InValue);
	void SetValue(const FTouchEngineDynamicVariableStruct* Other);
	
	void SetFrameLastUpdatedFromNextCookFrame(const UTouchEngineInfo* EngineInfo);

	bool HasSameValue(const FTouchEngineDynamicVariableStruct* Other) const;
	template <typename T>
	inline bool HasSameValueT(const T& InValue) const
	{
		return GetValue<T>() == InValue;
	}
	template <typename T>
	inline bool HasSameValueT(const T& InValue, int Index) const
	{
		TArray<T> CurrentValue = GetValue<TArray<T>>();
		return CurrentValue.IsValidIndex(Index) ? CurrentValue[Index] == InValue : false;
	}


	/** Return true if DefaultValue is valid and is different from the current value of a type for which this is defined */
	bool CanResetToDefault() const; // Could be made a template
	bool CanResetToDefault(int Index) const; // Could be made a template
	void ResetToDefault(); // Could be made a template
	void ResetToDefault(int Index); // Could be made a template
	
	/** Function called when serializing this struct to a FArchive */
	bool Serialize(FArchive& Ar);
	/** Function called when copying the object, exporting the Value as string */
	FString ExportValue(const EPropertyPortFlags PortFlags = PPF_Delimited) const;
	/**
	 * Function called when pasting the object, importing the Value from a string.
	 * @returns Buffer pointer advanced by the number of characters consumed when reading the text value, or nullptr if an error occured
	 */
	const TCHAR* ImportValue(const TCHAR* Buffer, const EPropertyPortFlags PortFlags = PPF_Delimited, FOutputDevice* ErrorText = reinterpret_cast<FOutputDevice*>(GWarn));

private:
	/**
	 * Exports a single value (as well as the default/min/max values) as string by calling FProperty::ExportTextItem_Direct. Used for copying/duplicating
	 * @tparam TProperty An FProperty able to export the given TValue
	 * @tparam TValue The type of the given value
	 * @param OutStr The string to export the property to
	 * @param InValue The value of the property
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 */
	template <typename TProperty, typename TValue>
	void ExportSingleValueWithDefaults(FString& OutStr, const TValue& InValue, const EPropertyPortFlags PortFlags) const;
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
	 * Exports an array of values (as well as the default/min/max values) as string by calling FArrayProperty::ExportTextInnerItem. Used for copying/duplicating
	 * @tparam TProperty An FProperty able to export a given array item
	 * @tparam TValue The inner type of the given array
	 * @param OutStr The string to export the property to
	 * @param InArray A pointer to the array
	 * @param InNumItems The number of  in the array
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 */
	template <typename TProperty, typename TValue>
	void ExportArrayValuesWithDefaults(FString& OutStr, const TArray<TValue>& InArray, const EPropertyPortFlags PortFlags) const;
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
	 * Imports a single value (as well as the default/min/max values) from the given buffer by calling FProperty::ImportText_Direct and calls SetValue is successful. Used for pasting/duplicating
	 * @tparam TProperty An FProperty able to import the given TValue
	 * @tparam TValue The type of the imported value
	 * @param Buffer Text representing the property value
	 * @param OutValue The value imported which will be passed to SetValue
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 * @param ErrorText Output device for throwing warnings or errors on import
	 * @returns Buffer pointer advanced by the number of characters consumed when reading the text value. Returns nullptr if unable to read the value
	 */
	template <typename TProperty, typename TValue>
	const TCHAR* ImportAndSetSingleValueWithDefaults(const TCHAR* Buffer, TValue& OutValue, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText = nullptr);
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
	 * Imports a TArray of values (as well as the default/min/max values) from the given buffer by calling FProperty::ImportText_Direct and calls SetValue is successful. Used for pasting/duplicating
	 * @tparam TProperty An FProperty able to import a given array item
	 * @tparam TValue The inner type of the given array
	 * @param Buffer Text representing the property value
	 * @param OutArray The value imported which will be passed to SetValue
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 * @param ErrorText Output device for throwing warnings or errors on import
	 * @returns Buffer pointer advanced by the number of characters consumed when reading the text value. Returns nullptr if unable to read the value
	 */
	template <typename TProperty, typename TValue>
	static const TCHAR* ImportArrayValues(const TCHAR* Buffer, TArray<TValue>& OutArray, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText = nullptr);
	/**
	 * Imports a TArray of values (as well as the default/min/max values) from the given buffer by calling FProperty::ImportText_Direct and calls SetValue is successful. Used for pasting/duplicating
	 * @tparam TProperty An FProperty able to import a given array item
	 * @tparam TValue The inner type of the given array
	 * @param Buffer Text representing the property value
	 * @param OutArray The value imported which will be passed to SetValue
	 * @param PortFlags Flags controlling the behavior when exporting the value
	 * @param ErrorText Output device for throwing warnings or errors on import
	 * @returns Buffer pointer advanced by the number of characters consumed when reading the text value. Returns nullptr if unable to read the value
	 */
	template <typename TProperty, typename TValue>
	const TCHAR* ImportAndSetArrayValuesWithDefaults(const TCHAR* Buffer, TArray<TValue>& OutArray, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText = nullptr);
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
	void SendInput(const UTouchEngineInfo* EngineInfo, const FTouchEngineInputFrameData& FrameData);
	/** Sends the input value to the VariableManager directly */
	void SendInput(UE::TouchEngine::FTouchVariableManager& VariableManager, const FTouchEngineInputFrameData& FrameData);

	/** Updates the output value from the engine info */
	void GetOutput(const UTouchEngineInfo* EngineInfo);

	/** Tooltip text for this parameter / input / output when shown in the display panel */
	FText GetTooltip() const;

private:
#if WITH_EDITORONLY_DATA
	
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

	UPROPERTY(EditAnywhere, Category = "Handle Creators", Transient)
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

#endif
	UPROPERTY()
	TMap<FString, int32> DropDownData_DEPRECATED = TMap<FString, int32>(); // todo: this doesn't seem to need to be a Map, and Array could be enough

	struct FDropDownEntry
	{
		int32 Index;
		FString Value;
		FString Label;
	};
	TArray<FDropDownEntry> DropDownData;

	// sets void pointer to UObject pointer, does not copy memory
	void SetValue(UObject* InValue, size_t InSize);
	void Clear();


#if WITH_EDITORONLY_DATA
	// Callbacks

	void HandleChecked(ECheckBoxState InState, const UTouchEngineInfo* EngineInfo);

	template <typename T>
	void HandleValueChanged(T InValue, const UTouchEngineInfo* EngineInfo);
	template <typename T>
	void HandleValueChangedWithIndex(T InValue, int32 Index, const UTouchEngineInfo* EngineInfo);

	void HandleTextBoxTextCommitted(const FText& NewText, const UTouchEngineInfo* EngineInfo);
	void HandleTextureChanged(const UTouchEngineInfo* EngineInfo);
	void HandleFloatBufferChanged(const UTouchEngineInfo* EngineInfo);
	void HandleStringArrayChanged(const UTouchEngineInfo* EngineInfo);
	void HandleDropDownBoxValueChanged(const TSharedPtr<FString>& Arg, const UTouchEngineInfo* EngineInfo);
#endif
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

	void SendInputs(const UTouchEngineInfo* EngineInfo, const FTouchEngineInputFrameData& FrameData);
	void SendInputs(UE::TouchEngine::FTouchVariableManager& VariableManager, const FTouchEngineInputFrameData& FrameData);
	void GetOutputs(const UTouchEngineInfo* EngineInfo);
	
	void SetupForFirstCook();

	/**
	 * This function will return a new FTouchEngineDynamicVariableContainer with a copy of the inputs that have changed this frame, and no outputs.
	 * This will also reset any Pulse variable to their default values
	 */
	TMap<FString, FTouchEngineDynamicVariableStruct> CopyInputsForCook(int64 CurrentFrameID);
	
	FTouchEngineDynamicVariableStruct* GetDynamicVariableByName(const FString& VarName);
	FTouchEngineDynamicVariableStruct* GetDynamicVariableByIdentifier(const FString& VarIdentifier);
};

// Templated function definitions


template <>
inline bool UE::TouchEngine::DynamicVariable::GetValueFromDynamicVariable<bool>(const FTouchEngineDynamicVariableStruct& DynVar)
{
	return DynVar.GetValueAsBool();
}

template <>
inline int UE::TouchEngine::DynamicVariable::GetValueFromDynamicVariable<int>(const FTouchEngineDynamicVariableStruct& DynVar)
{
	return DynVar.GetValueAsInt();
}
template <>
inline double UE::TouchEngine::DynamicVariable::GetValueFromDynamicVariable<double>(const FTouchEngineDynamicVariableStruct& DynVar)
{
	return DynVar.GetValueAsDouble();
}
template <>
inline float UE::TouchEngine::DynamicVariable::GetValueFromDynamicVariable<float>(const FTouchEngineDynamicVariableStruct& DynVar)
{
	return DynVar.GetValueAsFloat();
}
template <>
inline TArray<int> UE::TouchEngine::DynamicVariable::GetValueFromDynamicVariable<TArray<int>>(const FTouchEngineDynamicVariableStruct& DynVar)
{
	return DynVar.GetValueAsIntTArray();
}
template <>
inline TArray<float> UE::TouchEngine::DynamicVariable::GetValueFromDynamicVariable<TArray<float>>(const FTouchEngineDynamicVariableStruct& DynVar)
{
	return DynVar.GetValueAsFloatTArray();
}
template <>
inline TArray<double> UE::TouchEngine::DynamicVariable::GetValueFromDynamicVariable<TArray<double>>(const FTouchEngineDynamicVariableStruct& DynVar)
{
	return DynVar.GetValueAsDoubleTArray();
}
template <>
inline FLinearColor UE::TouchEngine::DynamicVariable::GetValueFromDynamicVariable(const FTouchEngineDynamicVariableStruct& DynVar)
{
	return DynVar.GetValueAsLinearColor();
}
template <>
inline FColor UE::TouchEngine::DynamicVariable::GetValueFromDynamicVariable(const FTouchEngineDynamicVariableStruct& DynVar)
{
	return DynVar.GetValueAsColor();
}
template <>
inline FString UE::TouchEngine::DynamicVariable::GetValueFromDynamicVariable(const FTouchEngineDynamicVariableStruct& DynVar)
{
	return DynVar.GetValueAsString();
}

template <typename T>
inline T UE::TouchEngine::DynamicVariable::GetClampedValue(const T& InValue, const FTouchEngineDynamicVariableStruct& DynVar)
{
	T ClampedValue = InValue;
	if (!DynVar.MinValue.IsEmpty() && DynVar.MinValue.GetType() == TVariantTraits<T>::GetType())
	{
		ClampedValue = FMath::Max(ClampedValue, DynVar.MinValue.GetValue<T>());
	}
	if (!DynVar.MaxValue.IsEmpty() && DynVar.MaxValue.GetType() == TVariantTraits<T>::GetType())
	{
		ClampedValue = FMath::Min(ClampedValue, DynVar.MaxValue.GetValue<T>());
	}
	return ClampedValue;
}
template <>
inline FLinearColor UE::TouchEngine::DynamicVariable::GetClampedValue(const FLinearColor& InValue, const FTouchEngineDynamicVariableStruct& DynVar)
{
	FLinearColor ClampedValue = InValue;
	if (!DynVar.MinValue.IsEmpty())
	{
		if (DynVar.VarType == EVarType::Double && DynVar.MinValue.GetType() == TVariantTraits<TArray<double>>::GetType())
		{
			TArray<double> MinValues = DynVar.MinValue.GetValue<TArray<double>>();
			ClampedValue.R = MinValues.IsValidIndex(0) ? FMath::Max(ClampedValue.R, MinValues[0]) : ClampedValue.R;
			ClampedValue.G = MinValues.IsValidIndex(1) ? FMath::Max(ClampedValue.G, MinValues[1]) : ClampedValue.G;
			ClampedValue.B = MinValues.IsValidIndex(2) ? FMath::Max(ClampedValue.B, MinValues[2]) : ClampedValue.B;
			ClampedValue.A = MinValues.IsValidIndex(3) ? FMath::Max(ClampedValue.A, MinValues[3]) : 0.f;
		}
		else if (DynVar.VarType == EVarType::Float && DynVar.MinValue.GetType() == TVariantTraits<TArray<float>>::GetType())
		{
			TArray<float> MinValues = DynVar.MinValue.GetValue<TArray<float>>();
			ClampedValue.R = MinValues.IsValidIndex(0) ? FMath::Max(ClampedValue.R, MinValues[0]) : ClampedValue.R;
			ClampedValue.G = MinValues.IsValidIndex(1) ? FMath::Max(ClampedValue.G, MinValues[1]) : ClampedValue.G;
			ClampedValue.B = MinValues.IsValidIndex(2) ? FMath::Max(ClampedValue.B, MinValues[2]) : ClampedValue.B;
			ClampedValue.A = MinValues.IsValidIndex(3) ? FMath::Max(ClampedValue.A, MinValues[3]) : 0.f;
		}
	}
	if (!DynVar.MaxValue.IsEmpty())
	{
		if (DynVar.VarType == EVarType::Double && DynVar.MaxValue.GetType() == TVariantTraits<TArray<double>>::GetType())
		{
			TArray<double> MaxValues = DynVar.MaxValue.GetValue<TArray<double>>();
			ClampedValue.R = MaxValues.IsValidIndex(0) ? FMath::Min(ClampedValue.R, MaxValues[0]) : ClampedValue.R;
			ClampedValue.G = MaxValues.IsValidIndex(1) ? FMath::Min(ClampedValue.G, MaxValues[1]) : ClampedValue.G;
			ClampedValue.B = MaxValues.IsValidIndex(2) ? FMath::Min(ClampedValue.B, MaxValues[2]) : ClampedValue.B;
			ClampedValue.A = MaxValues.IsValidIndex(3) ? FMath::Min(ClampedValue.A, MaxValues[3]) : 1.f;
		}
		else if (DynVar.VarType == EVarType::Float && DynVar.MaxValue.GetType() == TVariantTraits<TArray<float>>::GetType())
		{
			TArray<float> MaxValues = DynVar.MaxValue.GetValue<TArray<float>>();
			ClampedValue.R = MaxValues.IsValidIndex(0) ? FMath::Min(ClampedValue.R, MaxValues[0]) : ClampedValue.R;
			ClampedValue.G = MaxValues.IsValidIndex(1) ? FMath::Min(ClampedValue.G, MaxValues[1]) : ClampedValue.G;
			ClampedValue.B = MaxValues.IsValidIndex(2) ? FMath::Min(ClampedValue.B, MaxValues[2]) : ClampedValue.B;
			ClampedValue.A = MaxValues.IsValidIndex(3) ? FMath::Min(ClampedValue.A, MaxValues[3]) : 1.f;
		}
	}
	return ClampedValue;
}


/** Temporary structure used to import and export the Minimum, Maximum, Default Value and Value of a Dynamic Variable */
USTRUCT()
struct FExportedValue
{
	GENERATED_BODY()

	UPROPERTY()
	FString Value;
	UPROPERTY()
	bool bHasDefault = false;
	UPROPERTY()
	FString DefaultValue;
	UPROPERTY()
	bool bHasMin = false;
	UPROPERTY()
	FString MinValue;
	UPROPERTY()
	bool bHasMax = false;
	UPROPERTY()
	FString MaxValue;
};

template <typename TProperty, typename TValue>
void FTouchEngineDynamicVariableStruct::ExportSingleValueWithDefaults(FString& OutStr, const TValue& InValue, const EPropertyPortFlags PortFlags) const
{
	FExportedValue ExportedValue;
	ExportSingleValue<TProperty>(ExportedValue.Value, InValue, PortFlags);

	if (TVariantTraits<TValue>::GetType() != EVariantTypes::Empty)
	{
		if (!DefaultValue.IsEmpty() && DefaultValue.GetType() == TVariantTraits<TValue>::GetType())
		{
			ExportedValue.bHasDefault = true;
			ExportSingleValue<TProperty>(ExportedValue.DefaultValue, DefaultValue.GetValue<TValue>(), PortFlags);
		}
		if (!MinValue.IsEmpty() && MinValue.GetType() == TVariantTraits<TValue>::GetType())
		{
			ExportedValue.bHasMin = true;
			ExportSingleValue<TProperty>(ExportedValue.MinValue, MinValue.GetValue<TValue>(), PortFlags);
		}
		if (!MaxValue.IsEmpty() && MaxValue.GetType() == TVariantTraits<TValue>::GetType())
		{
			ExportedValue.bHasMax = true;
			ExportSingleValue<TProperty>(ExportedValue.MaxValue, MaxValue.GetValue<TValue>(), PortFlags);
		}
	}
	ExportStruct(OutStr, ExportedValue, PortFlags);
}

template <typename TProperty, typename TValue>
void FTouchEngineDynamicVariableStruct::ExportSingleValue(FString& OutStr, const TValue& InValue, const EPropertyPortFlags PortFlags)
{
	const TProperty* PropertyCDO = GetDefault<TProperty>();
	check(PropertyCDO);
	PropertyCDO->ExportTextItem_Direct(OutStr, &InValue, nullptr /*DefaultValue*/, nullptr /*Parent*/, PortFlags, nullptr /*ExportRootScope*/);
}

template <typename TProperty, typename TValue>
void FTouchEngineDynamicVariableStruct::ExportArrayValuesWithDefaults(FString& OutStr, const TArray<TValue>& InArray, const EPropertyPortFlags PortFlags) const
{
	FExportedValue ExportedValue;
	ExportArrayValues<TProperty>(ExportedValue.Value, InArray, PortFlags);

	if (TVariantTraits<TValue>::GetType() != EVariantTypes::Empty)
	{
		if (!DefaultValue.IsEmpty() && DefaultValue.GetType() == TVariantTraits<TArray<TValue>>::GetType())
		{
			ExportedValue.bHasDefault = true;
			ExportArrayValues<TProperty>(ExportedValue.DefaultValue, DefaultValue.GetValue<TArray<TValue>>(), PortFlags);
		}
		if (!MinValue.IsEmpty() && MinValue.GetType() == TVariantTraits<TArray<TValue>>::GetType())
		{
			ExportedValue.bHasMin = true;
			ExportArrayValues<TProperty>(ExportedValue.MinValue, MinValue.GetValue<TArray<TValue>>(), PortFlags);
		}
		if (!MaxValue.IsEmpty() && MaxValue.GetType() == TVariantTraits<TArray<TValue>>::GetType())
		{
			ExportedValue.bHasMax = true;
			ExportArrayValues<TProperty>(ExportedValue.MaxValue, MaxValue.GetValue<TArray<TValue>>(), PortFlags);
		}
	}
	ExportStruct(OutStr, ExportedValue, PortFlags);
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
const TCHAR* FTouchEngineDynamicVariableStruct::ImportAndSetSingleValueWithDefaults(const TCHAR* Buffer, TValue& OutValue, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText)
{
	FExportedValue ExportedValue;
	UScriptStruct* ExportedValueClass = FExportedValue::StaticStruct();
	Buffer = ExportedValueClass->ImportText(Buffer, &ExportedValue, nullptr, PortFlags, ErrorText, ExportedValueClass->GetName());

	const TProperty* PropertyCDO = GetDefault<TProperty>();
	if (PropertyCDO->ImportText_Direct(*ExportedValue.Value, &OutValue, nullptr, PortFlags, ErrorText))
	{
		SetValue(OutValue);
	}

	if (TVariantTraits<TValue>::GetType() != EVariantTypes::Empty)
	{
		if (ExportedValue.bHasDefault)
		{
			TValue ExportedDefaultValue;
			if (PropertyCDO->ImportText_Direct(*ExportedValue.DefaultValue, &ExportedDefaultValue, nullptr, PortFlags, ErrorText))
			{
				DefaultValue = ExportedDefaultValue;
			}
		}
		if (ExportedValue.bHasMin)
		{
			TValue ExportedMinValue;
			if (PropertyCDO->ImportText_Direct(*ExportedValue.MinValue, &ExportedMinValue, nullptr, PortFlags, ErrorText))
			{
				MinValue = ExportedMinValue;
			}
		}
		if (ExportedValue.bHasMax)
		{
			TValue ExportedMaxValue;
			if (PropertyCDO->ImportText_Direct(*ExportedValue.MaxValue, &ExportedMaxValue, nullptr, PortFlags, ErrorText))
			{
				MaxValue = ExportedMaxValue;
			}
		}
	}
	return Buffer;
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
const TCHAR* FTouchEngineDynamicVariableStruct::ImportArrayValues(const TCHAR* Buffer, TArray<TValue>& OutArray, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText)
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
	return Buffer;
}

template <typename TProperty, typename TValue>
const TCHAR* FTouchEngineDynamicVariableStruct::ImportAndSetArrayValuesWithDefaults(const TCHAR* Buffer, TArray<TValue>& OutArray, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText)
{
	FExportedValue ExportedValue;
	UScriptStruct* ExportedValueClass = FExportedValue::StaticStruct();
	Buffer = ExportedValueClass->ImportText(Buffer, &ExportedValue, nullptr, PortFlags, ErrorText, ExportedValueClass->GetName());

	ImportAndSetArrayValues<TProperty, TValue>(*ExportedValue.Value, OutArray, PortFlags, ErrorText); // this also calls SetValue
	
	if (TVariantTraits<TValue>::GetType() != EVariantTypes::Empty)
	{
		if (ExportedValue.bHasDefault)
		{
			TArray<TValue> ExportedDefaultValue;
			// PropertyCDO->ImportText_Direct(*ExportedValue.DefaultValue, &ExportedDefaultValue, nullptr, PortFlags, ErrorText);
			if (ImportArrayValues<TProperty, TValue>(*ExportedValue.DefaultValue, ExportedDefaultValue, PortFlags, ErrorText))
			{
				DefaultValue = ExportedDefaultValue;
			}
		}
		if (ExportedValue.bHasMin)
		{
			TArray<TValue> ExportedMinValue;
			// PropertyCDO->ImportText_Direct(*ExportedValue.MinValue, &ExportedMinValue, nullptr, PortFlags, ErrorText);
			if (ImportArrayValues<TProperty, TValue>(*ExportedValue.MinValue, ExportedMinValue, PortFlags, ErrorText))
			{
				MinValue = ExportedMinValue;
			}
		}
		if (ExportedValue.bHasMax)
		{
			TArray<TValue> ExportedMaxValue;
			// PropertyCDO->ImportText_Direct(*ExportedValue.MaxValue, &ExportedMaxValue, nullptr, PortFlags, ErrorText);
			if (ImportArrayValues<TProperty, TValue>(*ExportedValue.MaxValue, ExportedMaxValue, PortFlags, ErrorText))
			{
				MaxValue = ExportedMaxValue;
			}
		}
	}
	return Buffer;
}

template <typename TProperty, typename TValue>
const TCHAR* FTouchEngineDynamicVariableStruct::ImportAndSetArrayValues(const TCHAR* Buffer, TArray<TValue>& OutArray, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText)
{
	Buffer = ImportArrayValues<TProperty, TValue>(Buffer, OutArray, PortFlags, ErrorText);
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

#if WITH_EDITORONLY_DATA
template<typename T>
void FTouchEngineDynamicVariableStruct::HandleValueChanged(T InValue, const UTouchEngineInfo* EngineInfo)
{
	SetValue(InValue);
	SetFrameLastUpdatedFromNextCookFrame(EngineInfo);
}

template <typename T>
void FTouchEngineDynamicVariableStruct::HandleValueChangedWithIndex(T InValue, int32 Index, const UTouchEngineInfo* EngineInfo)
{
	if (!Value)
	{
		// if the value doesn't exist,
		Value = new T[Count];
		Size = sizeof(T) * Count;
	}

	static_cast<T*>(Value)[Index] = InValue;
	SetFrameLastUpdatedFromNextCookFrame(EngineInfo);
}
#endif
