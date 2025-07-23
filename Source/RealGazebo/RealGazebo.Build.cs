// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;

public class RealGazebo : ModuleRules
{
    public RealGazebo(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );
                
        
        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
            );
            
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                // ... add other public dependencies that you statically link with here ...
                "RHI",
                "RenderCore",
                "Renderer",
                "RHICore",
                "Sockets",
                "Networking"
            }
            );
            
        
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                // ... add private dependencies that you statically link with here ...	
                "EditorStyle",
                "ToolMenus",
                "Projects"
            }
            );
        
        
        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );

        // 플러그인 내장 GStreamer 라이브러리 경로 설정
        string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
        string GStreamerPath = Path.Combine(ModuleDirectory, "..", "ThirdParty", "GStreamer");

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            SetupWindowsGStreamer(GStreamerPath);
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            SetupLinuxGStreamer(GStreamerPath);
        }

        bEnableExceptions = true;

        // GLib/GStreamer와 언리얼 엔진 사이의 이름 충돌 방지
        PublicDefinitions.Add("GError=GStreamerGError");
        PublicDefinitions.Add("GThreadPool=GStreamerThreadPool");
        PublicDefinitions.Add("GType=GStreamerType");
        PublicDefinitions.Add("GTypeClass=GStreamerTypeClass");
        PublicDefinitions.Add("GST_DISABLE_DEPRECATED=1");

        // 경고 억제
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.Add("legacy_stdio_definitions.lib");
            bUseUnity = false;
        }
    }

    private void SetupWindowsGStreamer(string GStreamerPath)
    {
        string GStreamerLibPath = Path.Combine(GStreamerPath, "Win64", "lib");
        string GStreamerBinPath = Path.Combine(GStreamerPath, "Win64", "bin");
        string GStreamerIncludePath = Path.Combine(GStreamerPath, "Win64", "include");

        // Include 경로 추가
        PublicIncludePaths.Add(Path.Combine(GStreamerIncludePath, "gstreamer-1.0"));
        PublicIncludePaths.Add(Path.Combine(GStreamerIncludePath, "glib-2.0"));
        PublicIncludePaths.Add(Path.Combine(GStreamerLibPath));
        PublicIncludePaths.Add(Path.Combine(GStreamerLibPath, "glib-2.0", "include"));

        // 필수 라이브러리들
        string[] RequiredLibs = {
            "gstreamer-1.0.lib",
            "gstrtspserver-1.0.lib",
            "gstapp-1.0.lib",
            "gobject-2.0.lib",
            "glib-2.0.lib",
            "gio-2.0.lib",
            "gmodule-2.0.lib",
            "gstbase-1.0.lib",
            "gstvideo-1.0.lib",
            "gstrtp-1.0.lib",
            "gstrtsp-1.0.lib",
            "gstnet-1.0.lib",
            "gstsdp-1.0.lib",
            "x264.lib",
            "openh264.lib"
        };

        foreach (string Lib in RequiredLibs)
        {
            string LibPath = Path.Combine(GStreamerLibPath, Lib);
            if (File.Exists(LibPath))
            {
                PublicAdditionalLibraries.Add(LibPath);
            }
        }

        // x264 정적 라이브러리 추가
        string GstPluginPath = Path.Combine(GStreamerLibPath, "gstreamer-1.0");
        string GstX264Path = Path.Combine(GstPluginPath, "libgstx264.a");
        if (File.Exists(GstX264Path))
        {
            PublicAdditionalLibraries.Add(GstX264Path);
        }

        // 플러그인 바이너리 디렉토리 생성
        string PluginBinaryDir = Path.Combine(PluginDirectory, "Binaries", "Win64");
        Directory.CreateDirectory(PluginBinaryDir);

        // bin 폴더의 모든 DLL 파일들 복사


        if (Directory.Exists(GStreamerLibPath))
        {
            foreach (string DllFile in Directory.GetFiles(GStreamerLibPath, "*.dll"))
            {
                string FileName = Path.GetFileName(DllFile);
                string DestPath = Path.Combine(PluginBinaryDir, FileName);

                RuntimeDependencies.Add(DllFile);

                try
                {
                    File.Copy(DllFile, DestPath, true);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Failed to copy {FileName}: {ex.Message}");
                }
            }
        }


        if (Directory.Exists(GStreamerBinPath))
        {
            foreach (string DllFile in Directory.GetFiles(GStreamerBinPath, "*.dll"))
            {
                string FileName = Path.GetFileName(DllFile);
                string DestPath = Path.Combine(PluginBinaryDir, FileName);

                RuntimeDependencies.Add(DllFile);

                try
                {
                    File.Copy(DllFile, DestPath, true);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Failed to copy {FileName}: {ex.Message}");
                }
            }
        }

        // GStreamer 플러그인 디렉토리도 포함
        string PluginsPath = Path.Combine(GStreamerPath, "Win64", "lib", "gstreamer-1.0");
        if (Directory.Exists(PluginsPath))
        {
            RuntimeDependencies.Add(Path.Combine(PluginsPath, "*"));

            // 모든 GStreamer 플러그인 복사
            string GstPluginDir = Path.Combine(PluginDirectory, "Binaries", "Win64", "gstreamer-1.0");
            Directory.CreateDirectory(GstPluginDir);

            foreach (string PluginFile in Directory.GetFiles(PluginsPath, "*.dll"))
            {
                string FileName = Path.GetFileName(PluginFile);
                string DestPath = Path.Combine(GstPluginDir, FileName);

                // 플러그인 DLL을 런타임 의존성으로 추가
                RuntimeDependencies.Add(PluginFile);

                try
                {
                    File.Copy(PluginFile, DestPath, true);
                    Console.WriteLine($"Copied plugin: {FileName}");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Failed to copy plugin {FileName}: {ex.Message}");
                }
            }

            // 외부 GStreamer 설치에서 x264 플러그인 찾기 시도
            string SystemGstPath = Environment.GetEnvironmentVariable("GSTREAMER_1_0_ROOT_X86_64");
            if (!string.IsNullOrEmpty(SystemGstPath))
            {
                string SystemPluginsPath = Path.Combine(SystemGstPath, "lib", "gstreamer-1.0");
                if (Directory.Exists(SystemPluginsPath))
                {
                    string[] x264Files = {
                        "gstx264.dll",
                        "libgstx264.dll",
                        "gstlibx264.dll"
                    };

                    foreach (string x264File in x264Files)
                    {
                        string SourcePath = Path.Combine(SystemPluginsPath, x264File);
                        if (File.Exists(SourcePath))
                        {
                            string DestPath = Path.Combine(GstPluginDir, x264File);
                            try
                            {
                                File.Copy(SourcePath, DestPath, true);
                                Console.WriteLine($"Copied x264 plugin from system GStreamer: {x264File}");
                                break;
                            }
                            catch (Exception ex)
                            {
                                Console.WriteLine($"Failed to copy x264 plugin {x264File}: {ex.Message}");
                            }
                        }
                    }
                }
            }
        }

        PublicDefinitions.Add("GError=GStreamerGError");
    }

    private void SetupLinuxGStreamer(string GStreamerPath)
    {
        string GStreamerLibPath = Path.Combine(GStreamerPath, "Linux", "lib");
        string GStreamerIncludePath = Path.Combine(GStreamerPath, "Linux", "include");

        // Include 경로 추가
        PublicIncludePaths.Add(Path.Combine(GStreamerIncludePath, "gstreamer-1.0"));
        PublicIncludePaths.Add(Path.Combine(GStreamerIncludePath, "glib-2.0"));
        PublicIncludePaths.Add(Path.Combine(GStreamerIncludePath, "glib-2.0", "include"));

        // 필수 라이브러리들
        string[] RequiredLibs = {
            "libgstreamer-1.0.so",
            "libgstrtspserver-1.0.so",
            "libgstapp-1.0.so",
            "libgobject-2.0.so",
            "libglib-2.0.so",
            "libgio-2.0.so",
            "libgmodule-2.0.so",
            "libgstbase-1.0.so",
            "libgstvideo-1.0.so",
            "libgstrtp-1.0.so",
            "libgstrtsp-1.0.so",
            "libgstnet-1.0.so"
        };

        foreach (string Lib in RequiredLibs)
        {
            string LibPath = Path.Combine(GStreamerLibPath, Lib);
            if (File.Exists(LibPath))
            {
                PublicAdditionalLibraries.Add(LibPath);
                RuntimeDependencies.Add(LibPath);
            }
        }

        // GStreamer 플러그인 디렉토리도 포함
        string PluginsPath = Path.Combine(GStreamerPath, "Linux", "lib", "gstreamer-1.0");
        if (Directory.Exists(PluginsPath))
        {
            RuntimeDependencies.Add(Path.Combine(PluginsPath, "*"));
        }

        // 추가 링커 플래그
        PublicDefinitions.Add("LINUX_RPATH_ORIGIN=1");
    }
}