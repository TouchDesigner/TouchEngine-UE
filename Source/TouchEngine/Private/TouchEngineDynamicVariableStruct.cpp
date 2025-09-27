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

#include "TouchEngineDynamicVariableStruct.h"

#include "Logging.h"
#include "TouchEngineDynamicVariableStructVersion.h"
#include "Blueprint/TouchEngineComponent.h"
#include "Blueprint/TouchEngineInputFrameData.h"
#include "Engine/Texture2D.h"
#include "Engine/TouchEngine.h"
#include "Engine/TouchEngineInfo.h"
#include "Engine/Util/TouchFrameCooker.h"
#include "Styling/SlateTypes.h"
#include "Util/TouchEngineStatsGroup.h"

// ---------------------------------------------------------------------------------------------------------------------
// ------------------------- FTouchEngineDynamicVariableContainer
// ---------------------------------------------------------------------------------------------------------------------

void FTouchEngineDynamicVariableContainer::ToxParametersLoaded(const TArray<FTouchEngineDynamicVariableStruct>& VariablesIn, const TArray<FTouchEngineDynamicVariableStruct>& VariablesOut)
{
	// if we have no data loaded
	if ((DynVars_Input.Num() == 0 && DynVars_Output.Num() == 0))
	{
		DynVars_Input = VariablesIn;
		DynVars_Output = VariablesOut;
		return;
	}

	TArray<FTouchEngineDynamicVariableStruct> InVarsCopy = VariablesIn;
	TArray<FTouchEngineDynamicVariableStruct> OutVarsCopy = VariablesOut;


	// fill out the new "variablesIn" and "variablesOut" arrays with the existing values in the "DynVars_Input" and "DynVars_Output" if possible
	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		for (int j = 0; j < InVarsCopy.Num(); j++)
		{
			if (DynVars_Input[i].VarName == InVarsCopy[j].VarName && DynVars_Input[i].VarType == InVarsCopy[j].VarType && DynVars_Input[i].bIsArray == InVarsCopy[j].bIsArray)
			{
				// SetValue below will override the newer dropdown data, so we save it first. There should be a better way to handle this
				TArray<FTouchEngineDynamicVariableStruct::FDropDownEntry> OldDropDownData = InVarsCopy[j].DropDownData;
				InVarsCopy[j].SetValue(&DynVars_Input[i]);
				InVarsCopy[j].DropDownData = MoveTemp(OldDropDownData);
				break;
			}
		}
	}
	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		for (int j = 0; j < OutVarsCopy.Num(); j++)
		{
			if (DynVars_Output[i].VarName == OutVarsCopy[j].VarName && DynVars_Output[i].VarType == OutVarsCopy[j].VarType && DynVars_Output[i].bIsArray == OutVarsCopy[j].bIsArray)
			{
				// output variables are not supposed to have Dropdown, but to be sure
				TArray<FTouchEngineDynamicVariableStruct::FDropDownEntry> OldDropDownData = OutVarsCopy[j].DropDownData;
				OutVarsCopy[j].SetValue(&DynVars_Output[i]);
				OutVarsCopy[j].DropDownData = MoveTemp(OldDropDownData);
			}
		}
	}

	DynVars_Input = MoveTemp(InVarsCopy);
	DynVars_Output = MoveTemp(OutVarsCopy);
}

void FTouchEngineDynamicVariableContainer::EnsureMetadataIsSet(const TArray<FTouchEngineDynamicVariableStruct>& VariablesIn)
{
	for(FTouchEngineDynamicVariableStruct& DynVar : DynVars_Input)
	{
		for (int j = 0; j < VariablesIn.Num(); j++)
		{
			if (DynVar.VarName == VariablesIn[j].VarName && DynVar.VarType == VariablesIn[j].VarType && DynVar.bIsArray == VariablesIn[j].bIsArray)
			{
				DynVar.ClampMin = VariablesIn[j].ClampMin;
				DynVar.ClampMax = VariablesIn[j].ClampMax;
				DynVar.UIMin = VariablesIn[j].UIMin;
				DynVar.UIMax = VariablesIn[j].UIMax;
				DynVar.DefaultValue = VariablesIn[j].DefaultValue;
				DynVar.DropDownData = VariablesIn[j].DropDownData;
				break;
			}
		}
	}
}

void FTouchEngineDynamicVariableContainer::Reset()
{
	DynVars_Input = {};
	DynVars_Output = {};
}

void FTouchEngineDynamicVariableContainer::SendInputs(UE::TouchEngine::FTouchVariableManager& VariableManager, const FTouchEngineInputFrameData& FrameData)
{
	for (int32 i = 0; i < DynVars_Input.Num(); i++)
	{
		DynVars_Input[i].SendInput(VariableManager, FrameData);
	}
}

void FTouchEngineDynamicVariableContainer::GetOutputs(const UTouchEngineInfo* EngineInfo)
{
	for (int32 i = 0; i < DynVars_Output.Num(); i++)
	{
		DynVars_Output[i].GetOutput(EngineInfo);
	}
}

void FTouchEngineDynamicVariableContainer::SetupForFirstCook(const TSharedPtr<UE::TouchEngine::FTouchResourceProvider>& TouchResourceProvider)
{
	// Before start the first cook, we set the FrameLastUpdated to the first frame for all variables to ensure they will
	// all be sent on the first cook, which will pick up any value changed by the user
	for (FTouchEngineDynamicVariableStruct& Input : DynVars_Input) 
	{
		Input.FrameLastUpdated = UE::TouchEngine::FTouchFrameCooker::FIRST_FRAME_ID;
		Input.WeakTouchResourceProvider = TouchResourceProvider;
		if (Input.VarType == EVarType::Texture) // Ensure we have a valid texture content
		{
			Input.SetValue(Input.GetValueAsTexture());
		}
	}
}

TMap<FString, FTouchEngineDynamicVariableStruct> FTouchEngineDynamicVariableContainer::CopyInputsForCook(int64 CurrentFrameID)
{
	TMap<FString, FTouchEngineDynamicVariableStruct> VariablesForCook;
	
	for (FTouchEngineDynamicVariableStruct& Input : DynVars_Input)
	{
		if (Input.FrameLastUpdated == -1)
		{
			// Force sending Inputs that have not been set or that have been reset
			Input.FrameLastUpdated = CurrentFrameID;
		}
		if (Input.FrameLastUpdated == CurrentFrameID) 
		{
			VariablesForCook.Add(Input.VarIdentifier, Input);
			if (Input.bNeedBoolReset) // we reset the pulse values
			{
				Input.SetValue(false);
				Input.bNeedBoolReset = false;
			}
		}
	}
	
	return VariablesForCook;
}

FTouchEngineDynamicVariableStruct* FTouchEngineDynamicVariableContainer::GetDynamicVariableByName(const FString& VarName)
{
	FTouchEngineDynamicVariableStruct* Var = nullptr;

	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		if (DynVars_Input[i].VarName == VarName)
		{
			if (!Var)
			{
				Var = &DynVars_Input[i];
			}
			else
			{
				// variable with duplicate names, don't try to distinguish between them
				return nullptr;
			}
		}
	}

	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		if (DynVars_Output[i].VarName == VarName)
		{
			if (!Var)
			{
				Var = &DynVars_Output[i];
			}
			else
			{
				// variable with duplicate names, don't try to distinguish between them
				return nullptr;
			}
		}
	}
	return Var;
}

FTouchEngineDynamicVariableStruct* FTouchEngineDynamicVariableContainer::GetDynamicVariableByIdentifier(const FString& VarIdentifier)
{
	for (int32 i = 0; i < DynVars_Input.Num(); i++)
	{
		if (DynVars_Input[i].VarIdentifier.Equals(VarIdentifier) || DynVars_Input[i].VarLabel.Equals(VarIdentifier) || DynVars_Input[i].VarName.Equals(VarIdentifier))
		{
			return &DynVars_Input[i];
		}
	}

	for (int32 i = 0; i < DynVars_Output.Num(); i++)
	{
		if (DynVars_Output[i].VarIdentifier.Equals(VarIdentifier) || DynVars_Output[i].VarLabel.Equals(VarIdentifier) || DynVars_Output[i].VarName.Equals(VarIdentifier))
		{
			return &DynVars_Output[i];
		}
	}
	return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
// ------------------------- FTouchEngineDynamicVariableStruct
// ---------------------------------------------------------------------------------------------------------------------

FTouchEngineDynamicVariableStruct::~FTouchEngineDynamicVariableStruct()
{
	Clear();
}

void FTouchEngineDynamicVariableStruct::Copy(const FTouchEngineDynamicVariableStruct* Other)
{
	VarLabel = Other->VarLabel;
	VarName = Other->VarName;
	VarIdentifier = Other->VarIdentifier;
	ParentIdentifier = Other->ParentIdentifier;
	VarScope = Other->VarScope;
	VarType = Other->VarType;
	VarIntent = Other->VarIntent;
	Count = Other->Count;
	Size = Other->Size;
	bIsArray = Other->bIsArray;
	
	ClampMin = Other->ClampMin;
	ClampMax = Other->ClampMax;
	UIMin = Other->UIMin;
	UIMax = Other->UIMax;
	DefaultValue = Other->DefaultValue;

	WeakTouchResourceProvider = nullptr; // ensure we are not copying the texture by setting this to null, as we are going to use the same pointer
	SetValue(Other);
	ExportedTexture = Other->ExportedTexture; // As our ExportedTexture are never re-written into until they are unused, it should be safe to just have the same pointer
	WeakTouchResourceProvider = Other->WeakTouchResourceProvider;
	
	FrameLastUpdated = Other->FrameLastUpdated;
	DropDownData = Other->DropDownData;
}

void FTouchEngineDynamicVariableStruct::Clear()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("DynVar - Clear"), STAT_TE_FTouchEngineDynamicVariableStructClear, STATGROUP_TouchEngine);

	// We are not clearing the ClampMin, the ClampMax and the DefaultValue as this is called from SetValue which would reset them.
	// It should be fine as a DynamicVar is not supposed to change type
	
	if (Value == nullptr)
	{
		return;
	}

	switch (VarType)
	{
	case EVarType::Bool:
		{
			if (bIsArray)
			{
				delete[] static_cast<bool*>(Value);
			}
			else
			{
				delete static_cast<bool*>(Value);
			}
			break;
		}
	case EVarType::Int:
		{
			if (Count <= 1 && !bIsArray)
			{
				delete static_cast<int*>(Value);
			}
			else
			{
				delete[] static_cast<int*>(Value);
			}
			break;
		}
	case EVarType::Double:
		{
			if (Count <= 1 && !bIsArray)
			{
				delete static_cast<double*>(Value);
			}
			else
			{
				delete[] static_cast<double*>(Value);
			}
			break;
		}
	case EVarType::Float:
		{
			if (bIsArray)
			{
				delete[] static_cast<float*>(Value);
			}
			else
			{
				delete static_cast<float*>(Value);
			}
			break;
		}
	case EVarType::CHOP:
		{
			for (int i = 0; i < Count; i++)
			{
				delete[] static_cast<float**>(Value)[i];
			}

			delete[] static_cast<float**>(Value);
			break;
		}
	case EVarType::String:
		{
			if (!bIsArray)
			{
				delete[] static_cast<char*>(Value);
			}
			else
			{
				for (int i = 0; i < Count; i++)
				{
					delete[] static_cast<char**>(Value)[i];
				}

				delete[] static_cast<char**>(Value);
			}
			break;
		}
	case EVarType::Texture:
		{
			Value = nullptr;
			ExportedTexture = nullptr;
			break;
		}
	default:
		break;
	}

	Value = nullptr;
	ChannelNames.Reset();
}


