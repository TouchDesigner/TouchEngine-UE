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
			new string[]
			{
				// ... add public include paths required here ...
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
			{
				// ... add private include paths required here ...
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				// Engine dependencies
				"Core",
				"CoreUObject",
				"Engine",
				"Projects",
				"RHI",

				// Touch designer dependencies
				"TouchEngineAPI",
				// ... add other public dependencies that you statically link with here ...
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"D3D11RHI",
				//"D3D12RHI",
				"RenderCore",
				"RHI",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...
			});

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			});

		//AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
		//AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
		AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
	}
}
