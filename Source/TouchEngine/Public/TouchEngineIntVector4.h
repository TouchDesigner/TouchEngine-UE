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
