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
	if (parent && parent->EngineInfo)
	{
		OnToxLoaded.Remove(DelegateHandle);
	}
}

void FTouchEngineDynamicVariableContainer::ToxLoaded()
{
	OnToxLoaded.Broadcast();
}

void FTouchEngineDynamicVariableContainer::ToxParametersLoaded(TArray<FTEDynamicVariable> variablesIn, TArray<FTEDynamicVariable> variablesOut)
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
	if (parent && parent->EngineInfo)
	{
		OnToxLoadFailed.Remove(Handle);
	}
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

FTEDynamicVariable* FTouchEngineDynamicVariableContainer::GetDynamicVariableByName(FString varName)
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



TArray<FString> FTEDynamicVariable::GetValueAsStringArray() const
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

TArray<float> FTEDynamicVariable::GetValueAsFloatBuffer() const
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

void FTEDynamicVariable::SetValue(UObject* newValue, size_t _size)
{
	if (newValue == nullptr)
	{
		value = nullptr;
		return;
	}

	value = (void*)newValue;
	size = _size;
}

void FTEDynamicVariable::SetValue(void* newValue, size_t _size)
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

void FTEDynamicVariable::SetValue(TArray<float> _value)
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

		isArray = true;
	}
}

void FTEDynamicVariable::SetValue(FString _value)
{
	if (VarType == EVarType::VARTYPE_STRING)
		SetValue((void*)TCHAR_TO_ANSI(*_value), _value.Len());
}

void FTEDynamicVariable::SetValue(TArray<FString> _value)
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
		buffer[i] = TCHAR_TO_ANSI(*_value[i]);
		size += _value[i].Len();
	}

	value = buffer;
	count = _value.Num();
	isArray = true;
}

void FTEDynamicVariable::SetValue(UTextureRenderTarget2D* _value)
{
	if (VarType == EVarType::VARTYPE_TEXTURE)
	{
		SetValue((UObject*)_value, sizeof(UTextureRenderTarget2D));

		textureProperty = _value;
	}
}

void FTEDynamicVariable::SetValue(UTexture2D* _value)
{
	if (VarType == EVarType::VARTYPE_TEXTURE)
	{
		SetValue((UObject*)_value, sizeof(UTexture2D));
	}
}



void FTEDynamicVariable::HandleChecked(ECheckBoxState InState)
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

FText FTEDynamicVariable::HandleTextBoxText() const
{
	if (value)
		return FText::FromString(GetValueAsString());
	else
		return FText();
}

void FTEDynamicVariable::HandleTextBoxTextChanged(const FText& NewText)
{
	SetValue(NewText.ToString());
}

void FTEDynamicVariable::HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo)
{
	SetValue(NewText.ToString());
}

void FTEDynamicVariable::HandleTextureChanged()
{
	SetValue(textureProperty);
}

void FTEDynamicVariable::HandleFloatBufferChanged()
{
	SetValue(floatBufferProperty);
}

void FTEDynamicVariable::HandleFloatBufferChildChanged()
{
	SetValue(floatBufferProperty);
}

bool FTEDynamicVariable::Serialize(FArchive& Ar)
{
	// write / read all normal variables
	Ar << VarName;
	Ar << VarIdentifier;
	Ar << VarType;
	Ar << count;
	Ar << size;
	Ar << isArray;
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

void FTEDynamicVariable::SendInput(UTouchEngineInfo* engineInfo)
{
	if (!value)
		return;

	switch (VarType)
	{
	case EVarType::VARTYPE_BOOL:
	{
		FTouchOP<bool> op;
		op.data = GetValueAsBool();
		engineInfo->setBOPInput(VarIdentifier, op);
	}
	break;
	case EVarType::VARTYPE_INT:
	{
		FTouchOP<int32_t> op;
		op.data = GetValueAsInt();
		engineInfo->setIOPInput(VarIdentifier, op);
	}
	break;
	case EVarType::VARTYPE_DOUBLE:
	{
		FTouchOP<TArray<double>> op;

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
			FTouchOP<char*> op;
			op.data = TCHAR_TO_ANSI(*GetValueAsString());
			engineInfo->setSOPInput(VarIdentifier, op);
		}
		else
		{
			FTouchOP<TETable*> op;
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

void FTEDynamicVariable::GetOutput(UTouchEngineInfo* engineInfo)
{
	switch (VarType)
	{
	case EVarType::VARTYPE_BOOL:
	{
		FTouchOP<bool> op = engineInfo->getBOPOutput(VarIdentifier);
		SetValue(op.data);
	}
	break;
	case EVarType::VARTYPE_INT:
	{
		FTouchOP<int32_t> op = engineInfo->getIOPOutput(VarIdentifier);
		SetValue((int)op.data);
	}
	break;
	case EVarType::VARTYPE_DOUBLE:
	{
		FTouchOP<double> op = engineInfo->getDOPOutput(VarIdentifier);
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
			FTouchOP<TEString*> op = engineInfo->getSOPOutput(VarIdentifier);
			SetValue(FString(op.data->string));
		}
		else
		{
			FTouchOP<TETable*> op = engineInfo->getSTOPOutput(VarIdentifier);

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