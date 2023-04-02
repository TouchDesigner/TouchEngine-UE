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

#include "TouchEngineDynamicVariableStructVersion.h"
#include "Blueprint/TouchEngineComponent.h"
#include "Engine/TouchEngine.h"
#include "Engine/TouchEngineInfo.h"

#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/SlateTypes.h"


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
				InVarsCopy[j].SetValue(&DynVars_Input[i]);
			}
		}
	}
	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		for (int j = 0; j < OutVarsCopy.Num(); j++)
		{
			if (DynVars_Output[i].VarName == OutVarsCopy[j].VarName && DynVars_Output[i].VarType == OutVarsCopy[j].VarType && DynVars_Output[i].bIsArray == OutVarsCopy[j].bIsArray)
			{
				OutVarsCopy[j].SetValue(&DynVars_Output[i]);
			}
		}
	}

	DynVars_Input = MoveTemp(InVarsCopy);
	DynVars_Output = MoveTemp(OutVarsCopy);
}

void FTouchEngineDynamicVariableContainer::Reset()
{
	DynVars_Input = {};
	DynVars_Output = {};
}

void FTouchEngineDynamicVariableContainer::SendInputs(UTouchEngineInfo* EngineInfo)
{
	for (int32 i = 0; i < DynVars_Input.Num(); i++)
	{
		DynVars_Input[i].SendInput(EngineInfo);
	}
}

void FTouchEngineDynamicVariableContainer::GetOutputs(UTouchEngineInfo* EngineInfo)
{
	for (int32 i = 0; i < DynVars_Output.Num(); i++)
	{
		DynVars_Output[i].GetOutput(EngineInfo);
	}
}

void FTouchEngineDynamicVariableContainer::SendInput(UTouchEngineInfo* EngineInfo, const int32 Index)
{
	if (Index < DynVars_Input.Num())
	{
		DynVars_Input[Index].SendInput(EngineInfo);
	}
}

void FTouchEngineDynamicVariableContainer::GetOutput(UTouchEngineInfo* EngineInfo, const int32 Index)
{
	if (Index < DynVars_Output.Num())
	{
		DynVars_Output[Index].GetOutput(EngineInfo);
	}
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

FTouchEngineDynamicVariableStruct::~FTouchEngineDynamicVariableStruct()
{
	Clear();
}

void FTouchEngineDynamicVariableStruct::Copy(const FTouchEngineDynamicVariableStruct* Other)
{
	VarLabel = Other->VarLabel;
	VarName = Other->VarName;
	VarIdentifier = Other->VarIdentifier;
	VarType = Other->VarType;
	VarIntent = Other->VarIntent;
	Count = Other->Count;
	Size = Other->Size;
	bIsArray = Other->bIsArray;

	SetValue(Other);
}

void FTouchEngineDynamicVariableStruct::Clear()
{
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
				delete[](bool*)Value;
			}
			else
			{
				delete (bool*)Value;
			}
			break;
		}
	case EVarType::Int:
		{
			if (Count <= 1 && !bIsArray)
			{
				delete (int*)Value;
			}
			else
			{
				delete[](int*)Value;
			}
			break;
		}
	case EVarType::Double:
		{
			if (Count <= 1 && !bIsArray)
			{
				delete (double*)Value;
			}
			else
			{
				delete[](double*)Value;
			}
			break;
		}
	case EVarType::Float:
		{
			if (bIsArray)
			{
				delete[](float*)Value;
			}
			else
			{
				delete (float*)Value;
			}
			break;
		}
	case EVarType::CHOP:
		{
			for (int i = 0; i < Count; i++)
			{
				delete[]((float**)Value)[i];
			}

			delete[](float**)Value;
			break;
		}
	case EVarType::String:
		{
			if (!bIsArray)
			{
				delete[](char*)Value;
			}
			else
			{
				for (int i = 0; i < Count; i++)
				{
					delete[]((char**)Value)[i];
				}

				delete[](char**)Value;
			}
			break;
		}
	case EVarType::Texture:
		{
			Value = nullptr;
			break;
		}
	}

	Value = nullptr;
	ChannelNames.Reset();
}


bool FTouchEngineDynamicVariableStruct::GetValueAsBool() const
{
	return Value ? *(bool*)Value : false;
}

