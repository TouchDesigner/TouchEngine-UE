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

#pragma once

#include "CoreMinimal.h"
#include "Misc/Variant.h"
#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TEResult.h"
#include "TouchEngine/TouchObject.h"

enum class EVarType;
enum class EVarIntent;
enum class EVarScope;
struct FTouchEngineDynamicVariableStruct;

class FTouchEngineParserUtils
{
public:
	static TEResult Parse(TEInstance* Instance, const char* Identifier, TArray<FTouchEngineDynamicVariableStruct>& VariableList, const FTouchEngineDynamicVariableStruct* Parent = nullptr);

	static EVarType GetVarType(TELinkType Type);
	static EVarType GetVarType(const TouchObject<TELinkInfo>& Info)
	{
		return GetVarType(Info->type);
	}

	static bool GetVarTypeIsArray(TELinkType Type, int32_t Count);
	static bool GetVarTypeIsArray(const TouchObject<TELinkInfo>& Info)
	{
		return GetVarTypeIsArray(Info->type, Info->count);
	}

	static EVarIntent GetVarIntent(TELinkIntent Intent);
	static EVarIntent GetVarIntent(const TouchObject<TELinkInfo>& Info)
	{
		return GetVarIntent(Info->intent);
	}

	// This will not always differentiate Inputs from Parameters due to lack of information
	static EVarScope GetVarScope(TELinkDomain Domain, TEScope Scope);
	static EVarScope GetVarScope(const TouchObject<TELinkInfo>& Info)
	{
		return GetVarScope(Info->domain, Info->scope);
	}

	static FString GetVarDomainChar(EVarScope Scope);

private:
	static TEResult ParseChildren(TEInstance* Instance, const FTouchEngineDynamicVariableStruct& Variable, TArray<FTouchEngineDynamicVariableStruct>& VariableList);

	static void SetDefaultBoolValue(TEInstance* Instance, FTouchEngineDynamicVariableStruct& Variable);
	static void SetDefaultDoubleValue(TEInstance* Instance, FTouchEngineDynamicVariableStruct& Variable);
	static void SetDefaultIntValue(TEInstance* Instance, FTouchEngineDynamicVariableStruct& Variable);
	static void SetDefaultStringValue(TEInstance* Instance, FTouchEngineDynamicVariableStruct& Variable);
	static void SetDefaultTextureValue(TEInstance* Instance, FTouchEngineDynamicVariableStruct& Variable);
	static void SetDefaultCHOPValue(TEInstance* Instance, FTouchEngineDynamicVariableStruct& Variable);

	template <typename T>
	static TEResult GetNumericValue(TEInstance* instance, const char* identifier, TELinkValue which, T* value, int32_t count)
	{
		if constexpr (std::is_same_v<T, int>)
		{
			return TEInstanceLinkGetIntValue(instance, identifier, which, value, count);
		}
		else if constexpr (std::is_same_v<T, double>)
		{
			return TEInstanceLinkGetDoubleValue(instance, identifier, which, value, count);
		}
		return TEResultBadUsage;
	}
	
	template <typename T>
	static FVariant GetTEOptionalNumericValues(TEInstance *instance, const char *identifier, TELinkValue which, int32_t count);
	template <typename T>
	static FVariant GetTENumericValue(TEInstance *instance, const char *identifier, TELinkValue which);
};


template <typename T>
FVariant FTouchEngineParserUtils::GetTEOptionalNumericValues(TEInstance* instance, const char* identifier, TELinkValue which, int32_t count)
{
	TArray<T> Values;
	Values.AddUninitialized(count);
	if (GetNumericValue(instance, identifier, which, Values.GetData(), count) == TEResultSuccess)
	{
		bool bHasAny = false;
		TArray<TOptional<T>> OptionalValues;
		for (int i = 0; i < count; ++i)
		{
			if (TEInstanceLinkHasValue(instance, identifier, which, i))
			{
				bHasAny = true;
				OptionalValues.Add(Values[i]);
			}
			else
			{
				OptionalValues.Add({});
			}
		}
		return bHasAny ? FVariant{OptionalValues} : FVariant();
	}
	return FVariant();
}

template <typename T>
FVariant FTouchEngineParserUtils::GetTENumericValue(TEInstance* instance, const char* identifier, TELinkValue which)
{
	T Value;
	if (TEInstanceLinkHasValue(instance, identifier, which, 0) &&
		GetNumericValue(instance, identifier, which, &Value, 1) == TEResultSuccess)
	{
		return FVariant{Value};
	}
	return FVariant();
}
