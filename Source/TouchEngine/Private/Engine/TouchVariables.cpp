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

FString FTouchCHOPSingleSample::ToString() const
{
	const FString Data = FString::JoinBy(ChannelData,TEXT(","),[](const float& Value)
	{
		return FString::SanitizeFloat(Value);
	});
	return FString::Printf(TEXT("Channel `%s` [%s]"), *ChannelName, *Data);
}

bool FTouchCHOPSingleSample::operator==(const FTouchCHOPSingleSample& Other) const
{
	return ChannelName == Other.ChannelName && ChannelData == Other.ChannelData;
}

bool FTouchCHOPSingleSample::operator!=(const FTouchCHOPSingleSample& Other) const
{
	return !(*this==Other);
}

TArray<float> FTouchCHOPFull::GetCombinedValues() const
{
	if (!IsValid())
	{
		return TArray<float>();
	}

	const int32 Capacity = SampleData[0].ChannelData.Num(); // As it is a Valid Chop here, it is guaranteed that all Channels have the same capacity
	TArray<float> Data;
	Data.Reset(SampleData.Num() * Capacity);

	for(auto Channel : SampleData)
	{
		Data.Append(Channel.ChannelData);
	}

	return Data;
}

TArray<FString> FTouchCHOPFull::GetChannelNames() const
{
	if (!IsValid())
	{
		return TArray<FString>();
	}
	
	TArray<FString> ChannelNames;
	ChannelNames.Reset(SampleData.Num());
	
	for(const FTouchCHOPSingleSample& Channel : SampleData)
	{
		ChannelNames.Emplace(Channel.ChannelName);
	}
	
	return ChannelNames;
}

bool FTouchCHOPFull::GetChannelByName(const FString& InChannelName, FTouchCHOPSingleSample& OutChannelData)
{
	if (IsValid())
	{
		for (FTouchCHOPSingleSample& Channel: SampleData)
		{
			if (Channel.ChannelName == InChannelName)
			{
				OutChannelData = Channel;
				return true;
			}
		}
	}
	OutChannelData = FTouchCHOPSingleSample();
	return false;
}

FString FTouchCHOPFull::ToString() const
{
	const FString Data = FString::JoinBy(SampleData,TEXT("\n"),[](const FTouchCHOPSingleSample& Value)
	{
		return Value.ToString();
	});
	return FString::Printf(TEXT("Touch Engine CHOP Data%s\n%s"), (IsValid() ? TEXT("") : TEXT(" [INVALID]")), *Data);
}

bool FTouchCHOPFull::IsValid() const
{
	if(SampleData.IsEmpty())
	{
		return false;
	}

	const int32 Capacity = SampleData[0].ChannelData.Num();
	if (Capacity < 1)
	{
		return false;
	}
	
	for (const FTouchCHOPSingleSample& Channel : SampleData)
	{
		if (Capacity != Channel.ChannelData.Num())
		{
			return false;
		}
	}

	return true;
}

void FTouchCHOPFull::SetChannelNames(TArray<FString> InChannelNames)
{
	for (int i = 0; i < SampleData.Num(); i++)
	{
		SampleData[i].ChannelName = InChannelNames.IsValidIndex(i) ? InChannelNames[i] : FString();
	}
}

bool FTouchCHOPFull::operator==(const FTouchCHOPFull& Other) const
{
	if( SampleData.Num() != Other.SampleData.Num())
	{
		return false;
	}
	for (int i = 0; i < SampleData.Num(); ++i)
	{
		if(SampleData[i] != Other.SampleData[i])
		{
			return false;
		}
	}
	return true;
}

bool FTouchCHOPFull::operator!=(const FTouchCHOPFull& Other) const
{
	return !(*this==Other);
}

FTouchCHOPFull FTouchCHOPFull::FromChannels(float** FullChannel, const int InChannelCount, const int InChannelCapacity, TArray<FString> InChannelNames)
{
	FTouchCHOPFull Chop;

	for (int i = 0; i < InChannelCount; i++)
	{
		FTouchCHOPSingleSample Channel;
		Channel.ChannelName = InChannelNames.IsValidIndex(i) ? InChannelNames[i] : FString();
		
		for (int j = 0; j < InChannelCapacity; j++)
		{
			Channel.ChannelData.Add(FullChannel[i][j]);
		}
		Chop.SampleData.Emplace(MoveTemp(Channel));
	}

	return Chop;
}
