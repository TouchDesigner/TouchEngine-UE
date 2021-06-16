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
#include "TouchEngineComponent.h"
#include "UTouchEngine.h"
#include "TouchEngineInfo.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/UObjectIterator.h"


FTouchEngineDynamicVariableContainer::FTouchEngineDynamicVariableContainer()
{
}

FTouchEngineDynamicVariableContainer::~FTouchEngineDynamicVariableContainer()
{
	if (OnDestruction.IsBound())
		OnDestruction.Execute();

	if (parent)
		parent->UnbindDelegates();

	OnToxLoaded.Clear();
	OnToxFailedLoad.Clear();
}


FDelegateHandle FTouchEngineDynamicVariableContainer::CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (parent && parent->EngineInfo && parent->EngineInfo->IsLoaded())
	{
		// we're in a world object that is already loaded
		Delegate.Execute();
		// bind delegate anyway so this object gets called in future calls
		return OnToxLoaded.Add(Delegate);
	}
	else
	{
		// ensure delegate is valid and not already bound
		if (!(Delegate.GetUObject() != nullptr ? OnToxLoaded.IsBoundToObject(Delegate.GetUObject()) : false))
		{
			// bind delegate so it gets called when tox file is loaded
			return OnToxLoaded.Add(Delegate);
		}
	}

	// delegte is not valid or already bound
	return FDelegateHandle();
}

void FTouchEngineDynamicVariableContainer::Unbind_OnToxLoaded(FDelegateHandle DelegateHandle)
{
	if (DelegateHandle.IsValid() && OnToxLoaded.IsBound())
		OnToxLoaded.Remove(DelegateHandle);
}

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
			if (DynVars_Input[i].VarName == InVarsCopy[j].VarName && DynVars_Input[i].VarType == InVarsCopy[j].VarType && DynVars_Input[i].IsArray == InVarsCopy[j].IsArray)
			{
				InVarsCopy[j].SetValue(&DynVars_Input[i]);
			}
		}
	}
	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		for (int j = 0; j < OutVarsCopy.Num(); j++)
		{
			if (DynVars_Output[i].VarName == OutVarsCopy[j].VarName && DynVars_Output[i].VarType == OutVarsCopy[j].VarType && DynVars_Output[i].IsArray == OutVarsCopy[j].IsArray)
			{
				OutVarsCopy[j].SetValue(&DynVars_Output[i]);
			}
		}
	}

	DynVars_Input.Empty();
	DynVars_Output.Empty();

	DynVars_Input = InVarsCopy;
	DynVars_Output = OutVarsCopy;

	parent->OnToxLoaded.Broadcast();

	if (parent->SendMode == ETouchEngineSendMode::OnAccess)
	{
		if (parent->EngineInfo)
		{
			SendInputs(parent->EngineInfo);
			GetOutputs(parent->EngineInfo);
		}
	}

	OnToxLoaded.Broadcast();

	parent->UnbindDelegates();
}

void FTouchEngineDynamicVariableContainer::ValidateParameters(const TArray<FTouchEngineDynamicVariableStruct>& VariablesIn, const TArray<FTouchEngineDynamicVariableStruct>& VariablesOut)
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
			if (DynVars_Input[i].VarName == InVarsCopy[j].VarName && DynVars_Input[i].VarType == InVarsCopy[j].VarType && DynVars_Input[i].IsArray == InVarsCopy[j].IsArray)
			{
				InVarsCopy[j].SetValue(&DynVars_Input[i]);
			}
		}
	}
	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		for (int j = 0; j < OutVarsCopy.Num(); j++)
		{
			if (DynVars_Output[i].VarName == OutVarsCopy[j].VarName && DynVars_Output[i].VarType == OutVarsCopy[j].VarType && DynVars_Output[i].IsArray == OutVarsCopy[j].IsArray)
			{
				OutVarsCopy[j].SetValue(&DynVars_Output[i]);
			}
		}
	}

	DynVars_Input.Empty();
	DynVars_Output.Empty();

	DynVars_Input = InVarsCopy;
	DynVars_Output = OutVarsCopy;
}

