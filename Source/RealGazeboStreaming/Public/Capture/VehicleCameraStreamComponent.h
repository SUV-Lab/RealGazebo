#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
// PixelCapture forward declarations
namespace UE::PixelCapture { class FPixelCaptureCapturerRHIToI420CPU; }
#include "StreamingTypes.h"
#include "VehicleCameraStreamComponent.generated.h"

// Forward declarations
class FAdvancedRTSPStreamer;
class UAdvancedCaptureManager;
class FPixelCaptureCapturer;

/**
 * High-performance vehicle camera component with optimized streaming
 * 
 * Features:
 * - Integration with PixelCapture for GPU-optimized frame capture
 * - Multi-format support (I420, RGB, etc.)
 * - Automatic vehicle attachment and positioning
 * - Performance monitoring and frame rate control
 * - Advanced streaming pipeline with GStreamer
 */
UCLASS(ClassGroup=(RealGazebo), meta=(BlueprintSpawnableComponent))
class REALGAZEBOSTREAMING_API UVehicleCameraStreamComponent : public USceneCaptureComponent2D
{
    GENERATED_BODY()

public:
    UVehicleCameraStreamComponent();

    //----------------------------------------------------------
    // Component Lifecycle
    //----------------------------------------------------------
    
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    //----------------------------------------------------------
    // Configuration
    //----------------------------------------------------------

    /** Camera stream configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Stream")
    FCameraStreamConfig StreamConfig;

    /** Vehicle ID this camera belongs to */
    UPROPERTY(BlueprintReadOnly, Category = "Camera Stream")
    FVehicleID VehicleID;

    /** Camera mount position relative to vehicle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Stream")
    FVector CameraOffset = FVector(100.0f, 0.0f, 50.0f);

    /** Camera mount rotation relative to vehicle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Stream")
    FRotator CameraRotation = FRotator(0.0f, 0.0f, 0.0f);

    /** Enable performance optimizations */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Stream|Performance")
    bool bUsePixelCaptureOptimization = true;

    /** Capture format (affects performance and quality) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Stream|Performance")
    FString CaptureFormat = TEXT("I420");

    //----------------------------------------------------------
    // Streaming Control
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Camera Stream")
    bool StartStreaming();

    UFUNCTION(BlueprintCallable, Category = "Camera Stream")
    void StopStreaming();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Camera Stream")
    bool IsStreaming() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Camera Stream")
    FString GetStreamURL() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Camera Stream")
    FStreamID GetStreamID() const;

    //----------------------------------------------------------
    // Vehicle Integration
    //----------------------------------------------------------

    /** Initialize camera for specific vehicle */
    UFUNCTION(BlueprintCallable, Category = "Camera Stream")
    void InitializeForVehicle(const FVehicleID& InVehicleID, class AVehicleBasePawn* VehiclePawn);

    /** Update camera configuration */
    UFUNCTION(BlueprintCallable, Category = "Camera Stream")
    bool UpdateStreamConfig(const FCameraStreamConfig& NewConfig);

    /** Attach to vehicle with offset */
    UFUNCTION(BlueprintCallable, Category = "Camera Stream")
    void AttachToVehicle(AActor* VehicleActor);

    //----------------------------------------------------------
    // Performance Monitoring
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Camera Stream")
    float GetCurrentFPS() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Camera Stream")
    FStreamingRuntimeData GetRuntimeData() const;

    /** Get capture performance statistics */
    UFUNCTION(BlueprintCallable, Category = "Camera Stream")
    void GetCaptureStats(float& OutCaptureTime, float& OutStreamTime, int32& OutDroppedFrames) const;

    //----------------------------------------------------------
    // Events
    //----------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Camera Stream|Events")
    FOnStreamingStatusChanged OnStreamingStatusChanged;

protected:
    //----------------------------------------------------------
    // Capture Pipeline
    //----------------------------------------------------------

    /** Initialize render target and capture pipeline */
    void InitializeCaptureSystem();

