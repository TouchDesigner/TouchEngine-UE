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
		
		var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				// Engine dependencies
				"Core",
				"CoreUObject",
				"Engine",
				"Projects",
				"RenderCore",
				"RHI",
				"RHICore",

				// Touch designer dependencies
				"TouchEngineAPI",
			});
		
		if (!Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
		{
			PrecompileForTargets = PrecompileTargetsType.None;
		}

		if (Target.IsInPlatformGroup(UnrealPlatformGroup.Windows)
		    || Target.Platform == UnrealTargetPlatform.HoloLens)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"D3D11RHI",
					"D3D12RHI"
				});
			
			PrivateIncludePaths.AddRange(
				new string[] {
					Path.Combine(EngineDir, @"Source\Runtime\Windows\D3D11RHI\Private"),
					Path.Combine(EngineDir, @"Source\Runtime\Windows\D3D11RHI\Private\Windows"),
					Path.Combine(EngineDir, @"Source\Runtime\D3D12RHI\Private"),
					Path.Combine(EngineDir, @"Source\Runtime\D3D12RHI\Private\Windows")
				});

			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
			
			PublicSystemLibraries.AddRange(new string[] {
				"DXGI.lib",
				"d3d11.lib",
				"d3d12.lib"
			});

			PrivateIncludePaths.Add(Path.Combine(EngineDir, "Source/Runtime/VulkanRHI/Private/Windows"));
			
			if (Target.Platform != UnrealTargetPlatform.HoloLens)
			{
				AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
				AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelMetricsDiscovery");
				AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelExtensionsFramework");
			}
		}
		else if (Target.Platform == UnrealTargetPlatform.Win64
		         || Target.Platform == UnrealTargetPlatform.Mac
		         || Target.Platform == UnrealTargetPlatform.Linux)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"VulkanRHI"
				});

			PrivateIncludePaths.Add(Path.Combine(EngineDir, "Source/Runtime/VulkanRHI/Private"));
			
			AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");
		}
		
		PrivateDependencyModuleNames.AddRange(
		new string[]
		{
			"RenderCore",
			"RHI",
			"SlateCore",
		});

		AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
	}
}
