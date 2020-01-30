// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class TouchEngine : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    public TouchEngine(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);

        PublicIncludePaths.Add(EngineDirectory + "/Source/Runtime/Windows/D3D11RHI/Public/");

        PrivateIncludePaths.AddRange(
			new string[] {
                EngineDirectory + "/Source/Runtime/Windows/D3D11RHI/Private/",
                EngineDirectory + "/Source/Runtime/Windows/D3D11RHI/Private/Windows/"
            }
			);


        PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "TouchEngineAPI",
				"Core",
                "CoreUObject",
                "Engine",
				"Projects",
                "RHI",
                "RenderCore"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "RHI",
                "D3D11RHI"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
