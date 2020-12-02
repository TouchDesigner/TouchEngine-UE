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


UTouchEngine* UTouchEngineSubsystem::LoadTox(FString toxPath, UObject* outer)
{
	if (!engineInstances.Contains(toxPath))
	{
		UTouchEngine* newEngine = NewObject<UTouchEngine>();
		newEngine->loadTox(toxPath);
		engineInstances.Add(toxPath, newEngine);
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, FString::Printf(TEXT("Loaded Tox file")));
	}

	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, FString::Printf(TEXT("returned engine copy of tox file %s"), *toxPath));

	UTouchEngine* engineInstance = engineInstances[toxPath];
	UTouchEngine* dupe = DuplicateObject<UTouchEngine>(engineInstance, outer);

		
	//dupe = static_cast<UTouchEngine*>(StaticDuplicateObject(engineInstance, outer, NAME_None, RF_AllFlags & ~RF_Transient));
	//return NewObject<UTouchEngine>(engineInstances[toxPath]);
	//EditorUtilities::CopyActorProperties(engineInstance,dupe);
	/*

	StaticDuplicateObject(Instance, &OwnerMovieScene, TemplateName, RF_AllFlags & ~RF_Transient);
	*/
	//return NewObject<UTouchEngine>(outer, NAME_None, RF_NoFlags, engineInstance);
	return dupe;
}