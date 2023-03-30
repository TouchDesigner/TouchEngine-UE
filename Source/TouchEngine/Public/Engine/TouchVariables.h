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
struct TOUCHENGINE_API FTouchEngineCHOPChannelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	TArray<float> ChannelData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	FString ChannelName;

	FString ToString() const;

	bool operator==(const FTouchEngineCHOPChannelData& Other) const;
	bool operator!=(const FTouchEngineCHOPChannelData& Other) const;
};

USTRUCT(BlueprintType, DisplayName = "Touch Engine CHOP")
struct TOUCHENGINE_API FTouchEngineCHOPData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	TArray<FTouchEngineCHOPChannelData> Channels;

	TArray<float> GetCombinedValues() const;
	TArray<FString> GetChannelNames() const;

	bool GetChannelByName(const FString& InChannelName, FTouchEngineCHOPChannelData& OutChannelData);

	FString ToString() const;

	/**
	 * @brief An FTouchEngineCHOPData is valid when there is at least one channel and all channels have the same number of values.
	 */
	bool IsValid() const;

	void SetChannelNames(TArray<FString> InChannelNames);

	bool operator==(const FTouchEngineCHOPData& Other) const;
	bool operator!=(const FTouchEngineCHOPData& Other) const;

	static FTouchEngineCHOPData FromChannels(float** FullChannel, int InChannelCount, int InChannelCapacity, TArray<FString> InChannelNames);
};

USTRUCT(BlueprintType, DisplayName = "Touch Engine DAT Channel")
struct TOUCHENGINE_API FTouchEngineDATLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	TArray<FString> LineData;

	FString ToString() const;

	bool operator==(const FTouchEngineDATLine& Other) const;
	bool operator!=(const FTouchEngineDATLine& Other) const;
};

USTRUCT(BlueprintType, DisplayName = "Touch Engine DAT")
struct TOUCHENGINE_API FTouchEngineDATData
{
	GENERATED_BODY()

	/**
	 * if bIsRowMajor is true, Data is an array of Rows, otherwise Data is an Array of Columns
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	bool bIsRowMajor;

	/**
	 * if bIsRowMajor is true, Data is an array of Rows, otherwise Data is an Array of Columns
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	TArray<FTouchEngineDATLine> Data;


	TArray<FString> GetCombinedValues() const;
	
	int32 GetNumColumns() const;
	int32 GetNumRows() const;

	// bool GetRow(int32 Row, TArray<FString>& OutRowData) const;
	// bool GetRowByName(const FString& RowName, TArray<FString>& OutRowData) const;
	// bool GetColumn(int32 Column, TArray<FString>& OutColumnData) const;
	// bool GetColumnByName(const FString& ColumnName, TArray<FString>& OutColumnData) const;
	// bool GetCell(int32 Column, int32 Row, FString& OutCellValue) const;
	// bool GetCellByName(const FString& ColumnName, const FString& RowName, FString& OutCellValue) const;
	//
	// bool SetRow(int32 Row, const TArray<FString>& InRowData);
	// bool SetRowByName(const FString& RowName, const TArray<FString>& InRowData);
	// bool SetColumn(int32 Column, const TArray<FString>& InColumnData);
	// bool SetColumnByName(const FString& ColumnName, const TArray<FString>& InColumnData);
	// bool SetCell(int32 Column, int32 Row, const FString& InCellValue);
	// bool SetCellByName(const FString& ColumnName, const FString& RowName, const FString& InCellValue);
	//
	// // Add a Row and returns the index of the new Row
	// int32 AddRow(TArray<FString> InData);
	// // Add a Column and returns the index of the new Column
	// int32 AddColumn(TArray<FString> InData);


	FString ToString() const;

	/**
	 *  An FTouchEngineDATData is valid when there is at least one value, all Rows have the same number of values, and all Columns have the same number of values.
	 */
	bool IsValid() const;

	bool operator==(const FTouchEngineDATData& Other) const;
	bool operator!=(const FTouchEngineDATData& Other) const;

	static FTouchEngineDATData FromData(const TArray<FString>& AppendedArray, int32 RowCount, int32 ColumnCount);
};

struct FTouchDATFull
{
	TETable* ChannelData = nullptr;
	TArray<FString> RowNames;
	TArray<FString> ColumnNames;
};
