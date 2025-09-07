#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "GazeboBridgeTypes.h"
#include "RealGazeboManager.generated.h"

// Forward declarations
class UGazeboBridgeSubsystem;

/**
 * User-friendly RealGazebo Manager Actor for plug-and-play usage
 * 
 * This provides v1.0-style drag-and-drop usability while leveraging
 * the high-performance v2.0 subsystem architecture underneath.
 * 
 * Key Features:
 * - Drag-and-drop into level (like v1.0)
 * - Visual configuration in Details panel
 * - Per-level settings
 * - Plug-and-play - no project settings needed
 * - Automatic subsystem configuration
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "RealGazebo Manager"))
class REALGAZEBOBRIDGE_API ARealGazeboManager : public AActor
{
    GENERATED_BODY()

public:
    ARealGazeboManager();

    //----------------------------------------------------------
    // Core Settings - Essential Configuration
    //----------------------------------------------------------

    /** Vehicle Configuration DataTable */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo", 
              meta = (DisplayName = "Vehicle Data Table", DisplayPriority = "1"))
    UDataTable* VehicleDataTable;

    /** Auto-start bridge when level begins */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Runtime", 
              meta = (DisplayName = "Auto Start", DisplayPriority = "1"))
    bool bAutoStart = true;

    /** Automatically spawn vehicles when data is received */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Runtime", 
              meta = (DisplayName = "Auto Spawn Vehicles", DisplayPriority = "2"))
    bool bAutoSpawnVehicles = true;

    //----------------------------------------------------------
    // Network Settings - Connection & Communication
    //----------------------------------------------------------

    /** UDP Listen Port */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Network", 
              meta = (DisplayName = "Listen Port", DisplayPriority = "1"))
    int32 ListenPort = 5005;

    /** Server IP Address (empty = all interfaces) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Network", 
              meta = (DisplayName = "Server IP Address", DisplayPriority = "2"))
    FString ServerIPAddress = TEXT("127.0.0.1");

    //----------------------------------------------------------
    // Performance Settings - Core Performance & Pool Management
    //----------------------------------------------------------

    /** Update frequency in Hz */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Performance|Core", 
              meta = (DisplayName = "Update Frequency", DisplayPriority = "1", ClampMin = "10.0", ClampMax = "120.0"))
    float UpdateFrequency = 60.0f;

    /** Maximum number of active vehicles allowed simultaneously */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Performance|Limits", meta = (DisplayName = "Max Active Vehicles", DisplayPriority = "1", ClampMin = "50", ClampMax = "2000"))
    int32 MaxActiveVehicles = 256;

    /** Vehicle data update rate in Hz */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Performance|Limits", meta = (DisplayName = "Vehicle Update Rate", DisplayPriority = "2", ClampMin = "10.0", ClampMax = "120.0"))
    float VehicleUpdateRate = 60.0f;

    /** Use asynchronous processing for network data */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Performance|Limits", meta = (DisplayName = "Use Async Processing", DisplayPriority = "3", AdvancedDisplay))
    bool bUseAsyncProcessing = true;

    //----------------------------------------------------------
    // Vehicle Pool Management
    //----------------------------------------------------------

    /** Maximum actors per vehicle type in the pool */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Performance|Pool Management", meta = (DisplayName = "Max Actors Per Type", DisplayPriority = "1", ClampMin = "10", ClampMax = "1000"))
    int32 MaxActorsPerType = 100;

    /** Initial pool size per vehicle type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Performance|Pool Management", meta = (DisplayName = "Initial Pool Size", DisplayPriority = "2", ClampMin = "5", ClampMax = "100"))
    int32 InitialPoolSize = 10;

    /** Auto-expand pools when more vehicles are needed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Performance|Pool Management", meta = (DisplayName = "Auto Expand Pools", DisplayPriority = "3"))
    bool bAutoExpandPools = true;

    /** Pool expansion increment size */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Performance|Pool Management", meta = (DisplayName = "Pool Expansion Size", DisplayPriority = "4", ClampMin = "1", ClampMax = "50", EditCondition = "bAutoExpandPools"))
    int32 PoolExpansionSize = 5;

    /** Auto-shrink unused pools to save memory */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Performance|Pool Management", meta = (DisplayName = "Auto Shrink Pools", DisplayPriority = "5", AdvancedDisplay))
    bool bAutoShrinkPools = false;

    /** Time before unused actors are removed from pools (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Performance|Pool Management", meta = (DisplayName = "Unused Actor Timeout", DisplayPriority = "6", ClampMin = "10.0", ClampMax = "300.0", EditCondition = "bAutoShrinkPools", AdvancedDisplay))
    float UnusedActorTimeout = 30.0f;

    /** Process UDP packets in batches for better performance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Network|Processing", 
              meta = (DisplayName = "Enable Batch Processing", DisplayPriority = "1"))
    bool bEnableBatchProcessing = true;

    /** Number of packets to process per batch */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Network|Processing", 
              meta = (DisplayName = "Batch Size", DisplayPriority = "2", ClampMin = "1", ClampMax = "100", EditCondition = "bEnableBatchProcessing"))
    int32 BatchSize = 10;

    /** Batch processing interval in seconds (affects responsiveness) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Network|Processing", 
              meta = (DisplayName = "Batch Processing Interval", DisplayPriority = "3", ClampMin = "0.001", ClampMax = "0.1", EditCondition = "bEnableBatchProcessing", AdvancedDisplay))
    float BatchProcessingInterval = 0.016f; // ~60 FPS

    /** Validate packet sizes against expected protocol */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Network|Validation", 
              meta = (DisplayName = "Validate Packet Sizes", DisplayPriority = "1", AdvancedDisplay))
    bool bValidatePacketSizes = true;

    //----------------------------------------------------------
    // Vehicle Configuration
    //----------------------------------------------------------

    /** Enable smooth movement interpolation for all vehicles */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Vehicles|Movement", meta = (DisplayName = "Default Smooth Movement", DisplayPriority = "1"))
    bool bDefaultSmoothMovement = true;

    /** Default interpolation speed for vehicle movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Vehicles|Movement", meta = (DisplayName = "Default Interpolation Speed", DisplayPriority = "2", ClampMin = "1.0", ClampMax = "100.0", EditCondition = "bDefaultSmoothMovement"))
    float DefaultInterpolationSpeed = 15.0f;

    //----------------------------------------------------------
    // Debug & Monitoring Settings
    //----------------------------------------------------------

    /** Display pool statistics in Details panel */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Debug|Display", meta = (DisplayName = "Show Pool Statistics", DisplayPriority = "1"))
    bool bShowPoolStatistics = false;

    /** Statistics update interval for monitoring */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Debug|Display", meta = (DisplayName = "Statistics Update Interval", DisplayPriority = "2", ClampMin = "0.1", ClampMax = "10.0", EditCondition = "bShowPoolStatistics"))
    float StatisticsUpdateInterval = 1.0f;

    /** Log vehicle spawn and despawn events */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Debug|Logging", meta = (DisplayName = "Log Vehicle Spawns", DisplayPriority = "1"))
    bool bLogVehicleSpawns = false;

    /** Log network performance statistics */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Debug|Logging", meta = (DisplayName = "Log Network Stats", DisplayPriority = "2"))
    bool bLogNetworkStats = false;

    //----------------------------------------------------------
    // Runtime Control (Blueprint/C++ API)
    //----------------------------------------------------------

    /** Start the bridge (manual control) */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Control", meta = (DisplayName = "Start Bridge"))
    void StartBridge();

    /** Stop the bridge */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Control", meta = (DisplayName = "Stop Bridge"))
    void StopBridge();

    /** Check if bridge is running */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Control", meta = (DisplayName = "Is Bridge Active"))
    bool IsBridgeActive() const;

    /** Clear all spawned vehicles */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Control", meta = (DisplayName = "Clear All Vehicles"))
    void ClearAllVehicles();

    //----------------------------------------------------------
    // Status Information (for UI)
    //----------------------------------------------------------

    /** Get number of active vehicles */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Status", meta = (DisplayName = "Get Vehicle Count"))
    int32 GetActiveVehicleCount() const;

    /** Get network statistics */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Status", meta = (DisplayName = "Get Network Stats"))
    void GetNetworkStatistics(int32& OutValidPackets, int32& OutInvalidPackets, float& OutPacketsPerSecond) const;

    //----------------------------------------------------------
    // Events (for custom Blueprint logic)
    //----------------------------------------------------------

    /** Called when bridge starts successfully */
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Events")
    FOnVehicleDataReceived OnBridgeStarted;

    /** Called when bridge stops */
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Events")
    FOnVehicleDataReceived OnBridgeStopped;

    /** Called when vehicle is spawned */
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Events")
    FOnVehicleDataReceived OnVehicleSpawned;

