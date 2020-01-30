// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class TouchEngine : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../ThirdParty/")); }
    }

    private string LibraryPath
    {
        get { return Path.GetFullPath(Path.Combine(ThirdPartyPath, "TouchEngineAPI", "lib")); }
    }

    private string BinariesPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../Binaries/ThirdParty/")); }
    }

    public TouchEngine(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);

        string touchEngineIncludePath = ThirdPartyPath + "TouchEngineAPI/include/";

        if (!Directory.Exists(touchEngineIncludePath))
        {
            System.Console.WriteLine(string.Format("TouchEngine include path not found: {0}", touchEngineIncludePath));
        }
        else
        {
            System.Console.WriteLine(string.Format("Found include {0}", touchEngineIncludePath));

        }
        PublicIncludePaths.Add(touchEngineIncludePath);
        PublicIncludePaths.Add(EngineDirectory + "/Source/Runtime/Windows/D3D11RHI/Public/");

        PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "libTDP.lib"));

        PrivateIncludePaths.AddRange(
			new string[] {
                EngineDirectory + "/Source/Runtime/Windows/D3D11RHI/Private/",
                EngineDirectory + "/Source/Runtime/Windows/D3D11RHI/Private/Windows/"
            }
			);

        RuntimeDependencies.Add(ThirdPartyPath + "TouchEngineAPI/bin/libIPM.dll");
        RuntimeDependencies.Add(ThirdPartyPath + "TouchEngineAPI/bin/libTDP.dll");
        RuntimeDependencies.Add(ThirdPartyPath + "TouchEngineAPI/bin/libTPC.dll");


        PublicDependencyModuleNames.AddRange(
			new string[]
			{
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
