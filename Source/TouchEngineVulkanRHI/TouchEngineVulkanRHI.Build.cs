// Copyright © Derivative Inc. 2022

using System.IO;
using UnrealBuildTool;

public class TouchEngineVulkanRHI : ModuleRules
{
	public TouchEngineVulkanRHI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] 
			{ 
				"Core",
				"CoreUObject",
				"Engine"
			});
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"TouchEngine",
				"TouchEngineAPI",
				"RenderCore",
				"RHI", 
				"RHICore",
				"Vulkan",
				"VulkanRHI"
			});
		
		var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);
		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(EngineDir, "Source", "Runtime", "VulkanRHI", "Private"),
				Path.Combine(EngineDir, "Source", "ThirdParty", "Vulkan", "Include", "vulkan")
			});
		
		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			PrivateIncludePaths.Add(
				Path.Combine(EngineDir, "Source", "Runtime", "VulkanRHI", "Private", "Linux")
			);
		}
		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PrivateIncludePaths.Add(
				Path.Combine(EngineDir, "Source", "Runtime", "VulkanRHI", "Private", "Mac")
			);
		}
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PrivateIncludePaths.Add(
				Path.Combine(EngineDir, "Source", "Runtime", "VulkanRHI", "Private", "Windows")
				);
		}
	}
}
