// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineComponent.h"
#include "UTouchEngine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/UObjectIterator.h"

FTouchEngineDynamicVariableContainer::FTouchEngineDynamicVariableContainer()
{

}

FTouchEngineDynamicVariableContainer::~FTouchEngineDynamicVariableContainer()
{
	OnToxLoaded.Clear();
}



FDelegateHandle FTouchEngineDynamicVariableContainer::CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (parent && parent->EngineInfo && parent->EngineInfo->isLoaded())
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
	OnToxLoaded.Remove(DelegateHandle);
}

void FTouchEngineDynamicVariableContainer::ToxLoaded()
{
	//OnToxLoaded.Broadcast();
}

void FTouchEngineDynamicVariableContainer::ToxParametersLoaded(TArray<FTouchDynamicVar> variablesIn, TArray<FTouchDynamicVar> variablesOut)
{
	// if we have no data loaded 
	if ((DynVars_Input.Num() == 0 && DynVars_Output.Num() == 0))
	{
		DynVars_Input = variablesIn;
		DynVars_Output = variablesOut;
		return;
	}

	// fill out the new "variablesIn" and "variablesOut" arrays with the existing values in the "DynVars_Input" and "DynVars_Output" if possible
	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		for (int j = 0; j < variablesIn.Num(); j++)
		{
			if (DynVars_Input[i].VarName == variablesIn[j].VarName && DynVars_Input[i].VarType == variablesIn[j].VarType && DynVars_Input[i].isArray == variablesIn[j].isArray)
			{
				variablesIn[j].SetValue(&DynVars_Input[i]);
			}
		}
	}
	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		for (int j = 0; j < variablesOut.Num(); j++)
		{
			if (DynVars_Output[i].VarName == variablesOut[j].VarName && DynVars_Output[i].VarType == variablesOut[j].VarType && DynVars_Output[i].isArray == variablesOut[j].isArray)
			{
				variablesOut[j].SetValue(&DynVars_Output[i]);
			}
		}
	}

	DynVars_Input = variablesIn;
	DynVars_Output = variablesOut;

	parent->OnToxLoaded.Broadcast();

	OnToxLoaded.Broadcast();
}

FDelegateHandle FTouchEngineDynamicVariableContainer::CallOrBind_OnToxFailedLoad(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (parent && parent->EngineInfo && parent->EngineInfo->hasFailedLoad())
	{
		// we're in a world object with a tox file loaded
		Delegate.Execute();
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
	OnToxFailedLoad.Remove(Handle);
}

void FTouchEngineDynamicVariableContainer::ToxFailedLoad()
{
	parent->OnToxFailedLoad.Broadcast();
	OnToxFailedLoad.Broadcast();
}



void FTouchEngineDynamicVariableContainer::SendInputs(UTouchEngineInfo* engineInfo)
{
	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		DynVars_Input[i].SendInput(engineInfo);
	}
}

void FTouchEngineDynamicVariableContainer::GetOutputs(UTouchEngineInfo* engineInfo)
{
	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		DynVars_Output[i].GetOutput(engineInfo);
	}
}

void FTouchEngineDynamicVariableContainer::SendInput(UTouchEngineInfo* engineInfo, int index)
{
	if (index >= DynVars_Input.Num())
		return;

	DynVars_Input[index].SendInput(engineInfo);
}

void FTouchEngineDynamicVariableContainer::GetOutput(UTouchEngineInfo* engineInfo, int index)
{
	if (index >= DynVars_Output.Num())
		return;

	DynVars_Output[index].GetOutput(engineInfo);
}

FTouchDynamicVar* FTouchEngineDynamicVariableContainer::GetDynamicVariableByName(FString varName)
{
	FTouchDynamicVar* var = nullptr;

	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		if (DynVars_Input[i].VarName == varName)
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
		if (DynVars_Output[i].VarName == varName)
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

FTouchDynamicVar* FTouchEngineDynamicVariableContainer::GetDynamicVariableByIdentifier(FString varIdentifier)
{

	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		if (DynVars_Input[i].VarIdentifier.Equals(varIdentifier))
			return &DynVars_Input[i];
	}

	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		if (DynVars_Output[i].VarIdentifier.Equals(varIdentifier))
			return &DynVars_Output[i];
	}
	return nullptr;
}


