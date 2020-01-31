// Fill out your copyright notice in the Description page of Project Settings.

#include "TouchEngineSubsystem.h"
#include "Interfaces/IPluginManager.h"

#include <Runtime/Core/Public/Containers/StringConv.h>

void
UTouchEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	FString dllPath = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("TouchEngine"))->GetBaseDir(), TEXT("/Binaries/ThirdParty/Win64"));
	FPlatformProcess::PushDllDirectory(*dllPath);
	FString dll = FPaths::Combine(dllPath, TEXT("libTDP.dll"));
	if (!FPaths::FileExists(dll))
	{ 
//		UE_LOG(LogAjaMedia, Error, TEXT("Failed to find the binary folder for the AJA dll. Plug-in will not be functional."));
//		return false;
	}
	myLibHandle = FPlatformProcess::GetDllHandle(*dll);

	FPlatformProcess::PopDllDirectory(*dllPath);

	if (!myLibHandle)
	{

	}
}

void
UTouchEngineSubsystem::Deinitialize()
{
	if (myLibHandle)
	{
		FPlatformProcess::FreeDllHandle(myLibHandle);
		myLibHandle = nullptr;
	}

}

UTouchEngine*
UTouchEngineSubsystem::createEngine(FString toxPath)
{
	UTouchEngine *e = NewObject<UTouchEngine>();
	e->loadTox(toxPath);
	return e;
}

