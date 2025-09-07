#include "Capture/VehicleCameraManager.h"
#include "Capture/VehicleCameraStreamComponent.h"
#include "Vehicles/VehicleBasePawn.h"
#include "Components/SceneComponent.h"
#include "RealGazeboStreaming.h"

UVehicleCameraManager::UVehicleCameraManager()
{
    // Initialize object
}

//----------------------------------------------------------
// Camera Management
//----------------------------------------------------------

TArray<UVehicleCameraStreamComponent*> UVehicleCameraManager::CreateVehicleCameras(
    const FVehicleID& VehicleID, 
    AVehicleBasePawn* VehiclePawn, 
    const TArray<FCameraStreamConfig>& CameraConfigs)
{
    TArray<UVehicleCameraStreamComponent*> CreatedCameras;

    if (!VehiclePawn)
    {
        UE_LOG(LogRealGazeboStreaming, Warning, TEXT("Cannot create cameras: VehiclePawn is null"));
        return CreatedCameras;
    }

    // Remove existing cameras for this vehicle first
    RemoveVehicleCameras(VehicleID);

    // Create new cameras based on configuration
    for (const FCameraStreamConfig& CameraConfig : CameraConfigs)
    {
        UVehicleCameraStreamComponent* Camera = CreateSingleCamera(VehicleID, VehiclePawn, CameraConfig);
        if (Camera)
        {
            CreatedCameras.Add(Camera);
            
            // Store in management maps
            FStreamID StreamID(VehicleID, CameraConfig.CameraName);
            ManagedCameras.Add(StreamID, Camera);
            
            if (!VehicleCameraMap.Contains(VehicleID))
            {
                VehicleCameraMap.Add(VehicleID, TArray<UVehicleCameraStreamComponent*>());
            }
            VehicleCameraMap[VehicleID].Add(Camera);

            UE_LOG(LogRealGazeboStreaming, Log, TEXT("Created camera '%s' for vehicle %s"), 
                *CameraConfig.CameraName, *VehicleID.ToString());
        }
    }

    UE_LOG(LogRealGazeboStreaming, Log, TEXT("Created %d cameras for vehicle %s"), 
        CreatedCameras.Num(), *VehicleID.ToString());

    return CreatedCameras;
}

void UVehicleCameraManager::RemoveVehicleCameras(const FVehicleID& VehicleID)
{
    if (TArray<UVehicleCameraStreamComponent*>* VehicleCameras = VehicleCameraMap.Find(VehicleID))
    {
        for (UVehicleCameraStreamComponent* Camera : *VehicleCameras)
        {
            if (Camera)
            {
                CleanupCamera(Camera);
                
                // Remove from managed cameras map
                FStreamID StreamID = Camera->GetStreamID();
                ManagedCameras.Remove(StreamID);
            }
        }

        VehicleCameraMap.Remove(VehicleID);
        UE_LOG(LogRealGazeboStreaming, Log, TEXT("Removed all cameras for vehicle %s"), *VehicleID.ToString());
    }
}

TArray<UVehicleCameraStreamComponent*> UVehicleCameraManager::GetVehicleCameras(const FVehicleID& VehicleID) const
{
    TArray<UVehicleCameraStreamComponent*> Result;
    
    if (const TArray<UVehicleCameraStreamComponent*>* VehicleCameras = VehicleCameraMap.Find(VehicleID))
    {
        for (UVehicleCameraStreamComponent* Camera : *VehicleCameras)
        {
            if (Camera)
            {
                Result.Add(Camera);
            }
        }
    }
    
    return Result;
}

UVehicleCameraStreamComponent* UVehicleCameraManager::GetCamera(const FStreamID& StreamID) const
{
    if (UVehicleCameraStreamComponent* const* Camera = ManagedCameras.Find(StreamID))
    {
        return *Camera;
    }
    return nullptr;
}

TArray<UVehicleCameraStreamComponent*> UVehicleCameraManager::GetAllCameras() const
{
    TArray<UVehicleCameraStreamComponent*> Result;
    
    for (const auto& CameraPair : ManagedCameras)
    {
        if (CameraPair.Value)
        {
            Result.Add(CameraPair.Value);
        }
    }
    
    return Result;
}

//----------------------------------------------------------
// Utility Methods
//----------------------------------------------------------

UVehicleCameraStreamComponent* UVehicleCameraManager::CreateSingleCamera(
    const FVehicleID& VehicleID,
    AVehicleBasePawn* VehiclePawn,
    const FCameraStreamConfig& CameraConfig)
{
    // Create camera component
    UVehicleCameraStreamComponent* Camera = NewObject<UVehicleCameraStreamComponent>(VehiclePawn);
    if (!Camera)
    {
        UE_LOG(LogRealGazeboStreaming, Error, TEXT("Failed to create camera component"));
        return nullptr;
    }

    // Configure camera
    Camera->StreamConfig = CameraConfig;
    Camera->InitializeForVehicle(VehicleID, VehiclePawn);

    // Attach to vehicle
    Camera->AttachToVehicle(VehiclePawn);

    // Set up camera positioning based on configuration
    // You can extend this with more sophisticated positioning logic
    Camera->SetRelativeLocation(FVector(100.0f, 0.0f, 50.0f)); // Default camera offset
    
    return Camera;
}

void UVehicleCameraManager::CleanupCamera(UVehicleCameraStreamComponent* Camera)
{
    if (Camera)
    {
        // Stop streaming if active
        if (Camera->IsStreaming())
        {
            Camera->StopStreaming();
        }

        // Destroy component
        Camera->DestroyComponent();
    }
}