// returns value as bool
bool FTouchDynamicVar::GetValueAsBool() const
{
	return value ? *(bool*)value : false;
}
// returns value as bool
ECheckBoxState FTouchDynamicVar::GetValueAsCheckState() const
{
	return GetValueAsBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}
// returns value as integer
int FTouchDynamicVar::GetValueAsInt() const
{
	return value ? *(int*)value : 0;
}
// returns value as integer in a TOptional struct
TOptional<int> FTouchDynamicVar::GetValueAsOptionalInt() const
{
	return TOptional<int>(GetValueAsInt());
}
// returns indexed value as integer
int FTouchDynamicVar::GetValueAsIntIndexed(int index) const
{
	return value ? ((int*)value)[index] : 0;
}
// returns indexed value as integer in a TOptional struct
TOptional<int> FTouchDynamicVar::GetIndexedValueAsOptionalInt(int index) const
{
	return TOptional<int>(GetValueAsIntIndexed(index));
}
// returns value as integer array
int* FTouchDynamicVar::GetValueAsIntArray() const
{
	return value ? (int*)value : 0;
}
// returns value as double
double FTouchDynamicVar::GetValueAsDouble() const
{
	return value ? *(double*)value : 0;
}
// returns indexed value as double
double FTouchDynamicVar::GetValueAsDoubleIndexed(int index) const
{
	return value ? GetValueAsDoubleArray()[index] : 0;
}
// returns value as double in a TOptional struct
TOptional<double> FTouchDynamicVar::GetValueAsOptionalDouble() const
{
	return TOptional<double>(GetValueAsDouble());
}
// returns indexed value as double in a TOptional struct
TOptional<double> FTouchDynamicVar::GetIndexedValueAsOptionalDouble(int index) const
{
	return TOptional<double>(GetValueAsDoubleIndexed(index));
}
// returns value as double array
double* FTouchDynamicVar::GetValueAsDoubleArray() const
{
	return value ? (double*)value : 0;
}
// returns value as float
float FTouchDynamicVar::GetValueAsFloat() const
{
	return value ? *(float*)value : 0;
}
// returns value as float in a TOptional struct
TOptional<float> FTouchDynamicVar::GetValueAsOptionalFloat() const
{
	return TOptional<float>(GetValueAsFloat());
}
// returns value as fstring
FString FTouchDynamicVar::GetValueAsString() const
{
	return value ? FString((char*)value) : FString("");
}


TArray<FString> FTouchDynamicVar::GetValueAsStringArray() const
{
	TArray<FString> _value = TArray<FString>();

	if (!value)
		return _value;

	char** buffer = (char**)value;

	for (int i = 0; i < count; i++)
	{
		_value.Add(buffer[i]);
	}
	return _value;
}

// returns value as texture pointer
UTexture* FTouchDynamicVar::GetValueAsTexture() const
{
	return (UTexture*)value;
}

TArray<float> FTouchDynamicVar::GetValueAsFloatBuffer() const
{
	if (!value)
		return TArray<float>();

	float* floatArray = (float*)value;
	int _count = size / sizeof(float);

	TArray<float> returnValue;

	for (int i = 0; i < _count; i++)
	{
		returnValue.Add(floatArray[i]);
	}

	return returnValue;
}

void FTouchDynamicVar::SetValue(UObject* newValue, size_t _size)
{
	if (newValue == nullptr)
	{
		value = nullptr;
		return;
	}

	value = (void*)newValue;
	size = _size;
}

void FTouchDynamicVar::SetValue(void* newValue, size_t _size)
{
	if (newValue == nullptr)
	{
		value = nullptr;
		return;
	}

	value = malloc(_size);
	memcpy(value, newValue, _size);
	size = _size;
}

