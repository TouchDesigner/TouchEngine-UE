// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "TETable.h"

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
	if (parent && parent->EngineInfo)
	{
		OnToxLoaded.Remove(DelegateHandle);
	}
}

void FTouchEngineDynamicVariableContainer::ToxLoaded()
{
	//parent->EngineInfo.














	OnToxLoaded.Broadcast();
}

void FTouchEngineDynamicVariableContainer::ToxParametersLoaded(TArray<FTouchEngineDynamicVariable> variablesIn, TArray<FTouchEngineDynamicVariable> variablesOut)
{
	// if we have no data loaded 
	if ((DynVars_Input.Num() == 0 && DynVars_Output.Num() == 0))
	{
		DynVars_Input = variablesIn;
		DynVars_Output = variablesOut;
		return;
	}
	// if the already loaded count matches the new loaded counts
	if (DynVars_Input.Num() == variablesIn.Num() && DynVars_Output.Num() == variablesOut.Num())
	{
		bool varsMatched = true;

		for (int i = 0; i < DynVars_Input.Num(); i++)
		{
			if (DynVars_Input[i].VarType == variablesIn[i].VarType)
			{
				// variable type matches, continue
				continue;
			}
			else
			{
				// variable type does not match - we're loading a different set of parameters
				varsMatched = false;
			}
		}

		for (int i = 0; i < DynVars_Output.Num(); i++)
		{
			if (DynVars_Output[i].VarType == variablesOut[i].VarType)
			{
				// variable type matches, continue
				continue;
			}
			else
			{
				// variable type does not match - we're loading a different set of parameters
				varsMatched = false;
			}
		}

		if (!varsMatched)
		{
			DynVars_Input = variablesIn;
			DynVars_Output = variablesOut;
			return;
		}

	}
	// counts are different
	// TODO: add parsing for different variables that have the same name as variables, keep data
	else
	{
		DynVars_Input = variablesIn;
		DynVars_Output = variablesOut;
		return;
	}
	// the data that we already have stored matches the data format of the output from the tox file, do nothing
	return;
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
	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		if (DynVars_Input[i].VarName == varName)
			return &DynVars_Input[i];
	}

	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		if (DynVars_Output[i].VarName == varName)
			return &DynVars_Output[i];
	}
	return nullptr;
}



TArray<FString> FTouchEngineDynamicVariable::GetValueAsStringArray() const
{
	TArray<FString> _value;

	if (!value)
		return _value;

	char** buffer = (char**)value;

	for (int i = 0; i < count; i++)
	{
		_value.Add(buffer[i]);
	}
	return _value;
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

	if (VarType == EVarType::VARTYPE_FLOATBUFFER)
	{
		float* floatBuffer = new float[_value.Num()];

		for (int i = 0; i < _value.Num(); i++)
		{
			floatBuffer[i] = _value[i];
		}

		SetValue((void*)floatBuffer, sizeof(float) * _value.Num());
	}
}

void FTouchEngineDynamicVariable::SetValue(FString _value)
{
	if (VarType == EVarType::VARTYPE_STRING)
		SetValue((void*)TCHAR_TO_ANSI(*_value), _value.Len());
}

void FTouchEngineDynamicVariable::SetValue(TArray<FString> _value)
{
	char** buffer = new char* [_value.Num()];
	size = 0;

	for (int i = 0; i < _value.Num(); i++)
	{
		buffer[i] = TCHAR_TO_ANSI(*_value[i]);
		size += _value[i].Len();
	}

	value = buffer;
	count = _value.Num();
}

void FTouchEngineDynamicVariable::SetValue(UTextureRenderTarget2D* _value)
{
	if (VarType == EVarType::VARTYPE_TEXTURE)
	{
		SetValue((UObject*)_value, sizeof(UTextureRenderTarget2D));

		textureProperty = _value;
	}
}

