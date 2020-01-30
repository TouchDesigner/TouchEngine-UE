// Fill out your copyright notice in the Description page of Project Settings.

#include "TouchEngineSubsystem.h"

#include <Runtime/Core/Public/Containers/StringConv.h>


UTouchEngine*
UTouchEngineSubsystem::createEngine(FString toxPath)
{
	UTouchEngine *e = NewObject<UTouchEngine>();
	e->loadTox(toxPath);
	return e;
}

