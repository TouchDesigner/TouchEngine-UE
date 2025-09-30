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

#include "TouchEngineParserUtils.h"

#include "Logging.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "Engine/TEDebug.h"
#include "TouchEngine/TEFloatBuffer.h"
#include "TouchEngine/TouchObject.h"

TEResult FTouchEngineParserUtils::Parse(TEInstance* Instance, const char* Identifier, TArray<FTouchEngineDynamicVariableStruct>& VariableList, const FTouchEngineDynamicVariableStruct* Parent)
{
	TouchObject<TELinkInfo> Info;
	TEResult Result = TEInstanceLinkGetInfo(Instance, Identifier, Info.take());
	if (Result != TEResultSuccess)
	{
		return Result;
	}

	FTouchEngineDynamicVariableStruct& Variable = VariableList.AddDefaulted_GetRef();
	Variable.VarLabel = FString(UTF8_TO_TCHAR(Info->label));
	Variable.VarIdentifier = FString(UTF8_TO_TCHAR(Info->identifier));
	Variable.ParentIdentifier = Parent ? Parent->VarIdentifier : FString();
	Variable.VarType = GetVarType(Info);
	Variable.bIsArray = GetVarTypeIsArray(Info);
	Variable.VarIntent = GetVarIntent(Info);
	// It is not always possible to differentiate parameters from inputs just by the LinkInfo,
	// but the root ones will always be set as expected to we rely on the parent one
	Variable.VarScope = Parent ? Parent->VarScope : GetVarScope(Info);
	ensure(Variable.VarScope != EVarScope::NotSet);

	Variable.VarName = GetVarDomainChar(Variable.VarScope) + UTF8_TO_TCHAR(Info->name);
	Variable.Count = Info->count;

	// For Groups and Sequences, we are now processing their Children
	if (Variable.VarType == EVarType::Group || Variable.VarType == EVarType::Sequence)
	{
		return ParseChildren(Instance, Variable, VariableList);
	}

	if (Variable.VarScope != EVarScope::Output)
	{
		// For Inputs and Parameters, we get default values
		switch (Variable.VarType)
		{
		case EVarType::Bool:
			SetDefaultBoolValue(Instance, Variable);
			break;
		case EVarType::Int:
			SetDefaultIntValue(Instance, Variable);
			break;
		case EVarType::Double:
			SetDefaultDoubleValue(Instance, Variable);
			break;
		case EVarType::Float: // todo: double check floats
			break;
		case EVarType::CHOP:
			SetDefaultCHOPValue(Instance, Variable);
			break;
		case EVarType::String:
			SetDefaultStringValue(Instance, Variable);
			break;
		case EVarType::Texture:
			SetDefaultTextureValue(Instance, Variable);
			break;
		default:
			break;
		}
	}

	return TEResultSuccess;
}

EVarType FTouchEngineParserUtils::GetVarType(TELinkType Type)
{
	switch (Type)
	{
	case TELinkTypeGroup:
		return EVarType::Group;
	case TELinkTypeSequence:
		return EVarType::Sequence;
	case TELinkTypeBoolean:
		return EVarType::Bool;
	case TELinkTypeDouble:
		return EVarType::Double;
	case TELinkTypeInt:
		return EVarType::Int;
	case TELinkTypeString:
		return EVarType::String;
	case TELinkTypeTexture:
		return EVarType::Texture;
	case TELinkTypeFloatBuffer:
		return EVarType::CHOP;
	case TELinkTypeStringData:
		return EVarType::String;
	case TELinkTypeComplex:
	case TELinkTypeSeparator:
	default:
		return EVarType::NotSet;
	}
}

bool FTouchEngineParserUtils::GetVarTypeIsArray(TELinkType Type, int32_t Count)
{
	switch (Type)
	{
	case TELinkTypeFloatBuffer:
	case TELinkTypeStringData:
		return true;
	default:
		return Count > 1; //todo: as per previous parsing code, to be checked
	}
}

EVarIntent FTouchEngineParserUtils::GetVarIntent(TELinkIntent Intent)
{
	switch (Intent)
	{
	case TELinkIntentColorRGBA:
		return EVarIntent::Color; //todo: some older Structs saved their colors as 4 values, so we lost the information if we could actually send an alpha value or not
	case TELinkIntentPositionXYZW:
		return EVarIntent::Position;
	case TELinkIntentSizeWH:
		return EVarIntent::Size;
	case TELinkIntentUVW:
		return EVarIntent::UVW;
	case TELinkIntentFilePath:
		return EVarIntent::FilePath;
	case TELinkIntentDirectoryPath:
		return EVarIntent::DirectoryPath;
	case TELinkIntentMomentary:
		return EVarIntent::Momentary;
	case TELinkIntentPulse:
		return EVarIntent::Pulse;
	default:
		return EVarIntent::NotSet;
	}
}