void FTouchEngineDynamicVariable::SetValue(UTexture2D* _value)
{
	if (VarType == EVarType::VARTYPE_TEXTURE)
	{
		SetValue((UObject*)_value, sizeof(UTexture2D));
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
	SetValue(textureProperty);
}

void FTouchEngineDynamicVariable::HandleFloatBufferChanged()
{
	SetValue(floatBufferProperty);
}

void FTouchEngineDynamicVariable::HandleFloatBufferChildChanged()
{
	SetValue(floatBufferProperty);
}

bool FTouchEngineDynamicVariable::Serialize(FArchive& Ar)
{
	// write / read all normal variables
	Ar << VarName;
	Ar << VarType;
	Ar << count;
	Ar << size;
	// write editor variables just in case they need to be used
	Ar << textureProperty;
	Ar << floatBufferProperty;

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
				for (int i = 0; i < count; i++)\
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
			FString tempValue = FString("");
			if (value)
				tempValue = GetValueAsString();
			Ar << tempValue;
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
			FString tempValue = FString("");
			Ar << tempValue;
			SetValue(tempValue);
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
		FTouchOP<bool> op;
		op.data = GetValueAsBool();
		engineInfo->setBOPInput(VarName, op);
	}
	break;
	case EVarType::VARTYPE_INT:
	{
		FTouchOP<int32_t> op;
		op.data = GetValueAsInt();
		engineInfo->setIOPInput(VarName, op);
	}
	break;
	case EVarType::VARTYPE_DOUBLE:
	{
		FTouchOP<double> op;
		op.data = GetValueAsDouble();
		engineInfo->setDOPInput(VarName, op);
	}
	break;
	case EVarType::VARTYPE_FLOAT:
	{
		FTouchCHOPSingleSample tcss;
		tcss.channelData.Add(GetValueAsFloat());
		engineInfo->setCHOPInputSingleSample(VarName, tcss);
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
		engineInfo->setCHOPInputSingleSample(VarName, tcss);
	}
	break;
	case EVarType::VARTYPE_STRING:
	{
		FTouchOP<char*> op;
		op.data = TCHAR_TO_ANSI(*GetValueAsString());
		engineInfo->setSOPInput(VarName, op);
	}
	break;
	case EVarType::VARTYPE_TEXTURE:
		engineInfo->setTOPInput(VarName, GetValueAsTextureRenderTarget());
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
		FTouchOP<bool> op = engineInfo->getBOPOutput(VarName);
		SetValue(op.data);
	}
	break;
	case EVarType::VARTYPE_INT:
	{
		FTouchOP<int32_t> op = engineInfo->getIOPOutput(VarName);
		SetValue((int)op.data);
	}
	break;
	case EVarType::VARTYPE_DOUBLE:
	{
		FTouchOP<double> op = engineInfo->getDOPOutput(VarName);
		SetValue(op.data);
	}
	break;
	case EVarType::VARTYPE_FLOAT:
	{
		FTouchCHOPSingleSample tcss = engineInfo->getCHOPOutputSingleSample(VarName);
		SetValue(tcss.channelData[0]);
	}
	break;
	case EVarType::VARTYPE_FLOATBUFFER:
	{
		//FTouchCHOPSingleSample tcss = engineInfo->getCHOPOutputSingleSample(VarName);
		SetValue(engineInfo->getCHOPOutputSingleSample(VarName).channelData);
	}
	break;
	case EVarType::VARTYPE_STRING:
	{
		if (count == 1)
		{
			FTouchOP<TEString*> op = engineInfo->getSOPOutput(VarName);
			SetValue(FString(op.data->string));
		}
		else
		{
			FTouchOP<TETable*> op = engineInfo->getSTOPOutput(VarName);
			
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
		FTouchTOP top = engineInfo->getTOPOutput(VarName);
		SetValue(top.texture);
	}
	break;
	default:
		// unimplemented type
		break;
	}
}