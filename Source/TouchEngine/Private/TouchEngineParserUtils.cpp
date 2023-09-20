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
#include "TouchEngineDynamicVariableStruct.h"
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
				double DefaultVal, MinVal, MaxVal;
				if (TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueMinimum, &MinVal, 1) == TEResultSuccess)
				{
					if (MinVal > std::numeric_limits<double>::lowest()) // if the value really has a lower bound
					{
						Variable.MinValue = MinVal;
					}
				}
				if (TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueMaximum, &MaxVal, 1) == TEResultSuccess)
				{
					if (MaxVal < std::numeric_limits<double>::max()) // if the value really has a higher bound
					{
						Variable.MaxValue = MaxVal;
					}
				}
				Result = TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueDefault, &DefaultVal, 1);
				if (Result == TEResultSuccess)
				{
					Variable.DefaultValue = DefaultVal;
					Variable.SetValue(DefaultVal);
				}
			}
			else
			{
				TArray<double> DefaultValues, MinValues, MaxValues;
				DefaultValues.AddUninitialized(Info->count);
				MinValues.AddUninitialized(Info->count);
				MaxValues.AddUninitialized(Info->count);
				
				if (TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueMinimum, MinValues.GetData(), Info->count) == TEResultSuccess)
				{
					for (double& MinVal : MinValues)
					{
						if (MinVal > std::numeric_limits<double>::lowest()) // if at least one value really have a lower bound
						{
							Variable.MinValue = MinValues;
							break;
						}
					}
				}
				if (TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueMaximum, MaxValues.GetData(), Info->count) == TEResultSuccess)
				{
					for (double& MaxVal : MaxValues)
					{
						if (MaxVal < std::numeric_limits<double>::max()) // if at least one value really have a higher bound
						{
							Variable.MaxValue = MaxValues;
							break;
						}
					}
				}
				Result = TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueDefault, DefaultValues.GetData(), Info->count);
				if (Result == TEResultSuccess)
				{
					Variable.DefaultValue = DefaultValues;
					Variable.SetValue(DefaultValues);
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
				
				int32 DefaultVal, MinVal, MaxVal;
				if (TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueMinimum, &MinVal, 1) == TEResultSuccess)
				{
					if (MinVal > std::numeric_limits<int32>::lowest()) // if the value really has a lower bound
					{
						Variable.MinValue = MinVal;
					}
				}
				if (TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueMaximum, &MaxVal, 1) == TEResultSuccess)
				{
					if (MaxVal < std::numeric_limits<int32>::max()) // if the value really has a higher bound
					{
						Variable.MaxValue = MaxVal;
					}
				}
				Result = TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueDefault, &DefaultVal, 1);
				if (Result == TEResultSuccess)
				{
					Variable.DefaultValue = DefaultVal;
					Variable.SetValue(DefaultVal);
				}
			}
			else
			{
				TArray<int32> DefaultValues, MinValues, MaxValues;
				DefaultValues.AddUninitialized(Info->count);
				MinValues.AddUninitialized(Info->count);
				MaxValues.AddUninitialized(Info->count);
				
				if (TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueMinimum, MinValues.GetData(), Info->count) == TEResultSuccess)
				{
					for (int32& MinVal : MinValues)
					{
						if (MinVal > std::numeric_limits<int32>::lowest()) // if at least one value really have a lower bound
						{
							Variable.MinValue = MinValues;
							break;
						}
					}
				}
				if (TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueMaximum, MaxValues.GetData(), Info->count) == TEResultSuccess)
				{
					for (int32& MaxVal : MaxValues)
					{
						if (MaxVal < std::numeric_limits<int32>::max()) // if at least one value really have a higher bound
						{
							Variable.MaxValue = MaxValues;
							break;
						}
					}
				}
				Result = TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueDefault, DefaultValues.GetData(), Info->count);
				if (Result == TEResultSuccess)
				{
					Variable.DefaultValue = DefaultValues;
					Variable.SetValue(DefaultValues);
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