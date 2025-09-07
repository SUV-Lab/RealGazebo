#include "RealGazeboStreaming.h"
#include "Core/StreamingSubsystem.h"
#include "Streaming/AdvancedRTSPStreamer.h" 
#include "Integration/GStreamerWrapper.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/ConsoleManager.h"
#include "Misc/DateTime.h"

DEFINE_LOG_CATEGORY(LogRealGazeboStreaming);

#define LOCTEXT_NAMESPACE "FRealGazeboStreamingModule"

//----------------------------------------------------------
// Module Implementation
//----------------------------------------------------------

void FRealGazeboStreamingModule::StartupModule()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Starting RealGazebo Streaming Module"));

    // Initialize GStreamer first - TEMPORARILY DISABLED due to version mismatch
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("GStreamer initialization DISABLED - version mismatch with UE 5.1"));
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Need GStreamer 1.18.x or 1.20.x for UE 5.1 compatibility"));
    OnGStreamerStatusChanged.Broadcast(false);
    
    // TODO: Re-enable when compatible GStreamer version is installed
    /*
    if (!InitializeGStreamer())
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("GStreamer initialization failed - streaming will not be available"));
        OnGStreamerStatusChanged.Broadcast(false);
    }
    else
    {
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("GStreamer initialized successfully: %s"), *GetGStreamerVersion());
        OnGStreamerStatusChanged.Broadcast(true);
    }
    */

    // Initialize streaming integration
    InitializeStreamingIntegration();

    // Initialize editor integration if in editor
#if WITH_EDITOR
    InitializeEditorIntegration();
#endif

    // Register console commands
    RegisterConsoleCommands();

    // Set up performance monitoring
    // TODO: Implement performance monitoring with proper timer system
    // if (GEngine)
    // {
    //     GEngine->GetTimerManager().SetTimer(
    //         PerformanceTimerHandle,
    //         this,
    //         &FRealGazeboStreamingModule::UpdateGlobalPerformanceStats,
    //         1.0f, // Update every second
    //         true  // Loop
    //     );
    // }

    bIsModuleReady = true;
    OnStreamingSystemReady.Broadcast();

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RealGazebo Streaming Module started successfully"));
}

void FRealGazeboStreamingModule::ShutdownModule()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Shutting down RealGazebo Streaming Module"));

    bIsModuleReady = false;

    // Clear performance timer
    // TODO: Clear timer when implemented
    // if (GEngine && PerformanceTimerHandle.IsValid())
    // {
    //     GEngine->GetTimerManager().ClearTimer(PerformanceTimerHandle);
    // }

    // Unregister console commands
    UnregisterConsoleCommands();

    // Cleanup streaming resources
    CleanupStreamingResources();

    // Cleanup GStreamer
    if (bIsGStreamerInitialized)
    {
        RealGazebo::GStreamerUtils::DeinitializeGStreamer();
        bIsGStreamerInitialized = false;
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RealGazebo Streaming Module shutdown complete"));
}

bool FRealGazeboStreamingModule::SupportsDynamicReloading()
{
    return false; // GStreamer doesn't support dynamic reloading well
}

//----------------------------------------------------------
// Module Services
//----------------------------------------------------------

UStreamingSubsystem* FRealGazeboStreamingModule::GetStreamingSubsystem(const UObject* WorldContext) const
{
    if (!WorldContext)
    {
        return nullptr;
    }

    if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
    {
        return World->GetGameInstance()->GetSubsystem<UStreamingSubsystem>();
    }

    return nullptr;
}

TSharedPtr<FAdvancedRTSPStreamer> FRealGazeboStreamingModule::CreateRTSPStreamer() const
{
    if (!bIsGStreamerInitialized)
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Cannot create RTSP streamer - GStreamer not initialized"));
        return nullptr;
    }

    return MakeShared<FAdvancedRTSPStreamer>();
}

bool FRealGazeboStreamingModule::IsGStreamerAvailable() const
{
    return bIsGStreamerInitialized && RealGazebo::GStreamerUtils::IsGStreamerInitialized();
}

