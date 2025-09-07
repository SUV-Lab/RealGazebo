#include "Capture/VehicleCameraStreamComponent.h"
#include "Streaming/AdvancedRTSPStreamer.h"
#include "Capture/AdvancedCaptureManager.h"
#include "RealGazeboStreaming.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/StaticMeshComponent.h"

UVehicleCameraStreamComponent::UVehicleCameraStreamComponent()
{
    // Set defaults
    PrimaryComponentTick.bCanEverTick = true;
    bCaptureEveryFrame = false;
    bCaptureOnMovement = false;
    
    // Initialize streaming config with defaults
    StreamConfig = FCameraStreamConfig();
    
    // Initialize state
    bIsInitialized = false;
    bIsStreamingActive = false;
    bPixelCaptureReady = false;
    bHasPendingFrame = false;
    
    // Initialize performance counters
    LastFrameTime = 0.0f;
    TargetFrameTime = 1.0f / 30.0f; // 30 FPS default
    CurrentFPS = 0.0f;
    LastCaptureTime = 0.0f;
    LastStreamTime = 0.0f;
    DroppedFrameCount = 0;
    TotalFrameCount = 0;
    FrameAccumulator = 0.0f;
    
    // Initialize frame data
    PendingFrameWidth = 0;
    PendingFrameHeight = 0;
    PendingFrameChannels = 0;
}

//----------------------------------------------------------
// Component Lifecycle
//----------------------------------------------------------

void UVehicleCameraStreamComponent::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("VehicleCameraStreamComponent BeginPlay: %s"), 
        *StreamConfig.CameraName);
    
    InitializeCaptureSystem();
    
    if (StreamConfig.bAutoStart)
    {
        StartStreaming();
    }
}

void UVehicleCameraStreamComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("VehicleCameraStreamComponent EndPlay: %s"), 
        *StreamConfig.CameraName);
    
    StopStreaming();
    ShutdownCaptureSystem();
    
    Super::EndPlay(EndPlayReason);
}

void UVehicleCameraStreamComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (bIsStreamingActive)
    {
        FrameAccumulator += DeltaTime;
        
        // Process at target frame rate
        if (FrameAccumulator >= TargetFrameTime)
        {
            UpdateStreamingFrame();
            FrameAccumulator = 0.0f;
        }
    }
}

//----------------------------------------------------------
// Streaming Control
//----------------------------------------------------------

bool UVehicleCameraStreamComponent::StartStreaming()
{
    if (bIsStreamingActive)
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Camera '%s' is already streaming"), 
            *StreamConfig.CameraName);
        return true;
    }
    
    if (!bIsInitialized)
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Cannot start streaming: camera '%s' not initialized"), 
            *StreamConfig.CameraName);
        return false;
    }
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Starting streaming for camera '%s'"), 
        *StreamConfig.CameraName);
    
    // Create RTSP streamer if needed
    if (!RTSPStreamer.IsValid())
    {
        RTSPStreamer = MakeShared<FAdvancedRTSPStreamer>();
        if (!RTSPStreamer.IsValid())
        {
            UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to create RTSP streamer"));
            return false;
        }
    }
    
    // Start RTSP streaming
    if (!RTSPStreamer->StartStreaming(StreamConfig))
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to start RTSP streaming"));
        return false;
    }
    
    bIsStreamingActive = true;
    OnStreamingStatusChangedInternal(true);
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Streaming started successfully for camera '%s': %s"), 
        *StreamConfig.CameraName, *GetStreamURL());
    
    return true;
}

void UVehicleCameraStreamComponent::StopStreaming()
{
    if (!bIsStreamingActive)
    {
        return;
    }
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Stopping streaming for camera '%s'"), 
        *StreamConfig.CameraName);
    
    if (RTSPStreamer.IsValid())
    {
        RTSPStreamer->StopStreaming();
    }
    
    bIsStreamingActive = false;
    OnStreamingStatusChangedInternal(false);
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Streaming stopped for camera '%s'"), 
        *StreamConfig.CameraName);
}

bool UVehicleCameraStreamComponent::IsStreaming() const
{
    return bIsStreamingActive && RTSPStreamer.IsValid() && RTSPStreamer->IsStreaming();
}

FString UVehicleCameraStreamComponent::GetStreamURL() const
{
    if (RTSPStreamer.IsValid())
    {
        return RTSPStreamer->GetStreamURL();
    }
    return GenerateStreamURL();
}

FStreamID UVehicleCameraStreamComponent::GetStreamID() const
{
    return FStreamID(VehicleID, StreamConfig.CameraName);
}

//----------------------------------------------------------
// Vehicle Integration
//----------------------------------------------------------

