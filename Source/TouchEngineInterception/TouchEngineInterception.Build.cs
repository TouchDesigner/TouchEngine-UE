// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TouchEngineInterception : ModuleRules
{
	public TouchEngineInterception(ReadOnlyTargetRules Target) : base(Target)
	{
		// TODO, maybe we could avoid this dependency 
		PublicDependencyModuleNames.AddRange(new string[] { "TouchEngine" });
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Cbor",
				"Core",
				"CoreUObject",
				"Engine",
				"Serialization",
			});
	}
}