EVarScope FTouchEngineParserUtils::GetVarScope(TELinkDomain Domain, TEScope Scope)
{
	switch (Domain)
	{
	case TELinkDomainParameterPage:
	case TELinkDomainParameter:
		return EVarScope::Parameter;
	default:
		break;
	}

	switch (Scope)
	{
	case TEScopeInput:
		return EVarScope::Input;
	case TEScopeOutput:
		return EVarScope::Output;
	}

	return EVarScope::NotSet;
}

FString FTouchEngineParserUtils::GetVarDomainChar(EVarScope Scope)
{
	switch (Scope)
	{
	case EVarScope::Input:
		return FString(TEXT("i/"));
	case EVarScope::Output:
		return FString(TEXT("o/"));
	case EVarScope::Parameter:
		return FString(TEXT("p/"));
	default:
		return FString();
	}
}

TEResult FTouchEngineParserUtils::ParseChildren(TEInstance* Instance, const FTouchEngineDynamicVariableStruct& Variable, TArray<FTouchEngineDynamicVariableStruct>& VariableList)
{
	check(Variable.VarType == EVarType::Group || Variable.VarType == EVarType::Sequence);
	const auto AnsiString = StringCast<ANSICHAR>(*Variable.VarIdentifier);
	const char* Identifier = AnsiString.Get();

	TArray<FTouchEngineDynamicVariableStruct> ChildrenVariables;

	TouchObject<TEStringArray> Children;
	TEResult Result = TEInstanceLinkGetChildren(Instance, Identifier, Children.take());

	if (Result == TEResultSuccess)
	{
		ensure(Children->count == Variable.Count);

		int NextChildIndex = 0;
		for (int32 i = 0; i < Children->count; i++)
		{
			Result = Parse(Instance, Children->strings[i], ChildrenVariables, &Variable);
			if (Result != TEResultSuccess)
			{
				return Result;
			}
			if (Variable.VarType == EVarType::Sequence && ChildrenVariables.IsValidIndex(NextChildIndex))
			{
				// Sequences are composed of multiple groups without names. If they do not have names, we give the index as name
				FTouchEngineDynamicVariableStruct& GroupVar = ChildrenVariables[NextChildIndex];
				if (ensure(GroupVar.VarType == EVarType::Group) && GroupVar.VarLabel.IsEmpty())
				{
					GroupVar.VarLabel = FString::Printf(TEXT("%d"), i);
				}
			}
			NextChildIndex = ChildrenVariables.Num();
		}
	}

	VariableList.Append(ChildrenVariables);

	return Result;
}


void FTouchEngineParserUtils::SetDefaultBoolValue(TEInstance* Instance, FTouchEngineDynamicVariableStruct& Variable)
{
	check(Variable.VarType == EVarType::Bool);
	check(Variable.VarScope == EVarScope::Input || Variable.VarScope == EVarScope::Parameter);
	const auto AnsiString = StringCast<ANSICHAR>(*Variable.VarIdentifier);
	const char* Identifier = AnsiString.Get();

	bool DefaultVal;
	const TEResult Result = TEInstanceLinkGetBooleanValue(Instance, Identifier, TELinkValueDefault, &DefaultVal);

	if (Result == TEResultSuccess)
	{
		Variable.SetValue(DefaultVal);
	}
	else
	{
		UE_LOG(LogTouchEngine, Warning, TEXT("ParseInfo: TEInstanceLinkGetBooleanValue for Identifier '%hs' was not successful:  %s"), Identifier, *TEResultToString(Result))
	}
}

