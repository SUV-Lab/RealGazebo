#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"
#include "StreamingTypes.h"

// GStreamer integration
#include "GStreamerWrapper.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAdvancedStreamingStatusChanged, bool);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnStreamingPerformanceUpdate, float /*FPS*/, float /*Bitrate*/, int32 /*Clients*/);
// FOnStreamingError already declared in StreamingTypes.h

/**
 * Advanced high-performance RTSP streamer using GStreamer
 * 
 * Features:
 * - Multi-codec support (H.264, H.265, VP8, VP9)
 * - Hardware acceleration when available
 * - Adaptive bitrate based on client connections
 * - Frame buffering and drop recovery
 * - Performance monitoring and statistics
 * - Multi-client support with individual stream quality
 */
class REALGAZEBOSTREAMING_API FAdvancedRTSPStreamer : public FRunnable
{
public:
    FAdvancedRTSPStreamer();
    virtual ~FAdvancedRTSPStreamer();

    //----------------------------------------------------------
    // Streaming Control
    //----------------------------------------------------------

    /** Start RTSP streaming with advanced configuration */
    bool StartStreaming(const FCameraStreamConfig& Config);

    /** Stop streaming and cleanup resources */
    void StopStreaming();

    /** Check if streaming is active */
    bool IsStreaming() const;

    /** Get current stream URL */
    FString GetStreamURL() const;

    //----------------------------------------------------------
    // Frame Processing
    //----------------------------------------------------------

    /** Push frame data to streaming pipeline */
    bool PushFrame(const TArray<uint8>& FrameData, int32 Width, int32 Height, int32 Channels = 3);

    /** Push optimized I420 frame data */
    bool PushI420Frame(const uint8* YPlane, const uint8* UPlane, const uint8* VPlane, 
                       int32 Width, int32 Height, int32 YStride, int32 UVStride);

    /** Set frame rate dynamically */
    void SetFrameRate(float NewFrameRate);

    /** Set bitrate dynamically */
    void SetBitrate(int32 NewBitrate);

    //----------------------------------------------------------
    // Performance and Statistics
    //----------------------------------------------------------

    /** Get current streaming statistics */
    struct FStreamingStats
    {
        float CurrentFPS = 0.0f;
        float CurrentBitrate = 0.0f;
        int32 ConnectedClients = 0;
        int32 TotalFramesSent = 0;
        int32 DroppedFrames = 0;
        float AverageLatency = 0.0f;
        float BufferUtilization = 0.0f;
    };

    FStreamingStats GetStreamingStats() const;

    /** Enable/disable hardware acceleration */
    void SetHardwareAcceleration(bool bEnabled);

    /** Set maximum number of clients */
    void SetMaxClients(int32 MaxClients);

    //----------------------------------------------------------
    // Events
    //----------------------------------------------------------

    FOnAdvancedStreamingStatusChanged OnStreamingStatusChanged;
    FOnStreamingPerformanceUpdate OnPerformanceUpdate;
    FOnStreamingError OnStreamingError;

    //----------------------------------------------------------
    // FRunnable Interface
    //----------------------------------------------------------

    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

private:
    //----------------------------------------------------------
    // GStreamer Pipeline Management
    //----------------------------------------------------------

    bool InitializeGStreamerPipeline();
    void CleanupGStreamerPipeline();
    bool CreatePipelineElements();
    bool LinkPipelineElements();
    void ConfigureEncoder();
    void ConfigureRTSPServer();

    //----------------------------------------------------------
    // Frame Buffer Management
    //----------------------------------------------------------

    struct FFrameData
    {
        TArray<uint8> Data;
        int32 Width;
        int32 Height;
        int32 Channels;
        double Timestamp;
        
        FFrameData() : Width(0), Height(0), Channels(0), Timestamp(0.0) {}
    };

    /** Thread-safe frame queue */
    TQueue<FFrameData> FrameQueue;
    static const int32 MaxFrameQueueSize = 10;

    /** Process queued frames */
    void ProcessFrameQueue();

    /** Send frame to GStreamer pipeline */
    void SendFrameToGStreamer(const FFrameData& Frame);

    //----------------------------------------------------------
    // Performance Monitoring
    //----------------------------------------------------------