void UVehicleCameraStreamComponent::InitializeForVehicle(const FVehicleID& InVehicleID, AVehicleBasePawn* VehiclePawn)
{
    VehicleID = InVehicleID;
    AttachedVehicle = VehiclePawn;
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Camera '%s' initialized for vehicle %s"), 
        *StreamConfig.CameraName, *VehicleID.ToString());
}

bool UVehicleCameraStreamComponent::UpdateStreamConfig(const FCameraStreamConfig& NewConfig)
{
    FCameraStreamConfig OldConfig = StreamConfig;
    StreamConfig = NewConfig;
    
    // Update target frame time
    UpdateTargetFrameTime();
    
    // If streaming is active, restart with new config
    if (bIsStreamingActive)
    {
        StopStreaming();
        return StartStreaming();
    }
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Updated stream config for camera '%s'"), 
        *StreamConfig.CameraName);
    
    return true;
}

void UVehicleCameraStreamComponent::AttachToVehicle(AActor* VehicleActor)
{
    if (!VehicleActor)
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Cannot attach camera: VehicleActor is null"));
        return;
    }
    
    // Attach to vehicle's root component
    if (USceneComponent* RootComp = VehicleActor->GetRootComponent())
    {
        SetupAttachment(RootComp);
        SetRelativeLocation(CameraOffset);
        SetRelativeRotation(CameraRotation);
        
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Camera '%s' attached to vehicle"), 
            *StreamConfig.CameraName);
    }
}

//----------------------------------------------------------
// Performance Monitoring
//----------------------------------------------------------

float UVehicleCameraStreamComponent::GetCurrentFPS() const
{
    return CurrentFPS;
}

FStreamingRuntimeData UVehicleCameraStreamComponent::GetRuntimeData() const
{
    FStreamingRuntimeData RuntimeData;
    RuntimeData.VehicleID = VehicleID;
    RuntimeData.CameraName = StreamConfig.CameraName;
    RuntimeData.bIsStreaming = IsStreaming();
    RuntimeData.StreamURL = GetStreamURL();
    RuntimeData.CurrentFPS = GetCurrentFPS();
    RuntimeData.LastFrameTime = LastFrameTime;
    
    return RuntimeData;
}

void UVehicleCameraStreamComponent::GetCaptureStats(float& OutCaptureTime, float& OutStreamTime, int32& OutDroppedFrames) const
{
    OutCaptureTime = LastCaptureTime;
    OutStreamTime = LastStreamTime;
    OutDroppedFrames = DroppedFrameCount;
}

//----------------------------------------------------------
// Capture Pipeline
//----------------------------------------------------------

void UVehicleCameraStreamComponent::InitializeCaptureSystem()
{
    if (bIsInitialized)
    {
        return;
    }
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Initializing capture system for camera '%s'"), 
        *StreamConfig.CameraName);
    
    // Create render target
    StreamRenderTarget = CreateOptimizedRenderTarget(
        StreamConfig.StreamResolution.X, 
        StreamConfig.StreamResolution.Y
    );
    
    if (!StreamRenderTarget)
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to create render target"));
        return;
    }
    
    // Set render target  
    // SetTextureTarget(StreamRenderTarget); // TODO: Implement render target setup
    
    // Initialize PixelCapture (will be implemented later)
    InitializePixelCapture();
    
    // Update target frame time
    UpdateTargetFrameTime();
    
    bIsInitialized = true;
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Capture system initialized for camera '%s'"), 
        *StreamConfig.CameraName);
}

void UVehicleCameraStreamComponent::ShutdownCaptureSystem()
{
    if (!bIsInitialized)
    {
        return;
    }
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Shutting down capture system for camera '%s'"), 
        *StreamConfig.CameraName);
    
    // Clean up PixelCapture
    PixelCapturer.Reset();
    bPixelCaptureReady = false;
    
    // Clean up render target
    if (StreamRenderTarget)
    {
        StreamRenderTarget = nullptr;
    }
    
    bIsInitialized = false;
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Capture system shutdown complete for camera '%s'"), 
        *StreamConfig.CameraName);
}

void UVehicleCameraStreamComponent::InitializePixelCapture()
{
    // TODO: Initialize PixelCapture system
    // This will be implemented when PixelCapture integration is ready
    bPixelCaptureReady = false;
}

void UVehicleCameraStreamComponent::ProcessCapturedFrame()
{
    // TODO: Process captured frame through PixelCapture
    // This is a placeholder implementation
}

