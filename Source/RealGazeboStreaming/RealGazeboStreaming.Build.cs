using UnrealBuildTool;
using System.IO;

public class RealGazeboStreaming : ModuleRules
{
    public RealGazeboStreaming(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableUndefinedIdentifierWarnings = false;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "RHI",
            "RHICore",
            "Projects"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "InputCore",
            "ImageWrapper"
        });

        // GStreamer library integration
        string ThirdPartyPath = Path.Combine(ModuleDirectory, "..", "ThirdParty");
        
        if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            string LinuxGStreamerPath = Path.Combine(ThirdPartyPath, "GStreamer", "Linux");
            
            PublicIncludePaths.AddRange(new string[]
            {
                Path.Combine(LinuxGStreamerPath, "include", "gstreamer-1.0"),
                Path.Combine(LinuxGStreamerPath, "include", "glib-2.0"),
                Path.Combine(LinuxGStreamerPath, "lib", "x86_64-linux-gnu", "glib-2.0", "include")
            });

            PublicSystemLibraryPaths.Add(Path.Combine(LinuxGStreamerPath, "lib", "x86_64-linux-gnu"));

            PublicAdditionalLibraries.AddRange(new string[]
            {
                "gstreamer-1.0",
                "gobject-2.0", 
                "glib-2.0",
                "gstbase-1.0",
                "gstapp-1.0",
                "gstrtspserver-1.0"
            });

            PublicDefinitions.Add("WITH_GSTREAMER=1");
            
            // Avoid name collisions between GLib/GStreamer and Unreal Engine
            PublicDefinitions.Add("GST_DISABLE_DEPRECATED=1");
        }
        else if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string Win64GStreamerPath = Path.Combine(ThirdPartyPath, "GStreamer", "Win64");
            
            PublicIncludePaths.AddRange(new string[]
            {
                Path.Combine(Win64GStreamerPath, "include", "gstreamer-1.0"),
                Path.Combine(Win64GStreamerPath, "include", "glib-2.0"),
                Path.Combine(Win64GStreamerPath, "lib", "glib-2.0", "include")
            });

            PublicSystemLibraryPaths.Add(Path.Combine(Win64GStreamerPath, "lib"));

            PublicAdditionalLibraries.AddRange(new string[]
            {
                "gstreamer-1.0.lib",
                "gobject-2.0.lib",
                "glib-2.0.lib",
                "gstbase-1.0.lib",
                "gstapp-1.0.lib",
                "gstrtspserver-1.0.lib"
            });
            
            // Add runtime DLL dependencies for packaging
            string BinPath = Path.Combine(Win64GStreamerPath, "bin");
            RuntimeDependencies.Add(Path.Combine(BinPath, "*.dll"));
            
            PublicDefinitions.Add("WITH_GSTREAMER=1");
            
            // Avoid name collisions between GLib/GStreamer and Unreal Engine
            PublicDefinitions.Add("GST_DISABLE_DEPRECATED=1");
        }
    }
}