void FTouchDynamicVar::SetValue(TArray<float> _value)
{
	if (_value.Num() == 0)
	{
		value = nullptr;
		return;
	}

	if (VarType == EVarType::VARTYPE_FLOATBUFFER || (VarType == EVarType::VARTYPE_FLOAT && isArray))
	{
		float* floatBuffer = new float[_value.Num()];

		for (int i = 0; i < _value.Num(); i++)
		{
			floatBuffer[i] = _value[i];
		}

		SetValue((void*)floatBuffer, sizeof(float) * _value.Num());
#if WITH_EDITORONLY_DATA
		floatBufferProperty = _value;
#endif

		count = _value.Num();
		isArray = true;
	}
	else if (VarType == EVarType::VARTYPE_DOUBLE && isArray)
	{
#if WITH_EDITORONLY_DATA
		switch (VarIntent)
		{
		case EVarIntent::VARINTENT_COLOR:
			if (_value.Num() == 4)
				colorProperty = FColor(_value[0], _value[1], _value[2], _value[3]);
			else if (_value.Num() == 3)
				colorProperty = FColor(_value[0], _value[1], _value[2], 1.f);
			break;
		case EVarIntent::VARINTENT_UVW:
			if (_value.Num() == 3)
				vectorProperty = FVector(_value[0], _value[1], _value[2]);
			else if (_value.Num() == 2)
				vectorProperty = FVector(_value[0], _value[1], 1.f);
			break;
		case EVarIntent::VARINTENT_POSITION:
			if (_value.Num() == 4)
				vector4Property = FVector4(_value[0], _value[1], _value[2], _value[3]);
			else if (_value.Num() == 3)
				vector4Property = FVector4(_value[0], _value[1], _value[2], 1.f);
			break;
		default:
			break;
		}
#endif

		double* doubleBuffer = new double[_value.Num()];

		for (int i = 0; i < _value.Num(); i++)
		{
			doubleBuffer[i] = _value[i];
		}

		SetValue((void*)doubleBuffer, sizeof(double) * _value.Num());

		count = _value.Num();
		isArray = true;

	}
}

void FTouchDynamicVar::SetValue(FString _value)
{
	if (VarType == EVarType::VARTYPE_STRING)
		SetValue((void*)TCHAR_TO_ANSI(*_value), _value.Len() + 1);
}

void FTouchDynamicVar::SetValue(TArray<FString> _value)
{
	if (_value.Num() == 0)
	{
		value = nullptr;
		count = 0;
		size = 0;
		return;
	}

	char** buffer = new char* [_value.Num()];
	size = 0;

	for (int i = 0; i < _value.Num(); i++)
	{
		char* tempvalue = TCHAR_TO_ANSI(*(_value[i]));
		buffer[i] = new char[(_value[i]).Len() + 1];
		size += _value[i].Len() + 1;
		for (int j = 0; j < _value[i].Len() + 1; j++)
		{
			buffer[i][j] = tempvalue[j];
		}
	}

#if WITH_EDITORONLY_DATA
	stringArrayProperty = _value;
#endif

	value = buffer;
	count = _value.Num();
	isArray = true;
}

void FTouchDynamicVar::SetValue(UTexture* _value)
{
	if (VarType == EVarType::VARTYPE_TEXTURE)
	{
		SetValue((UObject*)_value, sizeof(UTexture));
	}
}

void FTouchDynamicVar::SetValue(FTouchDynamicVar* other)
{
	switch (other->VarType)
	{
	case EVarType::VARTYPE_BOOL:
	{
		SetValue(other->GetValueAsBool());
		break;
	}
	case EVarType::VARTYPE_INT:
	{
		SetValue(other->GetValueAsInt());
		break;
	}
	case EVarType::VARTYPE_DOUBLE:
	{
		if (!other->isArray)
			SetValue(other->GetValueAsDouble());
		else
			SetValue(other->GetValueAsDoubleArray(), sizeof(double) * other->count);
		break;
	}
	case EVarType::VARTYPE_FLOAT:
	{
		SetValue(other->GetValueAsFloat());
		break;
	}
	case EVarType::VARTYPE_FLOATBUFFER:
	{
		SetValue(other->GetValueAsFloatBuffer());
		break;
	}
	case EVarType::VARTYPE_STRING:
	{
		if (!other->isArray)
			SetValue(other->GetValueAsString());
		else
			SetValue(other->GetValueAsStringArray());
		break;
	}
	case EVarType::VARTYPE_TEXTURE:
	{
		SetValue(other->GetValueAsTexture());
		break;
	}
	}

#if WITH_EDITORONLY_DATA

	floatBufferProperty = other->floatBufferProperty;
	stringArrayProperty = other->stringArrayProperty;
	textureProperty = other->textureProperty;
	colorProperty = other->colorProperty;
	vectorProperty = other->vectorProperty;
	vector4Property = other->vector4Property;

#endif
}