void UVehicleCameraStreamComponent::UpdateStreamingFrame()
{
    if (!bIsStreamingActive || !bIsInitialized)
    {
        return;
    }
    
    const float CurrentTime = FPlatformTime::Seconds();
    LastFrameTime = CurrentTime;
    TotalFrameCount++;
    
    // Update FPS calculation
    static float LastFPSTime = CurrentTime;
    static int32 FramesThisSecond = 0;
    FramesThisSecond++;
    
    if (CurrentTime - LastFPSTime >= 1.0f)
    {
        CurrentFPS = FramesThisSecond / (CurrentTime - LastFPSTime);
        LastFPSTime = CurrentTime;
        FramesThisSecond = 0;
    }
    
    // Process capture (placeholder)
    ProcessCapturedFrame();
}

//----------------------------------------------------------
// Utility Methods
//----------------------------------------------------------

UTextureRenderTarget2D* UVehicleCameraStreamComponent::CreateOptimizedRenderTarget(int32 Width, int32 Height)
{
    UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(this);
    if (!RenderTarget)
    {
        return nullptr;
    }
    
    RenderTarget->RenderTargetFormat = RTF_RGBA8;
    RenderTarget->ClearColor = FLinearColor::Black;
    RenderTarget->bAutoGenerateMips = false;
    RenderTarget->InitAutoFormat(Width, Height);
    RenderTarget->UpdateResourceImmediate(true);
    
    return RenderTarget;
}

void UVehicleCameraStreamComponent::UpdateRenderTargetIfNeeded()
{
    if (!StreamRenderTarget)
    {
        return;
    }
    
    if (StreamRenderTarget->SizeX != StreamConfig.StreamResolution.X ||
        StreamRenderTarget->SizeY != StreamConfig.StreamResolution.Y)
    {
        StreamRenderTarget->ResizeTarget(StreamConfig.StreamResolution.X, StreamConfig.StreamResolution.Y);
    }
}

void UVehicleCameraStreamComponent::UpdateTargetFrameTime()
{
    TargetFrameTime = 1.0f / FMath::Max(StreamConfig.FrameRate, 1.0f);
}

bool UVehicleCameraStreamComponent::ValidateConfiguration() const
{
    if (StreamConfig.CameraName.IsEmpty())
    {
        return false;
    }
    
    if (StreamConfig.StreamResolution.X <= 0 || StreamConfig.StreamResolution.Y <= 0)
    {
        return false;
    }
    
    if (StreamConfig.FrameRate <= 0.0f)
    {
        return false;
    }
    
    return true;
}

FString UVehicleCameraStreamComponent::GenerateStreamURL() const
{
    return FString::Printf(TEXT("rtsp://localhost:%d%s"), 
        StreamConfig.StreamPort, 
        *StreamConfig.StreamPath);
}

void UVehicleCameraStreamComponent::OnStreamingStatusChangedInternal(bool bNewIsStreaming)
{
    OnStreamingStatusChanged.Broadcast(StreamConfig.CameraName, bNewIsStreaming);
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Camera '%s' streaming status changed: %s"), 
        *StreamConfig.CameraName, bNewIsStreaming ? TEXT("ACTIVE") : TEXT("INACTIVE"));
}

//----------------------------------------------------------
// Debug and Development
//----------------------------------------------------------

void UVehicleCameraStreamComponent::PrintCameraInfo() const
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("=== Camera Info: %s ==="), *StreamConfig.CameraName);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Vehicle ID: %s"), *VehicleID.ToString());
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Stream URL: %s"), *GetStreamURL());
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Resolution: %dx%d"), 
        StreamConfig.StreamResolution.X, StreamConfig.StreamResolution.Y);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Frame Rate: %.2f FPS"), StreamConfig.FrameRate);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Current FPS: %.2f"), GetCurrentFPS());
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Is Streaming: %s"), IsStreaming() ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Is Initialized: %s"), bIsInitialized ? TEXT("YES") : TEXT("NO"));
}

void UVehicleCameraStreamComponent::PrintPerformanceStats() const
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("=== Performance Stats: %s ==="), *StreamConfig.CameraName);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Total Frames: %d"), TotalFrameCount);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Dropped Frames: %d"), DroppedFrameCount);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Current FPS: %.2f"), CurrentFPS);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Target FPS: %.2f"), 1.0f / TargetFrameTime);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Last Capture Time: %.3f ms"), LastCaptureTime * 1000.0f);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Last Stream Time: %.3f ms"), LastStreamTime * 1000.0f);
}

bool UVehicleCameraStreamComponent::SaveCurrentFrameToDisk(const FString& FilePath) const
{
    // TODO: Implement frame saving functionality
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("SaveCurrentFrameToDisk not yet implemented"));
    return false;
}