void FTouchEngineParserUtils::SetDefaultDoubleValue(TEInstance* Instance, FTouchEngineDynamicVariableStruct& Variable)
{
	check(Variable.VarType == EVarType::Double);
	check(Variable.VarScope == EVarScope::Input || Variable.VarScope == EVarScope::Parameter);
	const auto AnsiString = StringCast<ANSICHAR>(*Variable.VarIdentifier);
	const char* Identifier = AnsiString.Get();

	if (!Variable.bIsArray)
	{
		Variable.ClampMin = GetTENumericValue<double>(Instance, Identifier, TELinkValueMinimum);
		Variable.ClampMax = GetTENumericValue<double>(Instance, Identifier, TELinkValueMaximum);
		Variable.UIMin = GetTENumericValue<double>(Instance, Identifier, TELinkValueUIMinimum);
		Variable.UIMax = GetTENumericValue<double>(Instance, Identifier, TELinkValueUIMaximum);

		double DefaultVal;
		const TEResult Result = TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueDefault, &DefaultVal, 1);
		if (Result == TEResultSuccess)
		{
			Variable.DefaultValue = DefaultVal;
			Variable.SetValue(DefaultVal);
		}
		else
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("ParseInfo: TEInstanceLinkGetDoubleValue for Identifier '%hs' was not successful:  %s"), Identifier, *TEResultToString(Result))
		}
	}
	else
	{
		Variable.ClampMin = GetTEOptionalNumericValues<double>(Instance, Identifier, TELinkValueMinimum, Variable.Count);
		Variable.ClampMax = GetTEOptionalNumericValues<double>(Instance, Identifier, TELinkValueMaximum, Variable.Count);
		Variable.UIMin = GetTEOptionalNumericValues<double>(Instance, Identifier, TELinkValueUIMinimum, Variable.Count);
		Variable.UIMax = GetTEOptionalNumericValues<double>(Instance, Identifier, TELinkValueUIMaximum, Variable.Count);

		TArray<double> DefaultValues;
		DefaultValues.AddUninitialized(Variable.Count);
		const TEResult Result = TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueDefault, DefaultValues.GetData(), Variable.Count);
		if (Result == TEResultSuccess)
		{
			Variable.DefaultValue = DefaultValues;
			Variable.SetValue(DefaultValues);
		}
		else
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("ParseInfo: TEInstanceLinkGetDoubleValue for Identifier '%hs' was not successful:  %s"), Identifier, *TEResultToString(Result))
		}
	}
}

void FTouchEngineParserUtils::SetDefaultIntValue(TEInstance* Instance, FTouchEngineDynamicVariableStruct& Variable)
{
	check(Variable.VarType == EVarType::Int);
	check(Variable.VarScope == EVarScope::Input || Variable.VarScope == EVarScope::Parameter);
	const auto AnsiString = StringCast<ANSICHAR>(*Variable.VarIdentifier);
	const char* Identifier = AnsiString.Get();

	if (!Variable.bIsArray)
	{
		// And Int dropdown down not have valid return values from TEInstanceLinkGetChoiceValues
		TouchObject<TEStringArray> ChoiceLabels;
		TEResult Result = TEInstanceLinkGetChoices(Instance, Identifier, ChoiceLabels.take(), nullptr);

		if (ChoiceLabels && ChoiceLabels->count > 0)
		{
			Variable.VarIntent = EVarIntent::DropDown;
			for (int i = 0; i < ChoiceLabels->count; i++)
			{
				Variable.DropDownData.Add({i, ChoiceLabels->strings[i], ChoiceLabels->strings[i]});
			}
		}

		Variable.ClampMin = GetTENumericValue<int>(Instance, Identifier, TELinkValueMinimum);
		Variable.ClampMax = GetTENumericValue<int>(Instance, Identifier, TELinkValueMaximum);
		Variable.UIMin = GetTENumericValue<int>(Instance, Identifier, TELinkValueUIMinimum);
		Variable.UIMax = GetTENumericValue<int>(Instance, Identifier, TELinkValueUIMaximum);

		int DefaultVal;
		Result = TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueDefault, &DefaultVal, 1);
		if (Result == TEResultSuccess)
		{
			Variable.DefaultValue = DefaultVal;
			Variable.SetValue(DefaultVal);
		}
		else
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("ParseInfo: TEInstanceLinkGetIntValue for Identifier '%hs' was not successful:  %s"), Identifier, *TEResultToString(Result))
		}
	}
	else
	{
		Variable.ClampMin = GetTEOptionalNumericValues<int>(Instance, Identifier, TELinkValueMinimum, Variable.Count);
		Variable.ClampMax = GetTEOptionalNumericValues<int>(Instance, Identifier, TELinkValueMaximum, Variable.Count);
		Variable.UIMin = GetTEOptionalNumericValues<int>(Instance, Identifier, TELinkValueUIMinimum, Variable.Count);
		Variable.UIMax = GetTEOptionalNumericValues<int>(Instance, Identifier, TELinkValueUIMaximum, Variable.Count);

		TArray<int> DefaultValues;
		DefaultValues.AddUninitialized(Variable.Count);
		TEResult Result = TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueDefault, DefaultValues.GetData(), Variable.Count);
		if (Result == TEResultSuccess)
		{
			Variable.DefaultValue = DefaultValues;
			Variable.SetValue(DefaultValues);
		}
		else
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("ParseInfo: TEInstanceLinkGetIntValue for Identifier '%hs' was not successful:  %s"), Identifier, *TEResultToString(Result))
		}
	}
}

