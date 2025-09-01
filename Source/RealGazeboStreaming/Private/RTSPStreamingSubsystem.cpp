#include "RTSPStreamingSubsystem.h"
#include "RealGazeboStreaming.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void URTSPStreamingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP Streaming Subsystem initialized"));
}

void URTSPStreamingSubsystem::Deinitialize()
{
    StopAllCameras();
    RegisteredManagers.Empty();
    
    Super::Deinitialize();
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("RTSP Streaming Subsystem deinitialized"));
}

URTSPStreamingSubsystem* URTSPStreamingSubsystem::Get(const UObject* WorldContext)
{
    if (!WorldContext)
    {
        return nullptr;
    }

    const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<URTSPStreamingSubsystem>();
}

bool URTSPStreamingSubsystem::RegisterCameraManager(URTSPCameraManager* CameraManager)
{
    if (!CameraManager)
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Cannot register null camera manager"));
        return false;
    }

    if (RegisteredManagers.Contains(CameraManager))
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Camera manager already registered"));
        return true;
    }

    RegisteredManagers.Add(CameraManager);
    CameraManager->OnCameraStreamingStatusChanged.AddDynamic(this, &URTSPStreamingSubsystem::OnCameraManagerStreamingStatusChanged);

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Registered camera manager with %d cameras"), CameraManager->GetCameraNames().Num());
    return true;
}

void URTSPStreamingSubsystem::UnregisterCameraManager(URTSPCameraManager* CameraManager)
{
    if (!CameraManager)
    {
        return;
    }

    if (RegisteredManagers.RemoveSingle(CameraManager) > 0)
    {
        CameraManager->OnCameraStreamingStatusChanged.RemoveDynamic(this, &URTSPStreamingSubsystem::OnCameraManagerStreamingStatusChanged);
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Unregistered camera manager"));
    }
}

TArray<FString> URTSPStreamingSubsystem::GetAllCameraNames() const
{
    TArray<FString> AllNames;
    
    for (URTSPCameraManager* Manager : RegisteredManagers)
    {
        if (Manager)
        {
            AllNames.Append(Manager->GetCameraNames());
        }
    }
    
    return AllNames;
}

TArray<FString> URTSPStreamingSubsystem::GetAllStreamingCameras() const
{
    TArray<FString> StreamingCameras;
    
    for (URTSPCameraManager* Manager : RegisteredManagers)
    {
        if (Manager)
        {
            StreamingCameras.Append(Manager->GetStreamingCameras());
        }
    }
    
    return StreamingCameras;
}

bool URTSPStreamingSubsystem::StartCameraStreaming(const FString& CameraName)
{
    for (URTSPCameraManager* Manager : RegisteredManagers)
    {
        if (Manager && Manager->GetCameraNames().Contains(CameraName))
        {
            return Manager->StartCameraStreaming(CameraName);
        }
    }
    
    UE_LOG(LogRealGazeboStreaming, Error, TEXT("Camera %s not found in any registered manager"), *CameraName);
    return false;
}

void URTSPStreamingSubsystem::StopCameraStreaming(const FString& CameraName)
{
    for (URTSPCameraManager* Manager : RegisteredManagers)
    {
        if (Manager && Manager->GetCameraNames().Contains(CameraName))
        {
            Manager->StopCameraStreaming(CameraName);
            return;
        }
    }
    
    UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Camera %s not found in any registered manager"), *CameraName);
}

void URTSPStreamingSubsystem::StartAllCameras()
{
    int32 TotalStarted = 0;
    int32 TotalCameras = 0;
    
    for (URTSPCameraManager* Manager : RegisteredManagers)
    {
        if (Manager)
        {
            TArray<FString> CameraNames = Manager->GetCameraNames();
            TotalCameras += CameraNames.Num();
            
            for (const FString& CameraName : CameraNames)
            {
                if (Manager->StartCameraStreaming(CameraName))
                {
                    TotalStarted++;
                }
            }
        }
    }
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Started streaming for %d/%d cameras"), TotalStarted, TotalCameras);
}

void URTSPStreamingSubsystem::StopAllCameras()
{
    for (URTSPCameraManager* Manager : RegisteredManagers)
    {
        if (Manager)
        {
            Manager->StopAllCameras();
        }
    }
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Stopped streaming for all cameras"));
}

bool URTSPStreamingSubsystem::IsCameraStreaming(const FString& CameraName) const
{
    for (URTSPCameraManager* Manager : RegisteredManagers)
    {
        if (Manager && Manager->GetCameraNames().Contains(CameraName))
        {
            return Manager->IsCameraStreaming(CameraName);
        }
    }
    
    return false;
}

FString URTSPStreamingSubsystem::GetCameraStreamURL(const FString& CameraName) const
{
    for (URTSPCameraManager* Manager : RegisteredManagers)
    {
        if (Manager && Manager->GetCameraNames().Contains(CameraName))
        {
            return Manager->GetCameraStreamURL(CameraName);
        }
    }
    
    return FString();
}

TMap<FString, FString> URTSPStreamingSubsystem::GetAllStreamURLs() const
{
    TMap<FString, FString> AllURLs;
    
    for (URTSPCameraManager* Manager : RegisteredManagers)
    {
        if (Manager)
        {
            TMap<FString, FString> ManagerURLs = Manager->GetAllStreamURLs();
            AllURLs.Append(ManagerURLs);
        }
    }
    
    return AllURLs;
}

void URTSPStreamingSubsystem::PrintStreamURLsToLog() const
{
    TMap<FString, FString> AllURLs = GetAllStreamURLs();
    
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("=== RTSP Stream URLs ==="));
    for (const auto& URLPair : AllURLs)
    {
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("%s: %s"), *URLPair.Key, *URLPair.Value);
    }
    UE_LOG(LogRealGazeboStreaming, Log, TEXT("======================"));
}

void URTSPStreamingSubsystem::OnCameraManagerStreamingStatusChanged(FString CameraName, bool bIsStreaming)
{
    OnGlobalStreamingStatusChanged.Broadcast(CameraName, bIsStreaming);
}