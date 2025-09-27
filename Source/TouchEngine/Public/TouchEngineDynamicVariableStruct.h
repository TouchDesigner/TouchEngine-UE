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
#include "Rendering/Exporting/ExportedTouchTexture.h"
#include "Util/TouchHelpers.h"
#include "Util/TouchEngineStatsGroup.h"
#include "TouchEngineDynamicVariableStruct.generated.h"

struct FTouchEngineInputFrameData;

namespace UE
{
	namespace TouchEngine
	{
		class FTouchResourceProvider;
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
	NotSet = 0,
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
	Group,
	Sequence,
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
	static constexpr EVariantTypes GetType() { return EVariantTypes::Custom; }
};
template<> struct TVariantTraits<TArray<TOptional<double>>>
{
	/** Only returns EVariantTypes::Custom. Be mindful that other TVariantTraits might use it */
	static constexpr EVariantTypes GetType() { return EVariantTypes::Custom; }
};
template<> struct TVariantTraits<TArray<float>>
{
	/** Only returns EVariantTypes::Custom. Be mindful that other TVariantTraits might use it */
	static constexpr EVariantTypes GetType() { return EVariantTypes::Custom; }
};
template<> struct TVariantTraits<TArray<TOptional<float>>>
{
	/** Only returns EVariantTypes::Custom. Be mindful that other TVariantTraits might use it */
	static constexpr EVariantTypes GetType() { return EVariantTypes::Custom; }
};
template<> struct TVariantTraits<TArray<int>>
{
	/** Only returns EVariantTypes::Custom. Be mindful that other TVariantTraits might use it */
	static constexpr EVariantTypes GetType() { return EVariantTypes::Custom; }
};
template<> struct TVariantTraits<TArray<TOptional<int>>>
{
	/** Only returns EVariantTypes::Custom. Be mindful that other TVariantTraits might use it */
	static constexpr EVariantTypes GetType() { return EVariantTypes::Custom; }
};

namespace UE::TouchEngine::DynamicVariable
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
	UPROPERTY(VisibleAnywhere, Category = "Properties")
	FString VarLabel = "ERROR_LABEL";

	// Name used to get / set variable by user
	UPROPERTY(VisibleAnywhere, Category = "Properties")
	FString VarName = "ERROR_NAME";

	// random characters used to identify the variable in TouchEngine
	UPROPERTY(VisibleAnywhere, Category = "Properties")
	FString VarIdentifier = "ERROR_IDENTIFIER"; //todo: should this be a FName?

	/** The parent of this struct, usually a Group. Used for the UI. */
	UPROPERTY(VisibleAnywhere, Category = "Properties")
	FString ParentIdentifier = FString();

	UPROPERTY(VisibleAnywhere, Category = "Properties")
	EVarScope VarScope = EVarScope::NotSet;

	// Variable data type
	UPROPERTY(VisibleAnywhere, Category = "Properties")
	EVarType VarType = EVarType::NotSet; //todo: this should be a const variable, a DynamicVariable should not be able to change type. Check if possible with serialize

	// Variable intent
	UPROPERTY(VisibleAnywhere, Category = "Properties")
	EVarIntent VarIntent = EVarIntent::NotSet;

	// Number of variables (if array)
	UPROPERTY(VisibleAnywhere, Category = "Properties")
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

	/* The minimum value this variable should be able to have. Retrieved from TELinkValueMinimum and is equivalent to the clamp min in TouchDesigner */
	FVariant ClampMin;
	/* The maximum value this variable should be able to have. Retrieved from TELinkValueMaximum and is equivalent to the clamp max in TouchDesigner */
	FVariant ClampMax;
	/* The minimum value of the UI slider for this variable. Retrieved from TELinkValueUIMinimum and is equivalent to the range min in TouchDesigner */
	FVariant UIMin;
	/* The maximum value of the UI slider for this variable. Retrieved from TELinkValueUIMaximum and is equivalent to the range max in TouchDesigner */
	FVariant UIMax;
	/* The maximum value of this variable. Retrieved from TELinkValueDefault and is equivalent to the default value in TouchDesigner */
	FVariant DefaultValue;
	
	UPROPERTY() //we need to save if this is an array or not
	bool bIsArray = false;

