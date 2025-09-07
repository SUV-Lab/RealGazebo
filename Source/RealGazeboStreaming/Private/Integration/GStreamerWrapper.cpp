#include "Integration/GStreamerWrapper.h"
#include "RealGazeboStreaming.h"

#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"

#if WITH_GSTREAMER

//----------------------------------------------------------
// GStreamer Initialization State
//----------------------------------------------------------

static bool bGStreamerInitialized = false;
static FString CachedGStreamerVersion;

namespace RealGazebo::GStreamerUtils
{
    bool InitializeGStreamer()
    {
        if (bGStreamerInitialized)
        {
            return true;
        }

        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Initializing GStreamer..."));

        // Initialize GStreamer
        GStreamerGError* Error = nullptr;
        if (!gst_init_check(nullptr, nullptr, &Error))
        {
            if (Error)
            {
                FString ErrorMessage = FString(UTF8_TO_TCHAR(Error->message));
                UE_LOG(LogRealGazeboStreaming, Error, TEXT("GStreamer initialization failed: %s"), *ErrorMessage);
                g_error_free(Error);
            }
            else
            {
                UE_LOG(LogRealGazeboStreaming, Error, TEXT("GStreamer initialization failed with unknown error"));
            }
            return false;
        }

        // Cache version information
        guint Major, Minor, Micro, Nano;
        gst_version(&Major, &Minor, &Micro, &Nano);
        CachedGStreamerVersion = FString::Printf(TEXT("%u.%u.%u.%u"), Major, Minor, Micro, Nano);

        UE_LOG(LogRealGazeboStreaming, Log, TEXT("GStreamer initialized successfully (Version: %s)"), *CachedGStreamerVersion);

        // Log available plugins (for debugging)
        // LogAvailablePlugins(); // TODO: Implement if needed

        bGStreamerInitialized = true;
        return true;
    }

    void DeinitializeGStreamer()
    {
        if (bGStreamerInitialized)
        {
            UE_LOG(LogRealGazeboStreaming, Log, TEXT("Deinitializing GStreamer..."));
            gst_deinit();
            bGStreamerInitialized = false;
            CachedGStreamerVersion.Empty();
            UE_LOG(LogRealGazeboStreaming, Log, TEXT("GStreamer deinitialized"));
        }
    }

    bool IsGStreamerInitialized()
    {
        return bGStreamerInitialized && gst_is_initialized();
    }

    FString GetGStreamerVersion()
    {
        if (bGStreamerInitialized)
        {
            return CachedGStreamerVersion;
        }
        return TEXT("Not initialized");
    }

    bool IsHardwareEncodingAvailable(const FString& Codec)
    {
        if (!bGStreamerInitialized)
        {
            return false;
        }

        // Define hardware encoder names for different codecs
        TArray<FString> HardwareEncoders;
        
        if (Codec == TEXT("H264"))
        {
            HardwareEncoders = {
                TEXT("nvh264enc"),     // NVIDIA
                TEXT("vaapih264enc"),  // VA-API (Intel/AMD)
                TEXT("omxh264enc"),    // OMX
                TEXT("msdkh264enc"),   // Intel Media SDK
                TEXT("qsvh264enc")     // Intel Quick Sync
            };
        }
        else if (Codec == TEXT("H265") || Codec == TEXT("HEVC"))
        {
            HardwareEncoders = {
                TEXT("nvh265enc"),
                TEXT("vaapih265enc"),
                TEXT("omxh265enc"),
                TEXT("msdkh265enc"),
                TEXT("qsvh265enc")
            };
        }
        else if (Codec == TEXT("VP8"))
        {
            HardwareEncoders = {
                TEXT("vaapivp8enc")
            };
        }
        else if (Codec == TEXT("VP9"))
        {
            HardwareEncoders = {
                TEXT("vaapivp9enc")
            };
        }

        // Check if any hardware encoder is available
        for (const FString& EncoderName : HardwareEncoders)
        {
            GstElementFactory* Factory = gst_element_factory_find(TCHAR_TO_UTF8(*EncoderName));
            if (Factory)
            {
                gst_object_unref(Factory);
                UE_LOG(LogRealGazeboStreaming, Verbose, TEXT("Hardware encoder found: %s"), *EncoderName);
                return true;
            }
        }

        return false;
    }

