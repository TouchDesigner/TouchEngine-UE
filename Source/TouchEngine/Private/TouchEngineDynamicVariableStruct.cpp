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
	if (parent)
		parent->UnbindDelegates();

	OnToxLoaded.Clear();
	OnToxFailedLoad.Clear();
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
	if (DelegateHandle.IsValid() && OnToxLoaded.IsBound())
		OnToxLoaded.Remove(DelegateHandle);
}

/*
void FTouchEngineDynamicVariableContainer::ToxLoaded()
{
	//OnToxLoaded.Broadcast();
}
*/

void FTouchEngineDynamicVariableContainer::ToxParametersLoaded(TArray<FTouchEngineDynamicVariable> variablesIn, TArray<FTouchEngineDynamicVariable> variablesOut)
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

	DynVars_Input.Empty();
	DynVars_Output.Empty();

	DynVars_Input = variablesIn;
	DynVars_Output = variablesOut;

	parent->OnToxLoaded.Broadcast();

	OnToxLoaded.Broadcast();

	parent->UnbindDelegates();
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
	if (Handle.IsValid() && OnToxFailedLoad.IsBound())
		OnToxFailedLoad.Remove(Handle);
}

void FTouchEngineDynamicVariableContainer::ToxFailedLoad()
{
	parent->OnToxFailedLoad.Broadcast();
	OnToxFailedLoad.Broadcast();

	parent->UnbindDelegates();
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

FTouchEngineDynamicVariable* FTouchEngineDynamicVariableContainer::GetDynamicVariableByName(FString varName)
{
	FTouchEngineDynamicVariable* var = nullptr;

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

FTouchEngineDynamicVariable* FTouchEngineDynamicVariableContainer::GetDynamicVariableByIdentifier(FString varIdentifier)
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

bool FTouchEngineDynamicVariableContainer::Serialize(FArchive& Ar)
{
	return false;
}



FTouchEngineDynamicVariable::FTouchEngineDynamicVariable()
{
}

FTouchEngineDynamicVariable::~FTouchEngineDynamicVariable()
{
	Clear();

}
// copies value from one touch engine dynamic variable struct to this one
void FTouchEngineDynamicVariable::Copy(const FTouchEngineDynamicVariable* other)
{
	VarLabel = other->VarLabel;
	VarName = other->VarName;
	VarIdentifier = other->VarIdentifier;
	VarType = other->VarType;
	VarIntent = other->VarIntent;
	count = other->count;
	size = other->size;
	isArray = other->isArray;

	SetValue(other);
}

void FTouchEngineDynamicVariable::Clear()
{
	if (!value)
	{
		return;
	}

	switch (VarType)
	{
	case EVarType::VARTYPE_BOOL:
	{
		delete[](bool*)value;
	}
	break;
	case EVarType::VARTYPE_INT:
	{
		if (count <= 1 && !isArray)
		{
			delete (int*)value;
		}
		else
		{
			delete[](int*) value;
		}
	}
	break;
	case EVarType::VARTYPE_DOUBLE:
	{
		if (count <= 1 && !isArray)
		{
			delete (double*)value;
		}
		else
		{
			delete[](double*)value;
		}
	}
	break;
	case EVarType::VARTYPE_FLOAT:
	{
		delete[](float*)value;
	}
	break;
	case EVarType::VARTYPE_FLOATBUFFER:
	{
	}
	break;
	case EVarType::VARTYPE_STRING:
	{
		if (!isArray)
		{
			delete[](char*)value;
		}
		else
		{
			for (int i = 0; i < count; i++)
			{
				delete[]((char**)value)[i];
			}

			delete[](char**)value;
		}
	}
	break;
	case EVarType::VARTYPE_TEXTURE:
	{
		value = nullptr;
	}
	break;
	}

	value = nullptr;
}

// returns value as bool
bool FTouchEngineDynamicVariable::GetValueAsBool() const
{
	return value ? *(bool*)value : false;
}
// returns value as integer
int FTouchEngineDynamicVariable::GetValueAsInt() const
{
	return value ? *(int*)value : 0;
}
// returns value as integer in a TOptional struct
TOptional<int> FTouchEngineDynamicVariable::GetValueAsOptionalInt() const
{
	return TOptional<int>(GetValueAsInt());
}
// returns indexed value as integer
int FTouchEngineDynamicVariable::GetValueAsIntIndexed(int index) const
{
	return value ? ((int*)value)[index] : 0;
}
// returns indexed value as integer in a TOptional struct
TOptional<int> FTouchEngineDynamicVariable::GetIndexedValueAsOptionalInt(int index) const
{
	return TOptional<int>(GetValueAsIntIndexed(index));
}
// returns value as integer array
int* FTouchEngineDynamicVariable::GetValueAsIntArray() const
{
	return value ? (int*)value : 0;
}

TArray<int> FTouchEngineDynamicVariable::GetValueAsIntTArray() const
{
	if (VarType != EVarType::VARTYPE_INT || !isArray)
		return TArray<int>();

	TArray<int> returnArray;
	for (int i = 0; i < count; i++)
	{
		returnArray.Add(((int*)value)[i]);
	}
	return returnArray;
}

double FTouchEngineDynamicVariable::GetValueAsDouble() const
{
	return value ? *(double*)value : 0;
}

double FTouchEngineDynamicVariable::GetValueAsDoubleIndexed(int index) const
{
	return value ? GetValueAsDoubleArray()[index] : 0;
}

TOptional<double> FTouchEngineDynamicVariable::GetValueAsOptionalDouble() const
{
	return TOptional<double>(GetValueAsDouble());
}

TOptional<double> FTouchEngineDynamicVariable::GetIndexedValueAsOptionalDouble(int index) const
{
	return TOptional<double>(GetValueAsDoubleIndexed(index));
}

double* FTouchEngineDynamicVariable::GetValueAsDoubleArray() const
{
	return value ? (double*)value : 0;
}

TArray<double> FTouchEngineDynamicVariable::GetValueAsDoubleTArray() const
{
	if (VarType != EVarType::VARTYPE_DOUBLE || !isArray)
		return TArray<double>();

	TArray<double> returnArray;
	for (int i = 0; i < count; i++)
	{
		returnArray.Add(((double*)value)[i]);
	}
	return returnArray;
}

float FTouchEngineDynamicVariable::GetValueAsFloat() const
{
	return value ? *(float*)value : 0;
}

TOptional<float> FTouchEngineDynamicVariable::GetValueAsOptionalFloat() const
{
	return TOptional<float>(GetValueAsFloat());
}

FString FTouchEngineDynamicVariable::GetValueAsString() const
{
	return value ? FString((char*)value) : FString("");
}

TArray<FString> FTouchEngineDynamicVariable::GetValueAsStringArray() const
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

UTexture* FTouchEngineDynamicVariable::GetValueAsTexture() const
{
	return (UTexture*)value;
}

TArray<float> FTouchEngineDynamicVariable::GetValueAsFloatBuffer() const
{
	if (!value)
		return TArray<float>();

	float* floatArray = (float*)value;

	TArray<float> returnValue;

	for (int i = 0; i < count; i++)
	{
		returnValue.Add(floatArray[i]);
	}

	return returnValue;
}



void FTouchEngineDynamicVariable::SetValue(UObject* newValue, size_t _size)
{
	if (newValue == nullptr)
	{
		value = nullptr;
		return;
	}

	value = (void*)newValue;
	size = _size;
}

void FTouchEngineDynamicVariable::SetValue(bool _value)
{
	if (VarType == EVarType::VARTYPE_BOOL)
	{
		Clear();

		value = new bool;
		*((bool*)value) = _value;
	}
}

void FTouchEngineDynamicVariable::SetValue(int _value)
{
	if (VarType == EVarType::VARTYPE_INT)
	{
		Clear();

		value = new int;
		*((int*)value) = _value;
	}
}

void FTouchEngineDynamicVariable::SetValue(TArray<int> _value)
{
	if (VarType == EVarType::VARTYPE_INT && isArray)
	{
		Clear();

		value = new int* [_value.Num()];

		for (int i = 0; i < _value.Num(); i++)
		{
			((int*)value)[i] = _value[i];
		}

		count = _value.Num();
		size = sizeof(int) * count;
	}
}

void FTouchEngineDynamicVariable::SetValue(double _value)
{
	if (VarType == EVarType::VARTYPE_DOUBLE)
	{
		Clear();

		value = new double;
		*((double*)value) = _value;
	}
}

void FTouchEngineDynamicVariable::SetValue(TArray<double> _value)
{
	if (VarType == EVarType::VARTYPE_DOUBLE && isArray)
	{
		Clear();

		value = new double* [_value.Num()];

		for (int i = 0; i < _value.Num(); i++)
		{
			((double*)value)[i] = _value[i];
		}

		count = _value.Num();
		size = sizeof(double) * count;
	}
}

void FTouchEngineDynamicVariable::SetValue(float _value)
{
	if (VarType == EVarType::VARTYPE_FLOAT)
	{
		Clear();

		value = new float;
		*((float*)value) = _value;
	}
}

void FTouchEngineDynamicVariable::SetValue(TArray<float> _value)
{
	if (_value.Num() == 0)
	{
		Clear();

		value = nullptr;
		return;
	}

	if (VarType == EVarType::VARTYPE_FLOATBUFFER || (VarType == EVarType::VARTYPE_FLOAT && isArray))
	{
		Clear();

		value = new float[_value.Num()];

		for (int i = 0; i < _value.Num(); i++)
		{
			((float*)value)[i] = _value[i];
		}

#if WITH_EDITORONLY_DATA
		floatBufferProperty = _value;
#endif

		count = _value.Num();
		size = count * sizeof(float);
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

		value = new double[_value.Num()];

		for (int i = 0; i < _value.Num(); i++)
		{
			((double*)value)[i] = _value[i];
		}


		count = _value.Num();
		size = count * sizeof(double);
		isArray = true;
	}
}

void FTouchEngineDynamicVariable::SetValue(FString _value)
{
	if (VarType == EVarType::VARTYPE_STRING)
	{
		Clear();

		char* buffer = TCHAR_TO_ANSI(*_value);

		value = new char[_value.Len() + 1];

		for (int i = 0; i < _value.Len() + 2; i++)
		{
			((char*)value)[i] = buffer[i];
		}
	}
}

void FTouchEngineDynamicVariable::SetValue(TArray<FString> _value)
{
	if (_value.Num() == 0)
	{
		Clear();

		value = nullptr;
		count = 0;
		size = 0;
		return;
	}

	value = new char* [_value.Num()];
	size = 0;

	for (int i = 0; i < _value.Num(); i++)
	{
		Clear();

		char* tempvalue = TCHAR_TO_ANSI(*(_value[i]));
		((char**)value)[i] = new char[(_value[i]).Len() + 1];
		size += _value[i].Len() + 1;
		for (int j = 0; j < _value[i].Len() + 1; j++)
		{
			((char**)value)[i][j] = tempvalue[j];
		}
	}

#if WITH_EDITORONLY_DATA
	stringArrayProperty = _value;
#endif

	count = _value.Num();
	isArray = true;
}

void FTouchEngineDynamicVariable::SetValue(UTexture* _value)
{
	if (VarType == EVarType::VARTYPE_TEXTURE)
	{
		Clear();

		SetValue((UObject*)_value, sizeof(UTexture));
	}
}

void FTouchEngineDynamicVariable::SetValue(const FTouchEngineDynamicVariable* other)
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
		if (!other->isArray)
			SetValue(other->GetValueAsInt());
		else
			SetValue(other->GetValueAsIntTArray());

		break;
	}
	case EVarType::VARTYPE_DOUBLE:
	{
		if (!other->isArray)
			SetValue(other->GetValueAsDouble());
		else
			SetValue(other->GetValueAsDoubleTArray());
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
	dropDownData = other->dropDownData;

#endif
}



void FTouchEngineDynamicVariable::HandleChecked(ECheckBoxState InState)
{
	//FTouchDynamicVar oldValue; oldValue.Copy(this);

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

	//UpdateInstances(blueprintOwner, parentComponent, oldValue);
}

FText FTouchEngineDynamicVariable::HandleTextBoxText() const
{
	if (value)
		return FText::FromString(GetValueAsString());
	else
		return FText();
}

void FTouchEngineDynamicVariable::HandleTextBoxTextChanged(const FText& NewText)
{
	//FTouchDynamicVar oldValue; oldValue.Copy(this);

	SetValue(NewText.ToString());

	//UpdateInstances(blueprintOwner, parentComponent, oldValue);
}

void FTouchEngineDynamicVariable::HandleTextBoxTextCommited(const FText& NewText)
{
	//FTouchDynamicVar oldValue; oldValue.Copy(this);

	SetValue(NewText.ToString());

	//UpdateInstances(blueprintOwner, parentComponent, oldValue);
}

void FTouchEngineDynamicVariable::HandleTextureChanged()
{
#if WITH_EDITORONLY_DATA
	//FTouchDynamicVar oldValue; oldValue.Copy(this);

	SetValue(textureProperty);

	//UpdateInstances(blueprintOwner, parentComponent, oldValue);
#endif
}

void FTouchEngineDynamicVariable::HandleColorChanged()
{
	//FTouchDynamicVar oldValue; oldValue.Copy(this);

#if WITH_EDITORONLY_DATA
	TArray<float> buffer;

	buffer.Add(colorProperty.R);
	buffer.Add(colorProperty.G);
	buffer.Add(colorProperty.B);
	buffer.Add(colorProperty.A);

	SetValue(buffer);
#endif
	//UpdateInstances(blueprintOwner, parentComponent, oldValue);
}

void FTouchEngineDynamicVariable::HandleVector4Changed()
{
	//FTouchDynamicVar oldValue; oldValue.Copy(this);

#if WITH_EDITORONLY_DATA
	TArray<float> buffer;

	buffer.Add(vector4Property.X);
	buffer.Add(vector4Property.Y);
	buffer.Add(vector4Property.Z);
	buffer.Add(vector4Property.W);

	SetValue(buffer);
#endif
	//UpdateInstances(blueprintOwner, parentComponent, oldValue);
}

void FTouchEngineDynamicVariable::HandleVectorChanged()
{
	//FTouchDynamicVar oldValue; oldValue.Copy(this);

#if WITH_EDITORONLY_DATA
	TArray<float> buffer;

	buffer.Add(vectorProperty.X);
	buffer.Add(vectorProperty.Y);
	buffer.Add(vectorProperty.Z);

	SetValue(buffer);
#endif
	//UpdateInstances(blueprintOwner, parentComponent, oldValue);
}

void FTouchEngineDynamicVariable::HandleFloatBufferChanged()
{
#if WITH_EDITORONLY_DATA
	//FTouchDynamicVar oldValue; oldValue.Copy(this);

	SetValue(floatBufferProperty);

	//UpdateInstances(blueprintOwner, parentComponent, oldValue);
#endif
}

void FTouchEngineDynamicVariable::HandleFloatBufferChildChanged()
{
	//FTouchDynamicVar oldValue; oldValue.Copy(this);

#if WITH_EDITORONLY_DATA
	SetValue(floatBufferProperty);
#endif
	//UpdateInstances(blueprintOwner, parentComponent, oldValue);
}

void FTouchEngineDynamicVariable::HandleStringArrayChanged()
{
#if WITH_EDITORONLY_DATA
	//FTouchDynamicVar oldValue; oldValue.Copy(this);

	SetValue(stringArrayProperty);

	//UpdateInstances(blueprintOwner, parentComponent, oldValue);
#endif
}

void FTouchEngineDynamicVariable::HandleStringArrayChildChanged()
{
#if WITH_EDITORONLY_DATA
	//FTouchDynamicVar oldValue; oldValue.Copy(this);

	SetValue(stringArrayProperty);

	//UpdateInstances(blueprintOwner, parentComponent, oldValue);
#endif
}

void FTouchEngineDynamicVariable::HandleDropDownBoxValueChanged(TSharedPtr<FString> arg)
{
#if WITH_EDITORONLY_DATA
	SetValue((int)dropDownData[*arg]);
#endif
}



bool FTouchEngineDynamicVariable::Serialize(FArchive& Ar)
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
	Ar << dropDownData;
#endif


	// write void pointer
	if (Ar.IsSaving())
	{
		// writing dynamic variable to archive
		switch (VarType)
		{
		case EVarType::VARTYPE_BOOL:
		{
			bool tempBool = false;
			if (value)
			{
				tempBool = GetValueAsBool();
			}
			Ar << tempBool;
		}
		break;
		case EVarType::VARTYPE_INT:
		{
			if (count <= 1)
			{
				int tempInt = 0;
				if (value)
				{
					tempInt = GetValueAsInt();
				}
				Ar << tempInt;
			}
			else
			{
				for (int i = 0; i < count; i++)
				{
					if (value)
					{
						int tempIntIndex = ((int*)value)[i];
						Ar << tempIntIndex;
					}
					else
					{
						int tempIntIndex = 0;
						Ar << tempIntIndex;
					}
				}
			}
		}
		break;
		case EVarType::VARTYPE_DOUBLE:
		{
			if (count <= 1)
			{
				double tempDouble = 0;
				if (value)
					tempDouble = GetValueAsDouble();
				Ar << tempDouble;
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
						double tempDoubleIndex = 0;
						Ar << tempDoubleIndex;
					}
				}
			}
		}
		break;
		case EVarType::VARTYPE_FLOAT:
		{
			float tempFloat = 0;
			if (value)
				tempFloat = GetValueAsFloat();
			Ar << tempFloat;
		}
		break;
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			TArray<float> tempFloatArray = TArray<float>();
			if (value)
				tempFloatArray = GetValueAsFloatBuffer();
			Ar << tempFloatArray;
		}
		break;
		case EVarType::VARTYPE_STRING:
		{
			if (!isArray)
			{
				FString tempString = FString("");
				if (value)
					tempString = GetValueAsString();
				Ar << tempString;
			}
			else
			{
				TArray<FString> tempStringArray;
				if (value)
					tempStringArray = GetValueAsStringArray();
				Ar << tempStringArray;
			}
		}
		break;
		case EVarType::VARTYPE_TEXTURE:
		{
			UTexture* tempTexture = nullptr;
			if (value)
				//tempValue = GetValueAsTextureRenderTarget();
				tempTexture = GetValueAsTexture();
			Ar << tempTexture;
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
			bool tempBool;
			Ar << tempBool;
			SetValue(tempBool);
		}
		break;
		case EVarType::VARTYPE_INT:
		{
			if (count <= 1)
			{
				int tempInt;
				Ar << tempInt;
				SetValue(tempInt);
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
				double tempDouble;
				Ar << tempDouble;
				SetValue(tempDouble);
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
			float tempFloat;
			Ar << tempFloat;
			SetValue(tempFloat);
		}
		break;
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			TArray<float> tempFloatArray;
			Ar << tempFloatArray;
			SetValue(tempFloatArray);
		}
		break;
		case EVarType::VARTYPE_STRING:
		{
			if (!isArray)
			{
				FString tempString = FString("");
				Ar << tempString;
				SetValue(tempString);
			}
			else
			{
				TArray<FString> tempStringArray;
				Ar << tempStringArray;
				SetValue(tempStringArray);
			}
		}
		break;
		case EVarType::VARTYPE_TEXTURE:
		{
			UTexture* tempTexture;
			Ar << tempTexture;
			SetValue(tempTexture);
		}
		break;
		default:
			// unsupported type
			break;
		}
	}

	return true;
}