	/** Used for Pulse type of inputs, will be set to true if the current variable need to be reset to false after cooking it. */
	UPROPERTY(Transient)
	bool bNeedBoolReset = false;
	
	/** Used to keep track when the variable was last updated. The value should be -1 if it was never updated, and is only updated in GetOutput / SetFrameLastUpdatedFromNextCookFrame */
	UPROPERTY(Transient)
	int64 FrameLastUpdated = -1;

	bool IsInputVariable() const { return VarScope == EVarScope::Input; }
	bool IsOutputVariable() const { return VarScope == EVarScope::Output; }
	bool IsParameterVariable() const { return  VarScope == EVarScope::Parameter; }
	/** Returns the variable name without the prefix */
	FString GetCleanVariableName() const;

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
	const TSharedPtr<UE::TouchEngine::FExportedTouchTexture>& GetExportedTexture() const;
	UDEPRECATED_TouchEngineCHOPMinimal* GetValueAsCHOP_DEPRECATED() const;
	FTouchEngineCHOP GetValueAsCHOP() const;
	FTouchEngineCHOP GetValueAsCHOP(const UTouchEngineInfo* EngineInfo) const;
	UTouchEngineDAT* GetValueAsDAT() const;

	/** Return a value clamped by ClampMin and ClampMax if available*/
	template <typename T>
	inline T GetClampedValue(const T& InValue) const
	{
		return UE::TouchEngine::DynamicVariable::GetClampedValue<T>(InValue, *this);
	}
	template <typename T>
	inline T GetClampedValue(const T& InValue, int Index) const
	{
		T ClampedValue = InValue;
		if (!ClampMin.IsEmpty() && ClampMin.GetType() == TVariantTraits<TArray<TOptional<T>>>::GetType())
		{
			TArray<TOptional<T>> MinValues = ClampMin.GetValue<TArray<TOptional<T>>>();
			if (MinValues.IsValidIndex(Index) && MinValues[Index].IsSet())
			{
				ClampedValue = FMath::Max(ClampedValue, MinValues[Index].GetValue());
			}
		}
		if (!ClampMax.IsEmpty() && ClampMax.GetType() == TVariantTraits<TArray<TOptional<T>>>::GetType())
		{
			TArray<TOptional<T>> MaxValues = ClampMax.GetValue<TArray<TOptional<T>>>();
			if (MaxValues.IsValidIndex(Index) && MaxValues[Index].IsSet())
			{
				ClampedValue = FMath::Min(ClampedValue, MaxValues[Index].GetValue());
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

	/** Sends the input value to the VariableManager directly */
	TFuture<bool> SendInput(UE::TouchEngine::FTouchVariableManager& VariableManager, const FTouchEngineInputFrameData& FrameData);

	/** Updates the output value from the engine info */
	void GetOutput(const UTouchEngineInfo* EngineInfo);

	/** Tooltip text for this parameter / input / output when shown in the display panel */
	FText GetTooltip() const;

private:
	// container used by DynVars to hold the Exported texture, which lets the pool know when all DynVars are done using this texture
	struct FExportedTouchTextureContainer
	{
		TSharedPtr<UE::TouchEngine::FExportedTouchTexture> Texture;
	};
	// The copy of the texture content ready to be exported to TouchEngine
	TSharedPtr<FExportedTouchTextureContainer> ExportedTexture;
	TWeakPtr<UE::TouchEngine::FTouchResourceProvider> WeakTouchResourceProvider;

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

	template<typename T>
	FString GetDefaultValueTooltipString() const
	{
		FString DefaultValueStr;
		if (!DefaultValue.IsEmpty())
		{
			if (!bIsArray && ensure(DefaultValue.GetType() == TVariantTraits<T>::GetType()))
			{
				if constexpr(std::is_same_v<T, int>)
				{
					DefaultValueStr = FString::FormatAsNumber(DefaultValue.GetValue<T>());
				}
				else
				{
					DefaultValueStr = FString::SanitizeFloat(DefaultValue.GetValue<T>());
				}
			}
			else if (ensure(DefaultValue.GetType() == TVariantTraits<TArray<T>>::GetType()))
			{
				const TArray<T> Data = DefaultValue.GetValue<TArray<T>>();
				DefaultValueStr = TEXT("[") + FString::JoinBy(Data, TEXT(", "), [](const T& Value)
				{
					if constexpr(std::is_same_v<T, int>)
					{
						return FString::FormatAsNumber(Value);
					}
					else
					{
						return FString::SanitizeFloat(Value);
					}
				}) + TEXT("]");
			}
		}
		return DefaultValueStr;
	}
	template<typename T>
	FString GetMinOrMaxTooltipString(const FVariant& MinOrMax) const
	{
		FString MinOrMaxValueStr;
		if (!MinOrMax.IsEmpty())
		{
			if (!bIsArray && ensure(MinOrMax.GetType() == TVariantTraits<T>::GetType()))
			{
				if constexpr(std::is_same_v<T, int>)
				{
					MinOrMaxValueStr = FString::FormatAsNumber(MinOrMax.GetValue<T>());
				}
				else
				{
					MinOrMaxValueStr = FString::SanitizeFloat(MinOrMax.GetValue<T>());
				}
			}
			else if (ensure(MinOrMax.GetType() == TVariantTraits<TArray<TOptional<T>>>::GetType()))
			{
				TArray<TOptional<T>> Data = MinOrMax.GetValue<TArray<TOptional<T>>>();
				bool bIsAnyValueSet = false;
				MinOrMaxValueStr = TEXT("[") + FString::JoinBy(Data, TEXT(", "), [&bIsAnyValueSet](const TOptional<T>& Value)
				{
					bIsAnyValueSet |= Value.IsSet();
					if constexpr(std::is_same_v<T, int>)
					{
						return Value.IsSet() ? FString::FormatAsNumber(Value.GetValue()) : TEXT("_ ");
					}
					else
					{
						return Value.IsSet() ? FString::SanitizeFloat(Value.GetValue()) : TEXT("_ ");
					}
				}) + TEXT("]");
				if (!bIsAnyValueSet)
				{
					MinOrMaxValueStr = {};
				}
			}
		}
		return MinOrMaxValueStr;
	}
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
	/* Copies the Default, Min, Max and Dropdown values from the passed in variables */
	void EnsureMetadataIsSet(const TArray<FTouchEngineDynamicVariableStruct>& VariablesIn);
	void Reset();

	void SendInputs(UE::TouchEngine::FTouchVariableManager& VariableManager, const FTouchEngineInputFrameData& FrameData);
	void GetOutputs(const UTouchEngineInfo* EngineInfo);
	
	void SetupForFirstCook(const TSharedPtr<UE::TouchEngine::FTouchResourceProvider>& TouchResourceProvider);

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
	if (!DynVar.ClampMin.IsEmpty() && DynVar.ClampMin.GetType() == TVariantTraits<T>::GetType())
	{
		ClampedValue = FMath::Max(ClampedValue, DynVar.ClampMin.GetValue<T>());
	}
	if (!DynVar.ClampMax.IsEmpty() && DynVar.ClampMax.GetType() == TVariantTraits<T>::GetType())
	{
		ClampedValue = FMath::Min(ClampedValue, DynVar.ClampMax.GetValue<T>());
	}
	return ClampedValue;
}
template <>
inline FLinearColor UE::TouchEngine::DynamicVariable::GetClampedValue(const FLinearColor& InValue, const FTouchEngineDynamicVariableStruct& DynVar)
{
	FLinearColor ClampedValue = InValue;
	if (!DynVar.ClampMin.IsEmpty())
	{
		if (DynVar.VarType == EVarType::Double && DynVar.ClampMin.GetType() == TVariantTraits<TArray<TOptional<double>>>::GetType())
		{
			TArray<TOptional<double>> MinValues = DynVar.ClampMin.GetValue<TArray<TOptional<double>>>();
			ClampedValue.R = MinValues.IsValidIndex(0) && MinValues[0].IsSet() ? FMath::Max(ClampedValue.R, MinValues[0].GetValue()) : ClampedValue.R;
			ClampedValue.G = MinValues.IsValidIndex(1) && MinValues[1].IsSet() ? FMath::Max(ClampedValue.G, MinValues[1].GetValue()) : ClampedValue.G;
			ClampedValue.B = MinValues.IsValidIndex(2) && MinValues[2].IsSet() ? FMath::Max(ClampedValue.B, MinValues[2].GetValue()) : ClampedValue.B;
			ClampedValue.A = FMath::Max(ClampedValue.A, MinValues.IsValidIndex(3) && MinValues[3].IsSet() ? MinValues[3].GetValue() : 0.0);
		}
		else if (DynVar.VarType == EVarType::Float && DynVar.ClampMin.GetType() == TVariantTraits<TArray<TOptional<float>>>::GetType())
		{
			TArray<TOptional<float>> MinValues = DynVar.ClampMin.GetValue<TArray<TOptional<float>>>();
			ClampedValue.R = MinValues.IsValidIndex(0) && MinValues[0].IsSet() ? FMath::Max(ClampedValue.R, MinValues[0].GetValue()) : ClampedValue.R;
			ClampedValue.G = MinValues.IsValidIndex(1) && MinValues[1].IsSet() ? FMath::Max(ClampedValue.G, MinValues[1].GetValue()) : ClampedValue.G;
			ClampedValue.B = MinValues.IsValidIndex(2) && MinValues[2].IsSet() ? FMath::Max(ClampedValue.B, MinValues[2].GetValue()) : ClampedValue.B;
			ClampedValue.A = FMath::Max(ClampedValue.A, MinValues.IsValidIndex(3) && MinValues[3].IsSet() ? MinValues[3].GetValue() : 0.f);
		}
	}
	if (!DynVar.ClampMax.IsEmpty())
	{
		if (DynVar.VarType == EVarType::Double && DynVar.ClampMax.GetType() == TVariantTraits<TArray<TOptional<double>>>::GetType())
		{
			TArray<TOptional<double>> MaxValues = DynVar.ClampMax.GetValue<TArray<TOptional<double>>>();
			ClampedValue.R = MaxValues.IsValidIndex(0) && MaxValues[0].IsSet() ? FMath::Min(ClampedValue.R, MaxValues[0].GetValue()) : ClampedValue.R;
			ClampedValue.G = MaxValues.IsValidIndex(1) && MaxValues[1].IsSet() ? FMath::Min(ClampedValue.G, MaxValues[1].GetValue()) : ClampedValue.G;
			ClampedValue.B = MaxValues.IsValidIndex(2) && MaxValues[2].IsSet() ? FMath::Min(ClampedValue.B, MaxValues[2].GetValue()) : ClampedValue.B;
			ClampedValue.A = FMath::Min(ClampedValue.A, MaxValues.IsValidIndex(3) && MaxValues[3].IsSet() ? MaxValues[3].GetValue() : 1.0);
		}
		else if (DynVar.VarType == EVarType::Float && DynVar.ClampMax.GetType() == TVariantTraits<TArray<TOptional<float>>>::GetType())
		{
			TArray<TOptional<float>> MaxValues = DynVar.ClampMax.GetValue<TArray<TOptional<float>>>();
			ClampedValue.R = MaxValues.IsValidIndex(0) && MaxValues[0].IsSet() ? FMath::Min(ClampedValue.R, MaxValues[0].GetValue()) : ClampedValue.R;
			ClampedValue.G = MaxValues.IsValidIndex(1) && MaxValues[1].IsSet() ? FMath::Min(ClampedValue.G, MaxValues[1].GetValue()) : ClampedValue.G;
			ClampedValue.B = MaxValues.IsValidIndex(2) && MaxValues[2].IsSet() ? FMath::Min(ClampedValue.B, MaxValues[2].GetValue()) : ClampedValue.B;
			ClampedValue.A = FMath::Min(ClampedValue.A, MaxValues.IsValidIndex(3) && MaxValues[3].IsSet() ? MaxValues[3].GetValue() : 1.f);
		}
	}
	return ClampedValue;
}


/** Temporary structure used to import and export the Minimum, Maximum, Default Value and Value of a Dynamic Variable */
USTRUCT() //todo
struct FExportedValue
{
	GENERATED_BODY()

	UPROPERTY()
	FString Value;
	UPROPERTY()
	FString DefaultValue{};
	UPROPERTY()
	FString ClampMinHasValue{};
	UPROPERTY()
	FString ClampMinValue{};
	UPROPERTY()
	FString ClampMaxHasValue{};
	UPROPERTY()
	FString ClampMaxValue{};
	UPROPERTY()
	FString UIMinHasValue{};
	UPROPERTY()
	FString UIMinValue{};
	UPROPERTY()
	FString UIMaxHasValue{};
	UPROPERTY()
	FString UIMaxValue{};
	UPROPERTY()
	TArray<int32> DropDownIndices;
	UPROPERTY()
	TArray<FString> DropDownValues;
	UPROPERTY()
	TArray<FString> DropDownLabels;
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
			ExportSingleValue<TProperty>(ExportedValue.DefaultValue, DefaultValue.GetValue<TValue>(), PortFlags);
		}
		if (!ClampMin.IsEmpty() && ClampMin.GetType() == TVariantTraits<TValue>::GetType())
		{
			ExportSingleValue<TProperty>(ExportedValue.ClampMinValue, ClampMin.GetValue<TValue>(), PortFlags);
		}
		if (!ClampMax.IsEmpty() && ClampMax.GetType() == TVariantTraits<TValue>::GetType())
		{
			ExportSingleValue<TProperty>(ExportedValue.ClampMaxValue, ClampMax.GetValue<TValue>(), PortFlags);
		}
		if (!UIMin.IsEmpty() && UIMin.GetType() == TVariantTraits<TValue>::GetType())
		{
			ExportSingleValue<TProperty>(ExportedValue.UIMinValue, UIMin.GetValue<TValue>(), PortFlags);
		}
		if (!UIMax.IsEmpty() && UIMax.GetType() == TVariantTraits<TValue>::GetType())
		{
			ExportSingleValue<TProperty>(ExportedValue.UIMaxValue, UIMax.GetValue<TValue>(), PortFlags);
		}
	}
	if (VarIntent == EVarIntent::DropDown && !DropDownData.IsEmpty())
	{
		for (const FDropDownEntry& Entry : DropDownData)
		{
			ExportedValue.DropDownIndices.Add(Entry.Index);
			ExportedValue.DropDownValues.Add(Entry.Value);
			ExportedValue.DropDownLabels.Add(Entry.Label);
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
			ExportArrayValues<TProperty>(ExportedValue.DefaultValue, DefaultValue.GetValue<TArray<TValue>>(), PortFlags);
		}
		if (!ClampMin.IsEmpty() && ClampMin.GetType() == TVariantTraits<TArray<TOptional<TValue>>>::GetType())
		{
			TArray<bool> HasValues;
			TArray<TValue> Values;
			TArray<TOptional<TValue>> OptionalValues = ClampMin.GetValue<TArray<TOptional<TValue>>>();
			for (TOptional<TValue>& OptionalValue : OptionalValues)
			{
				HasValues.Add(OptionalValue.IsSet());
				Values.Add(OptionalValue.Get(TValue{}));
			}
			ExportArrayValues<FBoolProperty>(ExportedValue.ClampMinHasValue, HasValues, PortFlags);
			ExportArrayValues<TProperty>(ExportedValue.ClampMinValue, Values, PortFlags);
		}
		if (!ClampMax.IsEmpty() && ClampMax.GetType() == TVariantTraits<TArray<TOptional<TValue>>>::GetType())
		{
			TArray<bool> HasValues;
			TArray<TValue> Values;
			TArray<TOptional<TValue>> OptionalValues = ClampMax.GetValue<TArray<TOptional<TValue>>>();
			for (TOptional<TValue>& OptionalValue : OptionalValues)
			{
				HasValues.Add(OptionalValue.IsSet());
				Values.Add(OptionalValue.Get(TValue{}));
			}
			ExportArrayValues<FBoolProperty>(ExportedValue.ClampMaxHasValue, HasValues, PortFlags);
			ExportArrayValues<TProperty>(ExportedValue.ClampMaxValue, Values, PortFlags);
		}
		if (!UIMin.IsEmpty() && UIMin.GetType() == TVariantTraits<TArray<TOptional<TValue>>>::GetType())
		{
			TArray<bool> HasValues;
			TArray<TValue> Values;
			TArray<TOptional<TValue>> OptionalValues = UIMin.GetValue<TArray<TOptional<TValue>>>();
			for (TOptional<TValue>& OptionalValue : OptionalValues)
			{
				HasValues.Add(OptionalValue.IsSet());
				Values.Add(OptionalValue.Get(TValue{}));
			}
			ExportArrayValues<FBoolProperty>(ExportedValue.UIMinHasValue, HasValues, PortFlags);
			ExportArrayValues<TProperty>(ExportedValue.UIMinValue, Values, PortFlags);
		}
		if (!UIMax.IsEmpty() && UIMax.GetType() == TVariantTraits<TArray<TOptional<TValue>>>::GetType())
		{
			TArray<bool> HasValues;
			TArray<TValue> Values;
			TArray<TOptional<TValue>> OptionalValues = UIMax.GetValue<TArray<TOptional<TValue>>>();
			for (TOptional<TValue>& OptionalValue : OptionalValues)
			{
				HasValues.Add(OptionalValue.IsSet());
				Values.Add(OptionalValue.Get(TValue{}));
			}
			ExportArrayValues<FBoolProperty>(ExportedValue.UIMinHasValue, HasValues, PortFlags);
			ExportArrayValues<TProperty>(ExportedValue.UIMaxValue, Values, PortFlags);
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
	FExportedValue ImportedValue;
	UScriptStruct* ExportedValueClass = FExportedValue::StaticStruct();
	Buffer = ExportedValueClass->ImportText(Buffer, &ImportedValue, nullptr, PortFlags, ErrorText, ExportedValueClass->GetName());

	const TProperty* PropertyCDO = GetDefault<TProperty>();
	if (PropertyCDO->ImportText_Direct(*ImportedValue.Value, &OutValue, nullptr, PortFlags, ErrorText))
	{
		SetValue(OutValue);
	}

	if (TVariantTraits<TValue>::GetType() != EVariantTypes::Empty)
	{
		if (!ImportedValue.DefaultValue.IsEmpty())
		{
			TValue ExportedDefaultValue;
			if (PropertyCDO->ImportText_Direct(*ImportedValue.DefaultValue, &ExportedDefaultValue, nullptr, PortFlags, ErrorText))
			{
				DefaultValue = ExportedDefaultValue;
			}
		}
		if (!ImportedValue.ClampMinValue.IsEmpty())
		{
			TValue ExportedMinValue;
			if (PropertyCDO->ImportText_Direct(*ImportedValue.ClampMinValue, &ExportedMinValue, nullptr, PortFlags, ErrorText))
			{
				ClampMin = ExportedMinValue;
			}
		}
		if (!ImportedValue.ClampMaxValue.IsEmpty())
		{
			TValue ExportedMaxValue;
			if (PropertyCDO->ImportText_Direct(*ImportedValue.ClampMaxValue, &ExportedMaxValue, nullptr, PortFlags, ErrorText))
			{
				ClampMax = ExportedMaxValue;
			}
		}
		if (!ImportedValue.UIMinValue.IsEmpty())
		{
			TValue ExportedMinValue;
			if (PropertyCDO->ImportText_Direct(*ImportedValue.UIMinValue, &ExportedMinValue, nullptr, PortFlags, ErrorText))
			{
				UIMin = ExportedMinValue;
			}
		}
		if (!ImportedValue.UIMaxValue.IsEmpty())
		{
			TValue ExportedMaxValue;
			if (PropertyCDO->ImportText_Direct(*ImportedValue.UIMaxValue, &ExportedMaxValue, nullptr, PortFlags, ErrorText))
			{
				UIMax = ExportedMaxValue;
			}
		}
	}
	if (VarIntent == EVarIntent::DropDown)
	{
		DropDownData.Empty();
		const int Num = FMath::Min3(ImportedValue.DropDownIndices.Num(), ImportedValue.DropDownValues.Num(), ImportedValue.DropDownLabels.Num());
		for (int i = 0; i < Num; ++i)
		{
			ensure(ImportedValue.DropDownIndices[i] == i);
			DropDownData.Add({ImportedValue.DropDownIndices[i], ImportedValue.DropDownValues[i], ImportedValue.DropDownLabels[i]});
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
	FExportedValue ImportedValue;
	UScriptStruct* ExportedValueClass = FExportedValue::StaticStruct();
	Buffer = ExportedValueClass->ImportText(Buffer, &ImportedValue, nullptr, PortFlags, ErrorText, ExportedValueClass->GetName());

	ImportAndSetArrayValues<TProperty, TValue>(*ImportedValue.Value, OutArray, PortFlags, ErrorText); // this also calls SetValue
	
	if (TVariantTraits<TValue>::GetType() != EVariantTypes::Empty)
	{
		if (!ImportedValue.DefaultValue.IsEmpty())
		{
			TArray<TValue> ExportedDefaultValue;
			if (ImportArrayValues<TProperty, TValue>(*ImportedValue.DefaultValue, ExportedDefaultValue, PortFlags, ErrorText))
			{
				DefaultValue = ExportedDefaultValue;
			}
		}
		if (!ImportedValue.ClampMinValue.IsEmpty())
		{
			TArray<bool> HasValues;
			TArray<TValue> Values;
			if (ImportArrayValues<FBoolProperty, bool>(*ImportedValue.ClampMinHasValue, HasValues, PortFlags, ErrorText) &&
				ImportArrayValues<TProperty, TValue>(*ImportedValue.ClampMinValue, Values, PortFlags, ErrorText))
			{
				TArray<TOptional<TValue>> OptionalValues;
				for (int i = 0; i < FMath::Min(HasValues.Num(), Values.Num()); ++i)
				{
					OptionalValues.Add(HasValues[i] ? Values[i] : TOptional<TValue>{});
				}
				ClampMin = OptionalValues;
			}
		}
		if (!ImportedValue.ClampMaxValue.IsEmpty())
		{
			TArray<bool> HasValues;
			TArray<TValue> Values;
			if (ImportArrayValues<FBoolProperty, bool>(*ImportedValue.ClampMaxHasValue, HasValues, PortFlags, ErrorText) &&
				ImportArrayValues<TProperty, TValue>(*ImportedValue.ClampMaxValue, Values, PortFlags, ErrorText))
			{
				TArray<TOptional<TValue>> OptionalValues;
				for (int i = 0; i < FMath::Min(HasValues.Num(), Values.Num()); ++i)
				{
					OptionalValues.Add(HasValues[i] ? Values[i] : TOptional<TValue>{});
				}
				ClampMax = OptionalValues;
			}
		}
		if (!ImportedValue.UIMinValue.IsEmpty())
		{
			TArray<bool> HasValues;
			TArray<TValue> Values;
			if (ImportArrayValues<FBoolProperty, bool>(*ImportedValue.UIMinHasValue, HasValues, PortFlags, ErrorText) &&
				ImportArrayValues<TProperty, TValue>(*ImportedValue.UIMinValue, Values, PortFlags, ErrorText))
			{
				TArray<TOptional<TValue>> OptionalValues;
				for (int i = 0; i < FMath::Min(HasValues.Num(), Values.Num()); ++i)
				{
					OptionalValues.Add(HasValues[i] ? Values[i] : TOptional<TValue>{});
				}
				UIMin = OptionalValues;
			}
		}
		if (!ImportedValue.UIMaxValue.IsEmpty())
		{
			TArray<bool> HasValues;
			TArray<TValue> Values;
			if (ImportArrayValues<FBoolProperty, bool>(*ImportedValue.UIMaxHasValue, HasValues, PortFlags, ErrorText) &&
				ImportArrayValues<TProperty, TValue>(*ImportedValue.UIMaxValue, Values, PortFlags, ErrorText))
			{
				TArray<TOptional<TValue>> OptionalValues;
				for (int i = 0; i < FMath::Min(HasValues.Num(), Values.Num()); ++i)
				{
					OptionalValues.Add(HasValues[i] ? Values[i] : TOptional<TValue>{});
				}
				UIMax = OptionalValues;
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
	const FObjectProperty* PropertyCDO = GetDefault<FObjectProperty>();

	FObjectProperty PtrProperty(nullptr, PropertyCDO->NamePrivate, PropertyCDO->GetFlags());
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
