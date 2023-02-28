// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Features/IModularFeature.h"
#include "ITouchEngineInterceptionCommands.h"


/**
 * Base RCI feature interface
 */
template <class TResponseType>
class ITouchEngineInterceptionFeature
	: public IModularFeature
	, public ITouchEngineInterceptionCommands<TResponseType>
{
	static_assert(std::is_same<ETEIResponse, TResponseType>::value || std::is_void<TResponseType>::value, "Only \"ERCIResponse\" and \"void\" template parameters are allowed");

public:
	virtual ~ITouchEngineInterceptionFeature() = default;
};


/**
 * RCI command interceptor feature
 */
class ITouchEngineInterceptionFeatureInterceptor
	: public ITouchEngineInterceptionFeature<ETEIResponse>
{
public:
	static FName GetName()
	{
		static const FName FeatureName = TEXT("TouchEngineInterception_Feature_Interceptor");
		return FeatureName;
	}
};


/**
 * RCI command processor feature
 */
class ITouchEngineInterceptionFeatureProcessor
	: public ITouchEngineInterceptionFeature<void>
{
public:
	static FName GetName()
	{
		static const FName FeatureName = TEXT("TouchEngineInterception_Feature_Processor");
		return FeatureName;
	}
};
