#include "RTSPCameraManager.h"
#include "RealGazeboStreaming.h"
#include "Engine/World.h"
#include "EngineUtils.h"

URTSPCameraManager::URTSPCameraManager()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URTSPCameraManager::BeginPlay()
{
    Super::BeginPlay();

    // Auto-discover cameras if enabled
    if (bAutoDiscoverCameras)
    {
        AutoDiscoverCameras();
    }

    // Set up default camera configurations
    for (const FRTSPCameraConfig& Config : DefaultCameraConfigs)
    {
        // Try to find a camera component with the matching name
        for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
        {
            AActor* Actor = *ActorItr;
            if (Actor && Actor->GetName() == Config.CameraName)
            {
                URTSPCameraComponent* CameraComponent = Actor->FindComponentByClass<URTSPCameraComponent>();
                if (CameraComponent)
                {
                    AddCamera(Config.CameraName, CameraComponent, Config);
                    break;
                }
            }
        }
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP Camera Manager initialized with %d cameras"), ManagedCameras.Num());
}

void URTSPCameraManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopAllCameras();
    ManagedCameras.Empty();
    ComponentToCameraName.Empty();
    
    Super::EndPlay(EndPlayReason);
}

bool URTSPCameraManager::AddCamera(const FString& CameraName, URTSPCameraComponent* CameraComponent, const FRTSPCameraConfig& Config)
{
    if (!CameraComponent)
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Cannot add null camera component for %s"), *CameraName);
        return false;
    }

    if (ManagedCameras.Contains(CameraName))
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Camera %s already managed"), *CameraName);
        return false;
    }

    // Configure the camera
    FRTSPCameraConfig FinalConfig = Config;
    if (FinalConfig.StreamPort == BasePortNumber)
    {
        FinalConfig.StreamPort = GetNextAvailablePort();
    }

    SetupCameraFromConfig(CameraComponent, FinalConfig);

    // Add to managed cameras
    ManagedCameras.Add(CameraName, FCameraInfo(CameraComponent, FinalConfig));
    ComponentToCameraName.Add(CameraComponent, CameraName);

    // Bind to streaming status changes
    CameraComponent->OnStreamingStatusChanged.AddDynamic(this, &URTSPCameraManager::OnCameraStreamingStatusChangedInternal);

    // Auto-start if configured
    if (FinalConfig.bAutoStart)
    {
        StartCameraStreaming(CameraName);
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Added camera %s on port %d"), *CameraName, FinalConfig.StreamPort);
    return true;
}

bool URTSPCameraManager::RemoveCamera(const FString& CameraName)
{
    FCameraInfo* CameraInfo = ManagedCameras.Find(CameraName);
    if (!CameraInfo)
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Camera %s not found"), *CameraName);
        return false;
    }

    // Stop streaming if active
    StopCameraStreaming(CameraName);

    // Unbind delegates
    if (CameraInfo->Component)
    {
        CameraInfo->Component->OnStreamingStatusChanged.RemoveDynamic(this, &URTSPCameraManager::OnCameraStreamingStatusChangedInternal);
        ComponentToCameraName.Remove(CameraInfo->Component);
    }

    ManagedCameras.Remove(CameraName);

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Removed camera %s"), *CameraName);
    return true;
}

bool URTSPCameraManager::StartCameraStreaming(const FString& CameraName)
{
    FCameraInfo* CameraInfo = ManagedCameras.Find(CameraName);
    if (!CameraInfo || !CameraInfo->Component)
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Camera %s not found"), *CameraName);
        return false;
    }

    bool bSuccess = CameraInfo->Component->StartRTSPStreaming();
    if (bSuccess)
    {
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Started streaming for camera %s"), *CameraName);
    }
    else
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to start streaming for camera %s"), *CameraName);
    }

    return bSuccess;
}

void URTSPCameraManager::StopCameraStreaming(const FString& CameraName)
{
    FCameraInfo* CameraInfo = ManagedCameras.Find(CameraName);
    if (!CameraInfo || !CameraInfo->Component)
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Camera %s not found"), *CameraName);
        return;
    }

    CameraInfo->Component->StopRTSPStreaming();
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Stopped streaming for camera %s"), *CameraName);
}