FString FRealGazeboStreamingModule::GetGStreamerVersion() const
{
    if (bIsGStreamerInitialized)
    {
        return RealGazebo::GStreamerUtils::GetGStreamerVersion();
    }
    return TEXT("GStreamer not available");
}

bool FRealGazeboStreamingModule::IsHardwareEncodingAvailable(const FString& Codec) const
{
    if (!bIsGStreamerInitialized)
    {
        return false;
    }

    return RealGazebo::GStreamerUtils::IsHardwareEncodingAvailable(Codec);
}

//----------------------------------------------------------
// Performance Monitoring  
//----------------------------------------------------------

FStreamingPerformanceStats FRealGazeboStreamingModule::GetGlobalPerformanceStats() const
{
    // Update stats if needed
    const double CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime - LastPerformanceUpdate > 1.0) // Update every second
    {
        // UpdateGlobalPerformanceStats(); // TODO: Make const or remove from const function
        LastPerformanceUpdate = CurrentTime;
    }

    return CachedPerformanceStats;
}

void FRealGazeboStreamingModule::SetDebugLogging(bool bEnabled)
{
    bDebugLoggingEnabled = bEnabled;
    
    if (bEnabled)
    {
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Debug logging enabled for RealGazebo Streaming"));
    }
    else
    {
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Debug logging disabled for RealGazebo Streaming"));
    }
}

//----------------------------------------------------------
// Module Initialization
//----------------------------------------------------------

bool FRealGazeboStreamingModule::InitializeGStreamer()
{
#if WITH_GSTREAMER
    if (RealGazebo::GStreamerUtils::InitializeGStreamer())
    {
        bIsGStreamerInitialized = true;
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("GStreamer initialized: %s"), 
            *RealGazebo::GStreamerUtils::GetGStreamerVersion());
        return true;
    }
    else
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to initialize GStreamer"));
        return false;
    }
#else
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("GStreamer support not compiled in"));
    return false;
#endif
}

void FRealGazeboStreamingModule::InitializeStreamingIntegration()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Initializing streaming subsystem integration"));
    
    // The streaming subsystem will be automatically created by UE's subsystem manager
    // when the first world/game instance requests it
}

void FRealGazeboStreamingModule::InitializeEditorIntegration()
{
#if WITH_EDITOR
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Initializing editor integration"));
    
    // Editor-specific initialization could go here
    // For example, custom editor tools or streaming previews
#endif
}

void FRealGazeboStreamingModule::CleanupStreamingResources()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Cleaning up streaming resources"));
    
    // Streaming subsystems will be cleaned up automatically by UE's subsystem manager
}

//----------------------------------------------------------
// Console Commands
//----------------------------------------------------------

void FRealGazeboStreamingModule::RegisterConsoleCommands()
{
    // List all active streams
    ConsoleCommands.Add(new FAutoConsoleCommand(
        TEXT("RealGazebo.Streaming.List"),
        TEXT("List all active camera streams"),
        FConsoleCommandWithArgsDelegate::CreateRaw(this, &FRealGazeboStreamingModule::ExecuteListStreamsCommand)
    ));

    // Vehicle streaming control
    ConsoleCommands.Add(new FAutoConsoleCommand(
        TEXT("RealGazebo.Streaming.Vehicle"),
        TEXT("Control streaming for a vehicle. Usage: RealGazebo.Streaming.Vehicle <VehicleID> <start|stop>"),
        FConsoleCommandWithArgsDelegate::CreateRaw(this, &FRealGazeboStreamingModule::ExecuteVehicleStreamingCommand)
    ));

    // Performance statistics
    ConsoleCommands.Add(new FAutoConsoleCommand(
        TEXT("RealGazebo.Streaming.Stats"),
        TEXT("Show streaming performance statistics"),
        FConsoleCommandWithArgsDelegate::CreateRaw(this, &FRealGazeboStreamingModule::ExecuteStreamingStatsCommand)
    ));

    // GStreamer test
    ConsoleCommands.Add(new FAutoConsoleCommand(
        TEXT("RealGazebo.Streaming.TestGStreamer"),
        TEXT("Test GStreamer functionality"),
        FConsoleCommandWithArgsDelegate::CreateRaw(this, &FRealGazeboStreamingModule::ExecuteGStreamerTestCommand)
    ));

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Registered %d console commands"), ConsoleCommands.Num());
}

