
using UnrealBuildTool;
using System.IO;
using System;
using System.Linq;

public class RealGazeboBridge : ModuleRules
{
    public RealGazeboBridge(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        // Organized public include paths for new structure
        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "Public", "Core"),
                Path.Combine(ModuleDirectory, "Public", "Network"),
                Path.Combine(ModuleDirectory, "Public", "Vehicles")
            }
        );
                
        PrivateIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "Private", "Core"),
                Path.Combine(ModuleDirectory, "Private", "Network"),
                Path.Combine(ModuleDirectory, "Private", "Vehicles")
            }
        );
            
        // Core dependencies for high-performance bridge
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Sockets",
                "Networking",
                "RHI",
                "RenderCore"
            }
        );
            
        // Private dependencies for implementation details
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "DeveloperSettings"  // For subsystem settings
            }
        );

        // Editor-only dependencies
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "EditorStyle",
                    "ToolMenus",
                    "Projects",
                    "UnrealEd"
                }
            );
        }
        
        // Performance optimizations
        if (Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            PublicDefinitions.Add("REALGAZEBO_BRIDGE_DEBUG=1");
        }
        else
        {
            PublicDefinitions.Add("REALGAZEBO_BRIDGE_DEBUG=0");
        }

        // Enable optimizations for performance-critical code
        OptimizeCode = CodeOptimization.InShippingBuildsOnly;

        Console.WriteLine("======= RealGazeboBridge Plugin Build Configuration =======");
        Console.WriteLine($"Target Platform: {Target.Platform}");
        Console.WriteLine($"Target Configuration: {Target.Configuration}");
        Console.WriteLine($"Module Directory: {ModuleDirectory}");
        Console.WriteLine("Subsystem architecture");
        Console.WriteLine("Object pooling support");
        Console.WriteLine("Batch processing enabled");
        Console.WriteLine("Organized folder structure");
    }
}