FString FTouchEngineDynamicVariableStruct::GetCleanVariableName() const
{
	FString CleanName = VarName;
	CleanName.RemoveFromStart("p/");
	CleanName.RemoveFromStart("i/");
	CleanName.RemoveFromStart("o/");
	return CleanName;
}

bool FTouchEngineDynamicVariableStruct::GetValueAsBool() const
{
	return Value ? *static_cast<bool*>(Value) : false;
}

int FTouchEngineDynamicVariableStruct::GetValueAsInt() const
{
	return Value ? *static_cast<int*>(Value) : 0;
}

int FTouchEngineDynamicVariableStruct::GetValueAsIntIndexed(const int Index) const
{
	return Value ? GetValueAsIntTArray()[Index] : 0; //todo: handle out of bounds
}

int* FTouchEngineDynamicVariableStruct::GetValueAsIntArray() const
{
	return Value ? static_cast<int*>(Value) : nullptr;
}

TArray<int> FTouchEngineDynamicVariableStruct::GetValueAsIntTArray() const
{
	if (VarType != EVarType::Int || !bIsArray || !Value || Count == 0)
	{
		return TArray<int>();
	}

	TArray<int> returnArray(static_cast<int*>(Value), Count);
	return returnArray;
}

double FTouchEngineDynamicVariableStruct::GetValueAsDouble() const
{
	return Value ? *static_cast<double*>(Value) : 0;
}

double FTouchEngineDynamicVariableStruct::GetValueAsDoubleIndexed(const int Index) const
{
	return Value ? GetValueAsDoubleTArray()[Index] : 0; //todo: handle out of bounds
}

double* FTouchEngineDynamicVariableStruct::GetValueAsDoubleArray() const
{
	return Value ? static_cast<double*>(Value) : nullptr;
}

TArray<double> FTouchEngineDynamicVariableStruct::GetValueAsDoubleTArray() const
{
	if (VarType != EVarType::Double || !bIsArray || !Value || Count == 0)
	{
		return TArray<double>();
	}

	const TArray<double> returnArray(static_cast<double*>(Value), Count);
	return returnArray;
}

float FTouchEngineDynamicVariableStruct::GetValueAsFloat() const
{
	return Value ? *static_cast<float*>(Value) : 0;
}

double FTouchEngineDynamicVariableStruct::GetValueAsFloatIndexed(int Index) const
{
	return Value ? GetValueAsFloatTArray()[Index] : 0; //todo: handle out of bounds
}

float* FTouchEngineDynamicVariableStruct::GetValueAsFloatArray() const
{
	return Value ? static_cast<float*>(Value) : nullptr;
}

TArray<float> FTouchEngineDynamicVariableStruct::GetValueAsFloatTArray() const
{
	if (VarType != EVarType::Float || !bIsArray || !Value || Count == 0)
	{
		return TArray<float>();
	}

	const TArray<float> returnArray(static_cast<float*>(Value), Count);
	return returnArray;
}

FLinearColor FTouchEngineDynamicVariableStruct::GetValueAsLinearColor() const
{
	if (VarType == EVarType::Float && bIsArray)
	{
		const float* Values = GetValueAsFloatArray();
		if (Values && Count == 3)
		{
			return FLinearColor(Values[0], Values[1], Values[2]);
		}
		else if (Values && Count == 4)
		{
			return FLinearColor(Values[0], Values[1], Values[2], Values[3]);
		}
	}
	else if (VarType == EVarType::Double && bIsArray)
	{
		const double* Values = GetValueAsDoubleArray();
		if (Values && Count == 3)
		{
			return FLinearColor(Values[0], Values[1], Values[2]);
		}
		else if (Values && Count == 4)
		{
			return FLinearColor(Values[0], Values[1], Values[2], Values[3]);
		}
	}
	return FLinearColor();
}

FString FTouchEngineDynamicVariableStruct::GetValueAsString() const
{
	if (VarType == EVarType::Int && VarIntent == EVarIntent::DropDown)
	{
		const int Index = GetValueAsInt();
		return DropDownData.IsValidIndex(Index) ? DropDownData[Index].Value : FString();
	}
	if (VarType == EVarType::String)
	{
		return Value ? FString(UTF8_TO_TCHAR(static_cast<char*>(Value))) : FString();
	}
	return FString();
}

TArray<FString> FTouchEngineDynamicVariableStruct::GetValueAsStringArray() const
{
	TArray<FString> TempValue = TArray<FString>();

	if (!Value || Count == 0)
	{
		return TempValue;
	}

	char** Buffer = static_cast<char**>(Value);

	for (int i = 0; i < Count; i++)
	{
		TempValue.Add(Buffer[i]);
	}
	return TempValue;
}

UTexture* FTouchEngineDynamicVariableStruct::GetValueAsTexture() const
{
	return static_cast<UTexture*>(Value);
}

const TSharedPtr<UE::TouchEngine::FExportedTouchTexture>& FTouchEngineDynamicVariableStruct::GetExportedTexture() const
{
	static TSharedPtr<UE::TouchEngine::FExportedTouchTexture> EmptyTexture = nullptr;
	return ExportedTexture ? ExportedTexture->Texture : EmptyTexture;
}

UDEPRECATED_TouchEngineCHOPMinimal* FTouchEngineDynamicVariableStruct::GetValueAsCHOP_DEPRECATED() const
{
	if (!Value)
	{
		return nullptr;
	}
	
	UDEPRECATED_TouchEngineCHOPMinimal* RetVal = NewObject<UDEPRECATED_TouchEngineCHOPMinimal>();
	const FTouchEngineCHOP Chop = GetValueAsCHOP();
	RetVal->FillFromCHOP(Chop);

	return RetVal;
}

FTouchEngineCHOP FTouchEngineDynamicVariableStruct::GetValueAsCHOP() const
{
	if (!Value)
	{
		return FTouchEngineCHOP();
	}

	float** TempBuffer = static_cast<float**>(Value);
	const int ChannelLength = Count == 0 ? 0 : (Size / sizeof(float)) / Count;

	return FTouchEngineCHOP::FromChannels(TempBuffer, Count, ChannelLength, ChannelNames);
}

FTouchEngineCHOP FTouchEngineDynamicVariableStruct::GetValueAsCHOP(const UTouchEngineInfo* EngineInfo) const
{
	FTouchEngineCHOP Chop = GetValueAsCHOP();

	if (IsValid(EngineInfo))
	{
		Chop.SetChannelNames(EngineInfo->Engine->GetCHOPChannelNames(VarIdentifier));
	}

	return Chop;
}

UTouchEngineDAT* FTouchEngineDynamicVariableStruct::GetValueAsDAT() const
{
	if (!Value)
	{
		return nullptr;
	}

	TArray<FString> stringArrayBuffer = TArray<FString>();

	char** Buffer = static_cast<char**>(Value);

	//todo: Count and Size have a different meaning in SetValue(const TArray<FString>& InValue) and in SetValueAsDAT(const TArray<FString>& InValue, const int NumRows, const int NumColumns)
	for (int i = 0; i < Size; i++)
	{
		stringArrayBuffer.Add(UTF8_TO_TCHAR(Buffer[i]));
	}

	UTouchEngineDAT* RetVal = NewObject<UTouchEngineDAT>();
	RetVal->CreateChannels(stringArrayBuffer, Count, stringArrayBuffer.Num() / Count);

	return RetVal;
}


void FTouchEngineDynamicVariableStruct::SetValue(UObject* newValue, const size_t _size)
{
	//todo: when is this used? What should we clear and how to ensure this is cleared properly?
	if (newValue == nullptr)
	{
		Clear();
		Value = nullptr;
		return;
	}

	Value = static_cast<void*>(newValue);
	Size = _size;
}

