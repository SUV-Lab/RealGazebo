// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

using UnrealBuildTool;
using System.IO;

public class RealGazeboStreaming : ModuleRules
{
    public RealGazeboStreaming(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

        PublicIncludePaths.AddRange(new string[]
        {
            Path.Combine(ModuleDirectory, "Public", "Core"),
            Path.Combine(ModuleDirectory, "Public", "Camera"),
            Path.Combine(ModuleDirectory, "Public", "Encoder"),
            Path.Combine(ModuleDirectory, "Public", "RTSP"),
            Path.Combine(ModuleDirectory, "Public", "Pipeline")
        });

        PrivateIncludePaths.AddRange(new string[]
        {
            Path.Combine(ModuleDirectory, "Private", "Core"),
            Path.Combine(ModuleDirectory, "Private", "Camera"),
            Path.Combine(ModuleDirectory, "Private", "Encoder"),
            Path.Combine(ModuleDirectory, "Private", "RTSP"),
            Path.Combine(ModuleDirectory, "Private", "Pipeline")
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RHI",
            "RenderCore",
            "Live555",
            "RealGazeboBridge"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "DeveloperSettings"
        });

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "AVCodecsCore",
                "AVCodecsCoreRHI",
                "VTCodecs",
                "VTCodecsRHI"
            });

            PublicFrameworks.AddRange(new string[]
            {
                "AVFoundation",
                "VideoToolbox",
                "CoreMedia",
                "CoreVideo"
            });
        }
        else
        {
            PublicDependencyModuleNames.Add("AVEncoder");
            PrivateDependencyModuleNames.Add("CUDA");
        }

        if (Target.Platform == UnrealTargetPlatform.Win64 ||
            Target.Platform == UnrealTargetPlatform.Linux)
        {
            PrivateDependencyModuleNames.Add("VulkanRHI");
            PublicIncludePathModuleNames.Add("Vulkan");
        }

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "D3D11RHI",
                "D3D12RHI"
            });
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11", "DX12");
        }

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "EditorStyle",
                "ToolMenus",
                "Projects",
                "UnrealEd"
            });
        }

        PublicDefinitions.AddRange(new string[]
        {
            "REALGAZEBO_STREAMING_ENABLED=1",
            "REALGAZEBO_STREAMING_DEBUG=0",
            "RTSP_DEFAULT_PORT=8554",
            "MAX_CONCURRENT_STREAMS=32",
            "DEFAULT_FRAME_POOL_SIZE=3"
        });

        if (Target.Configuration == UnrealTargetConfiguration.Debug ||
            Target.Configuration == UnrealTargetConfiguration.DebugGame)
        {
            PublicDefinitions.Remove("REALGAZEBO_STREAMING_DEBUG=0");
            PublicDefinitions.Add("REALGAZEBO_STREAMING_DEBUG=1");
        }

        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
        bEnableUndefinedIdentifierWarnings = false;
        bEnableExceptions = false;
    }
}
