#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/GazeboBridgeTypes.h"
#include "UIVehicleDataInterface.generated.h"

// Forward declarations
class UGazeboBridgeSubsystem;
class AVehicleBasePawn;

/**
 * UI-friendly vehicle data structure
 * Simplified version of FVehicleRuntimeData for UI display
 */
USTRUCT(BlueprintType)
struct REALGAZEBOUI_API FUIVehicleData
{
    GENERATED_BODY()

    /** Unique vehicle ID */
    UPROPERTY(BlueprintReadOnly, Category = "Vehicle Data")
    FVehicleID VehicleID;

    /** Display name (e.g., "iris_1", "rover_3") */
    UPROPERTY(BlueprintReadOnly, Category = "Vehicle Data")
    FString DisplayName;

    /** Vehicle type name from DataTable */
    UPROPERTY(BlueprintReadOnly, Category = "Vehicle Data") 
    FString VehicleTypeName;

    /** Current position */
    UPROPERTY(BlueprintReadOnly, Category = "Vehicle Data")
    FVector Position;

    /** Current rotation */
    UPROPERTY(BlueprintReadOnly, Category = "Vehicle Data")
    FRotator Rotation;

    /** Battery level (0.0-1.0) */
    UPROPERTY(BlueprintReadOnly, Category = "Vehicle Data")
    float BatteryLevel;

    /** Vehicle status string */
    UPROPERTY(BlueprintReadOnly, Category = "Vehicle Data")
    FString Status;

    /** Is vehicle currently active/spawned */
    UPROPERTY(BlueprintReadOnly, Category = "Vehicle Data")
    bool bIsActive;

    /** Is vehicle selected in UI */
    UPROPERTY(BlueprintReadWrite, Category = "Vehicle Data")
    bool bIsSelected;

    /** Reference to actual vehicle pawn (if spawned) */
    UPROPERTY(BlueprintReadOnly, Category = "Vehicle Data")
    TWeakObjectPtr<AVehicleBasePawn> VehiclePawn;

    /** Last update time */
    UPROPERTY(BlueprintReadOnly, Category = "Vehicle Data")
    float LastUpdateTime;

    FUIVehicleData()
    {
        VehicleID = FVehicleID();
        DisplayName = TEXT("");
        VehicleTypeName = TEXT("");
        Position = FVector::ZeroVector;
        Rotation = FRotator::ZeroRotator;
        BatteryLevel = 1.0f;
        Status = TEXT("Unknown");
        bIsActive = false;
        bIsSelected = false;
        VehiclePawn = nullptr;
        LastUpdateTime = 0.0f;
    }
};

/**
 * Interface class for accessing vehicle data from RealGazeboBridge
 * Provides UI-friendly access to vehicle information
 */
UCLASS(BlueprintType)
class REALGAZEBOUI_API UUIVehicleDataInterface : public UObject
{
    GENERATED_BODY()

public:
    UUIVehicleDataInterface();

    /** Initialize the interface with bridge subsystem reference */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data Interface")
    void Initialize(UGazeboBridgeSubsystem* InBridgeSubsystem);

    /** Get all active vehicles as UI-friendly data */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data Interface")
    TArray<FUIVehicleData> GetAllVehicleData();

    /** Get specific vehicle data by ID */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data Interface")
    bool GetVehicleData(const FVehicleID& VehicleID, FUIVehicleData& OutVehicleData);

    /** Get vehicle data by display name */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data Interface")
    bool GetVehicleDataByName(const FString& DisplayName, FUIVehicleData& OutVehicleData);

    /** Get vehicle pawn by ID */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data Interface")
    AVehicleBasePawn* GetVehiclePawn(const FVehicleID& VehicleID);

    /** Get total number of active vehicles */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data Interface")
    int32 GetActiveVehicleCount() const;

    /** Check if a vehicle with given ID exists and is active */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data Interface")
    bool IsVehicleActive(const FVehicleID& VehicleID) const;

    /** Update UI data from bridge (call this regularly) */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data Interface")
    void RefreshVehicleData();

    /** Get bridge subsystem (for direct access if needed) */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Data Interface")
    UGazeboBridgeSubsystem* GetBridgeSubsystem() const { return BridgeSubsystem.Get(); }

protected:
    /** Convert runtime data to UI-friendly format */
    FUIVehicleData ConvertToUIData(const FVehicleID& VehicleID, const FVehicleRuntimeData& RuntimeData);

    /** Generate display name from vehicle ID and type */
    FString GenerateDisplayName(const FVehicleID& VehicleID, uint8 VehicleType);

    /** Get vehicle type name from DataTable */
    FString GetVehicleTypeName(uint8 VehicleType);

    /** Calculate battery level (placeholder - implement based on your data) */
    float CalculateBatteryLevel(const FVehicleRuntimeData& RuntimeData);

    /** Generate status string from vehicle data */
    FString GenerateStatusString(const FVehicleRuntimeData& RuntimeData);

private:
    /** Reference to the bridge subsystem */
    UPROPERTY()
    TWeakObjectPtr<UGazeboBridgeSubsystem> BridgeSubsystem;

    /** Cached UI vehicle data for performance */
    UPROPERTY()
    TArray<FUIVehicleData> CachedVehicleData;

    /** Last refresh time for caching */
    float LastRefreshTime;

    /** Refresh interval in seconds */
    static constexpr float RefreshInterval = 0.1f; // 10 Hz updates
};