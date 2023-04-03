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
#include "TouchEngine/TETable.h"
#include "TouchVariables.generated.h"

class UTexture2D;

USTRUCT(BlueprintType, DisplayName = "Touch Engine CHOP Channel")
struct TOUCHENGINE_API FTouchEngineCHOPChannel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	TArray<float> Values;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	FString Name;

	FString ToString() const;

	bool operator==(const FTouchEngineCHOPChannel& Other) const;
	bool operator!=(const FTouchEngineCHOPChannel& Other) const;
};

USTRUCT(BlueprintType, DisplayName = "Touch Engine CHOP")
struct TOUCHENGINE_API FTouchEngineCHOP
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	TArray<FTouchEngineCHOPChannel> Channels;

	void Clear();

	/** Returns the combined values of each Channel. If the FTouchEngineCHOP is not valid, returns false and an empty array. */
	bool GetCombinedValues(TArray<float>& OutValues) const;
	/** Returns the name of each Channel. Does not check if the FTouchEngineCHOP is Valid. */
	TArray<FString> GetChannelNames() const;
	/** Returns the first Channel with the name matching InChannelName. Returns true if found, otherwise false. */
	bool GetChannelByName(const FString& InChannelName, FTouchEngineCHOPChannel& OutChannel);

	/** Returns the number of Channels in the FTouchEngineCHOP, equivalent of breaking the FTouchEngineCHOP structure and getting the length of the Channels array.	*/
	int32 GetNumChannels() const;
	/**
	 * Returns the number of Samples in the FTouchEngineCHOP, equivalent of breaking the FTouchEngineCHOP structure, getting the first Channel and getting the length of the Values array.
	 * It is only guaranteed to be the same for each Channel if the FTouchEngineCHOP is valid.
	 */
	int32 GetNumSamples() const;

	FString ToString() const;

	/**
	 * Check if the FTouchEngineCHOP is valid. An FTouchEngineCHOP is valid when all channels have the same number of values. An FTouchEngineCHOP with no channels or with only empty channels is valid.
	 */
	bool IsValid() const;

	void SetChannelNames(const TArray<FString>& InChannelNames);

	bool operator==(const FTouchEngineCHOP& Other) const;
	bool operator!=(const FTouchEngineCHOP& Other) const;

	static FTouchEngineCHOP FromChannels(float** FullChannel, int InChannelCount, int InChannelCapacity, const TArray<FString>& InChannelNames);
	
	bool Serialize(FArchive& Ar);
};

struct FTouchDATFull
{
	TETable* ChannelData = nullptr;
	TArray<FString> RowNames;
	TArray<FString> ColumnNames;
};
