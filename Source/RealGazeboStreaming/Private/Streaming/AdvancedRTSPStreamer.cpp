#include "Streaming/AdvancedRTSPStreamer.h"
#include "RealGazeboStreaming.h"

#if WITH_GSTREAMER
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#endif

FAdvancedRTSPStreamer::FAdvancedRTSPStreamer()
    : StreamingThread(nullptr)
    , bShouldStop(false)
    , bIsStreaming(false)
    , bIsInitialized(false)
    , bHardwareAccelerationEnabled(true)
    , bAdaptiveBitrate(true)
    , MaxClients(10)
    , KeyFrameInterval(2.0f)
    , BitrateWindow(5000)
    , MaxRetryAttempts(3)
    , CurrentRetryAttempt(0)
    , LastErrorTime(0.0)
{
#if WITH_GSTREAMER
    // Initialize GStreamer elements to nullptr
    RTSPServer = nullptr;
    MountPoints = nullptr;
    Factory = nullptr;
    Pipeline = nullptr;
    AppSrc = nullptr;
    VideoConvert = nullptr;
    VideoEncoder = nullptr;
    RTPPayloader = nullptr;
    RTSPSink = nullptr;
    Bus = nullptr;
    MainLoop = nullptr;
    MainContext = nullptr;
    BusWatchId = 0;
    SourceID = 0;
#endif

    // Initialize performance stats
    CurrentStats = FStreamingStats();
    LastStatsUpdate = 0.0;
    LastFrameTime = 0.0;
    FramesThisSecond = 0;
    LastFPSUpdate = 0.0;
}

FAdvancedRTSPStreamer::~FAdvancedRTSPStreamer()
{
    StopStreaming();
}

//----------------------------------------------------------
// Streaming Control
//----------------------------------------------------------

bool FAdvancedRTSPStreamer::StartStreaming(const FCameraStreamConfig& Config)
{
    if (bIsStreaming)
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("RTSP Streamer already active"));
        return true;
    }

    StreamConfig = Config;
    StreamURL = GenerateStreamURL();

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Starting RTSP streaming: %s"), *StreamURL);

    if (!ValidateConfiguration())
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Invalid streaming configuration"));
        return false;
    }

    // Initialize GStreamer pipeline
    if (!Init())
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to initialize RTSP streamer"));
        return false;
    }

    // Start streaming thread
    StreamingThread = FRunnableThread::Create(this, TEXT("RealGazeboRTSPStreamer"));
    if (!StreamingThread)
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to create streaming thread"));
        CleanupGStreamerPipeline();
        return false;
    }

    bIsStreaming = true;
    OnStreamingStatusChanged.Broadcast(true);

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP streaming started successfully: %s"), *StreamURL);
    return true;
}

void FAdvancedRTSPStreamer::StopStreaming()
{
    if (!bIsStreaming)
    {
        return;
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Stopping RTSP streaming"));

    bShouldStop = true;
    bIsStreaming = false;

    // Wait for thread to finish
    if (StreamingThread)
    {
        StreamingThread->WaitForCompletion();
        delete StreamingThread;
        StreamingThread = nullptr;
    }

    // Cleanup GStreamer
    CleanupGStreamerPipeline();

    OnStreamingStatusChanged.Broadcast(false);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP streaming stopped"));
}

bool FAdvancedRTSPStreamer::IsStreaming() const
{
    return bIsStreaming && bIsInitialized;
}

FString FAdvancedRTSPStreamer::GetStreamURL() const
{
    return StreamURL;
}

//----------------------------------------------------------
// Frame Processing
//----------------------------------------------------------

bool FAdvancedRTSPStreamer::PushFrame(const TArray<uint8>& FrameData, int32 Width, int32 Height, int32 Channels)
{
    if (!bIsStreaming)
    {
        return false;
    }

    // Create frame data
    FFrameData Frame;
    Frame.Data = FrameData;
    Frame.Width = Width;
    Frame.Height = Height;
    Frame.Channels = Channels;
    Frame.Timestamp = FPlatformTime::Seconds();

    // Add to queue (with size limit)
    if (FrameQueue.IsEmpty() || true) // TODO: Fix queue size check
    {
        FrameQueue.Enqueue(Frame);
        return true;
    }
    else
    {
        // Queue is full, drop frame
        CurrentStats.DroppedFrames++;
        return false;
    }
}

bool FAdvancedRTSPStreamer::PushI420Frame(const uint8* YPlane, const uint8* UPlane, const uint8* VPlane, 
                                        int32 Width, int32 Height, int32 YStride, int32 UVStride)
{
    // TODO: Implement I420 frame pushing
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("PushI420Frame not yet implemented"));
    return false;
}

