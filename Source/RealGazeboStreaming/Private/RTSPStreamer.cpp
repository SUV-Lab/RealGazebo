#include "RTSPStreamer.h"
#include "RealGazeboStreaming.h"
#include "HAL/PlatformFileManager.h"

FRTSPStreamer::FRTSPStreamer()
    : StreamingThread(nullptr)
    , bShouldStop(false)
    , bIsStreaming(false)
    , StreamPort(8554)
    , FrameWidth(0)
    , FrameHeight(0)
    , FrameChannels(3)
    , bHasPendingFrame(false)
{
#if WITH_GSTREAMER
    RTSPServer = nullptr;
    MountPoints = nullptr;
    Factory = nullptr;
    Pipeline = nullptr;
    AppSrc = nullptr;
    Bus = nullptr;
    MainLoop = nullptr;
    MainContext = nullptr;
#endif
}

FRTSPStreamer::~FRTSPStreamer()
{
    StopStreaming();
}

bool FRTSPStreamer::StartStreaming(const FString& StreamPath, int32 Port)
{
    if (bIsStreaming)
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("RTSP Streamer is already running"));
        return false;
    }

    CurrentStreamPath = StreamPath;
    StreamPort = Port;

    bShouldStop = false;
    StreamingThread = FRunnableThread::Create(this, TEXT("RTSPStreamerThread"));

    if (!StreamingThread)
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to create RTSP streaming thread"));
        return false;
    }

    return true;
}

void FRTSPStreamer::StopStreaming()
{
    if (StreamingThread)
    {
        bShouldStop = true;
        StreamingThread->WaitForCompletion();
        delete StreamingThread;
        StreamingThread = nullptr;
    }

    bIsStreaming = false;
    OnStreamingStatusChanged.Broadcast(false);
}

bool FRTSPStreamer::IsStreaming() const
{
    return bIsStreaming;
}

void FRTSPStreamer::PushFrame(const TArray<uint8>& ImageData, int32 Width, int32 Height, int32 Channels)
{
    FScopeLock Lock(&FrameDataMutex);
    
    PendingFrameData = ImageData;
    FrameWidth = Width;
    FrameHeight = Height;
    FrameChannels = Channels;
    bHasPendingFrame = true;
}

bool FRTSPStreamer::Init()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Initializing RTSP Streamer"));
    
#if WITH_GSTREAMER
    if (!gst_is_initialized())
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("GStreamer not initialized"));
        return false;
    }
    
    InitializeGStreamerPipeline();
    return true;
#else
    UE_LOG(LogRealGazeboStreaming, Error, TEXT("GStreamer support not compiled"));
    return false;
#endif
}

uint32 FRTSPStreamer::Run()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP Streamer thread started"));
    
#if WITH_GSTREAMER
    while (!bShouldStop)
    {
        if (MainContext && MainLoop)
        {
            g_main_context_iteration(MainContext, FALSE);
            
            // Process pending frames
            {
                FScopeLock Lock(&FrameDataMutex);
                if (bHasPendingFrame && AppSrc)
                {
                    // Convert frame data to GStreamer buffer
                    gsize BufferSize = FrameWidth * FrameHeight * FrameChannels;
                    GstBuffer* Buffer = gst_buffer_new_allocate(nullptr, BufferSize, nullptr);
                    
                    GstMapInfo MapInfo;
                    if (gst_buffer_map(Buffer, &MapInfo, GST_MAP_WRITE))
                    {
                        FMemory::Memcpy(MapInfo.data, PendingFrameData.GetData(), BufferSize);
                        gst_buffer_unmap(Buffer, &MapInfo);
                        
                        // Set buffer timestamp
                        GST_BUFFER_PTS(Buffer) = gst_clock_get_time(gst_element_get_clock(AppSrc)) - gst_element_get_base_time(AppSrc);
                        
                        // Push buffer to appsrc
                        GstFlowReturn Ret = gst_app_src_push_buffer(GST_APP_SRC(AppSrc), Buffer);
                        if (Ret != GST_FLOW_OK)
                        {
                            UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Failed to push buffer to GStreamer: %d"), Ret);
                        }
                    }
                    else
                    {
                        gst_buffer_unref(Buffer);
                    }
                    
                    bHasPendingFrame = false;
                }
            }
        }
        
        FPlatformProcess::Sleep(0.001f); // 1ms sleep
    }
#endif

    CleanupGStreamerPipeline();
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP Streamer thread finished"));
    return 0;
}

void FRTSPStreamer::Stop()
{
    bShouldStop = true;
}