void FTouchDynamicVar::HandleChecked(ECheckBoxState InState)
{
	switch (InState)
	{
	case ECheckBoxState::Unchecked:
		SetValue(false);
		break;
	case ECheckBoxState::Checked:
		SetValue(true);
		break;
	case ECheckBoxState::Undetermined:
		break;
	default:
		break;
	}
}

FText FTouchDynamicVar::HandleTextBoxText() const
{
	if (value)
		return FText::FromString(GetValueAsString());
	else
		return FText();
}

void FTouchDynamicVar::HandleTextBoxTextChanged(const FText& NewText)
{
	SetValue(NewText.ToString());
}

void FTouchDynamicVar::HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo)
{
	SetValue(NewText.ToString());
}

void FTouchDynamicVar::HandleTextureChanged()
{
#if WITH_EDITORONLY_DATA
	SetValue(textureProperty);
#endif
}

void FTouchDynamicVar::HandleColorChanged()
{
	TArray<float> buffer;

	buffer.Add(colorProperty.R);
	buffer.Add(colorProperty.G);
	buffer.Add(colorProperty.B);
	buffer.Add(colorProperty.A);

	SetValue(buffer);
}

void FTouchDynamicVar::HandleVector4Changed()
{
	TArray<float> buffer;

	buffer.Add(vector4Property.X);
	buffer.Add(vector4Property.Y);
	buffer.Add(vector4Property.Z);
	buffer.Add(vector4Property.W);

	SetValue(buffer);
}

void FTouchDynamicVar::HandleVectorChanged()
{
	TArray<float> buffer;

	buffer.Add(vectorProperty.X);
	buffer.Add(vectorProperty.Y);
	buffer.Add(vectorProperty.Z);

	SetValue(buffer);
}

void FTouchDynamicVar::HandleFloatBufferChanged()
{
#if WITH_EDITORONLY_DATA
	SetValue(floatBufferProperty);
#endif
}

void FTouchDynamicVar::HandleFloatBufferChildChanged()
{
	SetValue(floatBufferProperty);
}

void FTouchDynamicVar::HandleStringArrayChanged()
{
#if WITH_EDITORONLY_DATA
	SetValue(stringArrayProperty);
#endif
}

void FTouchDynamicVar::HandleStringArrayChildChanged()
{
	SetValue(stringArrayProperty);
}


