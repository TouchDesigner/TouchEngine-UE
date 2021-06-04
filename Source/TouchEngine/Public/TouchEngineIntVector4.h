// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Math/IntVector.h"
#include "TouchEngineIntVector4.generated.h"

/**
 * 
 */
USTRUCT()
struct TOUCHENGINE_API FTouchEngineIntVector4
{
	GENERATED_BODY()

public:
	FTouchEngineIntVector4();
	FTouchEngineIntVector4(FIntVector4 _vector4);
	FTouchEngineIntVector4(int32 _X, int32 _Y, int32 _Z, int32 _W);
	~FTouchEngineIntVector4();

	UPROPERTY(EditAnywhere)
		int32 X;
	UPROPERTY(EditAnywhere)
		int32 Y;
	UPROPERTY(EditAnywhere)
		int32 Z;
	UPROPERTY(EditAnywhere)
		int32 W;

	FIntVector4 AsIntVector4();
};
