// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class TouchEngineAPI : ModuleRules
{
	public TouchEngineAPI(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

        if (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test)
        {
            PrivateIncludePathModuleNames.AddRange(new string[] { "MessageLog" });
        }

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
			// Add the import library
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));

			// Delay-load the DLL, so we can load it from the right place first
			PublicDelayLoadDLLs.Add("TouchEngine.dll");

            // Ensure that the DLL is staged along with the executable
            string BinDir = Path.Combine(ModuleDirectory, "../../../Binaries/ThirdParty/Win64/");

            RuntimeDependencies.Add("$(BinaryOutputDir)/TouchEngine.dll", Path.Combine("$(PluginDir)", "Binaries/ThirdParty/Win64/TouchEngine.dll"));
            PublicAdditionalLibraries.Add(Path.Combine(BinDir, "TouchEngine.lib"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            //PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "Mac", "Release", "libExampleLibrary.dylib"));
            //RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/TouchEngineLibrary/Mac/Release/libExampleLibrary.dylib");
        }
	}
}