int FTouchEngineDynamicVariableStruct::GetValueAsInt() const
{
	return Value ? *(int*)Value : 0;
}

int FTouchEngineDynamicVariableStruct::GetValueAsIntIndexed(const int Index) const
{
	return Value ? ((int*)Value)[Index] : 0;
}

int* FTouchEngineDynamicVariableStruct::GetValueAsIntArray() const
{
	return Value ? (int*)Value : 0;
}

TArray<int> FTouchEngineDynamicVariableStruct::GetValueAsIntTArray() const
{
	if (VarType != EVarType::Int || !bIsArray)
	{
		return TArray<int>();
	}

	TArray<int> returnArray((int*)Value, Count);
	return returnArray;
}

double FTouchEngineDynamicVariableStruct::GetValueAsDouble() const
{
	return Value ? *(double*)Value : 0;
}

double FTouchEngineDynamicVariableStruct::GetValueAsDoubleIndexed(const int Index) const
{
	return Value ? GetValueAsDoubleArray()[Index] : 0;
}

double* FTouchEngineDynamicVariableStruct::GetValueAsDoubleArray() const
{
	return Value ? (double*)Value : 0;
}

TArray<double> FTouchEngineDynamicVariableStruct::GetValueAsDoubleTArray() const
{
	if (VarType != EVarType::Double || !bIsArray)
	{
		return TArray<double>();
	}

	TArray<double> returnArray((double*)Value, Count);
	return returnArray;
}

float FTouchEngineDynamicVariableStruct::GetValueAsFloat() const
{
	return Value ? *(float*)Value : 0;
}

FString FTouchEngineDynamicVariableStruct::GetValueAsString() const
{
	return Value ? FString(UTF8_TO_TCHAR((char*)Value)) : FString("");
}

TArray<FString> FTouchEngineDynamicVariableStruct::GetValueAsStringArray() const
{
	TArray<FString> TempValue = TArray<FString>();

	if (!Value)
	{
		return TempValue;
	}

	char** Buffer = (char**)Value;

	for (int i = 0; i < Count; i++)
	{
		TempValue.Add(Buffer[i]);
	}
	return TempValue;
}

UTexture* FTouchEngineDynamicVariableStruct::GetValueAsTexture() const
{
	return (UTexture*)Value;
}

UDEPRECATED_TouchEngineCHOPMinimal* FTouchEngineDynamicVariableStruct::GetValueAsCHOP_DEPRECATED() const
{
	if (!Value)
	{
		return nullptr;
	}

	float** TempBuffer = (float**)Value;

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

	float** TempBuffer = (float**)Value;
	const int ChannelLength = (Size / sizeof(float)) / Count;

	return FTouchEngineCHOP::FromChannels(TempBuffer, Count, ChannelLength, ChannelNames);
}

FTouchEngineCHOP FTouchEngineDynamicVariableStruct::GetValueAsCHOP(const UTouchEngineInfo* EngineInfo) const
{
	FTouchEngineCHOP Chop = GetValueAsCHOP();

	if (EngineInfo && EngineInfo->Engine)
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

	char** Buffer = (char**)Value;

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
	if (newValue == nullptr)
	{
		Clear();
		Value = nullptr;
		return;
	}

	Value = (void*)newValue;
	Size = _size;
}

void FTouchEngineDynamicVariableStruct::SetValue(const bool InValue)
{
	if (VarType == EVarType::Bool)
	{
		Clear();

		Value = new bool;
		*((bool*)Value) = InValue;
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(const int32 InValue)
{
	if (VarType == EVarType::Int)
	{
		Clear();

		Value = new int;
		*((int*)Value) = InValue;
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
			((int*)Value)[i] = InValue[i];
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
		*((double*)Value) = InValue;
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
			((double*)Value)[i] = InValue[i];
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
				ColorProperty.R = InValue[0];
				ColorProperty.G = InValue[1];
				ColorProperty.B = InValue[2];
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
				ColorProperty.R = InValue[0];
				ColorProperty.G = InValue[1];
				ColorProperty.B = InValue[2];
				ColorProperty.A = InValue[3];
			}
			break;
		}
	}

#endif
}