    void UpdatePerformanceStats();
    void ResetPerformanceCounters();

    mutable FCriticalSection StatsMutex;
    FStreamingStats CurrentStats;
    double LastStatsUpdate = 0.0;
    double LastFrameTime = 0.0;
    int32 FramesThisSecond = 0;
    double LastFPSUpdate = 0.0;

    //----------------------------------------------------------
    // GStreamer Event Handlers
    //----------------------------------------------------------

#if WITH_GSTREAMER
    static void OnClientConnected(GstRTSPServer* Server, GstRTSPClient* Client, gpointer UserData);
    static void OnClientDisconnected(GstRTSPServer* Server, GstRTSPClient* Client, gpointer UserData);
    static void OnMediaConfigure(GstRTSPMediaFactory* Factory, GstRTSPMedia* Media, gpointer UserData);
    static gboolean OnBusMessage(GstBus* Bus, GstMessage* Message, gpointer UserData);
    static void OnNeedData(GstElement* AppSrc, guint Unused, gpointer UserData);
    static void OnEnoughData(GstElement* AppSrc, gpointer UserData);
    static gboolean OnSeekData(GstElement* AppSrc, guint64 Offset, gpointer UserData);

    /** GStreamer pipeline elements */
    GstRTSPServer* RTSPServer;
    GstRTSPMountPoints* MountPoints;
    GstRTSPMediaFactory* Factory;
    GstElement* Pipeline;
    GstElement* AppSrc;
    GstElement* VideoConvert;
    GstElement* VideoEncoder;
    GstElement* RTPPayloader;
    GstElement* RTSPSink;
    GstBus* Bus;
    GMainLoop* MainLoop;
    GMainContext* MainContext;
    guint BusWatchId;
    guint SourceID;

    /** Dynamic pipeline reconfiguration */
    void ReconfigurePipeline();
    bool CreateOptimizedEncoder();
    void UpdateEncoderSettings();
#endif

    //----------------------------------------------------------
    // Threading and State Management
    //----------------------------------------------------------

    FRunnableThread* StreamingThread;
    FThreadSafeBool bShouldStop;
    FThreadSafeBool bIsStreaming;
    FThreadSafeBool bIsInitialized;

    //----------------------------------------------------------
    // Configuration
    //----------------------------------------------------------

    FCameraStreamConfig StreamConfig;
    FString StreamURL;
    
    /** Advanced settings */
    bool bHardwareAccelerationEnabled = true;
    bool bAdaptiveBitrate = true;
    int32 MaxClients = 10;
    float KeyFrameInterval = 2.0f;
    int32 BitrateWindow = 5000; // 5 second window for bitrate adaptation

    //----------------------------------------------------------
    // Client Management
    //----------------------------------------------------------

    struct FClientInfo
    {
        FString ClientIP;
        double ConnectTime;
        int32 FramesSent;
        float PreferredBitrate;
    };

    TArray<FClientInfo> ConnectedClients;
    mutable FCriticalSection ClientsMutex;

    void OnClientConnectedInternal(const FString& ClientIP);
    void OnClientDisconnectedInternal(const FString& ClientIP);
    void UpdateClientStats();
    int32 CalculateOptimalBitrate() const;

    //----------------------------------------------------------
    // Error Handling
    //----------------------------------------------------------

    void HandleGStreamerError(const FString& Error);
    void HandlePipelineStateChange(GstState OldState, GstState NewState);
    bool RecoverFromError();

    /** Error recovery settings */
    int32 MaxRetryAttempts = 3;
    int32 CurrentRetryAttempt = 0;
    double LastErrorTime = 0.0;

    //----------------------------------------------------------
    // Utility Methods
    //----------------------------------------------------------

    FString GenerateStreamURL() const;
    bool ValidateConfiguration() const;
    void LogPipelineInfo() const;
    
    /** Convert RGB to I420 if needed */
    bool ConvertRGBToI420(const TArray<uint8>& RGBData, int32 Width, int32 Height, TArray<uint8>& OutI420Data);

    /** Platform-specific optimizations */
    void EnablePlatformOptimizations();
    FString GetOptimalVideoCodec() const;
    bool IsHardwareEncodingAvailable(const FString& Codec) const;
};