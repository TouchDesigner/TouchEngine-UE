// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

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

void FTouchEngineDynamicVariableContainer::ToxParametersLoaded(TArray<FTouchEngineDynamicVariableStruct> variablesIn, TArray<FTouchEngineDynamicVariableStruct> variablesOut)
{
	// if we have no data loaded 
	if ((DynVars_Input.Num() == 0 && DynVars_Output.Num() == 0))
	{
		DynVars_Input = variablesIn;
		DynVars_Output = variablesOut;
		return;
	}
	// if the already loaded data does not match the new loaded data formats
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
	// the data that we already have stored matches the data format of the output from the tox file, do nothing
	return;
}



void FTouchEngineDynamicVariableContainer::SetInputs(UTouchEngineInfo* engineInfo)
{
	for (int i = 0; i < DynVars_Input.Num(); i++)
	{
		DynVars_Input[i].SetInput(engineInfo);
	}
}

void FTouchEngineDynamicVariableContainer::GetOutputs(UTouchEngineInfo* engineInfo)
{
	for (int i = 0; i < DynVars_Output.Num(); i++)
	{
		DynVars_Output[i].GetOutput(engineInfo);
	}
}

void FTouchEngineDynamicVariableContainer::SetInput(UTouchEngineInfo* engineInfo, int index)
{
	if (index >= DynVars_Input.Num())
		return;

	DynVars_Input[index].SetInput(engineInfo);
}

void FTouchEngineDynamicVariableContainer::GetOutput(UTouchEngineInfo* engineInfo, int index)
{
	if (index >= DynVars_Output.Num())
		return;

	DynVars_Output[index].GetOutput(engineInfo);
}



TArray<float> FTouchEngineDynamicVariableStruct::GetValueAsFloatBuffer() const
{
	if (value == nullptr)
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

void FTouchEngineDynamicVariableStruct::SetValue(UObject* newValue, size_t _size)
{
	if (newValue == nullptr)
	{
		value = nullptr;
		return;
	}

	value = (void*)newValue;
	size = _size;
}

void FTouchEngineDynamicVariableStruct::SetValue(void* newValue, size_t _size)
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

void FTouchEngineDynamicVariableStruct::SetValue(TArray<float> _value)
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

		SetValue((void*)floatBuffer, sizeof(floatBuffer));
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(UTextureRenderTarget2D* _value)
{
	if (VarType == EVarType::VARTYPE_TEXTURE)
	{
		SetValue((UObject*)_value, sizeof(UTextureRenderTarget2D));

		textureProperty = _value;
	}
}

void FTouchEngineDynamicVariableStruct::SetValue(UTexture2D* _value)
{
	if (VarType == EVarType::VARTYPE_TEXTURE)
	{
		SetValue((UObject*)_value, sizeof(UTexture2D));
	}
}



void FTouchEngineDynamicVariableStruct::HandleChecked(ECheckBoxState InState)
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

FText FTouchEngineDynamicVariableStruct::HandleTextBoxText() const
{
	return FText::FromString(GetValueAsString());
}

void FTouchEngineDynamicVariableStruct::HandleTextBoxTextChanged(const FText& NewText)
{
	SetValue(NewText.ToString());
}

void FTouchEngineDynamicVariableStruct::HandleTextBoxTextCommited(const FText& NewText, ETextCommit::Type CommitInfo)
{
	SetValue(NewText.ToString());
}

void FTouchEngineDynamicVariableStruct::HandleTextureChanged()
{
	SetValue(textureProperty);
}

void FTouchEngineDynamicVariableStruct::HandleFloatBufferChanged()
{
	SetValue(floatBufferProperty);
}

void FTouchEngineDynamicVariableStruct::HandleFloatBufferChildChanged()
{
	SetValue(floatBufferProperty);
}

bool FTouchEngineDynamicVariableStruct::Serialize(FArchive& Ar)
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
			bool tempValue = GetValueAsBool();
			Ar << tempValue;
		}
		break;
		case EVarType::VARTYPE_INT:
		{
			int tempValue = GetValueAsInt();
			Ar << tempValue;
		}
		break;
		case EVarType::VARTYPE_DOUBLE:
		{
			double tempValue = GetValueAsDouble();
			Ar << tempValue;
		}
		break;
		case EVarType::VARTYPE_FLOAT:
		{
			float tempValue = GetValueAsFloat();
			Ar << tempValue;
		}
		break;
		case EVarType::VARTYPE_FLOATBUFFER:
		{
			TArray<float> tempValue = GetValueAsFloatBuffer();
			Ar << tempValue;
		}
		break;
		case EVarType::VARTYPE_STRING:
		{
			FString tempValue = GetValueAsString();
			Ar << tempValue;
		}
		break;
		case EVarType::VARTYPE_TEXTURE:
		{
			UTextureRenderTarget2D* tempValue = GetValueAsTextureRenderTarget();
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
			double tempValue;
			Ar << tempValue;
			SetValue(tempValue);
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
			FString tempValue;
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

void FTouchEngineDynamicVariableStruct::SetInput(UTouchEngineInfo* engineInfo)
{
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

void FTouchEngineDynamicVariableStruct::GetOutput(UTouchEngineInfo* engineInfo)
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
		FTouchCHOPSingleSample tcss = engineInfo->getCHOPOutputSingleSample(VarName);
		SetValue(tcss.channelData);
	}
	break;
	case EVarType::VARTYPE_STRING:
	{
		FTouchOP<TEString*> op = engineInfo->getSOPOutput(VarName);
		SetValue(FString(op.data->string));
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