bool FTouchDynamicVar::Serialize(FArchive& Ar)
{
	// write / read all normal variables
	Ar << VarLabel;
	Ar << VarName;
	Ar << VarIdentifier;
	Ar << VarType;
	Ar << VarIntent;
	Ar << count;
	Ar << size;
	Ar << isArray;
	// write editor variables just in case they need to be used
#if WITH_EDITORONLY_DATA
	Ar << textureProperty;
	Ar << floatBufferProperty;
	Ar << stringArrayProperty;
	Ar << colorProperty;
	Ar << vector4Property;
	Ar << vectorProperty;
#endif


	// write void pointer
	if (Ar.IsSaving())
	{
		// writing dynamic variable to archive
		switch (VarType)
		{
		case EVarType::VARTYPE_BOOL:
		{
			bool tempValue = false;
			if (value)
			{
				tempValue = GetValueAsBool();
			}
			Ar << tempValue;
		}
		break;
		case EVarType::VARTYPE_INT:
		{
			if (count <= 1)
			{
				int tempValue = 0;
				if (value)
				{
					tempValue = GetValueAsInt();
				}
				Ar << tempValue;
			}
			else
			{
				for (int i = 0; i < count; i++)
				{
					if (value)
					{
						int tempValue = ((int*)value)[i];
						Ar << tempValue;
					}
					else
					{
						int tempValue = 0;
						Ar << tempValue;
					}
				}
			}
		}
		break;
		case EVarType::VARTYPE_DOUBLE:
		{
			if (count <= 1)
			{
				double tempValue = 0;
				if (value)
					tempValue = GetValueAsDouble();
				Ar << tempValue;
			}
			else
			{
				for (int i = 0; i < count; i++)
				{
					if (value)
					{
						Ar << ((double*)value)[i];
					}
					else
					{
						double tempValue = 0;
						Ar << tempValue;
					}
				}
			}
		}
		break;
		case EVarType::VARTYPE_FLOAT:
		{
			float tempValue = 0;
			if (value)
				tempValue = GetValueAsFloat();
			Ar << tempValue;
		}
		break;
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			TArray<float> tempValue = TArray<float>();
			if (value)
				tempValue = GetValueAsFloatBuffer();
			Ar << tempValue;
		}
		break;
		case EVarType::VARTYPE_STRING:
		{
			if (!isArray)
			{
				FString tempValue = FString("");
				if (value)
					tempValue = GetValueAsString();
				Ar << tempValue;
			}
			else
			{
				TArray<FString> tempValue;
				if (value)
					tempValue = GetValueAsStringArray();
				Ar << tempValue;
			}
		}
		break;
		case EVarType::VARTYPE_TEXTURE:
		{
			UTexture* tempValue = nullptr;
			if (value)
				//tempValue = GetValueAsTextureRenderTarget();
				tempValue = GetValueAsTexture();
			Ar << tempValue;
		}
		break;
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
		case EVarType::VARTYPE_BOOL:
		{
			bool tempValue;
			Ar << tempValue;
			SetValue(tempValue);
		}
		break;
		case EVarType::VARTYPE_INT:
		{
			if (count <= 1)
			{
				int tempValue;
				Ar << tempValue;
				SetValue(tempValue);
			}
			else
			{
				value = new int[count];
				size = sizeof(int) * count;

				for (int i = 0; i < count; i++)
				{
					Ar << ((int*)value)[i];
				}
			}
		}
		break;
		case EVarType::VARTYPE_DOUBLE:
		{
			if (count <= 1)
			{
				double tempValue;
				Ar << tempValue;
				SetValue(tempValue);
			}
			else
			{
				value = new double[count];
				size = sizeof(double) * count;

				for (int i = 0; i < count; i++)
				{
					Ar << ((double*)value)[i];
				}
			}
		}
		break;
		case EVarType::VARTYPE_FLOAT:
		{
			float tempValue;
			Ar << tempValue;
			SetValue(tempValue);
		}
		break;
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			TArray<float> tempValue;
			Ar << tempValue;
			SetValue(tempValue);
		}
		break;
		case EVarType::VARTYPE_STRING:
		{
			if (!isArray)
			{
				FString tempValue = FString("");
				Ar << tempValue;
				SetValue(tempValue);
			}
			else
			{
				TArray<FString> tempValue;
				Ar << tempValue;
				SetValue(tempValue);
			}
		}
		break;
		case EVarType::VARTYPE_TEXTURE:
		{
			UTexture* tempValue;
			Ar << tempValue;
			SetValue(tempValue);
		}
		break;
		default:
			// unsupported type
			break;
		}
	}

	return true;
}