void FRealGazeboStreamingModule::UnregisterConsoleCommands()
{
    for (FAutoConsoleCommand* Command : ConsoleCommands)
    {
        delete Command;
    }
    ConsoleCommands.Empty();
}

void FRealGazeboStreamingModule::ExecuteListStreamsCommand(const TArray<FString>& Args)
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("=== Active Camera Streams ==="));
    
    // This would iterate through all active streams
    // Implementation would need access to the streaming subsystem
    if (GEngine && GEngine->GetWorld())
    {
        if (UStreamingSubsystem* StreamingSystem = GetStreamingSubsystem(GEngine->GetWorld()))
        {
            TArray<FStreamID> ActiveStreams = StreamingSystem->GetAllActiveStreamIDs();
            UE_LOG(LogRealGazeboStreaming, Log, TEXT("Found %d active streams:"), ActiveStreams.Num());
            
            for (const FStreamID& StreamID : ActiveStreams)
            {
                FString StreamURL = StreamingSystem->GetStreamURL(StreamID);
                bool bIsStreaming = StreamingSystem->IsCameraStreaming(StreamID);
                
                UE_LOG(LogRealGazeboStreaming, Log, TEXT("  %s: %s [%s]"), 
                    *StreamID.ToString(), 
                    *StreamURL,
                    bIsStreaming ? TEXT("ACTIVE") : TEXT("INACTIVE"));
            }
        }
    }
}

void FRealGazeboStreamingModule::ExecuteVehicleStreamingCommand(const TArray<FString>& Args)
{
    if (Args.Num() < 2)
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Usage: RealGazebo.Streaming.Vehicle <VehicleID> <start|stop>"));
        return;
    }

    // Parse vehicle ID and action
    FString VehicleIDStr = Args[0];
    FString Action = Args[1].ToLower();

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Vehicle streaming command: %s %s"), *VehicleIDStr, *Action);
    
    // Implementation would parse the vehicle ID and call appropriate streaming functions
}

void FRealGazeboStreamingModule::ExecuteStreamingStatsCommand(const TArray<FString>& Args)
{
    FStreamingPerformanceStats Stats = GetGlobalPerformanceStats();
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("=== Streaming Performance Statistics ==="));
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Active Cameras: %d"), Stats.TotalActiveCameras);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Active Streams: %d"), Stats.TotalActiveStreams);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Average FPS: %.2f"), Stats.AverageFrameRate);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Memory Usage: %.2f MB"), Stats.TotalMemoryUsageMB);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Dropped Frames: %d"), Stats.DroppedFrames);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Network Bandwidth: %.2f Mbps"), Stats.NetworkBandwidthMbps);
}

void FRealGazeboStreamingModule::ExecuteGStreamerTestCommand(const TArray<FString>& Args)
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("=== GStreamer Status ==="));
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Available: %s"), IsGStreamerAvailable() ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Version: %s"), *GetGStreamerVersion());
    
    // Test hardware encoding availability
    TArray<FString> Codecs = {TEXT("H264"), TEXT("H265"), TEXT("VP8"), TEXT("VP9")};
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Hardware Encoding Support:"));
    
    for (const FString& Codec : Codecs)
    {
        bool bSupported = IsHardwareEncodingAvailable(Codec);
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("  %s: %s"), *Codec, bSupported ? TEXT("YES") : TEXT("NO"));
    }
    
    // Create a test pattern stream
    UE_LOG(LogRealGazeboStreaming, Log, TEXT(""));
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("=== Starting Test Pattern Stream ==="));
    
