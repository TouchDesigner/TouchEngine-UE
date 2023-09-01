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

FString FTouchEngineCHOPChannel::ToString() const
{
	const FString Data = FString::JoinBy(Values,TEXT(","), [](const float& Value)
	{
		return FString::SanitizeFloat(Value);
	});
	return FString::Printf(TEXT("Channel `%s` [%s]"), *Name, Values.IsEmpty() ? TEXT("No Values") : *Data);
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

bool FTouchEngineCHOP::GetCombinedValues(TArray<float>& OutValues) const
{
	if (Channels.IsEmpty())
	{
		OutValues = TArray<float>();
		return true; // technically valid
	}

	const int32 NbChannels = Channels[0].Values.Num();
	OutValues.Empty(Channels.Num() * NbChannels);

	for (auto Channel : Channels)
	{
		if (Channel.Values.Num() != NbChannels)
		{
			OutValues = TArray<float>();
			return false; // invalid as not all the channels have the same number of values
		}
		OutValues.Append(Channel.Values);
	}

	return true;
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

int32 FTouchEngineCHOP::GetNumChannels() const
{
	return Channels.Num();
}

int32 FTouchEngineCHOP::GetNumSamples() const
{
	return Channels.IsEmpty() ? 0 : Channels[0].Values.Num();
}

FString FTouchEngineCHOP::ToString() const
{
	const int32 NbValues = Channels.IsEmpty() ? 0 : Channels[0].Values.Num();
	bool bIsValid = true;
	const FString Data = FString(TEXT("\n")) + FString::JoinBy(Channels,TEXT("\n"), [&](const FTouchEngineCHOPChannel& Channel)
	{
		bIsValid &= Channel.Values.Num() == NbValues;
		return Channel.ToString();
	});
	return FString::Printf(TEXT("TouchEngine CHOP Data%s%s"),
		bIsValid ? TEXT("") : TEXT(" [INVALID]"),
		Channels.IsEmpty() ? TEXT(" [No Channels]") : *Data);
}

bool FTouchEngineCHOP::IsValid() const
{
	if (Channels.IsEmpty())
	{
		return true;
	}

	const int32 Capacity = Channels[0].Values.Num();
	for (int i = 1; i < Channels.Num(); ++i)
	{
		if (Capacity != Channels[i].Values.Num())
		{
			return false;
		}
	}

	return true;
}

void FTouchEngineCHOP::SetChannelNames(const TArray<FString>& InChannelNames)
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

FTouchEngineCHOP FTouchEngineCHOP::FromChannels(float** FullChannel, const int InChannelCount, const int InChannelCapacity, const TArray<FString>& InChannelNames)
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
	FTouchEngineCHOP::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(this), FTouchEngineCHOP::StaticStruct(), nullptr);
	return true;
}


FString FTouchEngineDATLine::ToString() const
{
	const FString StringData = FString::JoinBy(LineData,TEXT(","), [](const FString& Value)
	{
		return Value;
	});
	return FString::Printf(TEXT("[%s]"), *StringData);
}

bool FTouchEngineDATLine::operator==(const FTouchEngineDATLine& Other) const
{
	if (LineData.Num() != Other.LineData.Num())
	{
		return false;
	}
	for (int i = 0; i < LineData.Num(); ++i)
	{
		if (LineData[i] != LineData[i])
		{
			return false;
		}
	}
	return true;
}

bool FTouchEngineDATLine::operator!=(const FTouchEngineDATLine& Other) const
{
	return !(*this == Other);
}


TArray<FString> FTouchEngineDATData::GetCombinedValues() const
{
	if (!IsValid())
	{
		return TArray<FString>();
	}

	TArray<FString> Values;

	for (auto Line : Data)
	{
		Values.Append(Line.LineData); //todo: handle Row/Column Major
	}
	
	return Values;
}

int32 FTouchEngineDATData::GetNumColumns() const
{
	if (!IsValid())
	{
		return 0;
	}
	return bIsRowMajor ? Data[0].LineData.Num() : Data.Num();
}

int32 FTouchEngineDATData::GetNumRows() const
{
	if (!IsValid())
	{
		return 0;
	}
	return bIsRowMajor ? Data.Num() : Data[0].LineData.Num();
}

FString FTouchEngineDATData::ToString() const
{
	FString Prefix(bIsRowMajor ? TEXT("Row: ") : TEXT("Col: "));
	const FString StringData = FString::JoinBy(Data,TEXT("\n"), [&Prefix](const FTouchEngineDATLine& Value)
	{
		return Prefix + Value.ToString();
	});
	return FString::Printf(TEXT("TouchEngine DAT (%s Major)%s\n%s"), (bIsRowMajor ? TEXT("Row") : TEXT("Column")), (IsValid() ? TEXT("") : TEXT(" [INVALID]")), *StringData);
}

bool FTouchEngineDATData::IsValid() const
{
	if (Data.IsEmpty())
	{
		return false;
	}
	const int32 Length = Data[0].LineData.Num();
	for (int i = 1; i < Data.Num(); ++i)
	{
		if (Length != Data[1].LineData.Num())
		{
			return false;
		}
	}
	return true;
}

bool FTouchEngineDATData::operator==(const FTouchEngineDATData& Other) const
{
	if (bIsRowMajor == Other.bIsRowMajor)
	{
		if (Data.Num() != Other.Data.Num())
		{
			return false;
		}
		for (int i = 0; i < Data.Num(); ++i)
		{
			if (Data[i].LineData != Other.Data[i].LineData)
			{
				return false;
			}
		}
		return true;
	}
	
	if (Data.IsEmpty() && Other.Data.IsEmpty())
	{
		return true;
	}
	if (Data.IsEmpty() || Other.Data.IsEmpty() ||
		GetNumColumns() != Other.GetNumColumns() || GetNumRows() != Other.GetNumRows())
	{
		return false;
	}

	// at this point, they have the same number of rows and columns
	for (int i = 0; i < Data.Num(); ++i)
	{
		for (int j = 0; j < Data[i].LineData.Num(); ++j)
		{
			if (Data[i].LineData[j] != Other.Data[j].LineData[i])
			{
				return false;
			}
		}
	}
	return true;
}

bool FTouchEngineDATData::operator!=(const FTouchEngineDATData& Other) const
{
	return !(*this == Other);
}