void FRTSPStreamer::InitializeGStreamerPipeline()
{
#if WITH_GSTREAMER
    // Create RTSP server
    RTSPServer = gst_rtsp_server_new();
    FString PortString = FString::Printf(TEXT("%d"), StreamPort);
    gst_rtsp_server_set_service(RTSPServer, TCHAR_TO_UTF8(*PortString));
    
    // Create mount points
    MountPoints = gst_rtsp_server_get_mount_points(RTSPServer);
    
    // Create media factory
    Factory = gst_rtsp_media_factory_new();
    
    // Set pipeline description for the factory
    FString PipelineDescription = FString::Printf(TEXT(
        "( appsrc name=mysrc ! "
        "videoconvert ! "
        "video/x-raw,format=I420 ! "
        "x264enc tune=zerolatency bitrate=2000 speed-preset=ultrafast ! "
        "rtph264pay name=pay0 pt=96 )"
    ));
    
    gst_rtsp_media_factory_set_launch(Factory, TCHAR_TO_UTF8(*PipelineDescription));
    gst_rtsp_media_factory_set_shared(Factory, TRUE);
    
    // Connect signals
    g_signal_connect(Factory, "media-configure", G_CALLBACK(OnMediaConfigure), this);
    g_signal_connect(RTSPServer, "client-connected", G_CALLBACK(OnClientConnected), this);
    
    // Add factory to mount points
    gst_rtsp_mount_points_add_factory(MountPoints, TCHAR_TO_UTF8(*CurrentStreamPath), Factory);
    
    // Attach server to main context
    MainContext = g_main_context_new();
    MainLoop = g_main_loop_new(MainContext, FALSE);
    gst_rtsp_server_attach(RTSPServer, MainContext);
    
    bIsStreaming = true;
    OnStreamingStatusChanged.Broadcast(true);
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP server initialized on port %d with path %s"), StreamPort, *CurrentStreamPath);
#endif
}

void FRTSPStreamer::CleanupGStreamerPipeline()
{
#if WITH_GSTREAMER
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
    
    if (Bus)
    {
        gst_object_unref(Bus);
        Bus = nullptr;
    }
    
    if (Pipeline)
    {
        gst_element_set_state(Pipeline, GST_STATE_NULL);
        gst_object_unref(Pipeline);
        Pipeline = nullptr;
    }
    
    if (Factory)
    {
        g_object_unref(Factory);
        Factory = nullptr;
    }
    
    if (MountPoints)
    {
        g_object_unref(MountPoints);
        MountPoints = nullptr;
    }
    
    if (RTSPServer)
    {
        g_object_unref(RTSPServer);
        RTSPServer = nullptr;
    }
    
    AppSrc = nullptr;
#endif
}

#if WITH_GSTREAMER
void FRTSPStreamer::OnClientConnected(GstRTSPServer* Server, GstRTSPClient* Client, gpointer UserData)
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP Client connected"));
}

void FRTSPStreamer::OnClientDisconnected(GstRTSPServer* Server, GstRTSPClient* Client, gpointer UserData)
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP Client disconnected"));
}

void FRTSPStreamer::OnMediaConfigure(GstRTSPMediaFactory* Factory, GstRTSPMedia* Media, gpointer UserData)
{
    FRTSPStreamer* Streamer = static_cast<FRTSPStreamer*>(UserData);
    
    GstElement* Element = gst_rtsp_media_get_element(Media);
    Streamer->AppSrc = gst_bin_get_by_name_recurse_up(GST_BIN(Element), "mysrc");
    
    if (Streamer->AppSrc)
    {
        // Configure appsrc
        g_object_set(G_OBJECT(Streamer->AppSrc),
            "caps", gst_caps_new_simple("video/x-raw",
                "format", G_TYPE_STRING, "RGB",
                "width", G_TYPE_INT, 1920,
                "height", G_TYPE_INT, 1080,
                "framerate", GST_TYPE_FRACTION, 30, 1,
                nullptr),
            "format", GST_FORMAT_TIME,
            "is-live", TRUE,
            nullptr);
            
        g_signal_connect(Streamer->AppSrc, "need-data", G_CALLBACK(OnNeedData), UserData);
        g_signal_connect(Streamer->AppSrc, "enough-data", G_CALLBACK(OnEnoughData), UserData);
    }
    
    gst_object_unref(Element);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP Media configured"));
}

gboolean FRTSPStreamer::OnBusMessage(GstBus* Bus, GstMessage* Message, gpointer UserData)
{
    FRTSPStreamer* Streamer = static_cast<FRTSPStreamer*>(UserData);
    
    switch (GST_MESSAGE_TYPE(Message))
    {
        case GST_MESSAGE_ERROR:
        {
            GStreamerGError* Error;
            gchar* Debug;
            gst_message_parse_error(Message, &Error, &Debug);
            UE_LOG(LogRealGazeboStreaming, Error, TEXT("GStreamer Error: %s"), UTF8_TO_TCHAR(Error->message));
            g_error_free(Error);
            g_free(Debug);
            break;
        }
        case GST_MESSAGE_WARNING:
        {
            GStreamerGError* Error;
            gchar* Debug;
            gst_message_parse_warning(Message, &Error, &Debug);
            UE_LOG(LogRealGazeboStreaming, Warning, TEXT("GStreamer Warning: %s"), UTF8_TO_TCHAR(Error->message));
            g_error_free(Error);
            g_free(Debug);
            break;
        }
        case GST_MESSAGE_EOS:
        {
            UE_LOG(LogRealGazeboStreaming, Log, TEXT("GStreamer End of Stream"));
            break;
        }
        default:
            break;
    }
    
    return TRUE;
}

void FRTSPStreamer::OnNeedData(GstElement* Appsrc, guint Unused, gpointer UserData)
{
    // Signal that we need more data
}

void FRTSPStreamer::OnEnoughData(GstElement* Appsrc, gpointer UserData)
{
    // Signal that we have enough data
}
#endif