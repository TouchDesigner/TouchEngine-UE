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

TEResult FTouchEngineParserUtils::ParseGroup(TEInstance* Instance, const char* Identifier, TArray<FTouchEngineDynamicVariableStruct>& VariableList)
{
	// load each group
	TouchObject<TELinkInfo> Group;
	TEResult Result = TEInstanceLinkGetInfo(Instance, Identifier, Group.take());

	if (Result != TEResultSuccess)
	{
		// failure
		return Result;
	}

	// use group data here

	// load children of each group
	TouchObject<TEStringArray> Children;
	Result = TEInstanceLinkGetChildren(Instance, Identifier, Children.take());

	if (Result != TEResultSuccess)
	{
		//failure
		return Result;
	}

	// use children data here
	for (int32 i = 0; i < Children->count; i++)
	{
		Result = ParseInfo(Instance, Children->strings[i], VariableList);
	}

	return Result;
}

TEResult FTouchEngineParserUtils::ParseInfo(TEInstance* Instance, const char* Identifier, TArray<FTouchEngineDynamicVariableStruct>& VariableList)
{
	TouchObject<TELinkInfo> Info;
	TEResult Result = TEInstanceLinkGetInfo(Instance, Identifier, Info.take());

	if (Result != TEResultSuccess)
	{
		return Result;
	}

	// parse our children into a dynamic Variable struct
	FTouchEngineDynamicVariableStruct Variable;

	Variable.VarLabel = FString(Info->label);

	FString DomainChar = "";
	bool bIsInput = false;

	switch (Info->domain)
	{
	case TELinkDomainNone:
	case TELinkDomainParameterPage:
		break;
	case TELinkDomainParameter:
		{
			DomainChar = "p";
			break;
		}
	case TELinkDomainOperator:
		{
			switch (Info->scope)
			{
			case TEScopeInput:
				{
					DomainChar = "i";
					bIsInput = true;
					break;
				}
			case TEScopeOutput:
				{
					DomainChar = "o";
					break;
				}
			}
			break;
		}
	}

	Variable.VarName = DomainChar.Append("/").Append(UTF8_TO_TCHAR(Info->name));
	Variable.VarIdentifier = FString(UTF8_TO_TCHAR(Info->identifier));
	Variable.Count = Info->count;
	if (Variable.Count > 1)
	{
		Variable.bIsArray = true;
	}

	// figure out what type
	switch (Info->type)
	{
	case TELinkTypeGroup:
	{
		Result = ParseGroup(Instance, Identifier, VariableList);
		break;
	}
	case TELinkTypeComplex:
	{
		Variable.VarType = EVarType::NotSet;
		break;
	}
	case TELinkTypeBoolean:
	{
		Variable.VarType = EVarType::Bool;
		if (Info->domain == TELinkDomainParameter || (Info->domain == TELinkDomainOperator && Info->scope == TEScopeInput))
		{
			bool DefaultVal;
			Result = TEInstanceLinkGetBooleanValue(Instance, Identifier, TELinkValueDefault, &DefaultVal);

			if (Result == TEResult::TEResultSuccess)
			{
				Variable.SetValue(DefaultVal);
			}
			else
			{
				UE_LOG(LogTouchEngine, Warning, TEXT("ParseInfo: TEInstanceLinkGetBooleanValue for Identifier '%hs' was not successful:  %s"), Identifier, *TEResultToString(Result))
			}
		}
		break;
	}
	case TELinkTypeDouble:
	{
		Variable.VarType = EVarType::Double;

		if (Info->domain == TELinkDomainParameter || (Info->domain == TELinkDomainOperator && Info->scope == TEScopeInput))
		{
			if (Info->count == 1)
			{
				Variable.ClampMin = GetTENumericValue<double>(Instance, Identifier, TELinkValueMinimum);
				Variable.ClampMax = GetTENumericValue<double>(Instance, Identifier, TELinkValueMaximum);
				Variable.UIMin = GetTENumericValue<double>(Instance, Identifier, TELinkValueUIMinimum);
				Variable.UIMax = GetTENumericValue<double>(Instance, Identifier, TELinkValueUIMaximum);

				double DefaultVal;
				Result = TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueDefault, &DefaultVal, 1);
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
				Variable.ClampMin = GetTEOptionalNumericValues<double>(Instance, Identifier, TELinkValueMinimum, Info->count);
				Variable.ClampMax = GetTEOptionalNumericValues<double>(Instance, Identifier, TELinkValueMaximum, Info->count);
				Variable.UIMin = GetTEOptionalNumericValues<double>(Instance, Identifier, TELinkValueUIMinimum, Info->count);
				Variable.UIMax = GetTEOptionalNumericValues<double>(Instance, Identifier, TELinkValueUIMaximum, Info->count);
				
				TArray<double> DefaultValues;
				DefaultValues.AddUninitialized(Info->count);
				Result = TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueDefault, DefaultValues.GetData(), Info->count);
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
		break;
	}
	case TELinkTypeInt:
	{
		Variable.VarType = EVarType::Int;

		if (Info->domain == TELinkDomainParameter || (Info->domain == TELinkDomainOperator && Info->scope == TEScopeInput))
		{
			if (Info->count == 1)
			{
				// And Int dropdown down not have valid return values from TEInstanceLinkGetChoiceValues
				TouchObject<TEStringArray> ChoiceLabels;
				Result = TEInstanceLinkGetChoiceLabels(Instance, Info->identifier, ChoiceLabels.take());

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
				Variable.ClampMin = GetTEOptionalNumericValues<int>(Instance, Identifier, TELinkValueMinimum, Info->count);
				Variable.ClampMax = GetTEOptionalNumericValues<int>(Instance, Identifier, TELinkValueMaximum, Info->count);
				Variable.UIMin = GetTEOptionalNumericValues<int>(Instance, Identifier, TELinkValueUIMinimum, Info->count);
				Variable.UIMax = GetTEOptionalNumericValues<int>(Instance, Identifier, TELinkValueUIMaximum, Info->count);

				TArray<int> DefaultValues;
				DefaultValues.AddUninitialized(Info->count);
				Result = TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueDefault, DefaultValues.GetData(), Info->count);
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
		break;
	}
	case TELinkTypeString:
	{
		Variable.VarType = EVarType::String;

		if (Info->domain == TELinkDomainParameter || (Info->domain == TELinkDomainOperator && Info->scope == TEScopeInput))
		{
			if (Info->count == 1)
			{
				TouchObject<TEStringArray> ChoiceValues;
				Result = TEInstanceLinkGetChoiceValues(Instance, Info->identifier, ChoiceValues.take());
				TouchObject<TEStringArray> ChoiceLabels;
				Result = TEInstanceLinkGetChoiceLabels(Instance, Info->identifier, ChoiceLabels.take());

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
				if (Result == TEResult::TEResultSuccess)
				{
					FString DefaultStr{UTF8_TO_TCHAR(DefaultVal->string)};
					Variable.DefaultValue = DefaultStr;
					Variable.SetValue(DefaultStr);
				}
				else
				{
					UE_LOG(LogTouchEngine, Warning, TEXT("ParseInfo: TEInstanceLinkGetStringValue for Identifier '%hs' was not successful:  %s"), Identifier, *TEResultToString(Result))
				}
			}
			else
			{
				TouchObject<TEString> DefaultVal;
				Result = TEInstanceLinkGetStringValue(Instance, Identifier, TELinkValueDefault, DefaultVal.take());

				if (Result == TEResult::TEResultSuccess)
				{
					TArray<FString> Values;
					for (int32 i = 0; i < Info->count; i++)
					{
						Values.Add(FString(UTF8_TO_TCHAR(DefaultVal[i].string)));
					}

					Variable.SetValue(Values);
				}
				else
				{
					UE_LOG(LogTouchEngine, Warning, TEXT("ParseInfo: TEInstanceLinkGetStringValue for Identifier '%hs' was not successful:  %s"), Identifier, *TEResultToString(Result))
				}
			}
		}
		break;
	}
	case TELinkTypeTexture:
	{
		Variable.VarType = EVarType::Texture;
		if (bIsInput)
		{
			TEInstanceLinkSetInterest(Instance, Identifier, TELinkInterestNoValues);
		}
		// textures have no valid default values
		Variable.SetValue(static_cast<UTexture*>(nullptr));
		break;
	}
	case TELinkTypeFloatBuffer:
	{
		Variable.VarType = EVarType::CHOP;
		Variable.bIsArray = true;

		if (Info->domain == TELinkDomainParameter || (Info->domain == TELinkDomainOperator && Info->scope == TEScopeInput))
		{
			TouchObject<TEFloatBuffer> Buf;
			Result = TEInstanceLinkGetFloatBufferValue(Instance, Identifier, TELinkValueDefault, Buf.take());

			if (Result == TEResult::TEResultSuccess)
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
			else
			{
				UE_LOG(LogTouchEngine, Warning, TEXT("ParseInfo: TEInstanceLinkGetFloatBufferValue for Identifier '%hs' was not successful:  %s"), Identifier, *TEResultToString(Result))
			}
		}
		break;
	}
	case TELinkTypeStringData:
		{
			Variable.VarType = EVarType::String;
			Variable.bIsArray = true;

			if (Info->domain == TELinkDomainParameter || (Info->domain == TELinkDomainOperator && Info->scope == TEScopeInput))
			{
			}
			break;
		}
	case TELinkTypeSeparator:
		{
			Variable.VarType = EVarType::NotSet;
			return Result;
		}
	}

	switch (Info->intent)
	{
	case TELinkIntentColorRGBA:
		{
			Variable.VarIntent = EVarIntent::Color; //todo: some older Structs saved their colors as 4 values, so we lost the information if we could actually send an alpha value or not
			break;
		}
	case TELinkIntentPositionXYZW:
		{
			Variable.VarIntent = EVarIntent::Position;
			break;
		}
	case TELinkIntentSizeWH:
		{
			Variable.VarIntent = EVarIntent::Size;
			break;
		}
	case TELinkIntentUVW:
		{
			Variable.VarIntent = EVarIntent::UVW;
			break;
		}
	case TELinkIntentFilePath:
		{
			Variable.VarIntent = EVarIntent::FilePath;
			break;
		}
	case TELinkIntentDirectoryPath:
		{
			Variable.VarIntent = EVarIntent::DirectoryPath;
			break;
		}
	case TELinkIntentMomentary:
		{
			Variable.VarIntent = EVarIntent::Momentary;
			break;
		}
	case TELinkIntentPulse:
		{
			Variable.VarIntent = EVarIntent::Pulse;
			break;
		}
	default:
		break;
	}

	VariableList.Add(Variable);

	return Result;
}