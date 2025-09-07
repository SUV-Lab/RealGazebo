#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

// Core streaming types
#include "StreamingTypes.h"

// Forward declarations
class UStreamingSubsystem;
class FAdvancedRTSPStreamer;

DECLARE_LOG_CATEGORY_EXTERN(LogRealGazeboStreaming, Log, All);

/**
 * RealGazebo Streaming Module
 * 
 * High-performance multi-camera RTSP streaming system for RealGazebo unmanned vehicles.
 * 
 * Key Features:
 * - Multi-vehicle, multi-camera streaming support
 * - Integration with RealGazebo vehicle system  
 * - High-performance capture using PixelCapture
 * - GStreamer-based RTSP streaming with hardware acceleration
 * - DataTable-driven camera configuration
 * - Performance monitoring and optimization
 */
class REALGAZEBOSTREAMING_API FRealGazeboStreamingModule : public IModuleInterface
{
public:
    //----------------------------------------------------------
    // Module Interface
    //----------------------------------------------------------
    
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /** Check if the module supports dynamic reloading */
    virtual bool SupportsDynamicReloading() override;

    //----------------------------------------------------------
    // Module Access
    //----------------------------------------------------------

    /**
     * Singleton-like access to this module's interface.
     * @return Returns singleton instance, loading the module on demand if needed
     */
    static inline FRealGazeboStreamingModule& Get()
    {
        return FModuleManager::LoadModuleChecked<FRealGazeboStreamingModule>("RealGazeboStreaming");
    }

    /**
     * Checks to see if this module is loaded and ready to use.
     * @return True if the module is loaded and ready to use
     */
    static inline bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("RealGazeboStreaming");
    }

    //----------------------------------------------------------
    // Module Services
    //----------------------------------------------------------

    /**
     * Get the streaming subsystem instance
     * @param WorldContext Context to get the subsystem from
     * @return Streaming subsystem instance or nullptr if not available
     */
    UStreamingSubsystem* GetStreamingSubsystem(const UObject* WorldContext) const;

    /**
     * Create a new RTSP streamer instance
     * @return Shared pointer to new streamer instance
     */
    TSharedPtr<FAdvancedRTSPStreamer> CreateRTSPStreamer() const;

    /**
     * Check if GStreamer is properly initialized and available
     * @return True if GStreamer is available for streaming
     */
    bool IsGStreamerAvailable() const;

    /**
     * Get GStreamer version information
     * @return Version string or error message if not available
     */
    FString GetGStreamerVersion() const;

    /**
     * Check if hardware-accelerated encoding is available
     * @param Codec Video codec to check (e.g., "H264", "H265")
     * @return True if hardware encoding is available for the codec
     */
    bool IsHardwareEncodingAvailable(const FString& Codec) const;

    //----------------------------------------------------------
    // Performance Monitoring
    //----------------------------------------------------------

    /**
     * Get global streaming performance statistics
     * @return Current performance statistics across all streams
     */
    FStreamingPerformanceStats GetGlobalPerformanceStats() const;

    /**
     * Enable or disable debug logging for streaming operations
     * @param bEnabled True to enable debug logging
     */
    void SetDebugLogging(bool bEnabled);

    //----------------------------------------------------------
    // Module Events
    //----------------------------------------------------------

    /** Called when the streaming system is fully initialized */
    DECLARE_MULTICAST_DELEGATE(FOnStreamingSystemReady);
    FOnStreamingSystemReady OnStreamingSystemReady;

    /** Called when a critical streaming error occurs */
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnStreamingSystemError, const FString& /* Error */);
    FOnStreamingSystemError OnStreamingSystemError;

    /** Called when GStreamer initialization status changes */
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnGStreamerStatusChanged, bool /* bAvailable */);
    FOnGStreamerStatusChanged OnGStreamerStatusChanged;

protected:
    //----------------------------------------------------------
    // Module Initialization
    //----------------------------------------------------------

    /** Initialize GStreamer subsystem */
    bool InitializeGStreamer();

    /** Initialize streaming subsystem integration */
    void InitializeStreamingIntegration();

    /** Initialize editor integration (if in editor) */
    void InitializeEditorIntegration();

    /** Cleanup all streaming resources */
    void CleanupStreamingResources();

    /** Register console commands */
    void RegisterConsoleCommands();

    /** Unregister console commands */
    void UnregisterConsoleCommands();

private:
    //----------------------------------------------------------
    // Module State
    //----------------------------------------------------------

    /** Is GStreamer initialized and available */
    bool bIsGStreamerInitialized = false;

    /** Is the module fully initialized */
    bool bIsModuleReady = false;

    /** Debug logging enabled */
    bool bDebugLoggingEnabled = false;

    //----------------------------------------------------------
    // Console Commands
    //----------------------------------------------------------

    /** Console command to list all active streams */
    void ExecuteListStreamsCommand(const TArray<FString>& Args);

    /** Console command to start/stop streaming for a vehicle */
    void ExecuteVehicleStreamingCommand(const TArray<FString>& Args);

    /** Console command to show streaming performance stats */
    void ExecuteStreamingStatsCommand(const TArray<FString>& Args);

    /** Console command to test GStreamer functionality */
    void ExecuteGStreamerTestCommand(const TArray<FString>& Args);

    /** Console command pointers for cleanup */
    TArray<class FAutoConsoleCommand*> ConsoleCommands;

    //----------------------------------------------------------
    // Performance Monitoring
    //----------------------------------------------------------

    /** Update global performance statistics */
    void UpdateGlobalPerformanceStats();

    /** Performance monitoring timer */
    FTimerHandle PerformanceTimerHandle;

    /** Cached performance statistics */
    mutable FStreamingPerformanceStats CachedPerformanceStats;
    mutable double LastPerformanceUpdate = 0.0;
    
    /** Test streamer for console command testing */
    TSharedPtr<class FAdvancedRTSPStreamer> TestStreamer;
};

//----------------------------------------------------------
// Module Utility Macros
//----------------------------------------------------------

/** Get the RealGazebo Streaming module instance */
#define GET_REALGAZEBO_STREAMING_MODULE() \
    (FRealGazeboStreamingModule::IsAvailable() ? &FRealGazeboStreamingModule::Get() : nullptr)

/** Check if streaming is available in the current context */
#define IS_STREAMING_AVAILABLE() \
    (FRealGazeboStreamingModule::IsAvailable() && FRealGazeboStreamingModule::Get().IsGStreamerAvailable())

/** Get streaming subsystem from world context */
#define GET_STREAMING_SUBSYSTEM(WorldContext) \
    (IS_STREAMING_AVAILABLE() ? FRealGazeboStreamingModule::Get().GetStreamingSubsystem(WorldContext) : nullptr)