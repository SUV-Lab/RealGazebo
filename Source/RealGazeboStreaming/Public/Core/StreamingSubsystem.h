#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "StreamingTypes.h"
#include "GazeboBridgeTypes.h"
#include "StreamingSubsystem.generated.h"

// Forward declarations
class UVehicleCameraManager;
class UAdvancedCaptureManager;
class UGazeboBridgeSubsystem;

/**
 * Advanced multi-camera streaming subsystem for RealGazebo
 * 
 * Key Features:
 * - Automatic camera creation based on vehicle spawning
 * - High-performance capture using PixelCapture integration  
 * - Multi-codec RTSP streaming with GStreamer
 * - DataTable-driven camera configuration
 * - Integration with RealGazebo vehicle system
 * - Performance monitoring and optimization
 */
UCLASS()
class REALGAZEBOSTREAMING_API UStreamingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UStreamingSubsystem();

    //----------------------------------------------------------
    // Subsystem Interface
    //----------------------------------------------------------
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    //----------------------------------------------------------
    // Configuration
    //----------------------------------------------------------

    /** DataTable containing camera configurations per vehicle type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming|Configuration")
    TSoftObjectPtr<UDataTable> VehicleCameraConfigTable;

    /** Global streaming settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming|Configuration")
    bool bAutoCreateCamerasOnVehicleSpawn = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming|Configuration")
    bool bAutoStartStreaming = false;

    /** Base RTSP port (each vehicle gets incremental ports) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming|Network")
    int32 BaseRTSPPort = 8554;

    /** Maximum concurrent streams */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming|Performance")
    int32 MaxConcurrentStreams = 50;

    //----------------------------------------------------------
    // Runtime Control
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Streaming|Control")
    void StartStreamingSystem();

    UFUNCTION(BlueprintCallable, Category = "Streaming|Control")
    void StopStreamingSystem();

    UFUNCTION(BlueprintCallable, Category = "Streaming|Control")
    bool IsStreamingSystemActive() const;

    //----------------------------------------------------------
    // Vehicle Integration (Auto Camera Management)
    //----------------------------------------------------------

    /** Called when a vehicle is spawned in RealGazebo */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Vehicles")
    void OnVehicleSpawned(const FVehicleID& VehicleID, class AVehicleBasePawn* VehiclePawn);

    /** Called when a vehicle is despawned */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Vehicles")
    void OnVehicleDespawned(const FVehicleID& VehicleID);

    /** Get all cameras for a specific vehicle */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Vehicles")
    TArray<FString> GetVehicleCameraNames(const FVehicleID& VehicleID) const;

    //----------------------------------------------------------
    // Manual Camera Management
    //----------------------------------------------------------

    /** Create camera manually for a vehicle */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Cameras")
    bool CreateVehicleCamera(const FVehicleID& VehicleID, const FCameraStreamConfig& CameraConfig);

    /** Remove camera from a vehicle */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Cameras")
    void RemoveVehicleCamera(const FVehicleID& VehicleID, const FString& CameraName);

    /** Update camera configuration */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Cameras")
    bool UpdateCameraConfig(const FStreamID& StreamID, const FCameraStreamConfig& NewConfig);

    //----------------------------------------------------------
    // Streaming Control
    //----------------------------------------------------------

    /** Start streaming for a specific camera */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Control")
    bool StartCameraStreaming(const FStreamID& StreamID);

    /** Stop streaming for a specific camera */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Control")
    void StopCameraStreaming(const FStreamID& StreamID);

    /** Start all cameras for a vehicle */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Control")
    void StartVehicleStreaming(const FVehicleID& VehicleID);

    /** Stop all cameras for a vehicle */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Control")
    void StopVehicleStreaming(const FVehicleID& VehicleID);

    /** Start all active cameras */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Control")
    void StartAllStreaming();

    /** Stop all active cameras */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Control")
    void StopAllStreaming();

    //----------------------------------------------------------
    // Status and Information
    //----------------------------------------------------------

    /** Check if a specific camera is streaming */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Streaming|Status")
    bool IsCameraStreaming(const FStreamID& StreamID) const;

    /** Get stream URL for a camera */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Streaming|Status")
    FString GetStreamURL(const FStreamID& StreamID) const;

    /** Get all active stream URLs */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Status")
    TMap<FString, FString> GetAllStreamURLs() const;

    /** Get streaming statistics */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Status")
    FStreamingPerformanceStats GetPerformanceStats() const;

    /** Get runtime data for a specific stream */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Status")
    FStreamingRuntimeData GetStreamRuntimeData(const FStreamID& StreamID) const;

    /** Get all active stream IDs */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Status")
    TArray<FStreamID> GetAllActiveStreamIDs() const;

    //----------------------------------------------------------
    // Events
    //----------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Streaming|Events")
    FOnStreamingStatusChanged OnStreamingStatusChanged;

    UPROPERTY(BlueprintAssignable, Category = "Streaming|Events")
    FOnMultiCameraStreamingUpdate OnMultiCameraStreamingUpdate;

    UPROPERTY(BlueprintAssignable, Category = "Streaming|Events")
    FOnStreamingError OnStreamingError;

    //----------------------------------------------------------
    // Performance and Debugging
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Streaming|Debug")
    void PrintAllStreamURLs() const;

    UFUNCTION(BlueprintCallable, Category = "Streaming|Debug")
    void PrintPerformanceStats() const;

    /** Enable/disable performance monitoring */
    UFUNCTION(BlueprintCallable, Category = "Streaming|Performance")
    void SetPerformanceMonitoring(bool bEnabled);

    //----------------------------------------------------------
    // Static Access
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Streaming|Access", meta = (CallInEditor = "true"))
    static UStreamingSubsystem* GetStreamingSubsystem(const UObject* WorldContext);

