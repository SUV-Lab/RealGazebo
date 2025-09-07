using UnrealBuildTool;
using System.IO;

public class RealGazeboStreaming : ModuleRules
{
    public RealGazeboStreaming(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableUndefinedIdentifierWarnings = false;

        // Organized public include paths
        PublicIncludePaths.AddRange(new string[]
        {
            Path.Combine(ModuleDirectory, "Public", "Core"),
            Path.Combine(ModuleDirectory, "Public", "Capture"),
            Path.Combine(ModuleDirectory, "Public", "Streaming"),
            Path.Combine(ModuleDirectory, "Public", "Integration")
        });

        // Core dependencies
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject", 
            "Engine",
            "RenderCore",
            "RHI",
            "RHICore",
            
            "Projects",
            "RealGazeboBridge", // Integration with RealGazebo plugin
            "PixelCapture"      // High-performance frame capture
        });

        // Private implementation dependencies
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "InputCore",
            "ImageWrapper",
            "PixelCaptureShaders"
        });

        // GStreamer library integration - Enhanced from your original
        string ThirdPartyPath = Path.Combine(ModuleDirectory, "..", "ThirdParty");
        
        if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            SetupGStreamerLinux(ThirdPartyPath);
        }
        else if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            SetupGStreamerWindows(ThirdPartyPath);
        }

        // Performance optimizations
        if (Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            PublicDefinitions.Add("REALGAZEBO_STREAMING_DEBUG=1");
        }
        else
        {
            PublicDefinitions.Add("REALGAZEBO_STREAMING_DEBUG=0");
        }

        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
    }

    private void SetupGStreamerLinux(string ThirdPartyPath)
    {
        string LinuxGStreamerPath = Path.Combine(ThirdPartyPath, "GStreamer", "Linux");
        
        PublicIncludePaths.AddRange(new string[]
        {
            Path.Combine(LinuxGStreamerPath, "include", "gstreamer-1.0"),
            Path.Combine(LinuxGStreamerPath, "include", "glib-2.0"),
            Path.Combine(LinuxGStreamerPath, "lib", "x86_64-linux-gnu", "glib-2.0", "include")
        });

        string LibPath = Path.Combine(LinuxGStreamerPath, "lib", "x86_64-linux-gnu");
        PublicSystemLibraryPaths.Add(LibPath);

        // Enhanced library list with full paths to ThirdParty libraries
        PublicAdditionalLibraries.AddRange(new string[]
        {
            Path.Combine(LibPath, "libgstreamer-1.0.so"),
            Path.Combine(LibPath, "libgobject-2.0.so"), 
            Path.Combine(LibPath, "libglib-2.0.so"),
            Path.Combine(LibPath, "libgstbase-1.0.so"),
            Path.Combine(LibPath, "libgstapp-1.0.so"),
            Path.Combine(LibPath, "libgstrtspserver-1.0.so"),
            Path.Combine(LibPath, "libgstvideo-1.0.so"),      // Video processing
            Path.Combine(LibPath, "libgstcodecparsers-1.0.so"), // H.264/H.265 parsing
            Path.Combine(LibPath, "libgstrtp-1.0.so")         // RTP support
        });

        PublicDefinitions.Add("WITH_GSTREAMER=1");
        PublicDefinitions.Add("GST_DISABLE_DEPRECATED=1");
        
        // Linux-specific optimizations
        PublicDefinitions.Add("GSTREAMER_LINUX_OPTIMIZED=1");
    }

    private void SetupGStreamerWindows(string ThirdPartyPath)
    {
        string Win64GStreamerPath = Path.Combine(ThirdPartyPath, "GStreamer", "Win64");
        
        PublicIncludePaths.AddRange(new string[]
        {
            Path.Combine(Win64GStreamerPath, "include", "gstreamer-1.0"),
            Path.Combine(Win64GStreamerPath, "include", "glib-2.0"),
            Path.Combine(Win64GStreamerPath, "lib", "glib-2.0", "include")
        });

        string WinLibPath = Path.Combine(Win64GStreamerPath, "lib");
        PublicSystemLibraryPaths.Add(WinLibPath);

        // Enhanced library list with full paths for Windows
        PublicAdditionalLibraries.AddRange(new string[]
        {
            Path.Combine(WinLibPath, "gstreamer-1.0.lib"),
            Path.Combine(WinLibPath, "gobject-2.0.lib"),
            Path.Combine(WinLibPath, "glib-2.0.lib"),
            Path.Combine(WinLibPath, "gstbase-1.0.lib"),
            Path.Combine(WinLibPath, "gstapp-1.0.lib"),
            Path.Combine(WinLibPath, "gstrtspserver-1.0.lib"),
            Path.Combine(WinLibPath, "gstvideo-1.0.lib"),
            Path.Combine(WinLibPath, "gstcodecparsers-1.0.lib"),
            Path.Combine(WinLibPath, "gstrtp-1.0.lib")
        });
        
        // Runtime DLL dependencies for packaging
        string BinPath = Path.Combine(Win64GStreamerPath, "bin");
        RuntimeDependencies.Add(Path.Combine(BinPath, "*.dll"));
        
        PublicDefinitions.Add("WITH_GSTREAMER=1");
        PublicDefinitions.Add("GST_DISABLE_DEPRECATED=1");
    }
}