    /** Shutdown capture system */
    void ShutdownCaptureSystem();

    /** Initialize PixelCapture integration */
    void InitializePixelCapture();

    /** Process captured frame for streaming */
    void ProcessCapturedFrame();

    /** Update streaming frame (called at target framerate) */
    void UpdateStreamingFrame();

    //----------------------------------------------------------
    // Frame Processing
    //----------------------------------------------------------

    /** Convert RHI texture to streaming format */
    void ConvertFrameToStreamingFormat(const TSharedPtr<class IPixelCaptureOutputFrame>& OutputFrame);

    /** Send frame to streaming pipeline */
    void SendFrameToStreamer(const TArray<uint8>& FrameData, int32 Width, int32 Height, int32 Channels);

private:
    //----------------------------------------------------------
    // Core Components
    //----------------------------------------------------------

    /** High-performance render target */
    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D> StreamRenderTarget;

    /** PixelCapture integration for optimized frame capture */
    TSharedPtr<UE::PixelCapture::FPixelCaptureCapturerRHIToI420CPU> PixelCapturer;

    /** Advanced RTSP streamer */
    TSharedPtr<FAdvancedRTSPStreamer> RTSPStreamer;

    /** Reference to capture manager */
    TWeakObjectPtr<UAdvancedCaptureManager> CaptureManager;

    //----------------------------------------------------------
    // Performance Tracking
    //----------------------------------------------------------

    /** Frame timing */
    float LastFrameTime = 0.0f;
    float TargetFrameTime = 1.0f / 30.0f;
    float CurrentFPS = 0.0f;
    
    /** Performance counters */
    float LastCaptureTime = 0.0f;
    float LastStreamTime = 0.0f;
    int32 DroppedFrameCount = 0;
    int32 TotalFrameCount = 0;

    /** Frame rate limiter */
    float FrameAccumulator = 0.0f;

    //----------------------------------------------------------
    // State Management
    //----------------------------------------------------------

    bool bIsInitialized = false;
    bool bIsStreamingActive = false;
    bool bPixelCaptureReady = false;

    /** Associated vehicle pawn */
    TWeakObjectPtr<AActor> AttachedVehicle;

    /** Thread-safe frame data exchange */
    mutable FCriticalSection FrameDataMutex;
    TArray<uint8> PendingFrameData;
    int32 PendingFrameWidth = 0;
    int32 PendingFrameHeight = 0;
    int32 PendingFrameChannels = 0;
    bool bHasPendingFrame = false;

    //----------------------------------------------------------
    // Utility Methods
    //----------------------------------------------------------

    /** Create optimized render target */
    UTextureRenderTarget2D* CreateOptimizedRenderTarget(int32 Width, int32 Height);

    /** Update render target if resolution changed */
    void UpdateRenderTargetIfNeeded();

    /** Calculate target frame time from config */
    void UpdateTargetFrameTime();

    /** Validate camera configuration */
    bool ValidateConfiguration() const;

    /** Generate stream URL */
    FString GenerateStreamURL() const;

    //----------------------------------------------------------
    // Event Handlers
    //----------------------------------------------------------

    /** Handle successful frame capture */
    void OnFrameCaptured(const TSharedPtr<class IPixelCaptureOutputFrame>& OutputFrame);

    /** Handle capture error */
    void OnCaptureError(const FString& ErrorMessage);

    /** Handle streaming status change */
    void OnStreamingStatusChangedInternal(bool bNewIsStreaming);

public:
    //----------------------------------------------------------
    // Debug and Development
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Camera Stream|Debug")
    void PrintCameraInfo() const;

    UFUNCTION(BlueprintCallable, Category = "Camera Stream|Debug")
    void PrintPerformanceStats() const;

    /** Save current frame to disk for debugging */
    UFUNCTION(BlueprintCallable, Category = "Camera Stream|Debug")
    bool SaveCurrentFrameToDisk(const FString& FilePath) const;
};