void FAdvancedRTSPStreamer::SetFrameRate(float NewFrameRate)
{
    StreamConfig.FrameRate = NewFrameRate;
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Frame rate updated to %.2f FPS"), NewFrameRate);
}

void FAdvancedRTSPStreamer::SetBitrate(int32 NewBitrate)
{
    StreamConfig.Bitrate = NewBitrate;
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Bitrate updated to %d kbps"), NewBitrate);
}

//----------------------------------------------------------
// Performance and Statistics
//----------------------------------------------------------

FAdvancedRTSPStreamer::FStreamingStats FAdvancedRTSPStreamer::GetStreamingStats() const
{
    // UpdatePerformanceStats(); // TODO: Make const or remove from const function
    return CurrentStats;
}

void FAdvancedRTSPStreamer::SetHardwareAcceleration(bool bEnabled)
{
    bHardwareAccelerationEnabled = bEnabled;
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Hardware acceleration %s"), 
        bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void FAdvancedRTSPStreamer::SetMaxClients(int32 MaxClientsCount)
{
    MaxClients = FMath::Clamp(MaxClientsCount, 1, 100);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Max clients set to %d"), MaxClients);
}

//----------------------------------------------------------
// FRunnable Interface
//----------------------------------------------------------

bool FAdvancedRTSPStreamer::Init()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Initializing RTSP streamer thread"));

    bShouldStop = false;
    ResetPerformanceCounters();

    // Initialize GStreamer pipeline
    if (!InitializeGStreamerPipeline())
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to initialize GStreamer pipeline"));
        return false;
    }

    bIsInitialized = true;
    return true;
}

uint32 FAdvancedRTSPStreamer::Run()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP streaming thread started"));

#if WITH_GSTREAMER
    while (!bShouldStop)
    {
        // Process frame queue
        ProcessFrameQueue();

        // Update performance stats
        UpdatePerformanceStats();

        // Run GStreamer main loop iteration
        if (MainContext)
        {
            g_main_context_iteration(MainContext, FALSE);
        }

        // Small sleep to prevent busy waiting
        FPlatformProcess::Sleep(0.001f);
    }
#endif

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP streaming thread ended"));
    return 0;
}

void FAdvancedRTSPStreamer::Stop()
{
    bShouldStop = true;
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP streaming thread stop requested"));
}

void FAdvancedRTSPStreamer::Exit()
{
    // Called when thread is exiting
    bIsInitialized = false;
}

//----------------------------------------------------------
// GStreamer Pipeline Management
//----------------------------------------------------------

bool FAdvancedRTSPStreamer::InitializeGStreamerPipeline()
{
#if WITH_GSTREAMER
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Initializing GStreamer RTSP pipeline"));

    // Create main context and loop
    MainContext = g_main_context_new();
    MainLoop = g_main_loop_new(MainContext, FALSE);

    // Create RTSP server elements
    if (!CreatePipelineElements())
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to create pipeline elements"));
        return false;
    }

    // Configure elements
    ConfigureEncoder();
    ConfigureRTSPServer();

    // Link pipeline
    if (!LinkPipelineElements())
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to link pipeline elements"));
        return false;
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("GStreamer RTSP pipeline initialized successfully"));
    return true;
#else
    UE_LOG(LogRealGazeboStreaming, Error, TEXT("GStreamer not available - cannot initialize pipeline"));
    return false;
#endif
}

