// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class TouchEngineEditor : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    public TouchEngineEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
			}
            );

        PublicIncludePaths.Add(EngineDirectory + "/Source/Runtime/Windows/D3D11RHI/Public/");
        PublicIncludePaths.Add(EngineDirectory + "/Source/Runtime/D3D12RHI/Public/");

        PrivateIncludePaths.AddRange(
            new string[] {
                EngineDirectory + "/Source/Runtime/Windows/D3D11RHI/Private/",
                EngineDirectory + "/Source/Runtime/Windows/D3D11RHI/Private/Windows/",
                EngineDirectory + "/Source/Runtime/D3D12RHI/Private/",
                EngineDirectory + "/Source/Runtime/D3D12RHI/Private/Windows/"
            });
        PrivateIncludePaths.AddRange(new string[]
        {
            Path.Combine(ModuleDirectory, "Private", "Customization"),
            Path.Combine(ModuleDirectory, "Private", "Factory"),
            Path.Combine(ModuleDirectory, "Private", "Nodes")
        });

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "DesktopWidgets",
                "Engine",
                "GraphEditor",
                "Projects",
                "RenderCore",
                "RHI",
                "Slate",
                "TouchEngine",
                "TouchEngineAPI",
                "ToolMenus",
                "UnrealEd",
				// ... add other public dependencies that you statically link with here ...
			}
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "InputCore",
                "SlateCore",
                "PropertyEditor",
                "EditorStyle",
                "ContentBrowser",
                "BlueprintGraph",
                "ClassViewer",
                "MessageLog",
                "Kismet",
                "KismetCompiler",
                "DetailCustomizations",
				// ... add private dependencies that you statically link with here ...
                "TouchEngine",
                "TouchEngineAPI",
                "AppFramework",
                "ApplicationCore", 
            }
        );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
        );

        //AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
        AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
        AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");

        // Note: Dev build-only. To be excluded for release build.
        OptimizeCode = CodeOptimization.Never;
    }
}