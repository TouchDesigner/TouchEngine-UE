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

#include "Engine/TouchVariables.h"
#include "Logging.h"

FString FTouchEngineCHOPChannelData::ToString() const
{
	const FString Data = FString::JoinBy(ChannelData,TEXT(","),[](const float& Value)
	{
		return FString::SanitizeFloat(Value);
	});
	return FString::Printf(TEXT("Channel `%s` [%s]"), *ChannelName, *Data);
}

bool FTouchEngineCHOPChannelData::operator==(const FTouchEngineCHOPChannelData& Other) const
{
	return ChannelName == Other.ChannelName && ChannelData == Other.ChannelData;
}

bool FTouchEngineCHOPChannelData::operator!=(const FTouchEngineCHOPChannelData& Other) const
{
	return !(*this==Other);
}

TArray<float> FTouchEngineCHOPData::GetCombinedValues() const
{
	if (!IsValid())
	{
		return TArray<float>();
	}

	const int32 Capacity = Channels[0].ChannelData.Num(); // As it is a Valid Chop here, it is guaranteed that all Channels have the same capacity
	TArray<float> Data;
	Data.Reset(Channels.Num() * Capacity);

	for(auto Channel : Channels)
	{
		Data.Append(Channel.ChannelData);
	}

	return Data;
}

TArray<FString> FTouchEngineCHOPData::GetChannelNames() const
{
	if (!IsValid())
	{
		return TArray<FString>();
	}
	
	TArray<FString> ChannelNames;
	ChannelNames.Reset(Channels.Num());
	
	for(const FTouchEngineCHOPChannelData& Channel : Channels)
	{
		ChannelNames.Emplace(Channel.ChannelName);
	}
	
	return ChannelNames;
}

bool FTouchEngineCHOPData::GetChannelByName(const FString& InChannelName, FTouchEngineCHOPChannelData& OutChannelData)
{
	if (IsValid())
	{
		for (FTouchEngineCHOPChannelData& Channel: Channels)
		{
			if (Channel.ChannelName == InChannelName)
			{
				OutChannelData = Channel;
				return true;
			}
		}
	}
	OutChannelData = FTouchEngineCHOPChannelData();
	return false;
}

FString FTouchEngineCHOPData::ToString() const
{
	const FString Data = FString::JoinBy(Channels,TEXT("\n"),[](const FTouchEngineCHOPChannelData& Value)
	{
		return Value.ToString();
	});
	return FString::Printf(TEXT("Touch Engine CHOP Data%s\n%s"), (IsValid() ? TEXT("") : TEXT(" [INVALID]")), *Data);
}

bool FTouchEngineCHOPData::IsValid() const
{
	if(Channels.IsEmpty())
	{
		return false;
	}

	const int32 Capacity = Channels[0].ChannelData.Num();
	if (Capacity < 1)
	{
		return false;
	}
	
	for (const FTouchEngineCHOPChannelData& Channel : Channels)
	{
		if (Capacity != Channel.ChannelData.Num())
		{
			return false;
		}
	}

	return true;
}

void FTouchEngineCHOPData::SetChannelNames(TArray<FString> InChannelNames)
{
	for (int i = 0; i < Channels.Num(); i++)
	{
		Channels[i].ChannelName = InChannelNames.IsValidIndex(i) ? InChannelNames[i] : FString();
	}
}

bool FTouchEngineCHOPData::operator==(const FTouchEngineCHOPData& Other) const
{
	if( Channels.Num() != Other.Channels.Num())
	{
		return false;
	}
	for (int i = 0; i < Channels.Num(); ++i)
	{
		if(Channels[i] != Other.Channels[i])
		{
			return false;
		}
	}
	return true;
}

bool FTouchEngineCHOPData::operator!=(const FTouchEngineCHOPData& Other) const
{
	return !(*this==Other);
}

FTouchEngineCHOPData FTouchEngineCHOPData::FromChannels(float** FullChannel, const int InChannelCount, const int InChannelCapacity, TArray<FString> InChannelNames)
{
	FTouchEngineCHOPData Chop;

	for (int i = 0; i < InChannelCount; i++)
	{
		FTouchEngineCHOPChannelData Channel;
		Channel.ChannelName = InChannelNames.IsValidIndex(i) ? InChannelNames[i] : FString();
		
		for (int j = 0; j < InChannelCapacity; j++)
		{
			Channel.ChannelData.Add(FullChannel[i][j]);
		}
		Chop.Channels.Emplace(MoveTemp(Channel));
	}

	return Chop;
}
