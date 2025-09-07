#include "Core/StreamingSubsystem.h"
#include "Capture/VehicleCameraManager.h"
#include "Capture/AdvancedCaptureManager.h"
#include "RealGazeboStreaming.h"

UStreamingSubsystem::UStreamingSubsystem()
{
    // Initialize subsystem
    bAutoCreateCamerasOnVehicleSpawn = true;
    bAutoStartStreaming = false;
    BaseRTSPPort = 8554;
    MaxConcurrentStreams = 50;
}

//----------------------------------------------------------
// USubsystem Interface
//----------------------------------------------------------

void UStreamingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("StreamingSubsystem: Initializing"));
    
    // Create manager instances
    CameraManager = NewObject<UVehicleCameraManager>(this);
    CaptureManager = NewObject<UAdvancedCaptureManager>(this);
    
    bIsStreamingSystemActive = true;
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("StreamingSubsystem: Initialized successfully"));
}

void UStreamingSubsystem::Deinitialize()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("StreamingSubsystem: Deinitializing"));
    
    // Stop all streaming
    StopAllStreaming();
    
    // Clean up managers
    CameraManager = nullptr;
    CaptureManager = nullptr;
    
    bIsStreamingSystemActive = false;
    
    Super::Deinitialize();
}

bool UStreamingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    // Only create in game instances, not in editor tools
    return Super::ShouldCreateSubsystem(Outer);
}

//----------------------------------------------------------
// Static Access
//----------------------------------------------------------

UStreamingSubsystem* UStreamingSubsystem::GetStreamingSubsystem(const UObject* WorldContext)
{
    if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UStreamingSubsystem>();
        }
    }
    return nullptr;
}

//----------------------------------------------------------
// Runtime Control
//----------------------------------------------------------

void UStreamingSubsystem::StartStreamingSystem()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Starting streaming system"));
}

void UStreamingSubsystem::StopStreamingSystem()
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Stopping streaming system"));
}

bool UStreamingSubsystem::IsStreamingSystemActive() const
{
    return bIsStreamingSystemActive;
}

//----------------------------------------------------------
// Vehicle Integration
//----------------------------------------------------------

void UStreamingSubsystem::OnVehicleSpawned(const FVehicleID& VehicleID, AVehicleBasePawn* VehiclePawn)
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Vehicle spawned: %s"), *VehicleID.ToString());
}

void UStreamingSubsystem::OnVehicleDespawned(const FVehicleID& VehicleID)
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Vehicle despawned: %s"), *VehicleID.ToString());
}

TArray<FString> UStreamingSubsystem::GetVehicleCameraNames(const FVehicleID& VehicleID) const
{
    return TArray<FString>();
}

//----------------------------------------------------------
// Manual Camera Management  
//----------------------------------------------------------

bool UStreamingSubsystem::CreateVehicleCamera(const FVehicleID& VehicleID, const FCameraStreamConfig& CameraConfig)
{
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("CreateVehicleCamera not yet implemented"));
    return false;
}

void UStreamingSubsystem::RemoveVehicleCamera(const FVehicleID& VehicleID, const FString& CameraName)
{
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("RemoveVehicleCamera not yet implemented"));
}

bool UStreamingSubsystem::UpdateCameraConfig(const FStreamID& StreamID, const FCameraStreamConfig& NewConfig)
{
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("UpdateCameraConfig not yet implemented"));
    return false;
}

//----------------------------------------------------------
// Streaming Control
//----------------------------------------------------------

bool UStreamingSubsystem::StartCameraStreaming(const FStreamID& StreamID)
{
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("StartCameraStreaming not yet implemented"));
    return false;
}

void UStreamingSubsystem::StopCameraStreaming(const FStreamID& StreamID)
{
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("StopCameraStreaming not yet implemented"));
}

void UStreamingSubsystem::StartVehicleStreaming(const FVehicleID& VehicleID)
{
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("StartVehicleStreaming not yet implemented"));
}

void UStreamingSubsystem::StopVehicleStreaming(const FVehicleID& VehicleID)
{
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("StopVehicleStreaming not yet implemented"));
}

void UStreamingSubsystem::StartAllStreaming()
{
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("StartAllStreaming not yet implemented"));
}

void UStreamingSubsystem::StopAllStreaming()
{
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("StopAllStreaming not yet implemented"));
}

//----------------------------------------------------------
// Status and Information
//----------------------------------------------------------

bool UStreamingSubsystem::IsCameraStreaming(const FStreamID& StreamID) const
{
    return false;
}

FString UStreamingSubsystem::GetStreamURL(const FStreamID& StreamID) const
{
    return FString();
}

TMap<FString, FString> UStreamingSubsystem::GetAllStreamURLs() const
{
    return TMap<FString, FString>();
}

FStreamingPerformanceStats UStreamingSubsystem::GetPerformanceStats() const
{
    return FStreamingPerformanceStats();
}

FStreamingRuntimeData UStreamingSubsystem::GetStreamRuntimeData(const FStreamID& StreamID) const
{
    return FStreamingRuntimeData();
}

TArray<FStreamID> UStreamingSubsystem::GetAllActiveStreamIDs() const
{
    return TArray<FStreamID>();
}

//----------------------------------------------------------
// Performance and Monitoring
//----------------------------------------------------------

void UStreamingSubsystem::SetPerformanceMonitoring(bool bEnabled)
{
    bPerformanceMonitoringEnabled = bEnabled;
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Performance monitoring %s"), 
        bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void UStreamingSubsystem::PrintPerformanceStats() const
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("=== Streaming Performance Stats ==="));
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("TODO: Implement performance stats"));
}

void UStreamingSubsystem::PrintAllStreamURLs() const
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("=== Active Stream URLs ==="));
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("TODO: Implement stream URL listing"));
}

//----------------------------------------------------------
// RealGazebo Integration
//----------------------------------------------------------

void UStreamingSubsystem::OnRealGazeboVehicleSpawned(const FBridgePoseData& VehicleData)
{
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RealGazebo vehicle spawned"));
}

//----------------------------------------------------------
// Internal Event Handlers  
//----------------------------------------------------------

void UStreamingSubsystem::OnStreamingStatusChangedInternal(const FString& CameraID, bool bIsStreaming)
{
    OnStreamingStatusChanged.Broadcast(CameraID, bIsStreaming);
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Stream status changed - Camera: %s, Active: %s"),
        *CameraID, bIsStreaming ? TEXT("YES") : TEXT("NO"));
}