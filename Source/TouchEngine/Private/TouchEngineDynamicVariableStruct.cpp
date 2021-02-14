// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "TETable.h"
#include "Input\Reply.h"

FTouchEngineDynamicVariableContainer::FTouchEngineDynamicVariableContainer()
{
	if (parent && parent->EngineInfo && parent->EngineInfo->isLoaded())
	{

	}
}

FTouchEngineDynamicVariableContainer::~FTouchEngineDynamicVariableContainer()
{
	OnToxLoaded.Clear();
}



FDelegateHandle FTouchEngineDynamicVariableContainer::CallOrBind_OnToxLoaded(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (parent && parent->EngineInfo && parent->EngineInfo->isLoaded())
	{
		Delegate.Execute();
		return OnToxLoaded.Add(Delegate);
	}
	else
	{
		if (!(Delegate.GetUObject() != nullptr ? OnToxLoaded.IsBoundToObject(Delegate.GetUObject()) : false))
		{
			return OnToxLoaded.Add(Delegate);
		}
	}

	return FDelegateHandle();
}

void FTouchEngineDynamicVariableContainer::Unbind_OnToxLoaded(FDelegateHandle DelegateHandle)
{
	if (!OnToxLoaded.IsBound())
		return;

	if (parent && parent->EngineInfo)
	{
		OnToxLoaded.Remove(DelegateHandle);
	}
}

void FTouchEngineDynamicVariableContainer::ToxLoaded()
{
	OnToxLoaded.Broadcast();
}

void FTouchEngineDynamicVariableContainer::ToxParametersLoaded(TArray<FTouchEngineDynamicVariable> variablesIn, TArray<FTouchEngineDynamicVariable> variablesOut)
{
	// just in case this isn't set already

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
			if (DynVars_Input[i].VarIdentifier == variablesIn[j].VarIdentifier && DynVars_Input[i].VarType == variablesIn[j].VarType && DynVars_Input[i].isArray == variablesIn[j].isArray)
			{
				variablesIn[j].SetValue(&DynVars_Input[i]);
			}
		}
	}

	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		for (int j = 0; j < variablesOut.Num(); j++)
		{
			if (DynVars_Output[i].VarIdentifier == variablesOut[j].VarIdentifier && DynVars_Output[i].VarType == variablesOut[j].VarType && DynVars_Output[i].isArray == variablesOut[j].isArray)
			{
				variablesOut[j].SetValue(&DynVars_Output[i]);
			}
		}
	}

	DynVars_Input = variablesIn;
	DynVars_Output = variablesOut;
}

FDelegateHandle FTouchEngineDynamicVariableContainer::CallOrBind_OnToxFailedLoad(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (parent && parent->EngineInfo && parent->EngineInfo->hasFailedLoad())
	{
		Delegate.Execute();
		return OnToxLoadFailed.Add(Delegate);
	}
	else
	{
		if (!(Delegate.GetUObject() != nullptr ? OnToxLoaded.IsBoundToObject(Delegate.GetUObject()) : false))
		{
			return OnToxLoadFailed.Add(Delegate);
		}
	}

	return FDelegateHandle();
}

void FTouchEngineDynamicVariableContainer::Unbind_OnToxFailedLoad(FDelegateHandle Handle)
{
	//if (parent && parent->EngineInfo)
	//{
	OnToxLoadFailed.Remove(Handle);
	//}
}

void FTouchEngineDynamicVariableContainer::ToxFailedLoad()
{
	OnToxLoadFailed.Broadcast();
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

bool FTouchEngineDynamicVariableContainer::HasInput(FString varName)
{
	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		if (DynVars_Input[i].VarName == varName)
			return true;
	}
	return false;
}

bool FTouchEngineDynamicVariableContainer::HasInput(FString varName, EVarType varType)
{
	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		if (DynVars_Input[i].VarName == varName && DynVars_Input[i].VarType == varType)
			return true;
	}
	return false;
}

bool FTouchEngineDynamicVariableContainer::HasOutput(FString varName)
{
	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		if (DynVars_Output[i].VarName == varName)
			return true;
	}
	return false;
}

