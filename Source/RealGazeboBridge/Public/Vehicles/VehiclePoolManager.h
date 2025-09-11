
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GazeboBridgeTypes.h"
#include "VehiclePoolManager.generated.h"

// Forward declarations
class AVehicleBasePawn;
class UWorld;

/**
 * Object pooling manager for vehicle pawns to improve spawn/despawn performance
 * 
 * Key Features:
 * - Pre-allocated pawn pools by vehicle type
 * - Automatic pool sizing based on demand
 * - Memory-efficient pawn reuse
 * - Performance monitoring and statistics
 */
UCLASS()
class REALGAZEBOBRIDGE_API UVehiclePoolManager : public UObject
{
    GENERATED_BODY()

public:
    UVehiclePoolManager();

    //----------------------------------------------------------
    // Pool Management
    //----------------------------------------------------------

    /** Initialize the pool system */
    void InitializePool(UWorld* InWorld);

    /** Shutdown and cleanup all pools */
    void ShutdownPool();

    /** Pre-allocate pawns for a specific vehicle type */
    UFUNCTION(BlueprintCallable, Category = "Pool|Management")
    void PreAllocateVehicles(uint8 VehicleType, int32 Count);

    /** Clear all pools and reset */
    UFUNCTION(BlueprintCallable, Category = "Pool|Management") 
    void ClearAllPools();

    //----------------------------------------------------------
    // Actor Acquisition/Release
    //----------------------------------------------------------

    /** Acquire an actor from the pool or spawn new if needed */
    AVehicleBasePawn* AcquireVehicle(uint8 VehicleType, const FVehicleID& VehicleID);

    /** Release actor back to the pool */
    void ReleaseVehicle(AVehicleBasePawn* Vehicle);

    /** Force release all active actors */
    void ReleaseAllActiveVehicles();

    //----------------------------------------------------------
    // Pool Statistics
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Pool|Statistics")
    int32 GetPoolSize(uint8 VehicleType) const;

    UFUNCTION(BlueprintCallable, Category = "Pool|Statistics")
    int32 GetActiveCount(uint8 VehicleType) const;

    UFUNCTION(BlueprintCallable, Category = "Pool|Statistics")
    int32 GetAvailableCount(uint8 VehicleType) const;

    UFUNCTION(BlueprintCallable, Category = "Pool|Statistics")
    int32 GetTotalActiveVehicles() const;

    UFUNCTION(BlueprintCallable, Category = "Pool|Statistics")
    float GetPoolMemoryUsageMB() const;

    //----------------------------------------------------------
    // Configuration
    //----------------------------------------------------------

    /** Maximum actors per pool type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool|Configuration")
    int32 MaxActorsPerType = 100;

    /** Initial pool size per type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool|Configuration")
    int32 InitialPoolSize = 10;

    /** Auto-expand pool when needed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool|Configuration")
    bool bAutoExpandPools = true;

    /** Pool expansion increment */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool|Configuration")
    int32 PoolExpansionSize = 5;

    /** Enable pool shrinking when actors are unused */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool|Configuration")
    bool bAutoShrinkPools = false;

    /** Time before unused actors are removed (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool|Configuration")
    float UnusedActorTimeout = 30.0f;

protected:
    //----------------------------------------------------------
    // Pool Data Structures
    //----------------------------------------------------------

    /** Pool of available actors by vehicle type */
    TMap<uint8, TArray<AVehicleBasePawn*>> AvailablePawnPools;

    /** Currently active (in-use) actors */
    TMap<uint8, TArray<AVehicleBasePawn*>> ActivePawnPools;

    /** Actor to vehicle type mapping for quick lookup */
    TMap<AVehicleBasePawn*, uint8> PawnToTypeMap;

    /** World reference for spawning */
    TWeakObjectPtr<UWorld> CachedWorld;

    //----------------------------------------------------------
    // Performance Tracking
    //----------------------------------------------------------

    /** Statistics for monitoring */
    mutable TMap<uint8, int32> SpawnCount;
    mutable TMap<uint8, int32> ReleaseCount;
    mutable TMap<uint8, float> LastUsageTime;

    /** Memory tracking */
    mutable float TotalMemoryUsage = 0.0f;

    //----------------------------------------------------------
    // Internal Methods
    //----------------------------------------------------------

    /** Create a new actor instance for the pool */
    AVehicleBasePawn* CreateVehiclePawn(uint8 VehicleType);

    /** Get the vehicle class for the given type */
    TSubclassOf<AVehicleBasePawn> GetVehicleClass(uint8 VehicleType) const;

    /** Initialize a newly created or reused actor */
    void InitializePawn(AVehicleBasePawn* Pawn, const FVehicleID& VehicleID);

    /** Reset actor to pool state */
    void ResetPawnForPool(AVehicleBasePawn* Pawn);

    /** Expand pool for the given type */
    void ExpandPool(uint8 VehicleType, int32 ExpansionCount);

    /** Shrink unused pools */
    void ShrinkUnusedPools();

    /** Update pool statistics */
    void UpdatePoolStatistics() const;

    //----------------------------------------------------------
    // Timer Callbacks
    //----------------------------------------------------------

    /** Cleanup timer callback */
    void OnPoolCleanupTimer();

    /** Timer handle for periodic cleanup */
    FTimerHandle CleanupTimerHandle;

    /** Cleanup interval (seconds) */
    static constexpr float CleanupInterval = 10.0f;

public:
    //----------------------------------------------------------
    // Debug and Utilities
    //----------------------------------------------------------

    /** Print detailed pool statistics to log */
    UFUNCTION(BlueprintCallable, Category = "Pool|Debug")
    void PrintPoolStatistics() const;

    /** Validate pool integrity (debug builds only) */
    UFUNCTION(BlueprintCallable, Category = "Pool|Debug") 
    bool ValidatePoolIntegrity() const;
};