bool FTouchDynamicVar::Identical(const FTouchDynamicVar* Other, uint32 PortFlags) const
{
	if (Other->VarType == VarType && Other->isArray == isArray)
	{
		switch (VarType)
		{
		case EVarType::VARTYPE_BOOL:
		{
			if (Other->GetValueAsBool() == GetValueAsBool())
				return true;
		}
		break;
		case EVarType::VARTYPE_INT:
		{
			if (count <= 1 && !isArray)
			{
				if (Other->GetValueAsInt() == GetValueAsInt())
					return true;
			}
			else
			{
				if (Other->count == count)
				{
					int* otherValue = Other->GetValueAsIntArray();
					int* thisValue = GetValueAsIntArray();

					TArray<int> otherTValue = TArray<int>(), thisTValue = TArray<int>();

					for (int i = 0; i < count; i++)
					{
						otherTValue.Add(otherValue[i]);
						thisTValue.Add(thisValue[i]);
					}

					if (otherTValue == thisTValue)
						return true;
				}
			}
		}
		break;
		case EVarType::VARTYPE_DOUBLE:
		{
			if (count <= 1 && !isArray)
			{
				if (Other->GetValueAsDouble() == GetValueAsDouble())
					return true;
			}
			else
			{
				if (Other->count == count)
				{
					double* otherValue = Other->GetValueAsDoubleArray();
					double* thisValue = GetValueAsDoubleArray();

					TArray<double> otherTValue = TArray<double>(), thisTValue = TArray<double>();

					for (int i = 0; i < count; i++)
					{
						otherTValue.Add(otherValue[i]);
						thisTValue.Add(thisValue[i]);
					}

					if (otherTValue == thisTValue)
						return true;
				}
			}
		}
		break;
		case EVarType::VARTYPE_FLOAT:
		{
			if (Other->GetValueAsFloat() == GetValueAsFloat())
				return true;
		}
		break;
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			if (Other->GetValueAsFloatBuffer() == GetValueAsFloatBuffer())
				return true;
		}
		break;
		case EVarType::VARTYPE_STRING:
		{
			if (!isArray)
			{
				if (Other->GetValueAsString() == GetValueAsString())
					return true;
			}
			else
			{
				if (Other->GetValueAsStringArray() == GetValueAsStringArray())
					return true;
			}
		}
		break;
		case EVarType::VARTYPE_TEXTURE:
		{
			if (Other->GetValueAsTexture() == GetValueAsTexture())
				return true;
		}
		break;
		default:
			return false;
			// unsupported type
			break;
		}
	}

	return false;
}

#if WITH_EDITORONLY_DATA

void FTouchDynamicVar::UpdateInstances(UObject* blueprintOwner, UTouchEngineComponentBase* parentComponent)
{
	// Propagate the change to all other instances of the component.

	// get all objects of the component base class and derived classes
	for (TObjectIterator<UTouchEngineComponentBase> it(RF_NoFlags); it; ++it)
	{
		UTouchEngineComponentBase* component = *it;

		// we don't need to update ourselves
		if (component == parentComponent)
		{
			continue;
		}

		// check to see if the outer class of this component is the same as the outer class of the component we're modifying
		if (component->GetOuter()->GetClass() == blueprintOwner->GetClass())
		{
			// see if we have the same tox loaded
			if (component->ToxFilePath == parentComponent->ToxFilePath)
			{
				// find this variable inside the component
				FTouchDynamicVar* dynVar = component->dynamicVariables.GetDynamicVariableByIdentifier(VarIdentifier);
				// didn't find the variable. This is an odd case, should probably reload the tox file and try again
				if (!dynVar)
				{
					continue;
				}
				// check if the value is the default value (passed in old value)
				
				// if it is, replace it
				dynVar->SetValue(this);
			}
		}

	}
}

#endif