bool FTouchEngineDynamicVariableContainer::HasOutput(FString varName, EVarType varType)
{
	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		if (DynVars_Output[i].VarName == varName && DynVars_Output[i].VarType == varType)
			return true;
	}
	return false;
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



// returns value as bool
bool FTouchEngineDynamicVariable::GetValueAsBool() const
{
	return value ? *(bool*)value : false;
}
// returns value as bool
ECheckBoxState FTouchEngineDynamicVariable::GetValueAsCheckState() const
{
	return GetValueAsBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
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
// returns value as double
double FTouchEngineDynamicVariable::GetValueAsDouble() const
{
	return value ? *(double*)value : 0;
}
// returns indexed value as double
double FTouchEngineDynamicVariable::GetValueAsDoubleIndexed(int index) const
{
	return value ? GetValueAsDoubleArray()[index] : 0;
}
// returns value as double in a TOptional struct
TOptional<double> FTouchEngineDynamicVariable::GetValueAsOptionalDouble() const
{
	return TOptional<double>(GetValueAsDouble());
}
// returns indexed value as double in a TOptional struct
TOptional<double> FTouchEngineDynamicVariable::GetIndexedValueAsOptionalDouble(int index) const
{
	return TOptional<double>(GetValueAsDoubleIndexed(index));
}
// returns value as double array
double* FTouchEngineDynamicVariable::GetValueAsDoubleArray() const
{
	return value ? (double*)value : 0;
}
// returns value as float
float FTouchEngineDynamicVariable::GetValueAsFloat() const
{
	return value ? *(float*)value : 0;
}
// returns value as float in a TOptional struct
TOptional<float> FTouchEngineDynamicVariable::GetValueAsOptionalFloat() const
{
	return TOptional<float>(GetValueAsFloat());
}
// returns value as fstring
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

// returns value as render target 2D pointer
UTextureRenderTarget2D* FTouchEngineDynamicVariable::GetValueAsTextureRenderTarget() const
{
	return (UTextureRenderTarget2D*)value;
}
// returns value as texture 2D pointer
UTexture2D* FTouchEngineDynamicVariable::GetValueAsTexture() const
{
	return (UTexture2D*)value;
}

TArray<float> FTouchEngineDynamicVariable::GetValueAsFloatBuffer() const
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

void FTouchEngineDynamicVariable::SetValue(void* newValue, size_t _size)
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

void FTouchEngineDynamicVariable::SetValue(TArray<float> _value)
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

void FTouchEngineDynamicVariable::SetValue(FString _value)
{
	if (VarType == EVarType::VARTYPE_STRING)
		SetValue((void*)TCHAR_TO_ANSI(*_value), _value.Len() + 1);
}

void FTouchEngineDynamicVariable::SetValue(TArray<FString> _value)
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

void FTouchEngineDynamicVariable::SetValue(UTextureRenderTarget2D* _value)
{
	if (VarType == EVarType::VARTYPE_TEXTURE)
	{
		SetValue((UObject*)_value, sizeof(UTextureRenderTarget2D));

#if WITH_EDITORONLY_DATA
		textureProperty = _value;
#endif
	}
}

void FTouchEngineDynamicVariable::SetValue(UTexture2D* _value)
{
	if (VarType == EVarType::VARTYPE_TEXTURE)
	{
		SetValue((UObject*)_value, sizeof(UTexture2D));
	}
}

void FTouchEngineDynamicVariable::SetValue(FTouchEngineDynamicVariable* other)
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
			SetValue(other->GetValueAsDoubleArray());
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
}



void FTouchEngineDynamicVariable::HandleChecked(ECheckBoxState InState)
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

FText FTouchEngineDynamicVariable::HandleTextBoxText() const
{
	if (value)
		return FText::FromString(GetValueAsString());
	else
		return FText();
}

void FTouchEngineDynamicVariable::HandleTextBoxTextChanged(const FText& NewText)
{
	SetValue(NewText.ToString());
}

void FTouchEngineDynamicVariable::HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo)
{
	SetValue(NewText.ToString());
}

