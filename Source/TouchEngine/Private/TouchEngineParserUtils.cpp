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

TEResult FTouchEngineParserUtils::ParseGroup(TEInstance* Instance, const char* Identifier, TArray<FTouchEngineDynamicVariableStruct>& VariableList)
{
	// load each group
	TELinkInfo* Group;
	TEResult Result = TEInstanceLinkGetInfo(Instance, Identifier, &Group);

	if (Result != TEResultSuccess)
	{
		// failure
		return Result;
	}

	// use group data here

	TERelease(&Group);

	// load children of each group
	TEStringArray* Children = nullptr;
	Result = TEInstanceLinkGetChildren(Instance, Identifier, &Children);

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

	TERelease(&Children);
	return Result;
}

TEResult FTouchEngineParserUtils::ParseInfo(TEInstance* Instance, const char* Identifier, TArray<FTouchEngineDynamicVariableStruct>& VariableList)
{
	TELinkInfo* Info;
	TEResult Result = TEInstanceLinkGetInfo(Instance, Identifier, &Info);

	if (Result != TEResultSuccess)
	{
		//failure
		return Result;
	}

	// parse our children into a dynamic Variable struct
	FTouchEngineDynamicVariableStruct Variable;

	Variable.VarLabel = FString(Info->label);

	FString DomainChar = "";

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
		Variable.bIsArray = true;

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
				double DefaultVal;
				Result = TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueDefault, &DefaultVal, 1);

				if (Result == TEResult::TEResultSuccess)
				{
					Variable.SetValue(DefaultVal);
				}
			}
			else
			{
				double* DefaultVal = static_cast<double*>(_alloca(sizeof(double) * Info->count));
				Result = TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueDefault, DefaultVal, Info->count);

				if (Result == TEResult::TEResultSuccess)
				{
					TArray<double> Buffer;

					for (int32 i = 0; i < Info->count; i++)
					{
						Buffer.Add(DefaultVal[i]);
					}

					Variable.SetValue(Buffer);
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
				TEStringArray* ChoiceLabels = nullptr;
				Result = TEInstanceLinkGetChoiceLabels(Instance, Info->identifier, &ChoiceLabels);

				if (ChoiceLabels)
				{
					Variable.VarIntent = EVarIntent::DropDown;

#if WITH_EDITORONLY_DATA
					for (int32 i = 0; i < ChoiceLabels->count; i++)
					{
						Variable.DropDownData.Add(ChoiceLabels->strings[i], i);
					}
#endif

					TERelease(&ChoiceLabels);
				}

				TTouchVar<int32> c;
				Result = TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueDefault, &c.Data, 1);

				if (Result == TEResult::TEResultSuccess)
				{
					Variable.SetValue(c.Data);
				}
			}
			else
			{
				TTouchVar<int32*> c;
				c.Data = static_cast<int32*>(_alloca(sizeof(int32) * 4));

				Result = TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueDefault, c.Data, Info->count);

				if (Result == TEResult::TEResultSuccess)
				{
					TArray<int32> Values;

					for (int32 i = 0; i < Info->count; i++)
					{
						Values.Add(c.Data[i]);
					}

					Variable.SetValue(Values);
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
				TEStringArray* ChoiceLabels = nullptr;
				Result = TEInstanceLinkGetChoiceLabels(Instance, Info->identifier, &ChoiceLabels);

				if (ChoiceLabels)
				{
					Variable.VarIntent = EVarIntent::DropDown;

#if WITH_EDITORONLY_DATA
					for (int i = 0; i < ChoiceLabels->count; i++)
					{
						Variable.DropDownData.Add(ChoiceLabels->strings[i], i);
					}
#endif

					TERelease(&ChoiceLabels);
				}


				TEString* DefaultVal = nullptr;
				Result = TEInstanceLinkGetStringValue(Instance, Identifier, TELinkValueDefault, &DefaultVal);

				if (Result == TEResult::TEResultSuccess)
				{
					Variable.SetValue(FString(UTF8_TO_TCHAR(DefaultVal->string)));
				}
				TERelease(&DefaultVal);
			}
			else
			{
				TEString* DefaultVal = nullptr;
				Result = TEInstanceLinkGetStringValue(Instance, Identifier, TELinkValueDefault, &DefaultVal);

				if (Result == TEResult::TEResultSuccess)
				{
					TArray<FString> Values;
					for (int32 i = 0; i < Info->count; i++)
					{
						Values.Add(FString(UTF8_TO_TCHAR(DefaultVal[i].string)));
					}

					Variable.SetValue(Values);
				}
				TERelease(&DefaultVal);
			}
		}
		break;
	}
	case TELinkTypeTexture:
	{
		Variable.VarType = EVarType::Texture;

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
			TEFloatBuffer* Buf = nullptr;
			Result = TEInstanceLinkGetFloatBufferValue(Instance, Identifier, TELinkValueDefault, &Buf);

			if (Result == TEResult::TEResultSuccess)
			{
				TArray<float> Values;
				const int32 MaxChannels = TEFloatBufferGetChannelCount(Buf);
				const float* const* Channels = TEFloatBufferGetValues(Buf);

				for (int32 i = 0; i < MaxChannels; i++)
				{
					Values.Add(*Channels[i]);
				}

				Variable.SetValue(Values);
			}
			TERelease(&Buf);
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
		Variable.VarIntent = EVarIntent::Color;
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
	}

	VariableList.Add(Variable);

	TERelease(&Info);
	return Result;
}