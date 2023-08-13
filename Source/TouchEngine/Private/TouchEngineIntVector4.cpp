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

#include "TouchEngineIntVector4.h"

FTouchEngineIntVector4::FTouchEngineIntVector4()
{
}


FTouchEngineIntVector4::FTouchEngineIntVector4(const FIntVector4& InVector4)
{
	X = InVector4.X;
	Y = InVector4.Y;
	Z = InVector4.Z;
	W = InVector4.W;
}

FTouchEngineIntVector4::FTouchEngineIntVector4(int32 InX, int32 InY, int32 InZ, int32 InW)
{
	X = InX; Y = InY; Z = InZ; W = InW;
}

FIntVector4 FTouchEngineIntVector4::AsIntVector4() const
{
	return FIntVector4(X, Y, Z, W);
}
