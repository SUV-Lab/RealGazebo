#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StreamingTypes.h"
#include "VehicleCameraManager.generated.h"

// Forward declarations
class UVehicleCameraStreamComponent;
class AVehicleBasePawn;

/**
 * Manager for vehicle camera components
 * 
 * Handles creation, management, and lifecycle of camera components
 * attached to RealGazebo vehicles for streaming purposes.
 */
UCLASS(BlueprintType)
class REALGAZEBOSTREAMING_API UVehicleCameraManager : public UObject
{
    GENERATED_BODY()

public:
    UVehicleCameraManager();

    //----------------------------------------------------------
    // Camera Management
    //----------------------------------------------------------

    /** Create camera components for a vehicle based on configuration */
    UFUNCTION(BlueprintCallable, Category = "Camera Manager")
    TArray<UVehicleCameraStreamComponent*> CreateVehicleCameras(
        const FVehicleID& VehicleID, 
        AVehicleBasePawn* VehiclePawn, 
        const TArray<FCameraStreamConfig>& CameraConfigs
    );

    /** Remove all cameras for a vehicle */
    UFUNCTION(BlueprintCallable, Category = "Camera Manager")
    void RemoveVehicleCameras(const FVehicleID& VehicleID);

    /** Get all cameras for a specific vehicle */
    UFUNCTION(BlueprintCallable, Category = "Camera Manager")
    TArray<UVehicleCameraStreamComponent*> GetVehicleCameras(const FVehicleID& VehicleID) const;

    /** Get camera by stream ID */
    UFUNCTION(BlueprintCallable, Category = "Camera Manager")
    UVehicleCameraStreamComponent* GetCamera(const FStreamID& StreamID) const;

    /** Get all managed cameras */
    UFUNCTION(BlueprintCallable, Category = "Camera Manager")
    TArray<UVehicleCameraStreamComponent*> GetAllCameras() const;

protected:
    //----------------------------------------------------------
    // Internal Management
    //----------------------------------------------------------

    /** Storage for all managed cameras */
    TMap<FStreamID, UVehicleCameraStreamComponent*> ManagedCameras;

    /** Vehicle to cameras mapping */
    TMap<FVehicleID, TArray<UVehicleCameraStreamComponent*>> VehicleCameraMap;

    //----------------------------------------------------------
    // Utility Methods
    //----------------------------------------------------------

    /** Create and configure a single camera component */
    UVehicleCameraStreamComponent* CreateSingleCamera(
        const FVehicleID& VehicleID,
        AVehicleBasePawn* VehiclePawn,
        const FCameraStreamConfig& CameraConfig
    );

    /** Cleanup camera component */
    void CleanupCamera(UVehicleCameraStreamComponent* Camera);
};