FDelegateHandle FTouchEngineDynamicVariableContainer::CallOrBind_OnToxFailedLoad(FTouchOnLoadFailed::FDelegate Delegate)
{
	if (parent && parent->EngineInfo && parent->EngineInfo->HasFailedLoad())
	{
		// we're in a world object with a tox file loaded
		Delegate.Execute(parent->EngineInfo->GetFailureMessage());
		// bind delegate anyway so this object gets called in future calls
		return OnToxFailedLoad.Add(Delegate);
	}
	else
	{
		// ensure delegate is valid and nout already bound
		if (!(Delegate.GetUObject() != nullptr ? OnToxLoaded.IsBoundToObject(Delegate.GetUObject()) : false))
		{
			// bind delegate so it gets called when tox file is loaded
			return OnToxFailedLoad.Add(Delegate);
		}
	}

	// delegte is not valid or already bound
	return FDelegateHandle();
}

void FTouchEngineDynamicVariableContainer::Unbind_OnToxFailedLoad(FDelegateHandle Handle)
{
	if (Handle.IsValid() && OnToxFailedLoad.IsBound())
		OnToxFailedLoad.Remove(Handle);
}

void FTouchEngineDynamicVariableContainer::ToxFailedLoad(FString Error)
{
	parent->OnToxFailedLoad.Broadcast(Error);
	OnToxFailedLoad.Broadcast(Error);

	parent->UnbindDelegates();
}



void FTouchEngineDynamicVariableContainer::SendInputs(UTouchEngineInfo* EngineInfo)
{
	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		DynVars_Input[i].SendInput(EngineInfo);
	}
}

void FTouchEngineDynamicVariableContainer::GetOutputs(UTouchEngineInfo* EngineInfo)
{
	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		DynVars_Output[i].GetOutput(EngineInfo);
	}
}

void FTouchEngineDynamicVariableContainer::SendInput(UTouchEngineInfo* EngineInfo, int Index)
{
	if (Index >= DynVars_Input.Num())
		return;

	DynVars_Input[Index].SendInput(EngineInfo);
}

void FTouchEngineDynamicVariableContainer::GetOutput(UTouchEngineInfo* EngineInfo, int Index)
{
	if (Index >= DynVars_Output.Num())
		return;

	DynVars_Output[Index].GetOutput(EngineInfo);
}

FTouchEngineDynamicVariableStruct* FTouchEngineDynamicVariableContainer::GetDynamicVariableByName(FString VarName)
{
	FTouchEngineDynamicVariableStruct* var = nullptr;

	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		if (DynVars_Input[i].VarName == VarName)
		{
			if (!var)
			{
				var = &DynVars_Input[i];
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
			if (!var)
			{
				var = &DynVars_Output[i];
			}
			else
			{
				// variable with duplicate names, don't try to distinguish between them
				return nullptr;
			}
		}
	}
	return var;
}

FTouchEngineDynamicVariableStruct* FTouchEngineDynamicVariableContainer::GetDynamicVariableByIdentifier(FString VarIdentifier)
{

	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		if (DynVars_Input[i].VarIdentifier.Equals(VarIdentifier) || DynVars_Input[i].VarLabel.Equals(VarIdentifier) || DynVars_Input[i].VarName.Equals(VarIdentifier))
			return &DynVars_Input[i];
	}

	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		if (DynVars_Output[i].VarIdentifier.Equals(VarIdentifier) || DynVars_Output[i].VarLabel.Equals(VarIdentifier) || DynVars_Output[i].VarName.Equals(VarIdentifier))
			return &DynVars_Output[i];
	}
	return nullptr;
}

bool FTouchEngineDynamicVariableContainer::Serialize(FArchive& Ar)
{
	return false;
}



FTouchEngineDynamicVariableStruct::FTouchEngineDynamicVariableStruct()
{
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
	IsArray = Other->IsArray;

	SetValue(Other);
}