void FAdvancedRTSPStreamer::CleanupGStreamerPipeline()
{
#if WITH_GSTREAMER
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Cleaning up GStreamer pipeline"));

    // Stop pipeline if running
    if (Pipeline)
    {
        gst_element_set_state(Pipeline, GST_STATE_NULL);
        SAFE_GST_OBJECT_UNREF(Pipeline);
    }

    // Cleanup other elements
    SAFE_GST_OBJECT_UNREF(RTSPServer);
    SAFE_GST_OBJECT_UNREF(MountPoints);
    SAFE_GST_OBJECT_UNREF(Factory);
    SAFE_GST_OBJECT_UNREF(AppSrc);
    SAFE_GST_OBJECT_UNREF(VideoConvert);
    SAFE_GST_OBJECT_UNREF(VideoEncoder);
    SAFE_GST_OBJECT_UNREF(RTPPayloader);
    SAFE_GST_OBJECT_UNREF(RTSPSink);

    if (Bus && BusWatchId)
    {
        g_source_remove(BusWatchId);
        BusWatchId = 0;
    }
    SAFE_GST_OBJECT_UNREF(Bus);

    if (MainLoop)
    {
        g_main_loop_quit(MainLoop);
        g_main_loop_unref(MainLoop);
        MainLoop = nullptr;
    }

    if (MainContext)
    {
        g_main_context_unref(MainContext);
        MainContext = nullptr;
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("GStreamer pipeline cleanup complete"));
#endif
}

bool FAdvancedRTSPStreamer::CreatePipelineElements()
{
#if WITH_GSTREAMER
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Creating GStreamer pipeline elements"));

    // Create RTSP server
    RTSPServer = gst_rtsp_server_new();
    if (!RTSPServer)
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to create RTSP server"));
        return false;
    }

    // Set server port
    FString PortString = FString::Printf(TEXT("%d"), StreamConfig.StreamPort);
    g_object_set(RTSPServer, "service", TCHAR_TO_UTF8(*PortString), nullptr);

    // Get mount points
    MountPoints = gst_rtsp_server_get_mount_points(RTSPServer);
    if (!MountPoints)
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to get mount points"));
        return false;
    }

    // Create media factory for test pattern
    Factory = gst_rtsp_media_factory_new();
    if (!Factory)
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to create media factory"));
        return false;
    }

    // Create a test pattern pipeline for now (will be replaced with actual frame data)
    FString PipelineDesc = FString::Printf(
        TEXT("( videotestsrc pattern=smpte ! video/x-raw,width=%d,height=%d,framerate=%d/1 ! videoconvert ! x264enc tune=zerolatency bitrate=%d ! rtph264pay config-interval=1 name=pay0 pt=96 )"),
        StreamConfig.StreamResolution.X,
        StreamConfig.StreamResolution.Y,
        (int32)StreamConfig.FrameRate,
        StreamConfig.Bitrate
    );

    gst_rtsp_media_factory_set_launch(Factory, TCHAR_TO_UTF8(*PipelineDesc));
    gst_rtsp_media_factory_set_shared(Factory, TRUE);

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Pipeline created: %s"), *PipelineDesc);
    return true;
#else
    UE_LOG(LogRealGazeboStreaming, Error, TEXT("GStreamer not available"));
    return false;
#endif
}

bool FAdvancedRTSPStreamer::LinkPipelineElements()
{
#if WITH_GSTREAMER
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Linking GStreamer pipeline elements"));

    // Mount the factory at the stream path
    gst_rtsp_mount_points_add_factory(MountPoints, TCHAR_TO_UTF8(*StreamConfig.StreamPath), Factory);

    // Attach the server to the default main context
    SourceID = gst_rtsp_server_attach(RTSPServer, MainContext);
    if (SourceID == 0)
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to attach RTSP server to context"));
        return false;
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP server attached successfully"));
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Stream available at: rtsp://localhost:%d%s"), 
        StreamConfig.StreamPort, *StreamConfig.StreamPath);

    return true;
#else
    UE_LOG(LogRealGazeboStreaming, Error, TEXT("GStreamer not available"));
    return false;
#endif
}

void FAdvancedRTSPStreamer::ConfigureEncoder()
{
    // Placeholder implementation - encoder configuration
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Configuring video encoder"));
}

void FAdvancedRTSPStreamer::ConfigureRTSPServer()
{
    // Placeholder implementation - RTSP server configuration
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Configuring RTSP server"));
}

//----------------------------------------------------------
// Frame Processing
//----------------------------------------------------------