void FTouchEngineDynamicVariableStruct::SetValue(const float InValue)
{
	if (VarType == EVarType::Float)
	{
		Clear();

		Value = new float;
		*((float*)Value) = InValue;
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
			((float*)Value)[i] = InValue[i];
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
					ColorProperty = FColor(InValue[0], InValue[1], InValue[2], InValue[3]);
				}
				else if (InValue.Num() == 3)
				{
					ColorProperty = FColor(InValue[0], InValue[1], InValue[2], 1.f);
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
		}

#endif

		Clear();
		Value = new double[InValue.Num()];

		for (int i = 0; i < InValue.Num(); i++)
		{
			((double*)Value)[i] = (double)(InValue[i]);
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

	const TArray<float>& Data = InValue.GetCombinedValues(); // will return an empty array if not valid
	if (Data.IsEmpty())
	{
		return;
	}
	
	Count = InValue.Channels.Num();
	const int32 ChannelLength = Data.Num() / Count;
	Size = Data.Num() * sizeof(float);
	bIsArray = true;

	Value = new float*[Count];

	int32 Index = 0;
	for (int i = 0; i < Count; i++)
	{
		((float**)Value)[i] = new float[ChannelLength];

		for (int j = 0; j < ChannelLength; j++)
		{
			((float**)Value)[i][j] = Data[Index];
			++Index;
		}
	}

	ChannelNames = InValue.GetChannelNames();

	TArray<FString> NonEmptyChannelNames(ChannelNames);
	NonEmptyChannelNames.Remove(FString());
	const TSet<FString> UniqueNames(NonEmptyChannelNames);
	if (!UniqueNames.IsEmpty() && UniqueNames.Num() != NonEmptyChannelNames.Num())
	{
		UE_LOG(LogTouchEngineComponent, Warning, TEXT("Some Channels of the CHOP Data sent to the Input `%s` have the same name:\n%s"), *VarLabel, *InValue.ToString());
	}
}

void FTouchEngineDynamicVariableStruct::SetValueAsCHOP(const TArray<float>& InValue, const int NumChannels, const int NumSamples)
{
	if (VarType != EVarType::CHOP)
	{
		return;
	}

	Clear();

	if (!InValue.Num() || InValue.Num() != NumChannels * NumSamples)
	{
		return;
	}

	Count = NumChannels;
	Size = NumSamples * NumChannels * sizeof(float);
	bIsArray = true;

	Value = new float*[Count];

	for (int i = 0; i < NumChannels; i++)
	{
		((float**)Value)[i] = new float[NumSamples];

		for (int j = 0; j < NumSamples; j++)
		{
			((float**)Value)[i][j] = InValue[i * NumSamples + j];
		}
	}
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
		Clear();

		const auto AnsiString = StringCast<ANSICHAR>(*InValue);
		const char* Buffer = AnsiString.Get();

		Value = new char[strlen(Buffer) + 1];

		for (int i = 0; i < strlen(Buffer) + 1; i++)
		{
			((char*)Value)[i] = Buffer[i];
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

		((char**)Value)[i] = new char[(InValue[i]).Len() + 1];
		Size += InValue[i].Len() + 1;
		for (int j = 0; j < InValue[i].Len() + 1; j++)
		{
			((char**)Value)[i][j] = TempValue[j];
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
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(const FTouchEngineDynamicVariableStruct* Other)
{
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
	}

#if WITH_EDITORONLY_DATA

	FloatBufferProperty = Other->FloatBufferProperty;
	StringArrayProperty = Other->StringArrayProperty;
	TextureProperty = Other->TextureProperty;

	Vector2DProperty = Other->Vector2DProperty;
	VectorProperty = Other->VectorProperty;
	Vector4Property = Other->Vector4Property;
	ColorProperty = Other->ColorProperty;

	IntPointProperty = Other->IntPointProperty;
	IntVectorProperty = Other->IntVectorProperty;
	IntVector4Property = Other->IntVector4Property;

	DropDownData = Other->DropDownData;

#endif
}


void FTouchEngineDynamicVariableStruct::HandleChecked(const ECheckBoxState InState)
{
	switch (InState)
	{
	case ECheckBoxState::Unchecked:
		{
			SetValue(false);
			break;
		}
	case ECheckBoxState::Checked:
		{
			SetValue(true);
			break;
		}
	case ECheckBoxState::Undetermined:
	default:
		break;
	}
}

void FTouchEngineDynamicVariableStruct::HandleTextBoxTextCommitted(const FText& NewText)
{
	SetValue(NewText.ToString());
}

void FTouchEngineDynamicVariableStruct::HandleTextureChanged()
{
#if WITH_EDITORONLY_DATA
	SetValue(TextureProperty);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleColorChanged()
{
#if WITH_EDITORONLY_DATA
	TArray<float> Buffer;

	Buffer.Add(ColorProperty.R);
	Buffer.Add(ColorProperty.G);
	Buffer.Add(ColorProperty.B);
	Buffer.Add(ColorProperty.A);

	SetValue(Buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleVector2Changed()
{
#if WITH_EDITORONLY_DATA
	TArray<float> Buffer;

	Buffer.Add(Vector2DProperty.X);
	Buffer.Add(Vector2DProperty.Y);

	SetValue(Buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleVectorChanged()
{
#if WITH_EDITORONLY_DATA
	TArray<float> Buffer;

	Buffer.Add(VectorProperty.X);
	Buffer.Add(VectorProperty.Y);
	Buffer.Add(VectorProperty.Z);

	SetValue(Buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleVector4Changed()
{
#if WITH_EDITORONLY_DATA
	TArray<float> Buffer;

	Buffer.Add(Vector4Property.X);
	Buffer.Add(Vector4Property.Y);
	Buffer.Add(Vector4Property.Z);
	Buffer.Add(Vector4Property.W);

	SetValue(Buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleIntVector2Changed()
{
#if WITH_EDITORONLY_DATA
	TArray<int> Buffer;

	Buffer.Add(IntPointProperty.X);
	Buffer.Add(IntPointProperty.Y);

	SetValue(Buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleIntVectorChanged()
{
#if WITH_EDITORONLY_DATA
	TArray<int> Buffer;

	Buffer.Add(IntVectorProperty.X);
	Buffer.Add(IntVectorProperty.Y);
	Buffer.Add(IntVectorProperty.Z);

	SetValue(Buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleIntVector4Changed()
{
#if WITH_EDITORONLY_DATA
	TArray<int> Buffer;

	Buffer.Add(IntVector4Property.X);
	Buffer.Add(IntVector4Property.Y);
	Buffer.Add(IntVector4Property.Z);
	Buffer.Add(IntVector4Property.W);

	SetValue(Buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleFloatBufferChanged()
{
#if WITH_EDITORONLY_DATA
	SetValue(FloatBufferProperty);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleFloatBufferChildChanged()
{
#if WITH_EDITORONLY_DATA
	SetValue(FloatBufferProperty);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleStringArrayChanged()
{
#if WITH_EDITORONLY_DATA

	SetValueAsDAT(StringArrayProperty, StringArrayProperty.Num(), 1);

#endif
}

void FTouchEngineDynamicVariableStruct::HandleStringArrayChildChanged()
{
#if WITH_EDITORONLY_DATA

	SetValueAsDAT(StringArrayProperty, StringArrayProperty.Num(), 1);

#endif
}

void FTouchEngineDynamicVariableStruct::HandleDropDownBoxValueChanged(const TSharedPtr<FString> Arg)
{
#if WITH_EDITORONLY_DATA
	SetValue((int)DropDownData[*Arg]);
#endif
}


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
	// write editor variables just in case they need to be used
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
	Ar << DropDownData;

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
	TMap<FString, int> DropDownData = TMap<FString, int>();


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
	Ar << DropDownData;

#endif

	
	// write void pointer
	if (Ar.IsSaving())
	{
		// writing dynamic variable to archive
		switch (VarType)
		{
		case EVarType::Bool:
			{
				bool TempBool = false;
				if (Value)
				{
					TempBool = GetValueAsBool();
				}
				Ar << TempBool;
				break;
			}
		case EVarType::Int:
			{
				if (Count <= 1)
				{
					int TempInt = 0;
					if (Value)
					{
						TempInt = GetValueAsInt();
					}
					Ar << TempInt;
				}
				else
				{
					for (int i = 0; i < Count; i++)
					{
						if (Value)
						{
							int TempIntIndex = ((int*)Value)[i];
							Ar << TempIntIndex;
						}
						else
						{
							int TempIntIndex = 0;
							Ar << TempIntIndex;
						}
					}
				}
				break;
			}
		case EVarType::Double:
			{
				if (Count <= 1)
				{
					double TempDouble = 0;
					if (Value)
					{
						TempDouble = GetValueAsDouble();
					}
					Ar << TempDouble;
				}
				else
				{
					for (int i = 0; i < Count; i++)
					{
						if (Value)
						{
							Ar << ((double*)Value)[i];
						}
						else
						{
							double TempDoubleIndex = 0;
							Ar << TempDoubleIndex;
						}
					}
				}
				break;
			}
		case EVarType::Float:
			{
				float TempFloat = 0;
				if (Value)
				{
					TempFloat = GetValueAsFloat();
				}
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
					FString TempString = FString("");

					if (Value)
					{
						TempString = GetValueAsString();
					}
					Ar << TempString;
				}
				else
				{
					TArray<FString> TempStringArray;

					if (Value)
					{
						TempStringArray = GetValueAsStringArray();
					}
					Ar << TempStringArray;
				}
				break;
			}
		case EVarType::Texture:
			{
				UTexture* TempTexture = nullptr;

				if (Value)
				{
					TempTexture = GetValueAsTexture();

					if (!IsValid(TempTexture))
					{
						TempTexture = nullptr;
					}
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
		// reading dynamic variables from archive
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
				if (Count <= 1)
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
						Ar << ((int*)Value)[i];
					}
				}
				break;
			}
		case EVarType::Double:
			{
				if (Count <= 1)
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
						Ar << ((double*)Value)[i];
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
						UE_LOG(LogTemp, Warning, TEXT("Not null!"));
						// SetValue(TempCHOP->ToCHOP());
					}
				}
				SetValue(TempCHOP);
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

					SetValueAsDAT(TempStringArray, Count, Size / Count);
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
				break;
			}
		}
	}

	return false;
}


void FTouchEngineDynamicVariableStruct::SendInput(UTouchEngineInfo* EngineInfo)
{
	if (!EngineInfo)
	{
		return;
	}

	switch (VarType)
	{
	case EVarType::Bool:
		{
			if (VarIntent == EVarIntent::Momentary || VarIntent == EVarIntent::Pulse)
			{
				if (GetValueAsBool() == true)
				{
					TTouchVar<bool> Op;
					Op.Data = true;
					EngineInfo->SetBooleanInput(VarIdentifier, Op);
					SetValue(false);
				}
			}
			else
			{
				TTouchVar<bool> Op;
				Op.Data = GetValueAsBool();
				EngineInfo->SetBooleanInput(VarIdentifier, Op);
			}
			break;
		}
	case EVarType::Int:
		{
			TTouchVar<TArray<int32_t>> Op;
			if (Count <= 1)
			{
				Op.Data.Add(GetValueAsInt());
			}
			else
			{
				const int* Buffer = GetValueAsIntArray();
				for (int i = 0; i < Count; i++)
				{
					Op.Data.Add(Buffer[i]);
				}
			}

			EngineInfo->SetIntegerInput(VarIdentifier, Op);
			break;
		}
	case EVarType::Double:
		{
			TTouchVar<TArray<double>> Op;

			if (Count > 1)
			{
				if (VarIntent == EVarIntent::Color) // Colors in UE are stored from 0-255, colors in TD are set from 0-1
				{
					const double* Buffer = GetValueAsDoubleArray();
					for (int i = 0; i < Count; i++)
					{
						Op.Data.Add((float)(Buffer[i]) / 255.f);
					}
				}
				else
				{
					const double* Buffer = GetValueAsDoubleArray();
					for (int i = 0; i < Count; i++)
					{
						Op.Data.Add(Buffer[i]);
					}
				}
			}
			else
			{
				Op.Data.Add(GetValueAsDouble());
			}

			EngineInfo->SetDoubleInput(VarIdentifier, Op);
			break;
		}
	case EVarType::Float:
		{
			FTouchEngineCHOPChannel TCSS;
			TCSS.Values.Add(GetValueAsFloat());
			EngineInfo->SetCHOPChannelInput(VarIdentifier, TCSS);
			break;
		}
	case EVarType::CHOP:
		{
			const FTouchEngineCHOP CHOP = GetValueAsCHOP(); //no need to check if valid as this is checked down the track
			EngineInfo->SetCHOPInput(VarIdentifier, CHOP);
			break;
		}
	case EVarType::String:
		{
			if (!bIsArray)
			{
				const auto AnsiString = StringCast<ANSICHAR>(*GetValueAsString());
				const char* TempValue = AnsiString.Get();
				TTouchVar<const char*> Op{TempValue};
				EngineInfo->SetStringInput(VarIdentifier, Op);
			}
			else
			{
				FTouchDATFull Op;
				Op.ChannelData = TETableCreate();

				TArray<FString> channel = GetValueAsStringArray();

				TETableResize(Op.ChannelData, channel.Num(), 1);

				for (int i = 0; i < channel.Num(); i++)
				{
					TETableSetStringValue(Op.ChannelData, i, 0, TCHAR_TO_UTF8(*channel[i]));
				}

				EngineInfo->SetTableInput(VarIdentifier, Op);
				TERelease(&Op.ChannelData);
			}
			break;
		}
	case EVarType::Texture:
		{
			EngineInfo->SetTOPInput(VarIdentifier, GetValueAsTexture());
			break;
		}
	default:
		{
			// unimplemented type

			break;
		}
	}
}

void FTouchEngineDynamicVariableStruct::GetOutput(UTouchEngineInfo* EngineInfo)
{
	if (!EngineInfo)
	{
		return;
	}

	switch (VarType)
	{
	case EVarType::Bool:
		{
			const TTouchVar<bool> Op = EngineInfo->GetBooleanOutput(VarIdentifier);
			SetValue(Op.Data);
			break;
		}
	case EVarType::Int:
		{
			const TTouchVar<int32> Op = EngineInfo->GetIntegerOutput(VarIdentifier);
			SetValue(Op.Data);
			break;
		}
	case EVarType::Double:
		{
			const TTouchVar<double> Op = EngineInfo->GetDoubleOutput(VarIdentifier);
			SetValue(Op.Data);
			break;
		}
	case EVarType::Float:
		{
			break;
		}
	case EVarType::CHOP:
		{
			const FTouchEngineCHOP Chop = EngineInfo->GetCHOPOutput(VarIdentifier); //no need to check if valid as this is checked down the track
			SetValue(Chop);

			break;
		}
	case EVarType::String:
		{
			if (!bIsArray)
			{
				const TTouchVar<TEString*> Op = EngineInfo->GetStringOutput(VarIdentifier);
				SetValue(FString(UTF8_TO_TCHAR(Op.Data->string)));
			}
			else
			{
				const FTouchDATFull Op = EngineInfo->GetTableOutput(VarIdentifier);

				TArray<FString> Buffer;

				const int32 rowcount = TETableGetRowCount(Op.ChannelData), columncount = TETableGetColumnCount(Op.ChannelData);

				for (int i = 0; i < rowcount; i++)
				{
					for (int j = 0; j < columncount; j++)
					{
						Buffer.Add(UTF8_TO_TCHAR(TETableGetStringValue(Op.ChannelData, i, j)));
					}
				}

				SetValueAsDAT(Buffer, rowcount, columncount);
			}
			break;
		}
	case EVarType::Texture:
		{
			UTexture2D* TOP = EngineInfo->GetTOPOutput(VarIdentifier);
			SetValue(TOP);
			break;
		}
	default:
		{
			// unimplemented type
			break;
		}
	}
}


FText FTouchEngineDynamicVariableStruct::GetTooltip() const
{
	FString OutString = VarName;
	OutString.RemoveFromStart("p/");
	OutString.RemoveFromStart("i/");
	OutString.RemoveFromStart("o/");

	return FText::FromString(OutString);
}

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

	ChannelsAppended = CHOP.GetCombinedValues(); // this returns an empty array if not valid
	if (!ChannelsAppended.IsEmpty())
	{
		NumChannels = CHOP.Channels.Num();
		NumSamples = CHOP.Channels[0].Values.Num();
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