public:
    //----------------------------------------------------------
    // Internal Management
    //----------------------------------------------------------

    /** Main storage for all stream runtime data */
    TMap<FStreamID, FStreamingRuntimeData> StreamDataMap;

    /** Get vehicle camera configuration from DataTable */
    const FVehicleCameraConfigRow* GetVehicleCameraConfig(uint8 VehicleType) const;

protected:
    //----------------------------------------------------------
    // Core Components
    //----------------------------------------------------------

    UPROPERTY()
    TObjectPtr<UVehicleCameraManager> CameraManager;

    UPROPERTY()
    TObjectPtr<UAdvancedCaptureManager> CaptureManager;

    //----------------------------------------------------------
    // Integration
    //----------------------------------------------------------

    /** Reference to RealGazebo bridge subsystem */
    UPROPERTY()
    TObjectPtr<UGazeboBridgeSubsystem> BridgeSubsystem;

    //----------------------------------------------------------
    // State Management
    //----------------------------------------------------------

    bool bIsStreamingSystemActive = false;
    bool bPerformanceMonitoringEnabled = true;

    /** Performance tracking */
    mutable FStreamingPerformanceStats CachedPerformanceStats;
    mutable float LastPerformanceUpdate = 0.0f;

    /** Port allocation */
    int32 NextAvailablePort = 8554;

    //----------------------------------------------------------
    // Internal Methods
    //----------------------------------------------------------

    /** Initialize integration with RealGazebo */
    void InitializeRealGazeboIntegration();

    /** Shutdown integration */
    void ShutdownRealGazeboIntegration();

    /** Auto-create cameras based on vehicle type configuration */
    void AutoCreateVehicleCameras(const FVehicleID& VehicleID, AVehicleBasePawn* VehiclePawn);

    /** Cleanup all cameras for a vehicle */
    void CleanupVehicleCameras(const FVehicleID& VehicleID);

    /** Allocate unique port for stream */
    int32 AllocateStreamPort();

    /** Update performance statistics */
    void UpdatePerformanceStats() const;

    /** Validate stream configuration */
    bool ValidateStreamConfig(const FCameraStreamConfig& Config) const;

    //----------------------------------------------------------
    // Event Handlers
    //----------------------------------------------------------

    UFUNCTION()
    void OnRealGazeboVehicleSpawned(const FBridgePoseData& VehicleData);

    UFUNCTION()
    void OnStreamingStatusChangedInternal(const FString& CameraID, bool bIsStreaming);

private:
    /** Cached world reference */
    TWeakObjectPtr<UWorld> CachedWorld;

    /** Performance monitoring timer */
    FTimerHandle PerformanceTimerHandle;

    /** Thread-safe access to stream data */
    mutable FCriticalSection StreamDataMutex;
};