void FTouchEngineParserUtils::SetDefaultStringValue(TEInstance* Instance, FTouchEngineDynamicVariableStruct& Variable)
{
	check(Variable.VarType == EVarType::String);
	check(Variable.VarScope == EVarScope::Input || Variable.VarScope == EVarScope::Parameter);
	const auto AnsiString = StringCast<ANSICHAR>(*Variable.VarIdentifier);
	const char* Identifier = AnsiString.Get();

	if (!Variable.bIsArray)
	{
		TouchObject<TEStringArray> ChoiceValues;
		TouchObject<TEStringArray> ChoiceLabels;
		TEResult Result = TEInstanceLinkGetChoices(Instance, Identifier, ChoiceLabels.take(), ChoiceValues.take());

		if (ChoiceValues && ensure(ChoiceLabels && ChoiceLabels->count == ChoiceValues->count))
		{
			Variable.VarIntent = EVarIntent::DropDown;
			for (int i = 0; i < ChoiceValues->count; i++)
			{
				Variable.DropDownData.Add({i, ChoiceValues->strings[i], ChoiceLabels->strings[i]});
			}
		}

		TouchObject<TEString> DefaultVal;
		Result = TEInstanceLinkGetStringValue(Instance, Identifier, TELinkValueDefault, DefaultVal.take());
		if (Result == TEResultSuccess)
		{
			FString DefaultStr{UTF8_TO_TCHAR(DefaultVal->string)};
			Variable.DefaultValue = DefaultStr;
			Variable.SetValue(DefaultStr);
		}
		else if (Result != TEResultNoMatchingEntity)
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("ParseInfo: TEInstanceLinkGetStringValue for Identifier '%hs' was not successful:  %s"), Identifier, *TEResultToString(Result))
		}
	}
	else
	{
		TouchObject<TEString> DefaultVal;
		TEResult Result = TEInstanceLinkGetStringValue(Instance, Identifier, TELinkValueDefault, DefaultVal.take());

		if (Result == TEResult::TEResultSuccess)
		{
			TArray<FString> Values;
			for (int32 i = 0; i < Variable.Count; i++)
			{
				Values.Add(FString(UTF8_TO_TCHAR(DefaultVal[i].string)));
			}

			Variable.SetValue(Values);
		}
		else if (Result != TEResultNoMatchingEntity)
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("ParseInfo: TEInstanceLinkGetStringValue for Identifier '%hs' was not successful:  %s"), Identifier, *TEResultToString(Result))
		}
	}
}

void FTouchEngineParserUtils::SetDefaultTextureValue(TEInstance* Instance, FTouchEngineDynamicVariableStruct& Variable)
{
	check(Variable.VarType == EVarType::Texture);
	check(Variable.VarScope == EVarScope::Input || Variable.VarScope == EVarScope::Parameter);
	const auto AnsiString = StringCast<ANSICHAR>(*Variable.VarIdentifier);
	const char* Identifier = AnsiString.Get();

	if (Variable.VarScope == EVarScope::Input)
	{
		TEInstanceLinkSetInterest(Instance, Identifier, TELinkInterestNoValues);
	}

	// textures have no valid default values
	Variable.SetValue(static_cast<UTexture*>(nullptr));
}

void FTouchEngineParserUtils::SetDefaultCHOPValue(TEInstance* Instance, FTouchEngineDynamicVariableStruct& Variable)
{
	check(Variable.VarType == EVarType::CHOP);
	check(Variable.VarScope == EVarScope::Input || Variable.VarScope == EVarScope::Parameter);
	const auto AnsiString = StringCast<ANSICHAR>(*Variable.VarIdentifier);
	const char* Identifier = AnsiString.Get();

	TouchObject<TEFloatBuffer> Buf;
	const TEResult LinkResult = TEInstanceLinkGetFloatBufferValue(Instance, Identifier, TELinkValueDefault, Buf.take());

	if (LinkResult == TEResultSuccess) // this should always be unsuccessful as there are no default values for Float Buffers
	{
		TArray<float> Values;
		const int32 MaxChannels = TEFloatBufferGetChannelCount(Buf);
		Values.Reserve(MaxChannels);
		const float* const* Channels = TEFloatBufferGetValues(Buf);

		for (int32 i = 0; i < MaxChannels; i++)
		{
			Values.Add(*Channels[i]);
		}

		Variable.SetValue(Values);
	}
}