void URTSPCameraManager::StartAllCameras()
{
    int32 StartedCount = 0;
    for (const auto& CameraPair : ManagedCameras)
    {
        if (StartCameraStreaming(CameraPair.Key))
        {
            StartedCount++;
        }
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Started streaming for %d/%d cameras"), StartedCount, ManagedCameras.Num());
}

void URTSPCameraManager::StopAllCameras()
{
    for (const auto& CameraPair : ManagedCameras)
    {
        StopCameraStreaming(CameraPair.Key);
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Stopped streaming for all cameras"));
}

bool URTSPCameraManager::IsCameraStreaming(const FString& CameraName) const
{
    const FCameraInfo* CameraInfo = ManagedCameras.Find(CameraName);
    if (!CameraInfo || !CameraInfo->Component)
    {
        return false;
    }

    return CameraInfo->Component->IsStreaming();
}

TArray<FString> URTSPCameraManager::GetCameraNames() const
{
    TArray<FString> Names;
    ManagedCameras.GetKeys(Names);
    return Names;
}

TArray<FString> URTSPCameraManager::GetStreamingCameras() const
{
    TArray<FString> StreamingCameras;
    
    for (const auto& CameraPair : ManagedCameras)
    {
        if (IsCameraStreaming(CameraPair.Key))
        {
            StreamingCameras.Add(CameraPair.Key);
        }
    }

    return StreamingCameras;
}

FString URTSPCameraManager::GetCameraStreamURL(const FString& CameraName) const
{
    const FCameraInfo* CameraInfo = ManagedCameras.Find(CameraName);
    if (!CameraInfo || !CameraInfo->Component)
    {
        return FString();
    }

    return CameraInfo->Component->GetStreamURL();
}

TMap<FString, FString> URTSPCameraManager::GetAllStreamURLs() const
{
    TMap<FString, FString> URLs;
    
    for (const auto& CameraPair : ManagedCameras)
    {
        if (CameraPair.Value.Component)
        {
            URLs.Add(CameraPair.Key, CameraPair.Value.Component->GetStreamURL());
        }
    }

    return URLs;
}

void URTSPCameraManager::OnCameraStreamingStatusChangedInternal(bool bIsStreaming)
{
    // Find which camera triggered this event
    URTSPCameraComponent* SourceComponent = Cast<URTSPCameraComponent>(GetWorld()->GetFirstPlayerController()->GetViewTarget()->FindComponentByClass<URTSPCameraComponent>());
    
    // This is a simplified approach - in a real implementation you'd need to track which component triggered the event
    for (const auto& CameraPair : ManagedCameras)
    {
        if (CameraPair.Value.Component && CameraPair.Value.Component->IsStreaming() == bIsStreaming)
        {
            OnCameraStreamingStatusChanged.Broadcast(CameraPair.Key, bIsStreaming);
            break;
        }
    }
}

void URTSPCameraManager::AutoDiscoverCameras()
{
    int32 DiscoveredCount = 0;

    for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        AActor* Actor = *ActorItr;
        if (!Actor)
            continue;

        URTSPCameraComponent* CameraComponent = Actor->FindComponentByClass<URTSPCameraComponent>();
        if (CameraComponent)
        {
            FString CameraName = Actor->GetName();
            
            // Skip if already managed
            if (ManagedCameras.Contains(CameraName))
                continue;

            // Create default config
            FRTSPCameraConfig Config;
            Config.CameraName = CameraName;
            Config.StreamPath = FString::Printf(TEXT("/camera_%d"), DiscoveredCount);
            Config.StreamPort = GetNextAvailablePort();
            Config.Resolution = FIntPoint(1920, 1080);
            Config.FrameRate = 30.0f;
            Config.bAutoStart = false; // Don't auto-start discovered cameras

            AddCamera(CameraName, CameraComponent, Config);
            DiscoveredCount++;
        }
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Auto-discovered %d RTSP camera components"), DiscoveredCount);
}

void URTSPCameraManager::SetupCameraFromConfig(URTSPCameraComponent* Camera, const FRTSPCameraConfig& Config)
{
    if (!Camera)
        return;

    Camera->StreamPath = Config.StreamPath;
    Camera->StreamPort = Config.StreamPort;
    Camera->StreamResolution = Config.Resolution;
    Camera->FrameRate = Config.FrameRate;
}

int32 URTSPCameraManager::GetNextAvailablePort() const
{
    TSet<int32> UsedPorts;
    
    for (const auto& CameraPair : ManagedCameras)
    {
        UsedPorts.Add(CameraPair.Value.Config.StreamPort);
    }

    for (const FRTSPCameraConfig& Config : DefaultCameraConfigs)
    {
        UsedPorts.Add(Config.StreamPort);
    }

    int32 Port = BasePortNumber;
    while (UsedPorts.Contains(Port))
    {
        Port++;
    }

    return Port;
}