#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Texture2D.h"
#include "GazeboBridgeTypes.h"
#include "RealGazeboVehicleListItem.generated.h"

/**
 * Data item for vehicle ListView entries
 * Contains all display data for a single vehicle in the monitoring UI
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API URealGazeboVehicleListItem : public UObject
{
    GENERATED_BODY()

public:
    URealGazeboVehicleListItem();

    //----------------------------------------------------------
    // Core Vehicle Data
    //----------------------------------------------------------

    /** Unique vehicle identifier */
    UPROPERTY(BlueprintReadWrite, Category = "Vehicle Data")
    FVehicleID VehicleID;

    /** Vehicle display name (e.g., "Iris_01", "Rover_02") */
    UPROPERTY(BlueprintReadWrite, Category = "Vehicle Data")
    FString VehicleName;

    /** Vehicle type name for display (e.g., "Drone", "Rover", "Boat") */
    UPROPERTY(BlueprintReadWrite, Category = "Vehicle Data")
    FString VehicleTypeName;

    /** Current vehicle position in Unreal Engine coordinates (cm, converted from Gazebo) */
    UPROPERTY(BlueprintReadWrite, Category = "Vehicle Data")
    FVector Position;

    /** Current vehicle rotation (converted from Gazebo to Unreal coordinate system) */
    UPROPERTY(BlueprintReadWrite, Category = "Vehicle Data")
    FRotator Rotation;

    //----------------------------------------------------------
    // Future Telemetry Data (Placeholders)
    //----------------------------------------------------------

    /** Battery percentage (0-100) - placeholder for future implementation */
    UPROPERTY(BlueprintReadWrite, Category = "Vehicle Data")
    float BatteryPercentage;

    /** Vehicle operational status */
    UPROPERTY(BlueprintReadWrite, Category = "Vehicle Data")
    FString Status;

    //----------------------------------------------------------
    // Update Methods
    //----------------------------------------------------------

    /** Update vehicle data from runtime data */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data")
    void UpdateFromRuntimeData(const FVehicleRuntimeData& RuntimeData);

    /** Update position and rotation */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data")
    void UpdateTransform(const FVector& NewPosition, const FRotator& NewRotation);

    /** Get formatted position string for display */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data")
    FString GetFormattedPosition() const;

    /** Get formatted status string with battery info */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data")
    FString GetFormattedStatus() const;

protected:
    //----------------------------------------------------------
    // Internal Data
    //----------------------------------------------------------

    /** Last update timestamp */
    double LastUpdateTime;

    /** Initialize default values */
    void InitializeDefaults();

    /** Determine status color from vehicle state */
    FLinearColor DetermineStatusColor() const;
};