    FString GetOptimalEncoder(const FString& Codec, bool bPreferHardware)
    {
        if (!bGStreamerInitialized)
        {
            return TEXT("");
        }

        struct FEncoderOption
        {
            FString Name;
            bool bIsHardware;
            int32 Priority; // Higher is better
        };

        TArray<FEncoderOption> Encoders;

        if (Codec == TEXT("H264"))
        {
            Encoders = {
                {TEXT("nvh264enc"), true, 100},      // NVIDIA - best quality/performance
                {TEXT("qsvh264enc"), true, 90},      // Intel Quick Sync - good balance
                {TEXT("vaapih264enc"), true, 80},    // VA-API - good compatibility
                {TEXT("msdkh264enc"), true, 70},     // Intel Media SDK
                {TEXT("omxh264enc"), true, 60},      // OMX - embedded systems
                {TEXT("x264enc"), false, 50},        // Software - high quality
                {TEXT("openh264enc"), false, 30}     // Software - fast
            };
        }
        else if (Codec == TEXT("H265") || Codec == TEXT("HEVC"))
        {
            Encoders = {
                {TEXT("nvh265enc"), true, 100},
                {TEXT("qsvh265enc"), true, 90},
                {TEXT("vaapih265enc"), true, 80},
                {TEXT("msdkh265enc"), true, 70},
                {TEXT("omxh265enc"), true, 60},
                {TEXT("x265enc"), false, 50}
            };
        }
        else if (Codec == TEXT("VP8"))
        {
            Encoders = {
                {TEXT("vaapivp8enc"), true, 80},
                {TEXT("vp8enc"), false, 50}
            };
        }
        else if (Codec == TEXT("VP9"))
        {
            Encoders = {
                {TEXT("vaapivp9enc"), true, 80},
                {TEXT("vp9enc"), false, 50}
            };
        }

        // Sort by priority, hardware preference
        Encoders.Sort([bPreferHardware](const FEncoderOption& A, const FEncoderOption& B)
        {
            if (bPreferHardware && A.bIsHardware != B.bIsHardware)
            {
                return A.bIsHardware > B.bIsHardware;
            }
            return A.Priority > B.Priority;
        });

        // Find first available encoder
        for (const FEncoderOption& Encoder : Encoders)
        {
            GstElementFactory* Factory = gst_element_factory_find(TCHAR_TO_UTF8(*Encoder.Name));
            if (Factory)
            {
                gst_object_unref(Factory);
                UE_LOG(LogRealGazeboStreaming, Log, TEXT("Selected encoder for %s: %s (%s)"), 
                    *Codec, *Encoder.Name, Encoder.bIsHardware ? TEXT("Hardware") : TEXT("Software"));
                return Encoder.Name;
            }
        }

        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("No suitable encoder found for codec: %s"), *Codec);
        return TEXT("");
    }

    FString CreateVideoCapsString(int32 Width, int32 Height, float FrameRate, const FString& Format)
    {
        return FString::Printf(TEXT("video/x-raw,format=%s,width=%d,height=%d,framerate=%d/1"),
            *Format, Width, Height, FMath::RoundToInt(FrameRate));
    }

    FString UEPixelFormatToGStreamerFormat(EPixelFormat PixelFormat)
    {
        switch (PixelFormat)
        {
            case PF_B8G8R8A8:
                return TEXT("BGRA");
            case PF_R8G8B8A8:
                return TEXT("RGBA");
            case PF_FloatRGBA:
                return TEXT("RGBA");
            default:
                return TEXT("RGBA"); // Default fallback
        }
    }

    bool ValidateRTSPURL(const FString& URL)
    {
        // Basic RTSP URL validation
        if (!URL.StartsWith(TEXT("rtsp://")))
        {
            return false;
        }

        // Check for basic format: rtsp://host:port/path
        TArray<FString> Parts;
        URL.ParseIntoArray(Parts, TEXT("/"), true);
        
        if (Parts.Num() < 3) // rtsp:, empty, host:port, path...
        {
            return false;
        }

        // Check host:port format
        FString HostPort = Parts[2];
        if (HostPort.IsEmpty())
        {
            return false;
        }

        return true;
    }

    FString CreateSafeElementName(const FString& BaseName, int32 Index)
    {
        FString SafeName = BaseName.Replace(TEXT(" "), TEXT("_"))
                                 .Replace(TEXT("-"), TEXT("_"))
                                 .Replace(TEXT("."), TEXT("_"));
        
        if (Index > 0)
        {
            SafeName += FString::Printf(TEXT("_%d"), Index);
        }

        return SafeName;
    }

    void LogPipelineState(GstElement* Pipeline, const FString& PipelineName)
    {
        if (!Pipeline)
        {
            UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Pipeline '%s' is null"), *PipelineName);
            return;
        }

        GstState CurrentState, PendingState;
        GstStateChangeReturn Result = gst_element_get_state(Pipeline, &CurrentState, &PendingState, GST_CLOCK_TIME_NONE);

        FString StateString = TEXT("Unknown"); // StateToString(CurrentState); // TODO: Implement if needed
        FString PendingString = TEXT("None"); // (PendingState != GST_STATE_VOID_PENDING) ? StateToString(PendingState) : TEXT("None");

        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Pipeline '%s' - Current: %s, Pending: %s, Result: %s"), 
            *PipelineName, *StateString, *PendingString, *StateChangeResultToString(Result));
    }

    double GStreamerTimestampToUETime(GstClockTime Timestamp)
    {
        return static_cast<double>(Timestamp) / GST_SECOND;
    }

    GstClockTime UETimeToGStreamerTimestamp(double UETime)
    {
        return static_cast<GstClockTime>(UETime * GST_SECOND);
    }

    FString StateChangeResultToString(GstStateChangeReturn Result)
    {
        switch (Result)
        {
            case GST_STATE_CHANGE_SUCCESS:
                return TEXT("Success");
            case GST_STATE_CHANGE_ASYNC:
                return TEXT("Async");
            case GST_STATE_CHANGE_NO_PREROLL:
                return TEXT("No Preroll");
            case GST_STATE_CHANGE_FAILURE:
                return TEXT("Failure");
            default:
                return FString::Printf(TEXT("Unknown (%d)"), static_cast<int32>(Result));
        }
    }

    FString MessageTypeToString(GstMessageType Type)
    {
        switch (Type)
        {
            case GST_MESSAGE_ERROR:
                return TEXT("Error");
            case GST_MESSAGE_WARNING:
                return TEXT("Warning");
            case GST_MESSAGE_INFO:
                return TEXT("Info");
            case GST_MESSAGE_EOS:
                return TEXT("End of Stream");
            case GST_MESSAGE_STATE_CHANGED:
                return TEXT("State Changed");
            case GST_MESSAGE_BUFFERING:
                return TEXT("Buffering");
            case GST_MESSAGE_NEW_CLOCK:
                return TEXT("New Clock");
            default:
                return FString::Printf(TEXT("Unknown (%d)"), static_cast<int32>(Type));
        }
    }

    FString StateToString(GstState State)
    {
        switch (State)
        {
            case GST_STATE_NULL:
                return TEXT("NULL");
            case GST_STATE_READY:
                return TEXT("READY");
            case GST_STATE_PAUSED:
                return TEXT("PAUSED");
            case GST_STATE_PLAYING:
                return TEXT("PLAYING");
            default:
                return FString::Printf(TEXT("Unknown (%d)"), static_cast<int32>(State));
        }
    }

    void LogAvailablePlugins()
    {
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Enumerating available GStreamer plugins..."));

        GList* PluginList = gst_registry_get_plugin_list(gst_registry_get());
        guint PluginCount = g_list_length(PluginList);

        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Found %u GStreamer plugins"), PluginCount);

        // Log some key plugins we're interested in
        TArray<FString> KeyPlugins = {
            TEXT("coreelements"), TEXT("videoconvert"), TEXT("videorate"), TEXT("videoscale"),
            TEXT("x264"), TEXT("x265"), TEXT("vpx"), TEXT("libav"),
            TEXT("rtsp"), TEXT("rtp"), TEXT("udp"), TEXT("tcp"),
            TEXT("nvenc"), TEXT("vaapi"), TEXT("qsv"), TEXT("omx"),
            TEXT("appsrc"), TEXT("appsink"), TEXT("filesrc"), TEXT("filesink")
        };

        for (const FString& PluginName : KeyPlugins)
        {
            GstPlugin* Plugin = gst_registry_find_plugin(gst_registry_get(), TCHAR_TO_UTF8(*PluginName));
            if (Plugin)
            {
                const gchar* Version = gst_plugin_get_version(Plugin);
                const gchar* Description = gst_plugin_get_description(Plugin);
                
                UE_LOG(LogRealGazeboStreaming, Verbose, TEXT("  %s v%s - %s"), 
                    *PluginName, 
                    Version ? UTF8_TO_TCHAR(Version) : TEXT("Unknown"),
                    Description ? UTF8_TO_TCHAR(Description) : TEXT("No description"));
                
                gst_object_unref(Plugin);
            }
        }

        gst_plugin_list_free(PluginList);
    }
}

