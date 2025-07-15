// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RealGazebo : ModuleRules
{
    public RealGazebo(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
    
        PublicIncludePaths.AddRange(
            new string[] {
                // Public headers that other modules can access
            }
        );
                
        PrivateIncludePaths.AddRange(
            new string[] {
                // Private headers only for this module
            }
        );
            
        // Public dependencies - exposed to other modules
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                // These are public so other modules can use your DataTable types
            }
        );
            
        // Private dependencies - only for this module
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // UI
                "Slate",
                "SlateCore",
                "InputCore",
                
                // Networking
                "Networking",
                "Sockets",
                
                // Data handling
                "Json",
                "JsonUtilities",
                
                // Gameplay
                "GameplayTags",
                "GameplayTasks",
                
                // Development
                "DeveloperSettings",
                
                // Rendering (if needed for vehicle visualization)
                "RenderCore",
                "RHI"
            }
        );
        
        // Editor-specific dependencies
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "EditorSubsystem",
                    "EditorWidgets",
                    "ToolMenus",
                    "EditorStyle",
                    "PropertyEditor",
                    "BlueprintGraph",
                    "KismetCompiler",
                    "AssetTools",
                    "AssetRegistry",
                    "ContentBrowser"
                }
            );
        }
        
        // Platform-specific settings
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // Windows-specific settings if needed
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            // Linux-specific settings if needed
        }
        
        // Uncomment if you need to load modules at runtime
        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // Modules loaded dynamically
            }
        );
        
        // Performance optimizations
        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
        
        // Enable exceptions if needed for error handling
        bEnableExceptions = false;
        
        // Enable RTTI if using dynamic_cast
        bUseRTTI = false;
        
        // Compiler version
        CppStandard = CppStandardVersion.Cpp17;
        
        // Disable unity builds for this module if having issues
        // bUseUnity = false;
    }
}