#include "RealGazeboStreaming.h"
#include "Modules/ModuleManager.h"
#include "GStreamerWrapper.h"

DEFINE_LOG_CATEGORY(LogRealGazeboStreaming);

void FRealGazeboStreamingModule::StartupModule()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RealGazeboStreaming: StartupModule"));
    
#if WITH_GSTREAMER
    if (!gst_is_initialized())
    {
        gst_init(nullptr, nullptr);
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("GStreamer initialized successfully"));
    }
#endif
}

void FRealGazeboStreamingModule::ShutdownModule()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RealGazeboStreaming: ShutdownModule"));
    
#if WITH_GSTREAMER
    if (gst_is_initialized())
    {
        gst_deinit();
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("GStreamer deinitialized"));
    }
#endif
}

IMPLEMENT_MODULE(FRealGazeboStreamingModule, RealGazeboStreaming)