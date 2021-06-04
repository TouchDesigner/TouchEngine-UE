// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineIntVector4.h"

FTouchEngineIntVector4::FTouchEngineIntVector4()
{
}


FTouchEngineIntVector4::FTouchEngineIntVector4(FIntVector4 _vector4)
{
	X = _vector4.X;
	Y = _vector4.Y;
	Z = _vector4.Z;
	W = _vector4.W;
}

FTouchEngineIntVector4::FTouchEngineIntVector4(int32 _X, int32 _Y, int32 _Z, int32 _W)
{
	X = _X; Y = _Y; Z = _Z; W = _W;
}

FTouchEngineIntVector4::~FTouchEngineIntVector4()
{
}

FIntVector4 FTouchEngineIntVector4::AsIntVector4()
{
	return FIntVector4(X, Y, Z, W);
}