void FTouchEngineDynamicVariable::HandleTextureChanged()
{
#if WITH_EDITORONLY_DATA
	SetValue(textureProperty);
#endif
}

void FTouchEngineDynamicVariable::HandleColorChanged()
{
	double buffer[4];

	buffer[0] = colorProperty.R;
	buffer[1] = colorProperty.G;
	buffer[2] = colorProperty.B;
	buffer[3] = colorProperty.A;

	SetValue((void*)buffer, sizeof(double) * 4);
}

void FTouchEngineDynamicVariable::HandleVector4Changed()
{
	double buffer[4];

	buffer[0] = vector4Property.X;
	buffer[1] = vector4Property.Y;
	buffer[2] = vector4Property.Z;
	buffer[3] = vector4Property.W;

	SetValue((void*)buffer, sizeof(double) * 4);
}

void FTouchEngineDynamicVariable::HandleVectorChanged()
{
	double buffer[3];

	buffer[0] = vectorProperty.X;
	buffer[1] = vectorProperty.Y;
	buffer[2] = vectorProperty.Z;

	SetValue((void*)buffer, sizeof(double) * 3);
}

void FTouchEngineDynamicVariable::HandleFloatBufferChanged()
{
#if WITH_EDITORONLY_DATA
	SetValue(floatBufferProperty);
#endif
}

void FTouchEngineDynamicVariable::HandleFloatBufferChildChanged()
{
	SetValue(floatBufferProperty);
}

void FTouchEngineDynamicVariable::HandleStringArrayChanged()
{
#if WITH_EDITORONLY_DATA
	SetValue(stringArrayProperty);
#endif
}

void FTouchEngineDynamicVariable::HandleStringArrayChildChanged()
{
	SetValue(stringArrayProperty);
}

bool FTouchEngineDynamicVariable::Serialize(FArchive& Ar)
{
	// write / read all normal variables
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
			int tempValue = 0;
			if (value)
			{
				tempValue = GetValueAsInt();
			}
			Ar << tempValue;
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
			UTextureRenderTarget2D* tempValue = nullptr;
			if (value)
				tempValue = GetValueAsTextureRenderTarget();
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
			int tempValue;
			Ar << tempValue;
			SetValue(tempValue);
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
				size = sizeof(value);

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
			UTextureRenderTarget2D* tempValue;
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

void FTouchEngineDynamicVariable::SendInput(UTouchEngineInfo* engineInfo)
{
	if (!value)
		return;

	switch (VarType)
	{
	case EVarType::VARTYPE_BOOL:
	{
		FTouchVar<bool> op;
		op.data = GetValueAsBool();
		engineInfo->setBOPInput(VarIdentifier, op);
	}
	break;
	case EVarType::VARTYPE_INT:
	{
		FTouchVar<int32_t> op;
		op.data = GetValueAsInt();
		engineInfo->setIOPInput(VarIdentifier, op);
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

		engineInfo->setDOPInput(VarIdentifier, op);
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
			engineInfo->setSOPInput(VarIdentifier, op);
		}
		else
		{
			FTouchVar<TETable*> op;
			op.data = TETableCreate();

			engineInfo->setSTOPInput(VarIdentifier, op);
			TERelease(&op.data);
		}
	}
	break;
	case EVarType::VARTYPE_TEXTURE:
		engineInfo->setTOPInput(VarIdentifier, GetValueAsTextureRenderTarget());
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
		FTouchVar<bool> op = engineInfo->getBOPOutput(VarIdentifier);
		SetValue(op.data);
	}
	break;
	case EVarType::VARTYPE_INT:
	{
		FTouchVar<int32_t> op = engineInfo->getIOPOutput(VarIdentifier);
		SetValue((int)op.data);
	}
	break;
	case EVarType::VARTYPE_DOUBLE:
	{
		FTouchVar<double> op = engineInfo->getDOPOutput(VarIdentifier);
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
			FTouchVar<TEString*> op = engineInfo->getSOPOutput(VarIdentifier);
			SetValue(FString(op.data->string));
		}
		else
		{
			FTouchVar<TETable*> op = engineInfo->getSTOPOutput(VarIdentifier);

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