#if WITH_GSTREAMER
    // Check if we should stop existing test stream
    if (Args.Num() > 0 && Args[0].ToLower() == TEXT("stop"))
    {
        if (TestStreamer.IsValid())
        {
            UE_LOG(LogRealGazeboStreaming, Log, TEXT("Stopping test stream..."));
            TestStreamer->StopStreaming();
            TestStreamer.Reset();
            UE_LOG(LogRealGazeboStreaming, Log, TEXT("Test stream stopped"));
        }
        else
        {
            UE_LOG(LogRealGazeboStreaming, Warning, TEXT("No test stream to stop"));
        }
        return;
    }
    
    // Create test configuration
    FCameraStreamConfig TestConfig;
    TestConfig.CameraName = TEXT("TestPattern");
    TestConfig.StreamPath = TEXT("/test");
    TestConfig.StreamPort = 8554;
    TestConfig.FrameRate = 30.0f;
    TestConfig.StreamResolution = FIntPoint(1280, 720);
    TestConfig.Bitrate = 2000;
    
    // Create and start test streamer
    if (!TestStreamer.IsValid())
    {
        TestStreamer = MakeShared<FAdvancedRTSPStreamer>();
    }
    
    if (TestStreamer->StartStreaming(TestConfig))
    {
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Test pattern stream started successfully!"));
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("View with VLC: vlc rtsp://localhost:8554/test"));
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("View with FFplay: ffplay rtsp://localhost:8554/test"));
        UE_LOG(LogRealGazeboStreaming, Log, TEXT(""));
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("To stop: RealGazebo.Streaming.TestGStreamer stop"));
    }
    else
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to start test pattern stream"));
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("   Check that GStreamer is properly installed"));
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("   and port 8554 is not in use"));
    }
#else
    UE_LOG(LogRealGazeboStreaming, Error, TEXT("GStreamer not compiled in - cannot create test stream"));
#endif
}

void FRealGazeboStreamingModule::UpdateGlobalPerformanceStats()
{
    // This would collect stats from all active streaming subsystems
    // For now, provide default/placeholder values
    
    CachedPerformanceStats.TotalActiveCameras = 0;
    CachedPerformanceStats.TotalActiveStreams = 0;
    CachedPerformanceStats.AverageFrameRate = 0.0f;
    CachedPerformanceStats.TotalMemoryUsageMB = 0.0f;
    CachedPerformanceStats.DroppedFrames = 0;
    CachedPerformanceStats.NetworkBandwidthMbps = 0.0f;

    // Collect stats from active worlds
    if (GEngine)
    {
        for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
        {
            if (WorldContext.World() && WorldContext.World()->GetGameInstance())
            {
                if (UStreamingSubsystem* StreamingSystem = WorldContext.World()->GetGameInstance()->GetSubsystem<UStreamingSubsystem>())
                {
                    FStreamingPerformanceStats WorldStats = StreamingSystem->GetPerformanceStats();
                    
                    // Accumulate stats
                    CachedPerformanceStats.TotalActiveCameras += WorldStats.TotalActiveCameras;
                    CachedPerformanceStats.TotalActiveStreams += WorldStats.TotalActiveStreams;
                    CachedPerformanceStats.TotalMemoryUsageMB += WorldStats.TotalMemoryUsageMB;
                    CachedPerformanceStats.DroppedFrames += WorldStats.DroppedFrames;
                    CachedPerformanceStats.NetworkBandwidthMbps += WorldStats.NetworkBandwidthMbps;
                }
            }
        }

        // Calculate average FPS
        if (CachedPerformanceStats.TotalActiveStreams > 0)
        {
            // This is a simplified calculation - in reality you'd need to weight by stream activity
            CachedPerformanceStats.AverageFrameRate /= CachedPerformanceStats.TotalActiveStreams;
        }
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRealGazeboStreamingModule, RealGazeboStreaming)