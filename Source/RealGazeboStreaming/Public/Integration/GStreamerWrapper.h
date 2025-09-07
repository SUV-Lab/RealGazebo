#pragma once

// Enhanced GStreamer wrapper with better UE5 compatibility and performance optimizations

// Store UE5's original definitions to restore later
#ifdef GError
#define UE_ORIGINAL_GERROR_DEFINED
#define UE_ORIGINAL_GERROR GError
#undef GError
#endif

#ifdef GType
#define UE_ORIGINAL_GTYPE_DEFINED
#define UE_ORIGINAL_GTYPE GType
#undef GType
#endif

#ifdef GThreadPool
#define UE_ORIGINAL_GTHREADPOOL_DEFINED
#define UE_ORIGINAL_GTHREADPOOL GThreadPool
#undef GThreadPool
#endif

#if WITH_GSTREAMER

// Disable all warnings for GStreamer headers
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#endif

// Define GStreamer-specific types to avoid conflicts
#define GError GStreamerGError
#define GType GStreamerType
#define GTypeClass GStreamerTypeClass
#define GThreadPool GStreamerThreadPool
#define GCallback GStreamerCallback
#define GMutex GStreamerMutex
#define GCond GStreamerCond
#define GSource GStreamerSource
#define GSourceFunc GStreamerSourceFunc

// Core GStreamer includes
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/video.h>
#include <gst/rtp/gstrtpbuffer.h>

// Additional includes for enhanced functionality
#include <gst/codecparsers/gsth264parser.h>
#include <gst/codecparsers/gsth265parser.h>

// Advanced video processing
#include <gst/video/gstvideometa.h>
#include <gst/video/video-frame.h>
#include <gst/video/video-info.h>

// Network and streaming utilities
#include <gst/net/gstnet.h>
#include <gst/rtsp-server/rtsp-media-factory.h>
#include <gst/rtsp-server/rtsp-media.h>
#include <gst/rtsp-server/rtsp-client.h>

// Restore warning state
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// Restore UE5's original definitions
#undef GError
#undef GType 
#undef GTypeClass
#undef GThreadPool
#undef GCallback
#undef GMutex
#undef GCond
#undef GSource
#undef GSourceFunc

#ifdef UE_ORIGINAL_GERROR_DEFINED
#define GError UE_ORIGINAL_GERROR
#undef UE_ORIGINAL_GERROR_DEFINED
#undef UE_ORIGINAL_GERROR
#else
// Restore UE5's GError declaration
extern class FOutputDeviceError* GError;
#endif

#ifdef UE_ORIGINAL_GTYPE_DEFINED
#define GType UE_ORIGINAL_GTYPE
#undef UE_ORIGINAL_GTYPE_DEFINED
#undef UE_ORIGINAL_GTYPE
#endif

#ifdef UE_ORIGINAL_GTHREADPOOL_DEFINED
#define GThreadPool UE_ORIGINAL_GTHREADPOOL
#undef UE_ORIGINAL_GTHREADPOOL_DEFINED
#undef UE_ORIGINAL_GTHREADPOOL
#endif

//----------------------------------------------------------
// GStreamer Utility Macros for RealGazebo
//----------------------------------------------------------

// Safe GStreamer object cleanup
#define SAFE_GST_OBJECT_UNREF(obj) \
    do { \
        if ((obj) != nullptr) { \
            gst_object_unref(obj); \
            (obj) = nullptr; \
        } \
    } while(0)

// Safe GLib object cleanup
#define SAFE_G_OBJECT_UNREF(obj) \
    do { \
        if ((obj) != nullptr) { \
            g_object_unref(obj); \
            (obj) = nullptr; \
        } \
    } while(0)

// Safe string cleanup
#define SAFE_G_FREE(ptr) \
    do { \
        if ((ptr) != nullptr) { \
            g_free(ptr); \
            (ptr) = nullptr; \
        } \
    } while(0)

// GStreamer state change helper
#define GST_STATE_CHANGE_ASYNC_TO_SUCCESS(ret) \
    ((ret) == GST_STATE_CHANGE_ASYNC ? GST_STATE_CHANGE_SUCCESS : (ret))

//----------------------------------------------------------
// GStreamer Helper Functions for RealGazebo
//----------------------------------------------------------

namespace RealGazebo::GStreamerUtils
{
    /** Initialize GStreamer with error handling */
    bool InitializeGStreamer();

