// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class TouchEngineAPI : ModuleRules
{
	public TouchEngineAPI(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Add the import library
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));
            //System.Console.WriteLine(string.Format("VVV {0}", ModuleDirectory));

			// Delay-load the DLL, so we can load it from the right place first
			PublicDelayLoadDLLs.Add("libTDP.dll");

            // Ensure that the DLL is staged along with the executable
            string BinDir = Path.Combine(ModuleDirectory, "../../../Binaries/ThirdParty/Win64/");

            RuntimeDependencies.Add(Path.Combine(BinDir, "libTDP.dll"));
            RuntimeDependencies.Add(Path.Combine(BinDir, "libIPM.dll"));
            RuntimeDependencies.Add(Path.Combine(BinDir, "libTPC.dll"));
            PublicAdditionalLibraries.Add(Path.Combine(BinDir, "libTDP.lib"));
            // This is needed so we link with our own d3d11.lib, which is newer than the one UE ships with
			PublicAdditionalLibraries.Add(Path.Combine(BinDir, "d3d11.lib"));
            //System.Console.WriteLine(string.Format("Found include {0}", touchEngineIncludePath));
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            //PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "Mac", "Release", "libExampleLibrary.dylib"));
            //RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/TouchEngineLibrary/Mac/Release/libExampleLibrary.dylib");
        }
	}
}