#else // !WITH_GSTREAMER

namespace RealGazebo::GStreamerUtils
{
    bool InitializeGStreamer()
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("GStreamer support not compiled in"));
        return false;
    }

    void DeinitializeGStreamer()
    {
        // Nothing to do
    }

    bool IsGStreamerInitialized()
    {
        return false;
    }

    FString GetGStreamerVersion()
    {
        return TEXT("GStreamer not available (not compiled in)");
    }

    bool IsHardwareEncodingAvailable(const FString& Codec)
    {
        return false;
    }

    FString GetOptimalEncoder(const FString& Codec, bool bPreferHardware)
    {
        return TEXT("");
    }

    FString CreateVideoCapsString(int32 Width, int32 Height, float FrameRate, const FString& Format)
    {
        return TEXT("");
    }

    FString UEPixelFormatToGStreamerFormat(EPixelFormat PixelFormat)
    {
        return TEXT("");
    }

    bool ValidateRTSPURL(const FString& URL)
    {
        return false;
    }

    FString CreateSafeElementName(const FString& BaseName, int32 Index)
    {
        return TEXT("");
    }

    void LogPipelineState(GstElement* Pipeline, const FString& PipelineName)
    {
        // Nothing to do
    }

    double GStreamerTimestampToUETime(GstClockTime Timestamp)
    {
        return 0.0;
    }

    GstClockTime UETimeToGStreamerTimestamp(double UETime)
    {
        return 0;
    }

    FString StateChangeResultToString(GstStateChangeReturn Result)
    {
        return TEXT("GStreamer not available");
    }

    FString MessageTypeToString(GstMessageType Type)
    {
        return TEXT("GStreamer not available");
    }
}

#endif // WITH_GSTREAMER