void FAdvancedRTSPStreamer::ProcessFrameQueue()
{
    FFrameData Frame;
    while (FrameQueue.Dequeue(Frame))
    {
        SendFrameToGStreamer(Frame);
        CurrentStats.TotalFramesSent++;
        FramesThisSecond++;
    }
}

void FAdvancedRTSPStreamer::SendFrameToGStreamer(const FFrameData& Frame)
{
    // Placeholder implementation - send frame to GStreamer
    // This would push frame data to AppSrc element
}

//----------------------------------------------------------
// Performance Monitoring
//----------------------------------------------------------

void FAdvancedRTSPStreamer::UpdatePerformanceStats()
{
    const double CurrentTime = FPlatformTime::Seconds();
    
    // Update FPS every second
    if (CurrentTime - LastFPSUpdate >= 1.0)
    {
        CurrentStats.CurrentFPS = FramesThisSecond / (CurrentTime - LastFPSUpdate);
        FramesThisSecond = 0;
        LastFPSUpdate = CurrentTime;
    }
    
    // Update other stats
    CurrentStats.ConnectedClients = ConnectedClients.Num();
    CurrentStats.CurrentBitrate = StreamConfig.Bitrate;
    
    LastStatsUpdate = CurrentTime;
}

void FAdvancedRTSPStreamer::ResetPerformanceCounters()
{
    CurrentStats = FStreamingStats();
    LastStatsUpdate = FPlatformTime::Seconds();
    LastFrameTime = 0.0;
    FramesThisSecond = 0;
    LastFPSUpdate = LastStatsUpdate;
}

//----------------------------------------------------------
// Client Management
//----------------------------------------------------------

void FAdvancedRTSPStreamer::OnClientConnectedInternal(const FString& ClientIP)
{
    FClientInfo ClientInfo;
    ClientInfo.ClientIP = ClientIP;
    ClientInfo.ConnectTime = FPlatformTime::Seconds();
    ClientInfo.FramesSent = 0;
    ClientInfo.PreferredBitrate = StreamConfig.Bitrate;
    
    ConnectedClients.Add(ClientInfo);
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Client connected: %s (Total: %d)"), 
        *ClientIP, ConnectedClients.Num());
}

void FAdvancedRTSPStreamer::OnClientDisconnectedInternal(const FString& ClientIP)
{
    ConnectedClients.RemoveAll([&ClientIP](const FClientInfo& Client) {
        return Client.ClientIP == ClientIP;
    });
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Client disconnected: %s (Total: %d)"), 
        *ClientIP, ConnectedClients.Num());
}

void FAdvancedRTSPStreamer::UpdateClientStats()
{
    // Update per-client statistics
    for (FClientInfo& Client : ConnectedClients)
    {
        Client.FramesSent++;
    }
}

int32 FAdvancedRTSPStreamer::CalculateOptimalBitrate() const
{
    if (!bAdaptiveBitrate || ConnectedClients.Num() == 0)
    {
        return StreamConfig.Bitrate;
    }
    
    // Simple adaptive bitrate based on client count
    int32 BaseBitrate = StreamConfig.Bitrate;
    int32 ClientCount = ConnectedClients.Num();
    
    // Reduce bitrate as more clients connect
    if (ClientCount > 5)
    {
        return FMath::Max(BaseBitrate / 2, 1000); // Minimum 1 Mbps
    }
    else if (ClientCount > 2)
    {
        return FMath::Max(BaseBitrate * 3 / 4, 1500); // 75% of original
    }
    
    return BaseBitrate;
}

//----------------------------------------------------------
// Utility Methods
//----------------------------------------------------------

FString FAdvancedRTSPStreamer::GenerateStreamURL() const
{
    return FString::Printf(TEXT("rtsp://localhost:%d%s"), 
        StreamConfig.StreamPort, *StreamConfig.StreamPath);
}

bool FAdvancedRTSPStreamer::ValidateConfiguration() const
{
    if (StreamConfig.CameraName.IsEmpty())
    {
        return false;
    }
    
    if (StreamConfig.StreamPort <= 0 || StreamConfig.StreamPort > 65535)
    {
        return false;
    }
    
    if (StreamConfig.FrameRate <= 0.0f || StreamConfig.Bitrate <= 0)
    {
        return false;
    }
    
    return true;
}