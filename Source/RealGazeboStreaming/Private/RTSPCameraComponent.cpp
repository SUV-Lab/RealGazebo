#include "RTSPCameraComponent.h"
#include "RTSPStreamer.h"
#include "RealGazeboStreaming.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "PixelFormat.h"
#include "RenderTargetPool.h"
#include "TextureResource.h"
#include "RHICommandList.h"

URTSPCameraComponent::URTSPCameraComponent()
    : StreamRenderTarget(nullptr)
    , LastFrameTime(0.0f)
    , bIsInitialized(false)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    
    // Set default capture settings
    CaptureSource = SCS_FinalColorLDR;
    bCaptureEveryFrame = true;
    bCaptureOnMovement = false;
}

void URTSPCameraComponent::BeginPlay()
{
    Super::BeginPlay();
    
    InitializeRenderTarget();
    
    // Create RTSP streamer
    RTSPStreamer = MakeShared<FRTSPStreamer>();
    RTSPStreamer->OnStreamingStatusChanged.AddUObject(this, &URTSPCameraComponent::OnStreamingStatusChangedInternal);
    
    bIsInitialized = true;
    
    if (bAutoStartStreaming)
    {
        StartRTSPStreaming();
    }
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP Camera Component initialized"));
}

void URTSPCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopRTSPStreaming();
    RTSPStreamer.Reset();
    
    Super::EndPlay(EndPlayReason);
}

void URTSPCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (bIsInitialized && IsStreaming())
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        float TargetFrameTime = 1.0f / FrameRate;
        
        if (CurrentTime - LastFrameTime >= TargetFrameTime)
        {
            UpdateStreamingFrame();
            LastFrameTime = CurrentTime;
        }
    }
}

bool URTSPCameraComponent::StartRTSPStreaming()
{
    if (!bIsInitialized || !RTSPStreamer.IsValid())
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("RTSP Camera Component not properly initialized"));
        return false;
    }
    
    if (RTSPStreamer->IsStreaming())
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("RTSP streaming already active"));
        return true;
    }
    
    bool bSuccess = RTSPStreamer->StartStreaming(StreamPath, StreamPort);
    if (bSuccess)
    {
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP streaming started on %s:%d%s"), 
               TEXT("*"), StreamPort, *StreamPath);
    }
    else
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to start RTSP streaming"));
    }
    
    return bSuccess;
}

void URTSPCameraComponent::StopRTSPStreaming()
{
    if (RTSPStreamer.IsValid())
    {
        RTSPStreamer->StopStreaming();
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP streaming stopped"));
    }
}

bool URTSPCameraComponent::IsStreaming() const
{
    return RTSPStreamer.IsValid() && RTSPStreamer->IsStreaming();
}

FString URTSPCameraComponent::GetStreamURL() const
{
    return FString::Printf(TEXT("rtsp://localhost:%d%s"), StreamPort, *StreamPath);
}

void URTSPCameraComponent::InitializeRenderTarget()
{
    if (!StreamRenderTarget)
    {
        StreamRenderTarget = NewObject<UTextureRenderTarget2D>(this);
        StreamRenderTarget->RenderTargetFormat = RTF_RGBA8;
        StreamRenderTarget->SizeX = StreamResolution.X;
        StreamRenderTarget->SizeY = StreamResolution.Y;
        StreamRenderTarget->bAutoGenerateMips = false;
        StreamRenderTarget->InitAutoFormat(StreamResolution.X, StreamResolution.Y);
        StreamRenderTarget->UpdateResourceImmediate();
        
        TextureTarget = StreamRenderTarget;
        
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Render target initialized: %dx%d"), 
               StreamResolution.X, StreamResolution.Y);
    }
}

void URTSPCameraComponent::UpdateStreamingFrame()
{
    if (!StreamRenderTarget || !RTSPStreamer.IsValid() || !IsStreaming())
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("UpdateStreamingFrame failed: StreamRenderTarget=%s, RTSPStreamer=%s, IsStreaming=%s"),
               StreamRenderTarget ? TEXT("Valid") : TEXT("Invalid"),
               RTSPStreamer.IsValid() ? TEXT("Valid") : TEXT("Invalid"),
               IsStreaming() ? TEXT("True") : TEXT("False"));
        return;
    }
    
    // Capture the current frame
    CaptureScene();
    UE_LOG(LogRealGazeboStreaming, VeryVerbose, TEXT("Frame captured for streaming"));
    
    // Read pixels from render target
    FTextureRenderTargetResource* RenderTargetResource = StreamRenderTarget->GameThread_GetRenderTargetResource();
    if (!RenderTargetResource)
    {
        return;
    }
    
    // Create a task to read pixels on the render thread
    ENQUEUE_RENDER_COMMAND(ReadRTSPPixels)(
        [this, RenderTargetResource](FRHICommandListImmediate& RHICmdList)
        {
            FIntPoint Size = RenderTargetResource->GetSizeXY();
            TArray<FColor> ColorData;
            ColorData.SetNumUninitialized(Size.X * Size.Y);
            
            // Read pixels from GPU
            RHICmdList.ReadSurfaceData(
                RenderTargetResource->GetRenderTargetTexture(),
                FIntRect(0, 0, Size.X, Size.Y),
                ColorData,
                FReadSurfaceDataFlags()
            );
            
            // Convert to RGB format for streaming
            TArray<uint8> RGBData;
            RGBData.SetNumUninitialized(Size.X * Size.Y * 3);
            
            for (int32 i = 0; i < ColorData.Num(); ++i)
            {
                RGBData[i * 3 + 0] = ColorData[i].R;
                RGBData[i * 3 + 1] = ColorData[i].G;
                RGBData[i * 3 + 2] = ColorData[i].B;
            }
            
            // Push frame to RTSP streamer on game thread
            AsyncTask(ENamedThreads::GameThread, [this, RGBData = MoveTemp(RGBData), Size]()
            {
                if (RTSPStreamer.IsValid() && RTSPStreamer->IsStreaming())
                {
                    RTSPStreamer->PushFrame(RGBData, Size.X, Size.Y, 3);
                    UE_LOG(LogRealGazeboStreaming, VeryVerbose, TEXT("Frame pushed to RTSP streamer: %dx%d"), Size.X, Size.Y);
                }
                else
                {
                    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Failed to push frame: RTSPStreamer invalid or not streaming"));
                }
            });
        }
    );
}

void URTSPCameraComponent::OnStreamingStatusChangedInternal(bool bIsStreaming)
{
    OnStreamingStatusChanged.Broadcast(bIsStreaming);
}