void FTouchDynamicVar::SendInput(UTouchEngineInfo* engineInfo)
{
	if (!value)
		return;

	switch (VarType)
	{
	case EVarType::VARTYPE_BOOL:
	{
		FTouchVar<bool> op;
		op.data = GetValueAsBool();
		engineInfo->setBooleanInput(VarIdentifier, op);
	}
	break;
	case EVarType::VARTYPE_INT:
	{
		FTouchVar<TArray<int32_t>> op;
		if (count <= 1)
		{
			op.data.Add(GetValueAsInt());
		}
		else
		{
			int* buffer = GetValueAsIntArray();
			for (int i = 0; i < count; i++)
			{
				op.data.Add(buffer[i]);
			}
		}

		engineInfo->setIntegerInput(VarIdentifier, op);
	}
	break;
	case EVarType::VARTYPE_DOUBLE:
	{
		FTouchVar<TArray<double>> op;

		if (count > 1)
		{
			double* buffer = GetValueAsDoubleArray();
			for (int i = 0; i < count; i++)
			{
				op.data.Add(buffer[i]);
			}
		}
		else
		{
			op.data.Add(GetValueAsDouble());
		}

		engineInfo->setDoubleInput(VarIdentifier, op);
	}
	break;
	case EVarType::VARTYPE_FLOAT:
	{
		FTouchCHOPSingleSample tcss;
		tcss.channelData.Add(GetValueAsFloat());
		engineInfo->setCHOPInputSingleSample(VarIdentifier, tcss);
	}
	break;
	case EVarType::VARTYPE_FLOATBUFFER:
	{
		TArray<float> floatArray = GetValueAsFloatBuffer();
		FTouchCHOPSingleSample tcss;
		for (int i = 0; i < floatArray.Num(); i++)
		{
			tcss.channelData.Add(floatArray[i]);
		}
		engineInfo->setCHOPInputSingleSample(VarIdentifier, tcss);
	}
	break;
	case EVarType::VARTYPE_STRING:
	{
		if (!isArray)
		{
			FTouchVar<char*> op;
			op.data = TCHAR_TO_ANSI(*GetValueAsString());
			engineInfo->setStringInput(VarIdentifier, op);
		}
		else
		{
			FTouchVar<TETable*> op;
			op.data = TETableCreate();

			engineInfo->setTableInput(VarIdentifier, op);
			TERelease(&op.data);
		}
	}
	break;
	case EVarType::VARTYPE_TEXTURE:
		//engineInfo->setTOPInput(VarIdentifier, GetValueAsTextureRenderTarget());
		engineInfo->setTOPInput(VarIdentifier, GetValueAsTexture());
		break;
	default:
		// unimplemented type
		break;
	}
}

void FTouchDynamicVar::GetOutput(UTouchEngineInfo* engineInfo)
{
	switch (VarType)
	{
	case EVarType::VARTYPE_BOOL:
	{
		FTouchVar<bool> op = engineInfo->getBooleanOutput(VarIdentifier);
		SetValue(op.data);
	}
	break;
	case EVarType::VARTYPE_INT:
	{
		FTouchVar<int32_t> op = engineInfo->getIntegerOutput(VarIdentifier);
		SetValue((int)op.data);
	}
	break;
	case EVarType::VARTYPE_DOUBLE:
	{
		FTouchVar<double> op = engineInfo->getDoubleOutput(VarIdentifier);
		SetValue(op.data);
	}
	break;
	case EVarType::VARTYPE_FLOAT:
	{
		FTouchCHOPSingleSample tcss = engineInfo->getCHOPOutputSingleSample(VarIdentifier);
		SetValue(tcss.channelData[0]);
	}
	break;
	case EVarType::VARTYPE_FLOATBUFFER:
	{
		//FTouchCHOPSingleSample tcss = engineInfo->getCHOPOutputSingleSample(VarIdentifier);
		SetValue(engineInfo->getCHOPOutputSingleSample(VarIdentifier).channelData);
	}
	break;
	case EVarType::VARTYPE_STRING:
	{
		if (!isArray)
		{
			FTouchVar<TEString*> op = engineInfo->getStringOutput(VarIdentifier);
			SetValue(FString(op.data->string));
		}
		else
		{
			FTouchVar<TETable*> op = engineInfo->getTableOutput(VarIdentifier);

			TArray<FString> buffer;

			int32 rowcount = TETableGetRowCount(op.data), columncount = TETableGetColumnCount(op.data);

			for (int i = 0; i < rowcount; i++)
			{
				for (int j = 0; j < columncount; j++)
				{
					buffer.Add(TETableGetStringValue(op.data, i, j));
				}
			}

			SetValue(buffer);
		}
	}
	break;
	case EVarType::VARTYPE_TEXTURE:
	{
		FTouchTOP top = engineInfo->getTOPOutput(VarIdentifier);
		SetValue(top.texture);
	}
	break;
	default:
		// unimplemented type
		break;
	}
}