bool FTouchEngineDynamicVariable::Identical(const FTouchEngineDynamicVariable* Other, uint32 PortFlags) const
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

					if (otherValue != nullptr)
					{
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

					if (otherValue != nullptr)
					{

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

//
//bool FTouchDynamicVar::ExportTextItem(FString& ValueStr, FTouchDynamicVar const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const
//{
//	/* example
//	ValueStr += TEXT("(");
//		for (TMap<int32, double>::TConstIterator It(Doubles); It; ++It)
//		{
//			ValueStr += FString::Printf( TEXT("(%d,%f)"),It.Key(), It.Value());
//		}
//		ValueStr += TEXT(")");
//		return true;
//	*/
//
//	/*
//	Ar << VarLabel;
//	Ar << VarName;
//	Ar << VarIdentifier;
//	Ar << VarType;
//	Ar << VarIntent;
//	Ar << count;
//	Ar << size;
//	Ar << isArray;
//	// write editor variables just in case they need to be used
//	Ar << textureProperty;
//	Ar << floatBufferProperty;
//	Ar << stringArrayProperty;
//	Ar << colorProperty;
//	Ar << vector4Property;
//	Ar << vectorProperty;
//	*/
//
//	ValueStr += TEXT("(");
//
//	ValueStr += VarLabel; ValueStr += TEXT(",");
//	ValueStr += VarName; ValueStr += TEXT(",");
//	ValueStr += VarIdentifier; ValueStr += TEXT(",");
//
//	ValueStr += FString::Printf(TEXT("%i,"), (int)VarType);
//	ValueStr += FString::Printf(TEXT("%i,"), (int)VarIntent);
//	ValueStr += FString::Printf(TEXT("%i,"), count);
//	ValueStr += FString::Printf(TEXT("%i,"), (int)size);
//	ValueStr += isArray ? FString(TEXT("true,")) : FString(TEXT("false,"));
//
//#if WITH_EDITORONLY_DATA
//
//	//FProperty texProp = FProperty::
//
//	//textureProperty.   GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Input)
//
//	//texProp.ExportTextItem(ValueStr, textureProperty, textureProperty->GetClass()->GetDefaultObject(), Parent, PortFlags, ExportRootScope);
//
//#endif
//
//
//
//
//
//
//	ValueStr += TEXT(")");
//
//	return true;
//}
//
//bool FTouchDynamicVar::ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText)
//{
//	/* example
//	check(*Buffer == TEXT('('));
//		Buffer++;
//		Doubles.Empty();
//		while (1)
//		{
//			const TCHAR* Start = Buffer;
//			while (*Buffer && *Buffer != TEXT(','))
//			{
//				if (*Buffer == TEXT(')'))
//				{
//					break;
//				}
//				Buffer++;
//			}
//			if (*Buffer == TEXT(')'))
//			{
//				break;
//			}
//			int32 Key = FCString::Atoi(Start);
//			if (*Buffer)
//			{
//				Buffer++;
//			}
//			Start = Buffer;
//			while (*Buffer && *Buffer != TEXT(')'))
//			{
//				Buffer++;
//			}
//			double Value = FCString::Atod(Start);
//
//			if (*Buffer)
//			{
//				Buffer++;
//			}
//			Doubles.Add(Key, Value);
//		}
//		if (*Buffer)
//		{
//			Buffer++;
//		}
//		return true;
//	}
//	*/
//
//	check(*Buffer == TEXT('('));
//
//	Buffer++;
//
//	//
//	/*
//	ValueStr += TEXT("(");
//
//	ValueStr += VarLabel;
//	ValueStr += VarName;
//	ValueStr += VarIdentifier;
//
//	ValueStr += FString::Printf(TEXT("%i,"), (int)VarType);
//	ValueStr += FString::Printf(TEXT("%i,"), (int)VarIntent);
//	ValueStr += FString::Printf(TEXT("%i,"), count);
//	ValueStr += FString::Printf(TEXT("%i,"), (int)size);
//	ValueStr += isArray ? FString(TEXT("true,")) : FString(TEXT("false,"));
//	*/
//
//	// get first 3 strings
//
//	const TCHAR* Start = Buffer;
//	int len = 0;
//	while (Buffer && *Buffer != TEXT(','))
//	{
//		Buffer++;
//		len++;
//	}
//	VarLabel = FString(len, Start);
//	if (*Buffer)
//		Buffer++;
//
//	Start = Buffer;
//	len = 0;
//	while (Buffer && *Buffer != TEXT(','))
//	{
//		Buffer++;
//		len++;
//	}
//	VarName = FString(len, Start);
//	if (*Buffer)
//		Buffer++;
//
//	Start = Buffer;
//	len = 0;
//	while (Buffer && *Buffer != TEXT(','))
//	{
//		Buffer++;
//		len++;
//	}
//	VarIdentifier = FString(len, Start);
//	if (*Buffer)
//		Buffer++;
//
//
//
//
//
//	if (*Buffer)
//	{
//		Buffer++;
//	}
//
//	return true;
//}



void FTouchEngineDynamicVariable::SendInput(UTouchEngineInfo* engineInfo)
{
	if (!value)
		return;

	switch (VarType)
	{
	case EVarType::VARTYPE_BOOL:
	{

		if (VarIntent == EVarIntent::VARINTENT_MOMENTARY || VarIntent == EVarIntent::VARINTENT_PULSE)
		{
			if (GetValueAsBool() == true)
			{
				FTouchVar<bool> op;
				op.data = true;
				engineInfo->setBooleanInput(VarIdentifier, op);
				SetValue(false);
			}
		}
		else
		{
			FTouchVar<bool> op;
			op.data = GetValueAsBool();
			engineInfo->setBooleanInput(VarIdentifier, op);
		}
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

void FTouchEngineDynamicVariable::GetOutput(UTouchEngineInfo* engineInfo)
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