#pragma once

// Prevent GStreamer headers from conflicting with Unreal Engine
#ifdef GError
#undef GError
#endif

#if WITH_GSTREAMER

// Push current warning state and disable warnings for external headers
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

// Define GStreamer-specific types to avoid conflicts with UE5
#define GError GStreamerGError
#define GThreadPool GStreamerThreadPool
#define GType GStreamerType  
#define GTypeClass GStreamerTypeClass

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/video.h>

// Restore warning state
#pragma clang diagnostic pop

// Restore UE5's GError after GStreamer includes
#undef GError
extern class FOutputDeviceError* GError;

#endif // WITH_GSTREAMER