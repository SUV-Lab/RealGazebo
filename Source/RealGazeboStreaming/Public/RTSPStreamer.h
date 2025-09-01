#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"
#include "Engine/Texture2D.h"

#include "GStreamerWrapper.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnStreamingStatusChanged, bool);

class REALGAZEBOSTREAMING_API FRTSPStreamer : public FRunnable
{
public:
    FRTSPStreamer();
    virtual ~FRTSPStreamer();

    bool StartStreaming(const FString& StreamPath, int32 Port = 8554);
    void StopStreaming();
    bool IsStreaming() const;

    void PushFrame(const TArray<uint8>& ImageData, int32 Width, int32 Height, int32 Channels = 3);

    FOnStreamingStatusChanged OnStreamingStatusChanged;

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;

private:
    void InitializeGStreamerPipeline();
    void CleanupGStreamerPipeline();
    
#if WITH_GSTREAMER
    static void OnClientConnected(GstRTSPServer* Server, GstRTSPClient* Client, gpointer UserData);
    static void OnClientDisconnected(GstRTSPServer* Server, GstRTSPClient* Client, gpointer UserData);
    static void OnMediaConfigure(GstRTSPMediaFactory* Factory, GstRTSPMedia* Media, gpointer UserData);
    static gboolean OnBusMessage(GstBus* Bus, GstMessage* Message, gpointer UserData);
    static void OnNeedData(GstElement* Appsrc, guint Unused, gpointer UserData);
    static void OnEnoughData(GstElement* Appsrc, gpointer UserData);
    
    GstRTSPServer* RTSPServer;
    GstRTSPMountPoints* MountPoints;
    GstRTSPMediaFactory* Factory;
    GstElement* Pipeline;
    GstElement* AppSrc;
    GstBus* Bus;
    GMainLoop* MainLoop;
    GMainContext* MainContext;
#endif

    FRunnableThread* StreamingThread;
    FThreadSafeBool bShouldStop;
    FThreadSafeBool bIsStreaming;
    
    FString CurrentStreamPath;
    int32 StreamPort;
    
    mutable FCriticalSection FrameDataMutex;
    TArray<uint8> PendingFrameData;
    int32 FrameWidth;
    int32 FrameHeight;
    int32 FrameChannels;
    bool bHasPendingFrame;
};