void FTouchEngineDynamicVariableStruct::Clear()
{
	if (!Value)
	{
		return;
	}

	
	switch (VarType)
	{
	case EVarType::Bool:
	{
		delete[](bool*)Value;
		break;
	}
	case EVarType::Int:
	{
		if (Count <= 1 && !IsArray)
		{
			delete (int*)Value;
		}
		else
		{
			delete[](int*) Value;
		}
		break;
	}
	case EVarType::Double:
	{
		if (Count <= 1 && !IsArray)
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
		delete[](float*)Value;
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
		if (!IsArray)
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
}


bool FTouchEngineDynamicVariableStruct::GetValueAsBool() const
{
	return Value ? *(bool*)Value : false;
}

int FTouchEngineDynamicVariableStruct::GetValueAsInt() const
{
	return Value ? *(int*)Value : 0;
}

int FTouchEngineDynamicVariableStruct::GetValueAsIntIndexed(int Index) const
{
	return Value ? ((int*)Value)[Index] : 0;
}

int* FTouchEngineDynamicVariableStruct::GetValueAsIntArray() const
{
	return Value ? (int*)Value : 0;
}

TArray<int> FTouchEngineDynamicVariableStruct::GetValueAsIntTArray() const
{
	if (VarType != EVarType::Int || !IsArray)
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

double FTouchEngineDynamicVariableStruct::GetValueAsDoubleIndexed(int Index) const
{
	return Value ? GetValueAsDoubleArray()[Index] : 0;
}

double* FTouchEngineDynamicVariableStruct::GetValueAsDoubleArray() const
{
	return Value ? (double*)Value : 0;
}

TArray<double> FTouchEngineDynamicVariableStruct::GetValueAsDoubleTArray() const
{
	if (VarType != EVarType::Double || !IsArray)
		return TArray<double>();

	TArray<double> returnArray((double*)Value, Count);
	return returnArray;
}

float FTouchEngineDynamicVariableStruct::GetValueAsFloat() const
{
	return Value ? *(float*)Value : 0;
}

FString FTouchEngineDynamicVariableStruct::GetValueAsString() const
{
	return Value ? FString((char*)Value) : FString("");
}

TArray<FString> FTouchEngineDynamicVariableStruct::GetValueAsStringArray() const
{
	TArray<FString> tempValue = TArray<FString>();

	if (!Value)
		return tempValue;

	char** buffer = (char**)Value;

	for (int i = 0; i < Count; i++)
	{
		tempValue.Add(buffer[i]);
	}
	return tempValue;
}

UTexture* FTouchEngineDynamicVariableStruct::GetValueAsTexture() const
{
	return (UTexture*)Value;
}

UTouchEngineCHOP* FTouchEngineDynamicVariableStruct::GetValueAsCHOP() const
{
	if (!Value)
		return nullptr;

	float** tempBuffer = (float**)Value;

	UTouchEngineCHOP* returnValue = NewObject<UTouchEngineCHOP>();

	int channelLength = (Size / sizeof(float)) / Count;

	returnValue->CreateChannels(tempBuffer, Count, channelLength);

	return returnValue;
}

UTouchEngineCHOP* FTouchEngineDynamicVariableStruct::GetValueAsCHOP(UTouchEngineInfo* EngineInfo) const
{
	UTouchEngineCHOP* retVal = GetValueAsCHOP();

	if (!retVal || !EngineInfo)
		return retVal;

	retVal->ChannelNames = EngineInfo->GetCHOPChannelNames(VarIdentifier);

	return retVal;
}

UTouchEngineDAT* FTouchEngineDynamicVariableStruct::GetValueAsDAT() const
{
	if (!Value)
	{
		return nullptr;
	}

	TArray<FString> stringArrayBuffer = TArray<FString>();

	char** buffer = (char**)Value;

	for (int i = 0; i < Size; i++)
	{
		stringArrayBuffer.Add(buffer[i]);
	}

	UTouchEngineDAT* returnVal = NewObject < UTouchEngineDAT>();
	returnVal->CreateChannels(stringArrayBuffer, Count, stringArrayBuffer.Num() / Count);

	return returnVal;
}



void FTouchEngineDynamicVariableStruct::SetValue(UObject* newValue, size_t _size)
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

void FTouchEngineDynamicVariableStruct::SetValue(bool InValue)
{
	if (VarType == EVarType::Bool)
	{
		Clear();

		Value = new bool;
		*((bool*)Value) = InValue;
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(int InValue)
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
	if (VarType == EVarType::Int && IsArray)
	{
		Clear();

		Value = new int* [InValue.Num()];

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

void FTouchEngineDynamicVariableStruct::SetValue(double InValue)
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
	if (VarType == EVarType::Double && IsArray)
	{
		Clear();

		Value = new double* [InValue.Num()];

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

void FTouchEngineDynamicVariableStruct::SetValue(float InValue)
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

	if (VarType == EVarType::Float && IsArray)
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
		IsArray = true;
	}
	else if (VarType == EVarType::Double && IsArray)
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

		Value = new double[InValue.Num()];

		for (int i = 0; i < InValue.Num(); i++)
		{
			((double*)Value)[i] = InValue[i];
		}


		Count = InValue.Num();
		Size = Count * sizeof(double);
		IsArray = true;
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(UTouchEngineCHOP* InValue)
{
	if (VarType != EVarType::CHOP)
		return;

	Clear();

	if (!InValue || InValue->NumChannels < 1)
	{
		return;
	}

	Count = InValue->NumChannels;
	Size = InValue->NumSamples * InValue->NumChannels * sizeof(float);
	IsArray = true;

	Value = new float* [Count];

	for (int i = 0; i < InValue->NumChannels; i++)
	{
		TArray<float> channel = InValue->GetChannel(i);

		((float**)Value)[i] = new float[channel.Num()];

		for (int j = 0; j < channel.Num(); j++)
		{
			((float**)Value)[i][j] = channel[j];
		}
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(UTouchEngineDAT* InValue)
{
	Clear();

	if (!InValue || InValue->NumColumns < 1 || InValue->NumRows < 1)
	{
		return;
	}

	SetValue(InValue->ValuesAppended);

	Count = InValue->NumRows;
	Size = InValue->NumColumns * InValue->NumRows;

	IsArray = true;
}

void FTouchEngineDynamicVariableStruct::SetValue(FString InValue)
{
	if (VarType == EVarType::String)
	{
		Clear();

		char* buffer = TCHAR_TO_ANSI(*InValue);

		Value = new char[InValue.Len() + 1];

		for (int i = 0; i < InValue.Len() + 1; i++)
		{
			((char*)Value)[i] = buffer[i];
		}
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(const TArray<FString>& InValue)
{
	if (VarType != EVarType::String || !IsArray)
		return;

	if (InValue.Num() == 0)
	{
		Clear();

		Value = nullptr;
		Count = 0;
		Size = 0;
		return;
	}

	Clear();

	Value = new char* [InValue.Num()];
	Size = 0;

	Count = InValue.Num();

	for (int i = 0; i < InValue.Num(); i++)
	{

		char* tempvalue = TCHAR_TO_ANSI(*(InValue[i]));
		((char**)Value)[i] = new char[(InValue[i]).Len() + 1];
		Size += InValue[i].Len() + 1;
		for (int j = 0; j < InValue[i].Len() + 1; j++)
		{
			((char**)Value)[i][j] = tempvalue[j];
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
		if (!Other->IsArray)
			SetValue(Other->GetValueAsInt());
		else
			SetValue(Other->GetValueAsIntTArray());

		break;
	}
	case EVarType::Double:
	{
		if (!Other->IsArray)
			SetValue(Other->GetValueAsDouble());
		else
			SetValue(Other->GetValueAsDoubleTArray());
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
		if (!Other->IsArray)
			SetValue(Other->GetValueAsString());
		else
			SetValue(Other->GetValueAsDAT());
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



void FTouchEngineDynamicVariableStruct::HandleChecked(ECheckBoxState InState)
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

void FTouchEngineDynamicVariableStruct::HandleTextBoxTextChanged(const FText& NewText)
{
	SetValue(NewText.ToString());
}

void FTouchEngineDynamicVariableStruct::HandleTextBoxTextCommited(const FText& NewText)
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
	TArray<float> buffer;

	buffer.Add(ColorProperty.R);
	buffer.Add(ColorProperty.G);
	buffer.Add(ColorProperty.B);
	buffer.Add(ColorProperty.A);

	SetValue(buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleVector2Changed()
{
#if WITH_EDITORONLY_DATA
	TArray<float> buffer;

	buffer.Add(Vector2DProperty.X);
	buffer.Add(Vector2DProperty.Y);

	SetValue(buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleVectorChanged()
{
#if WITH_EDITORONLY_DATA
	TArray<float> buffer;

	buffer.Add(VectorProperty.X);
	buffer.Add(VectorProperty.Y);
	buffer.Add(VectorProperty.Z);

	SetValue(buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleVector4Changed()
{
#if WITH_EDITORONLY_DATA
	TArray<float> buffer;

	buffer.Add(Vector4Property.X);
	buffer.Add(Vector4Property.Y);
	buffer.Add(Vector4Property.Z);
	buffer.Add(Vector4Property.W);

	SetValue(buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleIntVector2Changed()
{
#if WITH_EDITORONLY_DATA
	TArray<int> buffer;

	buffer.Add(IntPointProperty.X);
	buffer.Add(IntPointProperty.Y);

	SetValue(buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleIntVectorChanged()
{
#if WITH_EDITORONLY_DATA
	TArray<int> buffer;

	buffer.Add(IntVectorProperty.X);
	buffer.Add(IntVectorProperty.Y);
	buffer.Add(IntVectorProperty.Z);

	SetValue(buffer);
#endif
}

void FTouchEngineDynamicVariableStruct::HandleIntVector4Changed()
{
#if WITH_EDITORONLY_DATA
	TArray<int> buffer;

	buffer.Add(IntVector4Property.X);
	buffer.Add(IntVector4Property.Y);
	buffer.Add(IntVector4Property.Z);
	buffer.Add(IntVector4Property.W);

	SetValue(buffer);
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

	UTouchEngineDAT* tempValue = NewObject<UTouchEngineDAT>();
	tempValue->CreateChannels(StringArrayProperty, StringArrayProperty.Num(), 1);

	SetValue(tempValue);

#endif
}

void FTouchEngineDynamicVariableStruct::HandleStringArrayChildChanged()
{
#if WITH_EDITORONLY_DATA

	UTouchEngineDAT* tempValue = NewObject<UTouchEngineDAT>();
	tempValue->CreateChannels(StringArrayProperty, StringArrayProperty.Num(), 1);

	SetValue(tempValue);

#endif
}

void FTouchEngineDynamicVariableStruct::HandleDropDownBoxValueChanged(TSharedPtr<FString> Arg)
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
	Ar << IsArray;
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

	TArray<float> floatBufferProperty = TArray<float>();
	TArray<FString> stringArrayProperty = TArray<FString>();

	UTexture* textureProperty = nullptr;
	
	FVector2D vector2DProperty = FVector2D();
	FVector vectorProperty = FVector();
	FVector4 vector4Property = FVector4();
	
	FIntPoint intPointProperty = FIntPoint();
	FIntVector intVectorProperty = FIntVector();
	
	FTouchEngineIntVector4 intVector4Property = FTouchEngineIntVector4();
	
	FColor colorProperty = FColor();
	TMap<FString, int> dropDownData = TMap<FString, int>();


	Ar << floatBufferProperty;
	Ar << stringArrayProperty;
	
	Ar << textureProperty;
	
	Ar << vector2DProperty;
	Ar << vectorProperty;
	Ar << vector4Property;
	
	Ar << intPointProperty;
	Ar << intVectorProperty;

	Ar << intVector4Property.X;
	Ar << intVector4Property.Y;
	Ar << intVector4Property.Z;
	Ar << intVector4Property.W;

	Ar << colorProperty;
	Ar << dropDownData;

#endif


	// write void pointer
	if (Ar.IsSaving())
	{
		// writing dynamic variable to archive
		switch (VarType)
		{
		case EVarType::Bool:
		{
			bool tempBool = false;
			if (Value)
			{
				tempBool = GetValueAsBool();
			}
			Ar << tempBool;
			break;
		}
		case EVarType::Int:
		{
			if (Count <= 1)
			{
				int tempInt = 0;
				if (Value)
				{
					tempInt = GetValueAsInt();
				}
				Ar << tempInt;
			}
			else
			{
				for (int i = 0; i < Count; i++)
				{
					if (Value)
					{
						int tempIntIndex = ((int*)Value)[i];
						Ar << tempIntIndex;
					}
					else
					{
						int tempIntIndex = 0;
						Ar << tempIntIndex;
					}
				}
			}
			break;
		}
		case EVarType::Double:
		{
			if (Count <= 1)
			{
				double tempDouble = 0;
				if (Value)
				{
					tempDouble = GetValueAsDouble();
				}
				Ar << tempDouble;
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
						double tempDoubleIndex = 0;
						Ar << tempDoubleIndex;
					}
				}
			}
			break;
		}
		case EVarType::Float:
		{
			float tempFloat = 0;
			if (Value)
			{
				tempFloat = GetValueAsFloat();
			}
			Ar << tempFloat;
			break;
		}
		case EVarType::CHOP:
		{
			UTouchEngineCHOP* tempFloatBuffer = nullptr;

			if (Value)
			{
				tempFloatBuffer = GetValueAsCHOP();
			}

			Ar << tempFloatBuffer;
			break;
		}
		case EVarType::String:
		{
			if (!IsArray)
			{
				FString tempString = FString("");

				if (Value)
				{
					tempString = GetValueAsString();
				}
				Ar << tempString;
			}
			else
			{
				TArray<FString> tempStringArray;

				if (Value)
				{
					tempStringArray = GetValueAsStringArray();
				}
				Ar << tempStringArray;
			}
			break;
		}
		case EVarType::Texture:
		{
			UTexture* tempTexture = nullptr;

			if (Value)
			{
				tempTexture = GetValueAsTexture();
			}
			Ar << tempTexture;
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
			bool tempBool;
			Ar << tempBool;
			SetValue(tempBool);
			break;
		}
		case EVarType::Int:
		{
			if (Count <= 1)
			{
				int tempInt;
				Ar << tempInt;
				SetValue(tempInt);
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
				double tempDouble;
				Ar << tempDouble;
				SetValue(tempDouble);
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
			float tempFloat;
			Ar << tempFloat;
			SetValue(tempFloat);
			break;
		}
		case EVarType::CHOP:
		{
			UTouchEngineCHOP* tempFloatBuffer;
			Ar << tempFloatBuffer;
			SetValue(tempFloatBuffer);
			break;
		}
		case EVarType::String:
		{
			if (!IsArray)
			{
				FString tempString = FString("");
				Ar << tempString;
				SetValue(tempString);
			}
			else
			{
				TArray<FString> tempStringArray;
				Ar << tempStringArray;

				UTouchEngineDAT* datBuffer = NewObject<UTouchEngineDAT>();
				if (Count && Size)
					datBuffer->CreateChannels(tempStringArray, Count, Size / Count);
				SetValue(datBuffer);
			}
			break;
		}
		case EVarType::Texture:
		{
			UTexture* tempTexture;
			Ar << tempTexture;
			SetValue(tempTexture);
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
	if (Other->VarType == VarType && Other->IsArray == IsArray)
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
			if (Count <= 1 && !IsArray)
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
					int* otherValue = Other->GetValueAsIntArray();
					int* thisValue = GetValueAsIntArray();

					if (otherValue != nullptr)
					{
						TArray<int> otherTValue = TArray<int>(), thisTValue = TArray<int>();

						for (int i = 0; i < Count; i++)
						{
							otherTValue.Add(otherValue[i]);
							thisTValue.Add(thisValue[i]);
						}

						if (otherTValue == thisTValue)
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
			if (Count <= 1 && !IsArray)
			{
				if (Other->GetValueAsDouble() == GetValueAsDouble())
					return true;
			}
			else
			{
				if (Other->Count == Count)
				{
					double* otherValue = Other->GetValueAsDoubleArray();
					double* thisValue = GetValueAsDoubleArray();

					if (otherValue != nullptr)
					{

						TArray<double> otherTValue = TArray<double>(), thisTValue = TArray<double>();

						for (int i = 0; i < Count; i++)
						{
							otherTValue.Add(otherValue[i]);
							thisTValue.Add(thisValue[i]);
						}

						if (otherTValue == thisTValue)
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
			if (!IsArray)
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
	if (!Value || !EngineInfo)
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
				FTouchVar<bool> op;
				op.Data = true;
				EngineInfo->SetBooleanInput(VarIdentifier, op);
				SetValue(false);
			}
		}
		else
		{
			FTouchVar<bool> op;
			op.Data = GetValueAsBool();
			EngineInfo->SetBooleanInput(VarIdentifier, op);
		}
		break;
	}
	case EVarType::Int:
	{
		FTouchVar<TArray<int32_t>> op;
		if (Count <= 1)
		{
			op.Data.Add(GetValueAsInt());
		}
		else
		{
			int* buffer = GetValueAsIntArray();
			for (int i = 0; i < Count; i++)
			{
				op.Data.Add(buffer[i]);
			}
		}

		EngineInfo->SetIntegerInput(VarIdentifier, op);
		break;
	}
	case EVarType::Double:
	{
		FTouchVar<TArray<double>> op;

		if (Count > 1)
		{
			double* buffer = GetValueAsDoubleArray();
			for (int i = 0; i < Count; i++)
			{
				op.Data.Add(buffer[i]);
			}
		}
		else
		{
			op.Data.Add(GetValueAsDouble());
		}

		EngineInfo->SetDoubleInput(VarIdentifier, op);
		break;
	}
	case EVarType::Float:
	{
		FTouchCHOPSingleSample tcss;
		tcss.ChannelData.Add(GetValueAsFloat());
		EngineInfo->SetCHOPInputSingleSample(VarIdentifier, tcss);
		break;
	}
	case EVarType::CHOP:
	{
		UTouchEngineCHOP* floatBuffer = GetValueAsCHOP();

		if (!floatBuffer)
			return;

		FTouchCHOPSingleSample tcss;

		for (int i = 0; i < floatBuffer->NumChannels; i++)
		{
			TArray<float> channel = floatBuffer->GetChannel(i);

			for (int j = 0; j < channel.Num(); j++)
			{
				tcss.ChannelData.Add(channel[j]);
			}
		}

		EngineInfo->SetCHOPInputSingleSample(VarIdentifier, tcss);
		break;
	}
	case EVarType::String:
	{
		if (!IsArray)
		{
			FTouchVar<char*> op;
			op.Data = TCHAR_TO_ANSI(*GetValueAsString());
			EngineInfo->SetStringInput(VarIdentifier, op);
		}
		else
		{
			FTouchDATFull op;
			op.ChannelData = TETableCreate();

			EngineInfo->SetTableInput(VarIdentifier, op);
			TERelease(&op.ChannelData);
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
		FTouchVar<bool> op = EngineInfo->GetBooleanOutput(VarIdentifier);
		SetValue(op.Data);
		break;
	}
	case EVarType::Int:
	{
		FTouchVar<int32_t> op = EngineInfo->GetIntegerOutput(VarIdentifier);
		SetValue((int)op.Data);
		break;
	}
	case EVarType::Double:
	{
		FTouchVar<double> op = EngineInfo->GetDoubleOutput(VarIdentifier);
		SetValue(op.Data);
		break;
	}
	case EVarType::Float:
	{
		break;
	}
	case EVarType::CHOP:
	{
		FTouchCHOPFull tcss = EngineInfo->GetCHOPOutputSingleSample(VarIdentifier);

		if (tcss.SampleData.Num() == 0)
		{
			return;
		}

		UTouchEngineCHOP* CHOP = NewObject<UTouchEngineCHOP>();
		CHOP->CreateChannels(tcss);

		SetValue(CHOP);
		break;
	}
	case EVarType::String:
	{
		if (!IsArray)
		{
			FTouchVar<TEString*> op = EngineInfo->GetStringOutput(VarIdentifier);
			SetValue(FString(op.Data->string));
		}
		else
		{
			FTouchDATFull op = EngineInfo->GetTableOutput(VarIdentifier);

			TArray<FString> buffer;

			int32 rowcount = TETableGetRowCount(op.ChannelData), columncount = TETableGetColumnCount(op.ChannelData);

			for (int i = 0; i < rowcount; i++)
			{
				for (int j = 0; j < columncount; j++)
				{
					buffer.Add(TETableGetStringValue(op.ChannelData, i, j));
				}
			}

			UTouchEngineDAT* bufferDAT = NewObject<UTouchEngineDAT>();
			bufferDAT->CreateChannels(buffer, rowcount, columncount);

			SetValue(bufferDAT);
		}
		break;
	}
	case EVarType::Texture:
	{
		FTouchTOP top = EngineInfo->GetTOPOutput(VarIdentifier);
		SetValue(top.Texture);
		break;
	}
	default:
	{
		// unimplemented type
		break;
	}
	}
}



TArray<float> UTouchEngineCHOP::GetChannel(int Index)
{
	if (Index < NumChannels)
	{
		TArray<float> returnValue;

		for (int i = Index * NumSamples; i < (Index * NumSamples) + NumSamples; i++)
		{
			returnValue.Add(ChannelsAppended[i]);
		}

		return returnValue;
	}
	else
	{
		return TArray<float>();
	}
}

TArray<float> UTouchEngineCHOP::GetChannelByName(FString Name)
{
	int Index;
	if (ChannelNames.Find(Name, Index))
	{
		return GetChannel(Index);
	}
	return TArray<float>();
}

void UTouchEngineCHOP::CreateChannels(float** FullChannel, int InChannelCount, int InChannelSize)
{
	Clear();

	NumChannels = InChannelCount;
	NumSamples = InChannelSize;

	for (int i = 0; i < NumChannels; i++)
	{
		for (int j = 0; j < NumSamples; j++)
		{
			ChannelsAppended.Add(FullChannel[i][j]);
		}
	}
}

void UTouchEngineCHOP::CreateChannels(FTouchCHOPFull CHOP)
{
	Clear();

	NumChannels = CHOP.SampleData.Num();

	if (NumChannels == 0)
		return;

	NumSamples = CHOP.SampleData[0].ChannelData.Num();

	for (int i = 0; i < NumChannels; i++)
	{
		FTouchCHOPSingleSample channel = CHOP.SampleData[i];

		for (int j = 0; j < channel.ChannelData.Num(); j++)
		{
			ChannelsAppended.Add(channel.ChannelData[j]);
		}
	}
}

void UTouchEngineCHOP::Clear()
{
	NumChannels = 0;
	NumSamples = 0;
	ChannelsAppended.Empty();
}



TArray<FString> UTouchEngineDAT::GetRow(int Row)
{
	if (Row < NumRows)
	{
		TArray<FString> retVal;

		for (int Index = Row * NumColumns; Index < Row * NumColumns + NumColumns; Index++)
		{
			retVal.Add(ValuesAppended[Index]);
		}

		return retVal;
	}
	else
	{
		return TArray<FString>();
	}
}


TArray<FString> UTouchEngineDAT::GetRowByName(FString RowName)
{
	for (int i = 0; i < NumRows; i++)
	{
		TArray<FString> row = GetRow(i);

		if (row.Num() != 0)
		{
			if (row[0] == RowName)
			{
				return row;
			}
		}
	}
	return TArray<FString>();
}

TArray<FString> UTouchEngineDAT::GetColumn(int Column)
{
	if (Column < NumColumns)
	{
		TArray<FString> retVal;

		for (int Index = Column; Index < NumRows * NumColumns; Index += NumColumns)
		{
			retVal.Add(ValuesAppended[Index]);
		}

		return retVal;
	}
	else
	{
		return TArray<FString>();
	}
}

TArray<FString> UTouchEngineDAT::GetColumnByName(FString ColumnName)
{
	for (int i = 0; i < NumColumns; i++)
	{
		TArray<FString> col = GetColumn(i);

		if (col.Num() != 0)
		{
			if (col[0] == ColumnName)
			{
				return col;
			}
		}
	}
	return TArray<FString>();
}

FString UTouchEngineDAT::GetCell(int Column, int Row)
{
	if (Column < NumColumns && Row < NumRows)
	{
		int Index = Row * NumColumns + Column;
		return ValuesAppended[Index];
	}
	else
	{
		return FString();
	}
}

FString UTouchEngineDAT::GetCellByName(FString ColumnName, FString RowName)
{
	int rowNum = -1, colNum = -1;

	for (int i = 0; i < NumRows; i++)
	{
		TArray<FString> row = GetRow(i);

		if (row[0] == RowName)
		{
			rowNum = i;
			break;
		}
	}

	for (int i = 0; i < NumColumns; i++)
	{
		TArray<FString> col = GetColumn(i);

		if (col[0] == ColumnName)
		{
			colNum = i;
			break;
		}
	}

	if (rowNum == -1 || colNum == -1)
	{
		return FString();
	}

	return GetCell(colNum, rowNum);
}

void UTouchEngineDAT::CreateChannels(TArray<FString> AppendedArray, int RowCount, int ColumnCount)
{
	if (RowCount * ColumnCount != AppendedArray.Num())
		return;

	ValuesAppended = AppendedArray;
	NumRows = RowCount;
	NumColumns = ColumnCount;
}