protected:
    //----------------------------------------------------------
    // Actor Lifecycle
    //----------------------------------------------------------

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    //----------------------------------------------------------
    // Internal Logic
    //----------------------------------------------------------

    /** Configure the subsystem with our settings */
    void ConfigureSubsystem();

    /** Configure vehicle pool management settings */
    void ConfigureVehiclePoolSettings();

    /** Configure network processing settings */
    void ConfigureNetworkProcessingSettings();

    /** Configure performance and debug settings */
    void ConfigurePerformanceAndDebugSettings();

    /** Validate configuration */
    bool ValidateConfiguration() const;

    /** Reference to the subsystem we're controlling */
    UPROPERTY()
    TWeakObjectPtr<UGazeboBridgeSubsystem> BridgeSubsystem;

    /** Track if we started the subsystem */
    bool bDidStartSubsystem = false;

    /** Status display for debugging */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Status", meta = (DisplayName = "Bridge Status"))
    FString BridgeStatus = TEXT("Not Started");

    /** Active vehicles count for display */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Status", meta = (DisplayName = "Active Vehicles"))
    int32 ActiveVehiclesCount = 0;

    //----------------------------------------------------------
    // Validation and Helpers
    //----------------------------------------------------------

    /** Update status display */
    void UpdateStatusDisplay();

    /** Validate DataTable format */
    bool IsValidVehicleDataTable(UDataTable* DataTable) const;

    /** Validate pool configuration values */
    bool ValidatePoolConfiguration() const;

    /** Validate network configuration values */
    bool ValidateNetworkConfiguration() const;

    /** Validate performance configuration values */
    bool ValidatePerformanceConfiguration() const;

private:
    /** Timer for periodic status updates */
    FTimerHandle StatusUpdateTimer;
};