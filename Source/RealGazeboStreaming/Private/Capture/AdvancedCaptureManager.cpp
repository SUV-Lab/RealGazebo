#include "Capture/AdvancedCaptureManager.h"
#include "Capture/VehicleCameraStreamComponent.h"
#include "RealGazeboStreaming.h"
#include "Engine/World.h"

UAdvancedCaptureManager::UAdvancedCaptureManager()
{
    bIsInitialized = false;
    TotalFramesCaptured = 0;
    LastPerformanceUpdate = 0.0f;
    CachedAverageFPS = 0.0f;
}

//----------------------------------------------------------
// Capture Management
//----------------------------------------------------------

bool UAdvancedCaptureManager::Initialize()
{
    if (bIsInitialized)
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("AdvancedCaptureManager already initialized"));
        return true;
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Initializing AdvancedCaptureManager"));

    // Initialize capture system
    RegisteredCameras.Empty();
    TotalFramesCaptured = 0;
    LastPerformanceUpdate = FPlatformTime::Seconds();

    bIsInitialized = true;
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("AdvancedCaptureManager initialized successfully"));
    return true;
}

void UAdvancedCaptureManager::Shutdown()
{
    if (!bIsInitialized)
    {
        return;
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Shutting down AdvancedCaptureManager"));

    // Cleanup registered cameras
    RegisteredCameras.Empty();

    bIsInitialized = false;
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("AdvancedCaptureManager shutdown complete"));
}

void UAdvancedCaptureManager::RegisterCamera(UVehicleCameraStreamComponent* Camera)
{
    if (!bIsInitialized)
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Cannot register camera: AdvancedCaptureManager not initialized"));
        return;
    }

    if (!Camera)
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Cannot register null camera"));
        return;
    }

    if (RegisteredCameras.Contains(Camera))
    {
        UE_LOG(LogRealGazeboStreaming, Verbose, TEXT("Camera already registered"));
        return;
    }

    RegisteredCameras.Add(Camera);
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Registered camera '%s' for capture management"), 
        *Camera->GetStreamID().ToString());

    // Optimize performance after adding new camera
    OptimizeCapturePerformance();
}

void UAdvancedCaptureManager::UnregisterCamera(UVehicleCameraStreamComponent* Camera)
{
    if (!Camera)
    {
        return;
    }

    if (RegisteredCameras.Remove(Camera) > 0)
    {
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Unregistered camera '%s' from capture management"), 
            *Camera->GetStreamID().ToString());

        // Optimize performance after removing camera
        OptimizeCapturePerformance();
    }
}

void UAdvancedCaptureManager::GetCaptureStats(int32& OutActiveCameras, float& OutAverageFPS, float& OutMemoryUsageMB) const
{
    UpdatePerformanceStats();

    OutActiveCameras = RegisteredCameras.Num();
    OutAverageFPS = CachedAverageFPS;
    OutMemoryUsageMB = 0.0f; // TODO: Implement memory usage tracking

    // Calculate active cameras (those currently streaming)
    int32 ActiveStreamingCameras = 0;
    for (const UVehicleCameraStreamComponent* Camera : RegisteredCameras)
    {
        if (Camera && Camera->IsStreaming())
        {
            ActiveStreamingCameras++;
        }
    }
    
    OutActiveCameras = ActiveStreamingCameras;
}

//----------------------------------------------------------
// Performance Optimization
//----------------------------------------------------------

void UAdvancedCaptureManager::OptimizeCapturePerformance()
{
    if (!bIsInitialized || RegisteredCameras.Num() == 0)
    {
        return;
    }

    UE_LOG(LogRealGazeboStreaming, Verbose, TEXT("Optimizing capture performance for %d cameras"), 
        RegisteredCameras.Num());

    // TODO: Implement performance optimization strategies:
    // - Adjust frame rates based on system load
    // - Coordinate capture timing across cameras
    // - Manage memory usage and buffer sizes
    // - Balance quality vs performance based on active stream count

    // For now, just log the optimization attempt
    UE_LOG(LogRealGazeboStreaming, Verbose, TEXT("Capture performance optimization completed"));
}

void UAdvancedCaptureManager::UpdatePerformanceStats() const
{
    const double CurrentTime = FPlatformTime::Seconds();
    const double TimeDelta = CurrentTime - LastPerformanceUpdate;

    if (TimeDelta >= 1.0) // Update every second
    {
        // Calculate average FPS across all cameras
        float TotalFPS = 0.0f;
        int32 ActiveCameras = 0;

        for (const UVehicleCameraStreamComponent* Camera : RegisteredCameras)
        {
            if (Camera && Camera->IsStreaming())
            {
                TotalFPS += Camera->GetCurrentFPS();
                ActiveCameras++;
            }
        }

        if (ActiveCameras > 0)
        {
            CachedAverageFPS = TotalFPS / ActiveCameras;
        }
        else
        {
            CachedAverageFPS = 0.0f;
        }

        LastPerformanceUpdate = CurrentTime;
    }
}