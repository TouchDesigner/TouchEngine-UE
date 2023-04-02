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

FString FTouchEngineCHOPChannel::ToString() const
{
	const FString Data = FString::JoinBy(Values,TEXT(","), [](const float& Value)
	{
		return FString::SanitizeFloat(Value);
	});
	return FString::Printf(TEXT("Channel `%s` [%s]"), *Name, *Data);
}

bool FTouchEngineCHOPChannel::operator==(const FTouchEngineCHOPChannel& Other) const
{
	return Name == Other.Name && Values == Other.Values;
}

bool FTouchEngineCHOPChannel::operator!=(const FTouchEngineCHOPChannel& Other) const
{
	return !(*this == Other);
}

void FTouchEngineCHOP::Clear()
{
	Channels.Empty();
}

TArray<float> FTouchEngineCHOP::GetCombinedValues() const
{
	if (Channels.IsEmpty())
	{
		return TArray<float>();
	}

	const int32 NbChannels = Channels[0].Values.Num();
	TArray<float> Data;
	Data.Reset(Channels.Num() * NbChannels);

	for (auto Channel : Channels)
	{
		if (Channel.Values.Num() != NbChannels)
		{
			return TArray<float>();
		}
		Data.Append(Channel.Values);
	}

	return Data;
}

TArray<FString> FTouchEngineCHOP::GetChannelNames() const
{
	TArray<FString> ChannelNames;
	ChannelNames.Reset(Channels.Num());

	for (const FTouchEngineCHOPChannel& Channel : Channels)
	{
		ChannelNames.Emplace(Channel.Name);
	}

	return ChannelNames;
}

bool FTouchEngineCHOP::GetChannelByName(const FString& InChannelName, FTouchEngineCHOPChannel& OutChannel)
{
	for (FTouchEngineCHOPChannel& Channel : Channels)
	{
		if (Channel.Name == InChannelName)
		{
			OutChannel = Channel;
			return true;
		}
	}
	
	OutChannel = FTouchEngineCHOPChannel();
	return false;
}

FString FTouchEngineCHOP::ToString() const
{
	const int32 NbValues = Channels.IsEmpty() ? 0 : Channels[0].Values.Num();
	bool bIsValid = NbValues > 0;
	const FString Data = FString::JoinBy(Channels,TEXT("\n"), [&](const FTouchEngineCHOPChannel& Channel)
	{
		bIsValid &= Channel.Values.Num() == NbValues;
		return Channel.ToString();
	});
	return FString::Printf(TEXT("Touch Engine CHOP Data%s\n%s"), (bIsValid ? TEXT("") : TEXT(" [INVALID]")), *Data);
}

bool FTouchEngineCHOP::IsValid() const
{
	if (Channels.IsEmpty())
	{
		return false;
	}

	const int32 Capacity = Channels[0].Values.Num();
	if (Capacity < 1)
	{
		return false;
	}

	for (int i = 0; i < Channels.Num(); ++i)
	{
		if (Capacity != Channels[0].Values.Num())
		{
			return false;
		}
	}

	return true;
}

void FTouchEngineCHOP::SetChannelNames(TArray<FString> InChannelNames)
{
	for (int i = 0; i < Channels.Num(); i++)
	{
		Channels[i].Name = InChannelNames.IsValidIndex(i) ? InChannelNames[i] : FString();
	}
}

bool FTouchEngineCHOP::operator==(const FTouchEngineCHOP& Other) const
{
	if (Channels.Num() != Other.Channels.Num())
	{
		return false;
	}
	for (int i = 0; i < Channels.Num(); ++i)
	{
		if (Channels[i] != Other.Channels[i])
		{
			return false;
		}
	}
	return true;
}

bool FTouchEngineCHOP::operator!=(const FTouchEngineCHOP& Other) const
{
	return !(*this == Other);
}

FTouchEngineCHOP FTouchEngineCHOP::FromChannels(float** FullChannel, const int InChannelCount, const int InChannelCapacity, TArray<FString> InChannelNames)
{
	FTouchEngineCHOP Chop;

	for (int i = 0; i < InChannelCount; i++)
	{
		FTouchEngineCHOPChannel Channel;
		Channel.Name = InChannelNames.IsValidIndex(i) ? InChannelNames[i] : FString();

		for (int j = 0; j < InChannelCapacity; j++)
		{
			Channel.Values.Add(FullChannel[i][j]);
		}
		Chop.Channels.Emplace(MoveTemp(Channel));
	}

	return Chop;
}

bool FTouchEngineCHOP::Serialize(FArchive& Ar)
{
	FTouchEngineCHOP::StaticStruct()->SerializeTaggedProperties(Ar, (uint8*)this, FTouchEngineCHOP::StaticStruct(), nullptr);
	return true;
}
