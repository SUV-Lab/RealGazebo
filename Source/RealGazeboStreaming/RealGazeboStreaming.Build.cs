using UnrealBuildTool;
using System.IO;

public class RealGazeboStreaming : ModuleRules
{
    public RealGazeboStreaming(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // Public include paths for streaming interfaces
        PublicIncludePaths.AddRange(new string[] {
            Path.Combine(ModuleDirectory, "Public"),
            Path.Combine(ModuleDirectory, "Public", "Capture"),
            Path.Combine(ModuleDirectory, "Public", "Streaming"),
            Path.Combine(ModuleDirectory, "Public", "Interfaces")
        });
        
        // Private include paths for implementation
        PrivateIncludePaths.AddRange(new string[] {
            Path.Combine(ModuleDirectory, "Private"),
            Path.Combine(ModuleDirectory, "Private", "Capture"),
            Path.Combine(ModuleDirectory, "Private", "Streaming"),
            Path.Combine(ModuleDirectory, "Private", "Processors")
        });

        // Core runtime dependencies for streaming
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "RHI",
            "RHICore",
            "Projects",
            "HTTP",
            "Json",
            "JsonUtilities",
            "RealGazeboBridge"  // Dependency on bridge for vehicle data
        });

        // Private implementation dependencies
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "InputCore",
            "ImageWrapper",
            "ApplicationCore",
            "Renderer",
            "PixelCapture",     // For GPU-optimized capture
            "MediaIOCore",      // For advanced media I/O
        });

        // Editor-specific dependencies
        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(new string[]
            {
                "EditorStyle",
                "ToolMenus",
                "UnrealEd"
            });
            
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "WorkspaceMenuStructure",
                "PropertyEditor",
                "DesktopPlatform",
                "MainFrame"
            });
        }

        // Platform-specific configurations
        if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnixCommonStartup"
            });
        }
        else if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "D3D12RHI",
                "D3D11RHI"
            });
        }

        // Preprocessor definitions
        PublicDefinitions.AddRange(new string[]
        {
            "WITH_REALGAZEBO_STREAMING=1",
            "REALGAZEBO_STREAMING_MODULE=1"
        });

        // Debug definitions
        if (Target.Configuration == UnrealTargetConfiguration.Debug ||
            Target.Configuration == UnrealTargetConfiguration.DebugGame)
        {
            PublicDefinitions.Add("REALGAZEBO_STREAMING_DEBUG=1");
        }

        // Optimization settings
        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
    }
}