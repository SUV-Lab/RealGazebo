// Copyright Epic Games, Inc. All Rights Reserved.

#include "RealGazebo.h"
#include "RTSPStreamer.h"
#include "Core.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FRealGazeboModule"

FRealGazeboModule* FRealGazeboModule::ModuleInstance = nullptr;

void FRealGazeboModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
    ModuleInstance = this;
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("====== RTSPStreamer in RealGazeboModule startup ======"));
    
    // GStreamer 스레드 시작
    StreamerThread = MakeUnique<FRTSPStreamerThread>();
    
    if (StreamerThread->IsServerRunning())
    {
        UE_LOG(LogRTSPStreamer, Display, TEXT("====== RTSP Server started successfully in RealGazebo ======"));
    }
    else
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("====== RTSP Server failed to start in RealGazebo ======"));
    }
    
    UE_LOG(LogRTSPStreamer, Log, TEXT("RTSPStreamer part of RealGazeboModule started"));
}

void FRealGazeboModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

    if (StreamerThread.IsValid())
    {
        StreamerThread->Stop();
        StreamerThread.Reset();
    }
    
    ModuleInstance = nullptr;
    
    UE_LOG(LogRTSPStreamer, Log, TEXT("RTSPStreamer part of RealGazeboModule shutdown"));
}

FRealGazeboModule& FRealGazeboModule::Get()
{
    return FModuleManager::LoadModuleChecked<FRealGazeboModule>("RealGazebo");
}

void FRealGazeboModule::RegisterStream(const FString& StreamPath, const FRTSPStreamSettings& Settings)
{
    UE_LOG(LogRTSPStreamer, Display, TEXT("[%s] RegisterStream in module called."), *StreamPath);
    if (StreamerThread.IsValid())
    {
        StreamerThread->AddStream(StreamPath, Settings);
    }
    else
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("[%s] StreamerThread is not valid!"), *StreamPath);
    }
}

void FRealGazeboModule::UnregisterStream(const FString& StreamPath)
{
    if (StreamerThread.IsValid())
    {
        StreamerThread->RemoveStream(StreamPath);
    }
}

void FRealGazeboModule::UpdateStream(const FString& StreamPath, const TArray<uint8>& FrameData)
{
    if (StreamerThread.IsValid())
    {
        StreamerThread->UpdateStreamFrame(StreamPath, FrameData);
    }
}

bool FRealGazeboModule::IsServerRunning() const
{
    return StreamerThread.IsValid() && StreamerThread->IsServerRunning();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRealGazeboModule, RealGazebo)