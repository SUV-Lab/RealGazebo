
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "GazeboBridgeTypes.h"
#include "GazeboBridgeSubsystem.generated.h"

// Forward declarations
class UDataStreamProcessor;
class UVehiclePoolManager;
class UWorld;
class APlayerCameraManager;

/**
 * Subsystem for managing PX4-Gazebo bridge
 * 
 * Key Features:
 * - Centralized vehicle data management
 * - Object pooling for memory management
 * - Simple rendering (all vehicles visible)
 * - Batch processing for network data
 */
UCLASS()
class REALGAZEBOBRIDGE_API UGazeboBridgeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UGazeboBridgeSubsystem();

    //----------------------------------------------------------
    // Subsystem Interface
    //----------------------------------------------------------
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    //----------------------------------------------------------
    // Configuration
    //----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Configuration")
    UDataTable* VehicleConfigTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Network")
    int32 ListenPort = 5005;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Network") 
    FString ServerIPAddress = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Performance")
    bool bAutoSpawnVehicles = true;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Performance")
    float UpdateFrequency = 60.0f; // Hz

    //----------------------------------------------------------
    // Runtime Control
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Bridge|Control")
    void StartBridge();

    UFUNCTION(BlueprintCallable, Category = "Bridge|Control")
    void StopBridge();

    UFUNCTION(BlueprintCallable, Category = "Bridge|Control")
    bool IsBridgeActive() const;

    UFUNCTION(BlueprintCallable, Category = "Bridge|Control")
    void ClearAllVehicles();

    //----------------------------------------------------------
    // Vehicle Management
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Bridge|Vehicles")
    int32 GetTotalVehicleCount() const;

    UFUNCTION(BlueprintCallable, Category = "Bridge|Vehicles")
    int32 GetActiveVehicleCount() const;

    UFUNCTION(BlueprintCallable, Category = "Bridge|Vehicles")
    int32 GetVisibleVehicleCount() const;

    UFUNCTION(BlueprintCallable, Category = "Bridge|Vehicles")
    FVehicleRuntimeData GetVehicleData(const FVehicleID& VehicleID) const;

    UFUNCTION(BlueprintCallable, Category = "Bridge|Vehicles")
    TArray<FVehicleID> GetAllVehicleIDs() const;

    UFUNCTION(BlueprintCallable, Category = "Bridge|Configuration")
    bool GetVehicleConfig(uint8 VehicleType, FBridgeVehicleConfigRow& OutConfig) const;

    /** Get vehicle configuration from DataTable (for internal use) */
    const FBridgeVehicleConfigRow* GetVehicleConfigInternal(uint8 VehicleType) const;

    /** Get vehicle pool manager for configuration */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Access")
    UVehiclePoolManager* GetVehiclePoolManager() const { return VehiclePool; }

    /** Get data stream processor for configuration */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Access")
    UDataStreamProcessor* GetDataStreamProcessor() const { return StreamProcessor; }

    //----------------------------------------------------------
    // Events (for Blueprint integration)
    //----------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Bridge|Events")
    FOnVehicleDataReceived OnVehicleSpawned;

    UPROPERTY(BlueprintAssignable, Category = "Bridge|Events")
    FOnVehicleDataReceived OnVehicleUpdated;

    //----------------------------------------------------------
    // Performance Monitoring
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Bridge|Performance")
    void GetPerformanceStats(int32& OutTotalVehicles, int32& OutVisibleVehicles, 
                           float& OutAverageUpdateTime, float& OutMemoryUsageMB) const;

    //----------------------------------------------------------
    // Static Access
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Bridge|Access", meta = (CallInEditor = "true"))
    static UGazeboBridgeSubsystem* GetBridgeSubsystem(const UObject* WorldContext);

public:
    //----------------------------------------------------------
    // Internal Vehicle Data Management 
    //----------------------------------------------------------

    /** Main vehicle data storage - optimized for fast lookup */
    TMap<FVehicleID, FVehicleRuntimeData> VehicleDataMap;

    /** Update vehicle data from network (called by DataStreamProcessor) */
    void UpdateVehicleData(const FBridgePoseData& PoseData);
    void UpdateVehicleMotorData(const FBridgeMotorSpeedData& MotorData);
    void UpdateVehicleServoData(const FBridgeServoData& ServoData);

    /** Batch update system */
    void BatchUpdateVehicles();

protected:
    //----------------------------------------------------------
    // Core Components
    //----------------------------------------------------------

    UPROPERTY()
    TObjectPtr<UDataStreamProcessor> StreamProcessor;

    UPROPERTY()
    TObjectPtr<UVehiclePoolManager> VehiclePool;

    //----------------------------------------------------------
    // Performance Optimization
    //----------------------------------------------------------


    /** Performance counters */
    mutable float LastUpdateTime = 0.0f;
    mutable int32 FrameCounter = 0;
    mutable float AverageFrameTime = 0.0f;

    /** Memory usage tracking */
    mutable float MemoryUsageMB = 0.0f;

    //----------------------------------------------------------
    // Internal Methods
    //----------------------------------------------------------

    /** Initialize network processing */
    void InitializeNetworkProcessing();

    /** Cleanup network processing */
    void ShutdownNetworkProcessing();



    /** Spawn visual pawn for vehicle */
    void SpawnVehiclePawn(const FVehicleID& VehicleID, FVehicleRuntimeData& VehicleData);

    /** Release visual pawn for vehicle */
    void ReleaseVehiclePawn(FVehicleRuntimeData& VehicleData);

    //----------------------------------------------------------
    // Event Handlers
    //----------------------------------------------------------

    UFUNCTION()
    void OnPoseDataReceived(const FBridgePoseData& PoseData);

    UFUNCTION()
    void OnMotorSpeedDataReceived(const FBridgeMotorSpeedData& MotorData);

    UFUNCTION()
    void OnServoDataReceived(const FBridgeServoData& ServoData);

private:
    /** Bridge active state */
    bool bIsBridgeActive = false;

    /** Cached world reference */
    TWeakObjectPtr<UWorld> CachedWorld;

    /** Update timer handle */
    FTimerHandle UpdateTimerHandle;

    /** Performance monitoring */
    mutable double LastPerformanceCheck = 0.0;
};