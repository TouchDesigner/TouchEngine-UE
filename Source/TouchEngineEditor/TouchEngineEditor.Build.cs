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
            }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "TouchEngine",
                "TouchEngineAPI",
                "Core",
                "CoreUObject",
                "Engine",
                "Projects",
                "RHI",
                "RenderCore",
                "Slate"
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
				// ... add private dependencies that you statically link with here ...	
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
    }
}