void FTouchEngineDynamicVariableStruct::SetValue(const bool InValue)
{
	if (VarType == EVarType::Bool)
	{
		Clear();

		Value = new bool;
		*static_cast<bool*>(Value) = InValue;

		if (InValue && VarIntent == EVarIntent::Pulse)
		{
			bNeedBoolReset = true;
		}
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(const int32 InValue)
{
	if (VarType == EVarType::Int)
	{
		Clear();

		Value = new int;
		*static_cast<int*>(Value) = InValue;
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(const TArray<int>& InValue)
{
	if (VarType == EVarType::Int && bIsArray)
	{
		Clear();

		Value = new int*[InValue.Num()];

		for (int i = 0; i < InValue.Num(); i++)
		{
			static_cast<int*>(Value)[i] = InValue[i];
		}

		Count = InValue.Num();
		Size = sizeof(int) * Count;


#if WITH_EDITORONLY_DATA

		switch (InValue.Num())
		{
		case 2:
			{
				IntPointProperty.X = InValue[0];
				IntPointProperty.Y = InValue[1];
				break;
			}
		case 3:
			{
				IntVectorProperty.X = InValue[0];
				IntVectorProperty.Y = InValue[1];
				IntVectorProperty.Z = InValue[2];
				break;
			}
		case 4:
			{
				IntVector4Property.X = InValue[0];
				IntVector4Property.Y = InValue[1];
				IntVector4Property.Z = InValue[2];
				IntVector4Property.W = InValue[3];
				break;
			}
		default:
			break;
		}

#endif
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(const double InValue)
{
	if (VarType == EVarType::Double)
	{
		Clear();

		Value = new double;
		*static_cast<double*>(Value) = InValue;
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(const TArray<double>& InValue)
{
	if (VarType == EVarType::Double && bIsArray)
	{
		Clear();

		Value = new double*[InValue.Num()];

		for (int i = 0; i < InValue.Num(); i++)
		{
			static_cast<double*>(Value)[i] = InValue[i];
		}

		Count = InValue.Num();
		Size = sizeof(double) * Count;
	}

#if WITH_EDITORONLY_DATA

	switch (InValue.Num())
	{
	case 2:
		{
			Vector2DProperty.X = InValue[0];
			Vector2DProperty.Y = InValue[1];
			break;
		}
	case 3:
		{
			VectorProperty.X = InValue[0];
			VectorProperty.Y = InValue[1];
			VectorProperty.Z = InValue[2];

			if (VarIntent == EVarIntent::Color)
			{
				ColorProperty = FLinearColor(InValue[0], InValue[1], InValue[2]).ToFColor(false);
			}
			break;
		}
	case 4:
		{
			Vector4Property.X = InValue[0];
			Vector4Property.Y = InValue[1];
			Vector4Property.Z = InValue[2];
			Vector4Property.W = InValue[3];

			if (VarIntent == EVarIntent::Color)
			{
				ColorProperty = FLinearColor(InValue[0], InValue[1], InValue[2], InValue[3]).ToFColor(false);
			}
			break;
		}
	default:
		break;
	}

#endif
}

void FTouchEngineDynamicVariableStruct::SetValue(const float InValue)
{
	if (VarType == EVarType::Float)
	{
		Clear();

		Value = new float;
		*static_cast<float*>(Value) = InValue;
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(const TArray<float>& InValue)
{
	if (InValue.Num() == 0)
	{
		Clear();

		Value = nullptr;
		return;
	}

	if (VarType == EVarType::Float && bIsArray)
	{
		Clear();

		Value = new float[InValue.Num()];

		for (int i = 0; i < InValue.Num(); i++)
		{
			static_cast<float*>(Value)[i] = InValue[i];
		}

#if WITH_EDITORONLY_DATA
		FloatBufferProperty = InValue;
#endif

		Count = InValue.Num();
		Size = Count * sizeof(float);
		bIsArray = true;
	}
	else if (VarType == EVarType::CHOP)
	{
		SetValueAsCHOP(InValue, InValue.Num(), 1);
	}
	else if (VarType == EVarType::Double && bIsArray)
	{
#if WITH_EDITORONLY_DATA

		switch (VarIntent)
		{
		case EVarIntent::Color:
			{
				if (InValue.Num() == 4)
				{
					ColorProperty = FLinearColor(InValue[0], InValue[1], InValue[2], InValue[3]).ToFColor(false);
				}
				else if (InValue.Num() == 3)
				{
					ColorProperty = FLinearColor(InValue[0], InValue[1], InValue[2], 1.f).ToFColor(false);
				}
				break;
			}
		case EVarIntent::UVW:
			{
				if (InValue.Num() == 3)
				{
					VectorProperty = FVector(InValue[0], InValue[1], InValue[2]);
				}
				else if (InValue.Num() == 2)
				{
					VectorProperty = FVector(InValue[0], InValue[1], 1.f);
				}
				break;
			}
		case EVarIntent::Position:
			{
				if (InValue.Num() == 4)
				{
					Vector4Property = FVector4(InValue[0], InValue[1], InValue[2], InValue[3]);
				}
				else if (InValue.Num() == 3)
				{
					Vector4Property = FVector4(InValue[0], InValue[1], InValue[2], 1.f);
				}
				break;
			}
		default:
			break;
		}

		switch (InValue.Num())
		{
		case 2:
			{
				Vector2DProperty.X = InValue[0];
				Vector2DProperty.Y = InValue[1];
				break;
			}
		case 3:
			{
				VectorProperty.X = InValue[0];
				VectorProperty.Y = InValue[1];
				VectorProperty.Z = InValue[2];
				break;
			}
		case 4:
			{
				Vector4Property.X = InValue[0];
				Vector4Property.Y = InValue[1];
				Vector4Property.Z = InValue[2];
				Vector4Property.W = InValue[3];
				break;
			}
		default:
			break;
		}

#endif

		Clear();
		Value = new double[InValue.Num()];

		for (int i = 0; i < InValue.Num(); i++)
		{
			static_cast<double*>(Value)[i] = static_cast<double>(InValue[i]);
		}

		Count = InValue.Num();
		Size = Count * sizeof(double);
		bIsArray = true;
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(const FTouchEngineCHOP& InValue)
{
	if (VarType != EVarType::CHOP)
	{
		return;
	}

	Clear();
#if WITH_EDITORONLY_DATA
	if (&CHOPProperty != & InValue)
	{
		CHOPProperty = FTouchEngineCHOP();
		FloatBufferProperty.Empty(); //todo: should this be in clear?
	}
#endif
	
	TArray<float> Data;
	{
		if (!InValue.GetCombinedValues(Data))
		{
			UE_LOG(LogTouchEngineComponent, Error, TEXT("The CHOP Data sent to the Input `%s` is invalid:\n%s"), *VarLabel, *InValue.ToString());
			return;
		}
	}
	
	Count = InValue.Channels.Num();
	const int32 ChannelLength = InValue.GetNumSamples(); //Count == 0 ? 0 : Data.Num() / Count;
	Size = Count * ChannelLength * sizeof(float); // Data.Num() * sizeof(float);
	bIsArray = true;

	Value = new float*[Count];
	{
		// int32 Index = 0;
		for (int i = 0; i < Count; i++)
		{
			static_cast<float**>(Value)[i] = new float[InValue.Channels[i].Values.Num()];
			for (int j = 0; j < ChannelLength; j++)
			{
				static_cast<float**>(Value)[i][j] = InValue.Channels[i].Values[j];
				// ++Index;
			}
		}
	}

	{
		// ChannelNames = InValue.GetChannelNames();
		TSet<FString> UniqueNames;
		int32 NbNonEmptyChannelNames = 0;
		{
			UniqueNames.Reserve(InValue.Channels.Num());
			ChannelNames.Reset(InValue.Channels.Num());
			for (const FTouchEngineCHOPChannel& Channel : InValue.Channels)
			{
				ChannelNames.Emplace(Channel.Name);
				if (!Channel.Name.IsEmpty())
				{
					++NbNonEmptyChannelNames;
					UniqueNames.Add(Channel.Name);
				}
			}
		}

		if (!UniqueNames.IsEmpty() && UniqueNames.Num() != NbNonEmptyChannelNames)
		{
			UE_LOG(LogTouchEngineComponent, Warning, TEXT("Some Channels of the CHOP Data sent to the Input `%s` have the same name:\n%s"), *VarLabel, *InValue.ToString());
		}
	}

#if WITH_EDITORONLY_DATA
	if (&CHOPProperty != & InValue)
	{
		CHOPProperty = InValue;
		FloatBufferProperty.Append(Data);
	}
#endif
}

void FTouchEngineDynamicVariableStruct::SetValueAsCHOP(const TArray<float>& InValue, const int NumChannels, const int NumSamples)
{
	if (VarType != EVarType::CHOP)
	{
		return;
	}

	Clear();
#if WITH_EDITORONLY_DATA
	if (&FloatBufferProperty != &InValue)
	{
		FloatBufferProperty.Empty(); //todo: should this be in clear?
	}
	CHOPProperty = FTouchEngineCHOP();
#endif
	
	if (!InValue.Num() || InValue.Num() != NumChannels * NumSamples)
	{
		// OnValueChanged.Broadcast(*this);
		return;
	}

	Count = NumChannels;
	Size = NumSamples * NumChannels * sizeof(float);
	bIsArray = true;

	Value = new float*[Count];

	for (int i = 0; i < NumChannels; i++)
	{
		static_cast<float**>(Value)[i] = new float[NumSamples];

		for (int j = 0; j < NumSamples; j++)
		{
			static_cast<float**>(Value)[i][j] = InValue[i * NumSamples + j];
		}
	}

#if WITH_EDITORONLY_DATA
	if (&FloatBufferProperty != &InValue)
	{
		FloatBufferProperty.Append(InValue);
	}
	CHOPProperty = GetValueAsCHOP();
#endif
}

void FTouchEngineDynamicVariableStruct::SetValueAsCHOP(const TArray<float>& InValue, const TArray<FString>& InChannelNames)
{
	if (VarType != EVarType::CHOP)
	{
		return;
	}

	Clear();

	const int32 NumChannels = InChannelNames.Num();
	const int32 NumSamples = InValue.Num() / NumChannels;

	if (!InValue.Num() || InValue.Num() != NumChannels * NumSamples)
	{
		return;
	}
	SetValueAsCHOP(InValue, NumChannels, NumSamples);
	ChannelNames = InChannelNames;
}

void FTouchEngineDynamicVariableStruct::SetValue(const UTouchEngineDAT* InValue)
{
	Clear();

	if (!InValue || InValue->NumColumns < 1 || InValue->NumRows < 1)
	{
		return;
	}

	SetValue(InValue->ValuesAppended);

	Count = InValue->NumRows;
	Size = InValue->NumColumns * InValue->NumRows;

	bIsArray = true;
}

void FTouchEngineDynamicVariableStruct::SetValueAsDAT(const TArray<FString>& InValue, const int NumRows, const int NumColumns)
{
	Clear();

	if (!InValue.Num() || InValue.Num() != NumRows * NumColumns)
	{
		return;
	}

	SetValue(InValue);

	Count = NumRows;
	Size = NumRows * NumColumns;

	bIsArray = true;
}

void FTouchEngineDynamicVariableStruct::SetValue(const FString& InValue)
{
	if (VarType == EVarType::String)
	{
		// Even if it is a dropdown, we do not force the value to be one of the dropdown value as per the description of TEInstanceLinkGetChoiceValues:
		//  "This list should not be considered exhaustive and users should be allowed to enter their own values as well as those in this list."
		
		Clear();

		const auto AnsiString = StringCast<ANSICHAR>(*InValue);
		const char* Buffer = AnsiString.Get();

		Value = new char[strlen(Buffer) + 1]; //todo: store the value as FString?

		for (int i = 0; i < strlen(Buffer) + 1; i++)
		{
			static_cast<char*>(Value)[i] = Buffer[i];
		}
	}
	else if (VarType == EVarType::Int && VarIntent == EVarIntent::DropDown)
	{
		const FDropDownEntry* EntryPtr = DropDownData.FindByPredicate([&InValue](const FDropDownEntry& Entry)
		{
			return Entry.Value == InValue;
		});

		if (EntryPtr)
		{
			SetValue(EntryPtr->Index);
		}
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(const TArray<FString>& InValue)
{
	if (VarType != EVarType::String || !bIsArray)
	{
		return;
	}

	if (InValue.Num() == 0)
	{
		Clear();

		Value = nullptr;
		Count = 0;
		Size = 0;
		return;
	}

	Clear();

	Value = new char*[InValue.Num()];
	Size = 0;

	Count = InValue.Num();

	for (int i = 0; i < InValue.Num(); i++)
	{
		const auto AnsiString = StringCast<ANSICHAR>(*(InValue[i]));
		const char* TempValue = AnsiString.Get();

		static_cast<char**>(Value)[i] = new char[(InValue[i]).Len() + 1];
		Size += InValue[i].Len() + 1;
		for (int j = 0; j < InValue[i].Len() + 1; j++)
		{
			static_cast<char**>(Value)[i][j] = TempValue[j];
		}
	}

#if WITH_EDITORONLY_DATA
	StringArrayProperty = InValue;
#endif
}

void FTouchEngineDynamicVariableStruct::SetValue(UTexture* InValue)
{
	if (VarType == EVarType::Texture)
	{
		Clear();

#if WITH_EDITORONLY_DATA
		TextureProperty = InValue;
#endif

		SetValue((UObject*)InValue, sizeof(UTexture));
		if (IsValid(InValue))
		{
			if (GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan) // todo: For UE 5.6, Vulkan Textures are not supported
			{
				return;
			}
			
			if (TSharedPtr<UE::TouchEngine::FTouchResourceProvider> ResourceProvider = WeakTouchResourceProvider.Pin())
			{
				UE_LOG(LogTouchEngine, Verbose, TEXT("[FTouchEngineDynamicVariableStruct::SetValue(UTexture* InValue)] GetOrCreateTexture for texture '%s' for var '%s'"), *InValue->GetName(), *VarName)
				// Try having ExportedTexture as a shared ptr to a struct which when deleted sets a value to FExportedTouchTexture that it is not in use anymore
				TSharedPtr<UE::TouchEngine::FExportedTouchTexture> Texture = ResourceProvider->GetTextureExporter().GetOrCreateTexture(InValue);
				if (Texture)
				{
					Texture->SetInUseByDynVars();
					ExportedTexture = MakeShareable<FExportedTouchTextureContainer>(new FExportedTouchTextureContainer(MoveTemp(Texture)),
					[](FExportedTouchTextureContainer* Container)
					{
						Container->Texture->ReleasedByDynVars();
					});
				}
			}
		}
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(const FTouchEngineDynamicVariableStruct* Other)
{
	if (!Other || Other->VarType != VarType || Other->bIsArray != bIsArray)
	{
		return;
	}
	
	switch (Other->VarType)
	{
	case EVarType::Bool:
		{
			SetValue(Other->GetValueAsBool());
			break;
		}
	case EVarType::Int:
		{
			if (!Other->bIsArray)
			{
				SetValue(Other->GetValueAsInt());
			}
			else
			{
				SetValue(Other->GetValueAsIntTArray());
			}

			break;
		}
	case EVarType::Double:
		{
			if (!Other->bIsArray)
			{
				SetValue(Other->GetValueAsDouble());
			}
			else
			{
				SetValue(Other->GetValueAsDoubleTArray());
			}
			break;
		}
	case EVarType::Float:
		{
			SetValue(Other->GetValueAsFloat());
			break;
		}
	case EVarType::CHOP:
		{
			SetValue(Other->GetValueAsCHOP());
			break;
		}
	case EVarType::String:
		{
			if (!Other->bIsArray)
			{
				SetValue(Other->GetValueAsString());
			}
			else
			{
				SetValue(Other->GetValueAsDAT());
			}
			break;
		}
	case EVarType::Texture:
		{
			SetValue(Other->GetValueAsTexture());
			break;
		}
	default:
		break;
	}

#if WITH_EDITORONLY_DATA

	FloatBufferProperty = Other->FloatBufferProperty;
	CHOPProperty = Other->CHOPProperty;
	StringArrayProperty = Other->StringArrayProperty;
	TextureProperty = Other->TextureProperty;

	Vector2DProperty = Other->Vector2DProperty;
	VectorProperty = Other->VectorProperty;
	Vector4Property = Other->Vector4Property;
	ColorProperty = Other->ColorProperty;

	IntPointProperty = Other->IntPointProperty;
	IntVectorProperty = Other->IntVectorProperty;
	IntVector4Property = Other->IntVector4Property;
#endif
	DropDownData = Other->DropDownData;
}

void FTouchEngineDynamicVariableStruct::SetFrameLastUpdatedFromNextCookFrame(const UTouchEngineInfo* EngineInfo)
{
	if (IsValid(EngineInfo))
	{
		FrameLastUpdated = EngineInfo->Engine->GetNextFrameID();
	}
}

bool FTouchEngineDynamicVariableStruct::HasSameValue(const FTouchEngineDynamicVariableStruct* Other) const
{
	if (!Other || Other->VarType != VarType || Other->bIsArray != bIsArray)
	{
		return false;
	}
	
	switch (Other->VarType)
	{
	case EVarType::Bool:
		return GetValueAsBool() == Other->GetValueAsBool();
	case EVarType::Int:
		{
			if (!Other->bIsArray)
			{
				return GetValueAsInt() == Other->GetValueAsInt();
			}
			else
			{
				return GetValueAsIntTArray() == Other->GetValueAsIntTArray();
			}
		}
	case EVarType::Double:
		{
			if (!Other->bIsArray)
			{
				return GetValueAsDouble() == Other->GetValueAsDouble();
			}
			else
			{
				return GetValueAsDoubleTArray() == Other->GetValueAsDoubleTArray();
			}
		}
	case EVarType::Float:
		return GetValueAsFloat() == Other->GetValueAsFloat();
	case EVarType::CHOP:
		return GetValueAsCHOP() == Other->GetValueAsCHOP();
	case EVarType::String:
		{
			if (!Other->bIsArray)
			{
				return GetValueAsString() == Other->GetValueAsString();
			}
			else
			{
				return GetValueAsDAT() == Other->GetValueAsDAT(); //todo: to check
			}
		}
	case EVarType::Texture:
		return GetValueAsTexture() == Other->GetValueAsTexture();
	default:
		return false;
	}
}


bool FTouchEngineDynamicVariableStruct::CanResetToDefault() const
{
	if (DefaultValue.IsEmpty())
	{
		return false;
	}
	
	switch (VarType)
	{
	case EVarType::Bool:
		{
			break;
		}
	case EVarType::Int:
		{
			if (!bIsArray)
			{
				return ensure(DefaultValue.GetType() == EVariantTypes::Int32) && GetValueAsInt() != DefaultValue.GetValue<int32>();
			}
			else
			{
				return ensure(DefaultValue.GetType() == TVariantTraits<TArray<int>>::GetType()) && GetValueAsIntTArray() != GetDefaultValueArrayMatchingCount<int>();
			}
		}
	case EVarType::Double:
		{
			if (!bIsArray)
			{
				return ensure(DefaultValue.GetType() == EVariantTypes::Double) && GetValueAsDouble() != DefaultValue.GetValue<double>();
			}
			else
			{
				return ensure(DefaultValue.GetType() == TVariantTraits<TArray<double>>::GetType()) && GetValueAsDoubleTArray() != GetDefaultValueArrayMatchingCount<double>();
			}
		}
	case EVarType::Float:
		{
			if (!bIsArray)
			{
				return ensure(DefaultValue.GetType() == EVariantTypes::Float) && GetValueAsFloat() != DefaultValue.GetValue<float>();
			}
			else
			{
				return ensure(DefaultValue.GetType() == TVariantTraits<TArray<float>>::GetType()) && GetValueAsFloatTArray() != GetDefaultValueArrayMatchingCount<float>();
			}
		}
	case EVarType::CHOP:
		{
			break;
		}
	case EVarType::String:
		{
			if (!bIsArray)
			{
				return ensure(DefaultValue.GetType() == EVariantTypes::String) && GetValueAsString() != DefaultValue.GetValue<FString>();
			}
			break;
		}
	case EVarType::Texture:
		{
			break;
		}
	default:
		break;
	}

	UE_LOG(LogTouchEngineComponent, Error, TEXT("CanResetToDefault is not defined for the type `%s` (called for variable `%s`)"),
		*UEnum::GetValueAsName(VarType).ToString(), *VarLabel);
	return false;
}

bool FTouchEngineDynamicVariableStruct::CanResetToDefault(int Index) const
{
	if (DefaultValue.IsEmpty() || !ensure(bIsArray))
	{
		return false;
	}
	
	switch (VarType)
	{
	case EVarType::Int:
		{
			if (ensure(DefaultValue.GetType() == TVariantTraits<TArray<int>>::GetType()))
			{
				TArray<int> CurrentVal = GetValueAsIntTArray();
				TArray<int> DefaultVal = DefaultValue.GetValue<TArray<int>>();
				return CurrentVal.IsValidIndex(Index) && DefaultVal.IsValidIndex(Index) && CurrentVal[Index] != DefaultVal[Index];
			}
			break;
		}
	case EVarType::Double:
		{
			if (ensure(DefaultValue.GetType() == TVariantTraits<TArray<double>>::GetType()))
			{
				TArray<double> CurrentVal = GetValueAsDoubleTArray();
				TArray<double> DefaultVal = DefaultValue.GetValue<TArray<double>>();
				return CurrentVal.IsValidIndex(Index) && DefaultVal.IsValidIndex(Index) && CurrentVal[Index] != DefaultVal[Index];
			}
		}
	case EVarType::Float:
		{
			break;
		}
	case EVarType::String:
		{
			break;
		}
	default:
		break;
	}

	UE_LOG(LogTouchEngineComponent, Error, TEXT("CanResetToDefault(int Index) is not defined for the type `%s` (called for variable `%s`)"),
		*UEnum::GetValueAsName(VarType).ToString(), *VarLabel);
	return false;
}

void FTouchEngineDynamicVariableStruct::ResetToDefault()
{
	if (DefaultValue.IsEmpty())
	{
		return;
	}
	
	switch (VarType)
	{
	case EVarType::Bool:
		{
			break;
		}
	case EVarType::Int:
		{
			if (!bIsArray && ensure(DefaultValue.GetType() == EVariantTypes::Int32))
			{
				SetValue(DefaultValue.GetValue<int32>());
				return;
			}
			else if (bIsArray && ensure(DefaultValue.GetType() == TVariantTraits<TArray<int32>>::GetType()))
			{
				SetValue(GetDefaultValueArrayMatchingCount<int32>());
				return;
			}
			break;
		}
	case EVarType::Double:
		{
			if (!bIsArray && ensure(DefaultValue.GetType() == EVariantTypes::Double))
			{
				SetValue(DefaultValue.GetValue<double>());
				return;
			}
			else if (bIsArray && ensure(DefaultValue.GetType() == TVariantTraits<TArray<double>>::GetType()))
			{
				SetValue(GetDefaultValueArrayMatchingCount<double>());
				return;
			}
			break;
		}
	case EVarType::Float:
		{
			if (!bIsArray && ensure(DefaultValue.GetType() == EVariantTypes::Float))
			{
				SetValue(DefaultValue.GetValue<float>());
				return;
			}
			else if (bIsArray && ensure(DefaultValue.GetType() == TVariantTraits<TArray<float>>::GetType()))
			{
				SetValue(GetDefaultValueArrayMatchingCount<float>());
				return;
			}
			break;
		}
	case EVarType::CHOP:
		{
			break;
		}
	case EVarType::String:
		{
			if (!bIsArray && ensure(DefaultValue.GetType() == EVariantTypes::String))
			{
				SetValue(DefaultValue.GetValue<FString>());
				return;
			}
			break;
		}
	case EVarType::Texture:
		{
			break;
		}
	default:
		break;
	}

	UE_LOG(LogTouchEngineComponent, Error, TEXT("ResetToDefault is not defined for the type `%s` (called for variable `%s`)"),
		*UEnum::GetValueAsName(VarType).ToString(), *VarLabel);
}

void FTouchEngineDynamicVariableStruct::ResetToDefault(int Index)
{
	if (DefaultValue.IsEmpty() && !ensure(bIsArray))
	{
		return;
	}
	
	switch (VarType)
	{
	case EVarType::Int:
		{
			if (ensure(DefaultValue.GetType() == TVariantTraits<TArray<int>>::GetType()))
			{
				TArray<int> CurrentVal = GetValueAsIntTArray();
				TArray<int> DefaultVal = DefaultValue.GetValue<TArray<int>>();
				if (CurrentVal.IsValidIndex(Index) && DefaultVal.IsValidIndex(Index))
				{
					CurrentVal[Index] = DefaultVal[Index];
					SetValue(CurrentVal);
				}
				return;
			}
			break;
		}
	case EVarType::Double:
		{
			if (ensure(DefaultValue.GetType() == TVariantTraits<TArray<double>>::GetType()))
			{
				TArray<double> CurrentVal = GetValueAsDoubleTArray();
				TArray<double> DefaultVal = DefaultValue.GetValue<TArray<double>>();
				if (CurrentVal.IsValidIndex(Index) && DefaultVal.IsValidIndex(Index))
				{
					CurrentVal[Index] = DefaultVal[Index];
					SetValue(CurrentVal);
				}
				return;
			}
			break;
		}
	case EVarType::Float:
		{
			break;
		}
	case EVarType::String:
		{
			if (!bIsArray && ensure(DefaultValue.GetType() == EVariantTypes::String))
			{
				SetValue(DefaultValue.GetValue<FString>());
				return;
			}
			break;
		}
	default:
		break;
	}

	UE_LOG(LogTouchEngineComponent, Error, TEXT("ResetToDefault(int Index) is not defined for the type `%s` (called for variable `%s`)"),
		*UEnum::GetValueAsName(VarType).ToString(), *VarLabel);
}


#if WITH_EDITORONLY_DATA

void FTouchEngineDynamicVariableStruct::HandleChecked(const ECheckBoxState InState, const UTouchEngineInfo* EngineInfo)
{
	switch (InState)
	{
	case ECheckBoxState::Unchecked:
		{
			SetValue(false);
			SetFrameLastUpdatedFromNextCookFrame(EngineInfo);
			break;
		}
	case ECheckBoxState::Checked:
		{
			SetValue(true);
			SetFrameLastUpdatedFromNextCookFrame(EngineInfo);
			break;
		}
	case ECheckBoxState::Undetermined:
	default:
		break;
	}
}

void FTouchEngineDynamicVariableStruct::HandleTextBoxTextCommitted(const FText& NewText, const UTouchEngineInfo* EngineInfo)
{
	SetValue(NewText.ToString());
	SetFrameLastUpdatedFromNextCookFrame(EngineInfo);
}

void FTouchEngineDynamicVariableStruct::HandleTextureChanged(const UTouchEngineInfo* EngineInfo)
{
	SetValue(TextureProperty);
	SetFrameLastUpdatedFromNextCookFrame(EngineInfo);
}

void FTouchEngineDynamicVariableStruct::HandleFloatBufferChanged(const UTouchEngineInfo* EngineInfo)
{
	// SetValue(FloatBufferProperty);
	SetValue(FloatBufferProperty);
	// SetValue(CHOPProperty);
	SetFrameLastUpdatedFromNextCookFrame(EngineInfo);
}

void FTouchEngineDynamicVariableStruct::HandleStringArrayChanged(const UTouchEngineInfo* EngineInfo)
{
	SetValueAsDAT(StringArrayProperty, StringArrayProperty.Num(), 1);
	SetFrameLastUpdatedFromNextCookFrame(EngineInfo);
}

void FTouchEngineDynamicVariableStruct::HandleDropDownBoxValueChanged(const TSharedPtr<FString>& Arg, const UTouchEngineInfo* EngineInfo)
{
	if (VarIntent == EVarIntent::DropDown)
	{
		const FDropDownEntry* EntryPtr = DropDownData.FindByPredicate([&Arg](const FDropDownEntry& Entry)
		{
			return Entry.Value == *Arg;
		});

		if(EntryPtr)
		{
			if (VarType == EVarType::Int)
			{
				SetValue(EntryPtr->Index);
				SetFrameLastUpdatedFromNextCookFrame(EngineInfo);
			}
			else if (VarType == EVarType::String)
			{
				SetValue(EntryPtr->Value);
				SetFrameLastUpdatedFromNextCookFrame(EngineInfo);
			}
		}
	}
}
#endif


bool FTouchEngineDynamicVariableStruct::Serialize(FArchive& Ar)
{
	// write / read all normal variables
	Ar << VarLabel;
	Ar << VarName;
	Ar << VarIdentifier;
	Ar << VarType;
	Ar << VarIntent;
	Ar << Count;
	Ar << Size;
	Ar << bIsArray;

	if (Ar.IsTransacting()) // we only care for the undo/redo buffer
	{
		//todo: this should be saved not just when transacting, so the values would have bounds before the tox file is loaded
		Ar << DefaultValue;
		Ar << ClampMin;
		Ar << ClampMax;
		Ar << UIMin;
		Ar << UIMax;
		
		int DropDownCount = DropDownData.Num();
		Ar << DropDownCount;
		for (int i = 0; i < DropDownCount; ++i)
		{
			if (DropDownData.Num() <= i)
			{
				DropDownData.Add({});
			}
			Ar << DropDownData[i].Index;
			Ar << DropDownData[i].Value;
			Ar << DropDownData[i].Label;
		}
		
		Ar << FrameLastUpdated;
	}
	
	// write editor variables just in case they need to be used //todo: do we need to?
#if WITH_EDITORONLY_DATA

	Ar << FloatBufferProperty;
	Ar << StringArrayProperty;

	Ar << TextureProperty;

	Ar << Vector2DProperty;
	Ar << VectorProperty;
	Ar << Vector4Property;

	Ar << IntPointProperty;
	Ar << IntVectorProperty;

	Ar << IntVector4Property.X;
	Ar << IntVector4Property.Y;
	Ar << IntVector4Property.Z;
	Ar << IntVector4Property.W;

	Ar << ColorProperty;
	TMap<FString, int> FakeDropDownData = TMap<FString, int>(); //todo: remove from serialize
	Ar << FakeDropDownData;

#else

	TArray<float> FloatBufferProperty = TArray<float>();
	TArray<FString> StringArrayProperty = TArray<FString>();

	UTexture* TextureProperty = nullptr;

	FVector2D Vector2DProperty = FVector2D();
	FVector VectorProperty = FVector();
	FVector4 Vector4Property = FVector4();

	FIntPoint IntPointProperty = FIntPoint();
	FIntVector IntVectorProperty = FIntVector();

	FTouchEngineIntVector4 IntVector4Property = FTouchEngineIntVector4();

	FColor ColorProperty = FColor();
	TMap<FString, int> FakeDropDownData = TMap<FString, int>();


	Ar << FloatBufferProperty;
	Ar << StringArrayProperty;

	Ar << TextureProperty;

	Ar << Vector2DProperty;
	Ar << VectorProperty;
	Ar << Vector4Property;

	Ar << IntPointProperty;
	Ar << IntVectorProperty;

	Ar << IntVector4Property.X;
	Ar << IntVector4Property.Y;
	Ar << IntVector4Property.Z;
	Ar << IntVector4Property.W;

	Ar << ColorProperty;
	Ar << FakeDropDownData;

#endif
	
	// write void pointer
	if (Ar.IsSaving())
	{
		// writing dynamic variable to archive
		switch (VarType)
		{
		case EVarType::Bool:
			{
				bool TempBool = GetValueAsBool();
				Ar << TempBool;
				break;
			}
		case EVarType::Int:
			{
				if (Count <= 1)// todo: count? Should we use bIsArray instead? What would happen with an array of 0 or 1 value?
				{
					int TempInt = GetValueAsInt();
					Ar << TempInt;
				}
				else
				{
					for (int i = 0; i < Count; i++)
					{
						int TempIntIndex = Value ? static_cast<int*>(Value)[i] : 0;
						Ar << TempIntIndex;
					}
				}
				break;
			}
		case EVarType::Double:
			{
				if (Count <= 1)// todo: count? Should we use bIsArray instead? What would happen with an array of 0 or 1 value?
				{
					double TempDouble = GetValueAsDouble();
					Ar << TempDouble;
				}
				else
				{
					for (int i = 0; i < Count; i++)
					{
						double TempDoubleIndex = Value ? static_cast<double*>(Value)[i] : 0.0;
						Ar << TempDoubleIndex;
					}
				}
				break;
			}
		case EVarType::Float:
			{
				float TempFloat = GetValueAsFloat();
				Ar << TempFloat;
				break;
			}
		case EVarType::CHOP:
			{
				FTouchEngineCHOP TempChop = GetValueAsCHOP();
				Ar.UsingCustomVersion(FTouchEngineDynamicVariableStructVersion::GUID);
				TempChop.Serialize(Ar);
				break;
			}
		case EVarType::String:
			{
				if (!bIsArray)
				{
					FString TempString = GetValueAsString();
					Ar << TempString;
				}
				else
				{
					TArray<FString> TempStringArray = GetValueAsStringArray();
					Ar << TempStringArray;
				}
				break;
			}
		case EVarType::Texture:
			{
				UTexture* TempTexture = GetValueAsTexture();
				if (!IsValid(TempTexture))
				{
					TempTexture = nullptr;
				}
				Ar << TempTexture;
				break;
			}
		default:
			// unsupported type
			break;
		}
	}
	// read void pointer
	else if (Ar.IsLoading())
	{
		switch (VarType)
		{
		case EVarType::Bool:
			{
				bool TempBool;
				Ar << TempBool;
				SetValue(TempBool);
				break;
			}
		case EVarType::Int:
			{
				if (Count <= 1) // todo: count? Should we use bIsArray instead? What would happen with an array of 0 or 1 value?
				{
					int TempInt;
					Ar << TempInt;
					SetValue(TempInt);
				}
				else
				{
					Value = new int[Count];
					Size = sizeof(int) * Count;

					for (int i = 0; i < Count; i++)
					{
						Ar << static_cast<int*>(Value)[i];
					}
				}
				break;
			}
		case EVarType::Double:
			{
				if (Count <= 1) // todo: count? Should we use bIsArray instead? What would happen with an array of 0 or 1 value?
				{
					double TempDouble;
					Ar << TempDouble;
					SetValue(TempDouble);
				}
				else
				{
					Value = new double[Count];
					Size = sizeof(double) * Count;

					for (int i = 0; i < Count; i++)
					{
						Ar << static_cast<double*>(Value)[i];
					}
				}
				break;
			}
		case EVarType::Float:
			{
				float TempFloat;
				Ar << TempFloat;
				SetValue(TempFloat);
				break;
			}
		case EVarType::CHOP:
			{
				FTouchEngineCHOP TempCHOP;
				
				Ar.UsingCustomVersion(FTouchEngineDynamicVariableStructVersion::GUID);
				const int32 CustomVersion = Ar.CustomVer(FTouchEngineDynamicVariableStructVersion::GUID);

				// we would just discard older data as it is not really needed
				if (CustomVersion >= FTouchEngineDynamicVariableStructVersion::RemovedUTouchEngineCHOP)
				{
					TempCHOP.Serialize(Ar);
				}
				else
				{
					UDEPRECATED_TouchEngineCHOPMinimal* TempUCHOP;
					Ar << TempUCHOP;
					if (TempUCHOP)
					{
						UE_LOG(LogTouchEngineComponent, Warning, TEXT("UDEPRECATED_TouchEngineCHOPMinimal is not null in Ar.IsLoading()"));
						// SetValue(TempCHOP->ToCHOP());
					}
				}

				if (TempCHOP.IsValid()) //todo: double check this part
				{
					SetValue(TempCHOP);
				}
#if WITH_EDITORONLY_DATA
				else if(CHOPProperty.IsValid())
				{
					SetValue(CHOPProperty);
				}
				else
				{
					SetValue(FloatBufferProperty);
				}
#endif
				
				break;
			}
		case EVarType::String:
			{
				if (!bIsArray)
				{
					FString TempString = FString("");
					Ar << TempString;
					SetValue(TempString);
				}
				else
				{
					TArray<FString> TempStringArray;
					Ar << TempStringArray;
					SetValueAsDAT(TempStringArray, Count, Count == 0 ? 0 : Size / Count);
				}
				break;
			}
		case EVarType::Texture:
			{
				UTexture* TempTexture;
				Ar << TempTexture;
				SetValue(TempTexture);
				break;
			}
		default:
			// unsupported type
			break;
		}
	}

	return true;
}

FString FTouchEngineDynamicVariableStruct::ExportValue(const EPropertyPortFlags PortFlags) const
{
	FString ValueStr;
	
	switch (VarType)
	{
	case EVarType::Bool:
		{
			const bool TempValue = GetValueAsBool();
			ExportSingleValue<FBoolProperty>(ValueStr, TempValue, PortFlags);
			break;
		}
	case EVarType::Int:
		{
			if (!bIsArray)
			{
				const int TempValue = GetValueAsInt();
				ExportSingleValueWithDefaults<FIntProperty>(ValueStr, TempValue, PortFlags);
			}
			else
			{
				const TArray<int> TempValue = GetValueAsIntTArray();
				ExportArrayValuesWithDefaults<FIntProperty>(ValueStr, TempValue, PortFlags);
			}
			break;
		}
	case EVarType::Double:
		{
			if (!bIsArray)
			{
				const double TempValue = GetValueAsDouble();
				ExportSingleValueWithDefaults<FDoubleProperty>(ValueStr, TempValue, PortFlags);
			}
			else
			{
				const TArray<double> TempValue = GetValueAsDoubleTArray();
				ExportArrayValuesWithDefaults<FDoubleProperty>(ValueStr, TempValue, PortFlags);
			}
			break;
		}
	case EVarType::Float:
		{
			if (!bIsArray)
			{
				const float TempValue = GetValueAsFloat();
				ExportSingleValueWithDefaults<FFloatProperty>(ValueStr, TempValue, PortFlags);
			}
			else
			{
				const TArray<float> TempValue = GetValueAsFloatTArray();
				ExportArrayValuesWithDefaults<FFloatProperty>(ValueStr, TempValue, PortFlags);
			}
			break;
		}
	case EVarType::CHOP:
		{
			const FTouchEngineCHOP TempValue = GetValueAsCHOP();
			ExportStruct(ValueStr, TempValue, PortFlags);
			break;
		}
	case EVarType::String:
		{
			if (!bIsArray)
			{
				const FString TempValue = GetValueAsString();
				ExportSingleValueWithDefaults<FStrProperty>(ValueStr, TempValue, PortFlags);
			}
			else
			{
				const TArray<FString> TempValue = GetValueAsStringArray();
				ExportArrayValues<FStrProperty>(ValueStr, TempValue, PortFlags);
			}
			break;
		}
	case EVarType::Texture:
		{
			const UTexture* TempValue = GetValueAsTexture();
			ExportSingleValue<FObjectProperty>(ValueStr, TempValue, PortFlags);
			break;
		}
	default:
		// unsupported type, leave empty and will not export
		break;
	}
	
	return ValueStr;
}

const TCHAR* FTouchEngineDynamicVariableStruct::ImportValue(const TCHAR* Buffer, const EPropertyPortFlags PortFlags, FOutputDevice* ErrorText)
{
	switch (VarType)
	{
	case EVarType::Bool:
		{
			bool TempValue = false;
			return ImportAndSetSingleValue<FBoolProperty>(Buffer, TempValue, PortFlags, ErrorText);
		}
	case EVarType::Int:
		{
			if (!bIsArray)
			{
				int TempValue = 0;
				return ImportAndSetSingleValueWithDefaults<FIntProperty>(Buffer, TempValue, PortFlags, ErrorText);
			}
			else
			{
				TArray<int> TempValue;
				return ImportAndSetArrayValuesWithDefaults<FIntProperty>(Buffer, TempValue, PortFlags, ErrorText);
			}
		}
	case EVarType::Double:
		{
			if (!bIsArray)
			{
				double TempValue = 0.0;
				return ImportAndSetSingleValueWithDefaults<FDoubleProperty>(Buffer, TempValue, PortFlags, ErrorText);
			}
			else
			{
				TArray<double> TempValue;
				return ImportAndSetArrayValuesWithDefaults<FDoubleProperty>(Buffer, TempValue, PortFlags, ErrorText);
			}
		}
	case EVarType::Float:
		{
			if (!bIsArray)
			{
				float TempValue = 0.f;
				return ImportAndSetSingleValueWithDefaults<FFloatProperty>(Buffer, TempValue, PortFlags, ErrorText);
			}
			else
			{
				TArray<float> TempValue;
				return ImportAndSetArrayValuesWithDefaults<FFloatProperty>(Buffer, TempValue, PortFlags, ErrorText);
			}
		}
	case EVarType::CHOP:
		{
			FTouchEngineCHOP TempValue;
			return ImportAndSetStruct(Buffer, TempValue, PortFlags, ErrorText);
		}
	case EVarType::String:
		{
			if (!bIsArray)
			{
				FString TempValue;
				return ImportAndSetSingleValueWithDefaults<FStrProperty>(Buffer, TempValue, PortFlags, ErrorText);
			}
			else
			{
				TArray<FString> TempValue;
				return ImportAndSetArrayValues<FStrProperty>(Buffer, TempValue, PortFlags, ErrorText);
			}
		}
	case EVarType::Texture:
		{
			UTexture* TempValue = nullptr;
			return ImportAndSetUObject(Buffer, TempValue, PortFlags, ErrorText);
		}
	default:
		// unsupported type
		break;
	}
	
	return nullptr;
}

bool FTouchEngineDynamicVariableStruct::Identical(const FTouchEngineDynamicVariableStruct* Other, uint32 PortFlags) const
{
	if (Other->VarType == VarType && Other->bIsArray == bIsArray)
	{
		switch (VarType)
		{
		case EVarType::Bool:
			{
				if (Other->GetValueAsBool() == GetValueAsBool())
				{
					return true;
				}
				break;
			}
		case EVarType::Int:
			{
				if (Count <= 1 && !bIsArray)
				{
					if (Other->GetValueAsInt() == GetValueAsInt())
					{
						return true;
					}
				}
				else
				{
					if (Other->Count == Count)
					{
						const int* OtherValue = Other->GetValueAsIntArray();
						const int* ThisValue = GetValueAsIntArray();

						if (OtherValue != nullptr)
						{
							TArray<int> OtherTValue = TArray<int>(), ThisTValue = TArray<int>();

							for (int i = 0; i < Count; i++)
							{
								OtherTValue.Add(OtherValue[i]);
								ThisTValue.Add(ThisValue[i]);
							}

							if (OtherTValue == ThisTValue)
							{
								return true;
							}
						}
					}
				}
				break;
			}
		case EVarType::Double:
			{
				if (Count <= 1 && !bIsArray)
				{
					if (Other->GetValueAsDouble() == GetValueAsDouble())
					{
						return true;
					}
				}
				else
				{
					if (Other->Count == Count)
					{
						const double* OtherValue = Other->GetValueAsDoubleArray();
						const double* ThisValue = GetValueAsDoubleArray();

						if (OtherValue != nullptr)
						{
							TArray<double> OtherTValue = TArray<double>(), ThisTValue = TArray<double>();

							for (int i = 0; i < Count; i++)
							{
								OtherTValue.Add(OtherValue[i]);
								ThisTValue.Add(ThisValue[i]);
							}

							if (OtherTValue == ThisTValue)
							{
								return true;
							}
						}
					}
				}
				break;
			}
		case EVarType::Float:
			{
				if (Other->GetValueAsFloat() == GetValueAsFloat())
				{
					return true;
				}
				break;
			}
		case EVarType::CHOP:
			{
				if (Other->GetValueAsCHOP() == GetValueAsCHOP())
				{
					return true;
				}
				break;
			}
		case EVarType::String:
			{
				if (!bIsArray)
				{
					if (Other->GetValueAsString() == GetValueAsString())
					{
						return true;
					}
				}
				else
				{
					if (Other->GetValueAsStringArray() == GetValueAsStringArray())
					{
						return true;
					}
				}
				break;
			}
		case EVarType::Texture:
			{
				if (Other->GetValueAsTexture() == GetValueAsTexture())
				{
					return true;
				}
				break;
			}
		default:
			{
				// unsupported type
				return false;
			}
		}
	}

	return false;
}


TFuture<bool> FTouchEngineDynamicVariableStruct::SendInput(UE::TouchEngine::FTouchVariableManager& VariableManager, const FTouchEngineInputFrameData& FrameData)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("  I.Bb [GT] Cook Frame - Send Input"), STAT_TE_I_Bb, STATGROUP_TouchEngine);
	
	switch (VarType)
	{
	case EVarType::Bool:
		{
			const bool Op = GetValueAsBool();
			VariableManager.SetBooleanInput(VarIdentifier, Op);
			break;
		}
	case EVarType::Int:
		{
			TArray<int32_t> Op;
			if (Count <= 1)
			{
				Op.Add(GetValueAsInt());
			}
			else
			{
				const int* Buffer = GetValueAsIntArray();
				Op.Append(Buffer, Count);
			}
			VariableManager.SetIntegerInput(VarIdentifier, Op);
			break;
		}
	case EVarType::Double:
		{
			TArray<double> Op;
			if (Count > 1)
			{
				const double* Buffer = GetValueAsDoubleArray();
				Op.Append(Buffer, Count);
			}
			else
			{
				Op.Add(GetValueAsDouble());
			}

			VariableManager.SetDoubleInput(VarIdentifier, Op);
			break;
		}
	case EVarType::Float:
		{
			FTouchEngineCHOPChannel CHOPChannel;
			CHOPChannel.Values.Add(GetValueAsFloat());
			VariableManager.SetCHOPInputSingleSample(VarIdentifier, CHOPChannel);
			break;
		}
	case EVarType::CHOP:
		{
			const FTouchEngineCHOP CHOP = GetValueAsCHOP(); //no need to check if valid as this is checked down the track
			VariableManager.SetCHOPInput(VarIdentifier, CHOP);
			break;
		}
	case EVarType::String:
		{
			if (!bIsArray)
			{
				const auto AnsiString = StringCast<ANSICHAR>(*GetValueAsString());
				const char* TempValue = AnsiString.Get();
				const char* Op{TempValue};
				VariableManager.SetStringInput(VarIdentifier, Op);
			}
			else
			{
				FTouchDATFull Op;
				Op.TableData = TouchObject<TETable>::make_take(TETableCreate());

				TArray<FString> channel = GetValueAsStringArray();

				TETableResize(Op.TableData, channel.Num(), 1);

				for (int i = 0; i < channel.Num(); i++)
				{
					TETableSetStringValue(Op.TableData, i, 0, TCHAR_TO_UTF8(*channel[i]));
				}

				VariableManager.SetTableInput(VarIdentifier, Op);
			}
			break;
		}
	case EVarType::Texture:
		{
			TPromise<bool> Promise;
			TFuture<bool> Future = Promise.GetFuture();
			VariableManager.SetTOPInput(VarIdentifier, GetExportedTexture(), FrameData).Next([Promise = MoveTemp(Promise)](bool bValue) mutable
			{
				Promise.EmplaceValue(bValue);
			});
			return Future;
		}
	default:
		{
			// unimplemented type
			break;
		}
	}

	return MakeFulfilledPromise<bool>(true).GetFuture();
}

void FTouchEngineDynamicVariableStruct::GetOutput(const UTouchEngineInfo* EngineInfo)
{
	if (!EngineInfo)
	{
		return;
	}

	FrameLastUpdated = EngineInfo->GetFrameLastUpdatedForParameter(VarIdentifier);
	
	switch (VarType)
	{
	case EVarType::Bool:
		{
			const bool Op = EngineInfo->GetBooleanOutput(VarIdentifier);
			SetValue(Op);
			break;
		}
	case EVarType::Int:
		{
			const int32 Op = EngineInfo->GetIntegerOutput(VarIdentifier);
			SetValue(Op);
			break;
		}
	case EVarType::Double:
		{
			const double Op = EngineInfo->GetDoubleOutput(VarIdentifier);
			SetValue(Op);
			break;
		}
	case EVarType::Float:
		{
			break;
		}
	case EVarType::CHOP:
		{
			FTouchEngineCHOP Chop;
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      III.B.1a [GT] Post Cook - DynVar - Get Output CHOP"), STAT_TE_III_B_1_CHOPa, STATGROUP_TouchEngine);
				Chop = EngineInfo->GetCHOPOutput(VarIdentifier); //no need to check if valid as this is checked down the track
			}
			{
				DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      III.B.1b [GT] Post Cook - DynVar - Get Output CHOP - SetValue"), STAT_TE_III_B_1_CHOPb, STATGROUP_TouchEngine);
				SetValue(Chop);
			}

			break;
		}
	case EVarType::String:
		{
			if (!bIsArray)
			{
				const TouchObject<TEString> Op = EngineInfo->GetStringOutput(VarIdentifier);
				SetValue(FString(UTF8_TO_TCHAR(Op->string)));
			}
			else
			{
				const FTouchDATFull Op = EngineInfo->GetTableOutput(VarIdentifier);

				TArray<FString> Buffer;

				const int32 RowCount = TETableGetRowCount(Op.TableData);
				const int32 ColumnCount = TETableGetColumnCount(Op.TableData);

				for (int i = 0; i < RowCount; i++)
				{
					for (int j = 0; j < ColumnCount; j++)
					{
						Buffer.Add(UTF8_TO_TCHAR(TETableGetStringValue(Op.TableData, i, j)));
					}
				}

				SetValueAsDAT(Buffer, RowCount, ColumnCount);
			}
			break;
		}
	case EVarType::Texture:
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("    III.B.1 [GT] Post Cook - DynVar - Get Output TOP"), STAT_TE_III_B_1_TOP, STATGROUP_TouchEngine);
			UTexture2D* TOP = EngineInfo->GetTOPOutput(VarIdentifier);
			SetValue(TOP);
			break;
		}
	default:
		{
			// unimplemented type
			return;
		}
	}
}


FText FTouchEngineDynamicVariableStruct::GetTooltip() const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("label"), FText::FromString(VarLabel));
	Args.Add(TEXT("name"), FText::FromString(GetCleanVariableName()));
	Args.Add(TEXT("id"), FText::FromString(VarIdentifier));
	Args.Add(TEXT("type"), StaticEnum<EVarType>()->GetDisplayNameTextByValue(static_cast<int64>(VarType))); //FText::FromString(UEnum::GetValueAsString(VarType))
	Args.Add(TEXT("intent"), StaticEnum<EVarIntent>()->GetDisplayNameTextByValue(static_cast<int64>(VarIntent))); //FText::FromString(UEnum::GetValueAsString(VarIntent))
	Args.Add(TEXT("count"), FText::AsNumber(Count));
	FString DefaultValueStr;
	if (!DefaultValue.IsEmpty())
	{
		switch (VarType)
		{
		case EVarType::Int:
			DefaultValueStr = GetDefaultValueTooltipString<int32>();
			break;
		case EVarType::Double:
			DefaultValueStr = GetDefaultValueTooltipString<double>();
			break;
		case EVarType::Float:
			DefaultValueStr = GetDefaultValueTooltipString<float>();
			break;
		case EVarType::String:
			if (ensure(DefaultValue.GetType() == EVariantTypes::String))
			{
				if (!bIsArray)
				{
					DefaultValueStr = DefaultValue.GetValue<FString>();
				}
			}
			break;
		default:
			break;
		}
	}
	Args.Add(TEXT("default"), !DefaultValueStr.IsEmpty() ? FText::FromString(TEXT("\nDefault:   ") + DefaultValueStr) : FText{});

	FString ClampMinValueStr;
	FString ClampMaxValueStr;
	if (!ClampMin.IsEmpty() || !ClampMax.IsEmpty())
	{
		switch (VarType)
		{
		case EVarType::Int:
			ClampMinValueStr = GetMinOrMaxTooltipString<int32>(ClampMin);
			ClampMaxValueStr = GetMinOrMaxTooltipString<int32>(ClampMax);
			break;
		case EVarType::Double:
			ClampMinValueStr = GetMinOrMaxTooltipString<double>(ClampMin);
			ClampMaxValueStr = GetMinOrMaxTooltipString<double>(ClampMax);
			break;
		case EVarType::Float:
			ClampMinValueStr = GetMinOrMaxTooltipString<float>(ClampMin);
			ClampMaxValueStr = GetMinOrMaxTooltipString<float>(ClampMax);
			break;
		default:
			break;
		}
	}
	Args.Add(TEXT("minmax"), (!ClampMinValueStr.IsEmpty() || !ClampMaxValueStr.IsEmpty()) ?
		FText::FromString(TEXT("\nMin < Max:   ") + ClampMinValueStr + TEXT("  <  ") + ClampMaxValueStr) : FText{});
	
	FString UIMinValueStr;
	FString UIMaxValueStr;
	if (!UIMin.IsEmpty() || !UIMax.IsEmpty())
	{
		switch (VarType)
		{
		case EVarType::Int:
			UIMinValueStr = GetMinOrMaxTooltipString<int32>(UIMin);
			UIMaxValueStr = GetMinOrMaxTooltipString<int32>(UIMax);
			break;
		case EVarType::Double:
			UIMinValueStr = GetMinOrMaxTooltipString<double>(UIMin);
			UIMaxValueStr = GetMinOrMaxTooltipString<double>(UIMax);
			break;
		case EVarType::Float:
			UIMinValueStr = GetMinOrMaxTooltipString<float>(UIMin);
			UIMaxValueStr = GetMinOrMaxTooltipString<float>(UIMax);
			break;
		default:
			break;
		}
	}
	Args.Add(TEXT("uiminmax"), (!UIMinValueStr.IsEmpty() || !UIMaxValueStr.IsEmpty()) ?
		FText::FromString(TEXT("\nUI Min < UI Max:   ") + UIMinValueStr + TEXT("  <  ") + UIMaxValueStr) : FText{});
	
	FText Tooltip = FText::Format(INVTEXT("Label:   {label}\nName:   {name}\nIdentifier:   {id}\nCount:   {count}\nType:   {type}\nIntent:   {intent}{default}{minmax}{uiminmax}"),Args);
	return Tooltip;
}

// ---------------------------------------------------------------------------------------------------------------------
// ------------------------- UDEPRECATED_TouchEngineCHOPMinimal
// ---------------------------------------------------------------------------------------------------------------------

TArray<float> UDEPRECATED_TouchEngineCHOPMinimal::GetChannel(const int32 Index) const
{
	if (Index < NumChannels)
	{
		TArray<float> RetVal;

		for (int32 i = Index * NumSamples; i < (Index * NumSamples) + NumSamples; i++)
		{
			RetVal.Add(ChannelsAppended[i]);
		}

		return RetVal;
	}
	return TArray<float>();
}

void UDEPRECATED_TouchEngineCHOPMinimal::FillFromCHOP(const FTouchEngineCHOP& CHOP)
{
	NumChannels = 0;
	NumSamples = 0;
	ChannelNames.Empty();

	if (CHOP.GetCombinedValues(ChannelsAppended)) //if valid
	{
		NumChannels = CHOP.Channels.Num();
		NumSamples = CHOP.Channels.IsEmpty() ? 0 : CHOP.Channels[0].Values.Num();
		ChannelNames = CHOP.GetChannelNames();
	}
}

FTouchEngineCHOP UDEPRECATED_TouchEngineCHOPMinimal::ToCHOP() const
{
	FTouchEngineCHOP CHOP;

	CHOP.Channels.Reserve(NumChannels);

	for (int i = 0; i < NumChannels; ++i)
	{
		FTouchEngineCHOPChannel ChopChannel;
		ChopChannel.Values = GetChannel(i);
		ChopChannel.Name = ChannelNames.IsValidIndex(i) ? ChannelNames[i] : FString();

		CHOP.Channels.Emplace(ChopChannel);
	}

	return CHOP;
}

// ---------------------------------------------------------------------------------------------------------------------
// ------------------------- UTouchEngineDAT
// ---------------------------------------------------------------------------------------------------------------------

TArray<FString> UTouchEngineDAT::GetRow(const int32 Row)
{
	if (Row < NumRows)
	{
		TArray<FString> RetVal;

		for (int32 Index = Row * NumColumns; Index < Row * NumColumns + NumColumns; Index++)
		{
			RetVal.Add(ValuesAppended[Index]);
		}

		return RetVal;
	}
	else
	{
		return TArray<FString>();
	}
}

TArray<FString> UTouchEngineDAT::GetRowByName(const FString& RowName)
{
	for (int32 i = 0; i < NumRows; i++)
	{
		TArray<FString> Row = GetRow(i);

		if (Row.Num() != 0)
		{
			if (Row[0] == RowName)
			{
				return Row;
			}
		}
	}
	return TArray<FString>();
}

TArray<FString> UTouchEngineDAT::GetColumn(const int32 Column)
{
	if (Column < NumColumns)
	{
		TArray<FString> RetVal;

		for (int Index = Column; Index < NumRows * NumColumns; Index += NumColumns)
		{
			RetVal.Add(ValuesAppended[Index]);
		}

		return RetVal;
	}
	else
	{
		return TArray<FString>();
	}
}

TArray<FString> UTouchEngineDAT::GetColumnByName(const FString& ColumnName)
{
	for (int32 i = 0; i < NumColumns; i++)
	{
		TArray<FString> Col = GetColumn(i);

		if (Col.Num() != 0)
		{
			if (Col[0] == ColumnName)
			{
				return Col;
			}
		}
	}
	return TArray<FString>();
}

FString UTouchEngineDAT::GetCell(const int32 Column, const int32 Row)
{
	if (Column < NumColumns && Row < NumRows)
	{
		const int32 Index = Row * NumColumns + Column;
		return ValuesAppended[Index];
	}
	else
	{
		return FString();
	}
}

FString UTouchEngineDAT::GetCellByName(const FString& ColumnName, const FString& RowName)
{
	int RowNum = -1, ColNum = -1;

	for (int32 i = 0; i < NumRows; i++)
	{
		TArray<FString> Row = GetRow(i);

		if (Row[0] == RowName)
		{
			RowNum = i;
			break;
		}
	}

	for (int i = 0; i < NumColumns; i++)
	{
		TArray<FString> Col = GetColumn(i);

		if (Col[0] == ColumnName)
		{
			ColNum = i;
			break;
		}
	}

	if (RowNum == -1 || ColNum == -1)
	{
		return FString();
	}

	return GetCell(ColNum, RowNum);
}

void UTouchEngineDAT::CreateChannels(const TArray<FString>& AppendedArray, const int32 RowCount, const int32 ColumnCount)
{
	if (RowCount * ColumnCount != AppendedArray.Num())
	{
		return;
	}

	ValuesAppended = AppendedArray;
	NumRows = RowCount;
	NumColumns = ColumnCount;
}
