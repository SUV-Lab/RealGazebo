#include "RealGazeboStreaming.h"

DEFINE_LOG_CATEGORY(LogRealGazeboStreaming);

void FRealGazeboStreamingModule::StartupModule()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RealGazeboStreaming module starting up"));
}

void FRealGazeboStreamingModule::ShutdownModule()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RealGazeboStreaming module shutting down"));
}

IMPLEMENT_MODULE(FRealGazeboStreamingModule, RealGazeboStreaming)