    /** Cleanup GStreamer resources */
    void DeinitializeGStreamer();

    /** Check if GStreamer is initialized */
    bool IsGStreamerInitialized();

    /** Get GStreamer version string */
    FString GetGStreamerVersion();

    /** Check if hardware encoding is available for codec */
    bool IsHardwareEncodingAvailable(const FString& Codec);

    /** Get optimal encoder for platform and codec */
    FString GetOptimalEncoder(const FString& Codec, bool bPreferHardware = true);

    /** Create optimized caps string for video format */
    FString CreateVideoCapsString(int32 Width, int32 Height, float FrameRate, const FString& Format = TEXT("I420"));

    /** Convert UE pixel format to GStreamer format string */
    FString UEPixelFormatToGStreamerFormat(EPixelFormat PixelFormat);

    /** Validate RTSP URL format */
    bool ValidateRTSPURL(const FString& URL);

    /** Create safe GStreamer element name */
    FString CreateSafeElementName(const FString& BaseName, int32 Index = 0);

    /** Log GStreamer pipeline state */
    void LogPipelineState(GstElement* Pipeline, const FString& PipelineName);

    /** Convert GStreamer timestamp to UE time */
    double GStreamerTimestampToUETime(GstClockTime Timestamp);

    /** Convert UE time to GStreamer timestamp */
    GstClockTime UETimeToGStreamerTimestamp(double UETime);

    /** Get human-readable state change result */
    FString StateChangeResultToString(GstStateChangeReturn Result);

    /** Get human-readable message type */
    FString MessageTypeToString(GstMessageType Type);
}

//----------------------------------------------------------
// Advanced GStreamer Integration Macros
//----------------------------------------------------------

// Error handling with UE logging
#define GST_ERROR_CHECK(result, operation) \
    do { \
        if ((result) != GST_STATE_CHANGE_SUCCESS && (result) != GST_STATE_CHANGE_ASYNC) { \
            UE_LOG(LogTemp, Error, TEXT("GStreamer Error in %s: %s"), \
                TEXT(#operation), *RealGazebo::GStreamerUtils::StateChangeResultToString(result)); \
            return false; \
        } \
    } while(0)

// Bus message handling helper
#define GST_HANDLE_MESSAGE(bus, msg, timeout, handler_func) \
    do { \
        GstMessage* _msg = gst_bus_timed_pop_filtered((bus), (timeout), \
            (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_WARNING)); \
        if (_msg) { \
            (handler_func)(_msg); \
            gst_message_unref(_msg); \
        } \
    } while(0)

// Element creation with error checking
#define GST_CREATE_ELEMENT(element, factory_name, element_name) \
    do { \
        (element) = gst_element_factory_make((factory_name), (element_name)); \
        if (!(element)) { \
            UE_LOG(LogTemp, Error, TEXT("Failed to create GStreamer element: %s (%s)"), \
                UTF8_TO_TCHAR(factory_name), UTF8_TO_TCHAR(element_name)); \
            return false; \
        } \
    } while(0)

#else // !WITH_GSTREAMER

// Dummy definitions when GStreamer is not available
namespace RealGazebo::GStreamerUtils
{
    inline bool InitializeGStreamer() { return false; }
    inline void DeinitializeGStreamer() {}
    inline bool IsGStreamerInitialized() { return false; }
    inline FString GetGStreamerVersion() { return TEXT("GStreamer not available"); }
    inline bool IsHardwareEncodingAvailable(const FString& Codec) { return false; }
    inline FString GetOptimalEncoder(const FString& Codec, bool bPreferHardware = true) { return TEXT(""); }
    inline FString CreateVideoCapsString(int32 Width, int32 Height, float FrameRate, const FString& Format = TEXT("I420")) { return TEXT(""); }
    inline FString UEPixelFormatToGStreamerFormat(EPixelFormat PixelFormat) { return TEXT(""); }
    inline bool ValidateRTSPURL(const FString& URL) { return false; }
    inline FString CreateSafeElementName(const FString& BaseName, int32 Index = 0) { return TEXT(""); }
}

// Dummy macros for non-GStreamer builds
#define GST_ERROR_CHECK(result, operation) do { } while(0)
#define GST_HANDLE_MESSAGE(bus, msg, timeout, handler_func) do { } while(0)
#define GST_CREATE_ELEMENT(element, factory_name, element_name) do { (element) = nullptr; } while(0)

#endif // WITH_GSTREAMER