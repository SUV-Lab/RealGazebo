
#include "VehiclePoolManager.h"
#include "VehicleBasePawn.h"
#include "GazeboBridgeTypes.h"
#include "Core/GazeboBridgeSubsystem.h"
#include "RealGazeboBridge.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"

UVehiclePoolManager::UVehiclePoolManager()
{
    MaxActorsPerType = 100;
    InitialPoolSize = 10;
    bAutoExpandPools = true;
    PoolExpansionSize = 5;
    bAutoShrinkPools = false;
    UnusedActorTimeout = 30.0f;
    
    TotalMemoryUsage = 0.0f;
}

void UVehiclePoolManager::InitializePool(UWorld* InWorld)
{
    CachedWorld = InWorld;
    
    UE_LOG(LogRealGazeboBridge, Display, TEXT("VehiclePoolManager: Initializing object pools"));
    
    // Clear existing pools
    AvailablePawnPools.Empty();
    ActivePawnPools.Empty();
    PawnToTypeMap.Empty();
    SpawnCount.Empty();
    ReleaseCount.Empty();
    LastUsageTime.Empty();
    
    // Start cleanup timer
    if (UWorld* World = CachedWorld.Get())
    {
        World->GetTimerManager().SetTimer(
            CleanupTimerHandle,
            this,
            &UVehiclePoolManager::OnPoolCleanupTimer,
            CleanupInterval,
            true
        );
    }
    
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Vehicle pool initialized with max %d actors per type"), MaxActorsPerType);
}

void UVehiclePoolManager::ShutdownPool()
{
    // Clear cleanup timer
    if (UWorld* World = CachedWorld.Get())
    {
        World->GetTimerManager().ClearTimer(CleanupTimerHandle);
    }
    
    // Destroy all actors in pools
    for (auto& PoolPair : AvailablePawnPools)
    {
        for (AVehicleBasePawn* Actor : PoolPair.Value)
        {
            if (IsValid(Actor))
            {
                Actor->Destroy();
            }
        }
    }
    
    for (auto& PoolPair : ActivePawnPools)
    {
        for (AVehicleBasePawn* Actor : PoolPair.Value)
        {
            if (IsValid(Actor))
            {
                Actor->Destroy();
            }
        }
    }
    
    // Clear all data structures
    AvailablePawnPools.Empty();
    ActivePawnPools.Empty();
    PawnToTypeMap.Empty();
    SpawnCount.Empty();
    ReleaseCount.Empty();
    LastUsageTime.Empty();
    
    CachedWorld.Reset();
    
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Vehicle pool shutdown complete"));
}

void UVehiclePoolManager::PreAllocateVehicles(uint8 VehicleType, int32 Count)
{
    if (Count <= 0 || !CachedWorld.IsValid())
    {
        return;
    }
    
    TArray<AVehicleBasePawn*>& AvailablePool = AvailablePawnPools.FindOrAdd(VehicleType);
    
    const int32 CurrentPoolSize = AvailablePool.Num();
    const int32 NeededActors = FMath::Min(Count - CurrentPoolSize, MaxActorsPerType - CurrentPoolSize);
    
    if (NeededActors <= 0)
    {
        return;
    }
    
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Pre-allocating %d vehicles of type %d"), NeededActors, VehicleType);
    
    for (int32 i = 0; i < NeededActors; i++)
    {
        AVehicleBasePawn* NewPawn = CreateVehiclePawn(VehicleType);
        if (NewPawn)
        {
            AvailablePool.Add(NewPawn);
            PawnToTypeMap.Add(NewPawn, VehicleType);
        }
    }
    
    UpdatePoolStatistics();
}

void UVehiclePoolManager::ClearAllPools()
{
    ShutdownPool();
    InitializePool(CachedWorld.Get());
}

AVehicleBasePawn* UVehiclePoolManager::AcquireVehicle(uint8 VehicleType, const FVehicleID& VehicleID)
{
    if (!CachedWorld.IsValid())
    {
        UE_LOG(LogRealGazeboBridge, Warning, TEXT("Cannot acquire vehicle - no valid world"));
        return nullptr;
    }
    
    // Get or create available pool
    TArray<AVehicleBasePawn*>& AvailablePool = AvailablePawnPools.FindOrAdd(VehicleType);
    TArray<AVehicleBasePawn*>& ActivePool = ActivePawnPools.FindOrAdd(VehicleType);
    
    AVehicleBasePawn* Actor = nullptr;
    
    // Try to get from available pool
    if (AvailablePool.Num() > 0)
    {
        Actor = AvailablePool.Pop();
    }
    else if (bAutoExpandPools)
    {
        // Expand pool if needed and allowed
        const int32 CurrentTotalSize = AvailablePool.Num() + ActivePool.Num();
        if (CurrentTotalSize < MaxActorsPerType)
        {
            Actor = CreateVehiclePawn(VehicleType);
            if (Actor)
            {
                PawnToTypeMap.Add(Actor, VehicleType);
            }
        }
    }
    
    if (!Actor)
    {
        UE_LOG(LogRealGazeboBridge, Warning, TEXT("Failed to acquire vehicle of type %d - pool limit reached"), VehicleType);
        return nullptr;
    }
    
    // Move to active pool
    ActivePool.Add(Actor);
    
    // Initialize actor
    InitializePawn(Actor, VehicleID);
    
    // Update statistics
    SpawnCount.FindOrAdd(VehicleType)++;
    LastUsageTime.FindOrAdd(VehicleType) = FPlatformTime::Seconds();
    
    UE_LOG(LogRealGazeboBridge, VeryVerbose, TEXT("Acquired vehicle type %d - Active: %d, Available: %d"), 
           VehicleType, ActivePool.Num(), AvailablePool.Num());
    
    return Actor;
}

void UVehiclePoolManager::ReleaseVehicle(AVehicleBasePawn* Vehicle)
{
    if (!IsValid(Vehicle))
    {
        return;
    }
    
    // Find vehicle type
    uint8* VehicleTypePtr = PawnToTypeMap.Find(Vehicle);
    if (!VehicleTypePtr)
    {
        UE_LOG(LogRealGazeboBridge, Warning, TEXT("Cannot release vehicle - type not found"));
        return;
    }
    
    const uint8 VehicleType = *VehicleTypePtr;
    
    // Get pools
    TArray<AVehicleBasePawn*>* AvailablePool = AvailablePawnPools.Find(VehicleType);
    TArray<AVehicleBasePawn*>* ActivePool = ActivePawnPools.Find(VehicleType);
    
    if (!ActivePool || !AvailablePool)
    {
        UE_LOG(LogRealGazeboBridge, Warning, TEXT("Cannot release vehicle - pools not found"));
        return;
    }
    
    // Remove from active pool
    const int32 RemovedCount = ActivePool->RemoveAll([Vehicle](AVehicleBasePawn* Actor) {
        return Actor == Vehicle;
    });
    
    if (RemovedCount == 0)
    {
        UE_LOG(LogRealGazeboBridge, Warning, TEXT("Vehicle not found in active pool"));
        return;
    }
    
    // Reset actor to pool state
    ResetPawnForPool(Vehicle);
    
    // Add to available pool
    AvailablePool->Add(Vehicle);
    
    // Update statistics
    ReleaseCount.FindOrAdd(VehicleType)++;
    
    UE_LOG(LogRealGazeboBridge, VeryVerbose, TEXT("Released vehicle type %d - Active: %d, Available: %d"), 
           VehicleType, ActivePool->Num(), AvailablePool->Num());
}

void UVehiclePoolManager::ReleaseAllActiveVehicles()
{
    TArray<AVehicleBasePawn*> ActorsToRelease;
    
    // Collect all active actors
    for (auto& ActivePoolPair : ActivePawnPools)
    {
        for (AVehicleBasePawn* Actor : ActivePoolPair.Value)
        {
            if (IsValid(Actor))
            {
                ActorsToRelease.Add(Actor);
            }
        }
    }
    
    // Release all collected actors
    for (AVehicleBasePawn* Actor : ActorsToRelease)
    {
        ReleaseVehicle(Actor);
    }
    
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Released %d active vehicles"), ActorsToRelease.Num());
}

int32 UVehiclePoolManager::GetPoolSize(uint8 VehicleType) const
{
    const TArray<AVehicleBasePawn*>* AvailablePool = AvailablePawnPools.Find(VehicleType);
    const TArray<AVehicleBasePawn*>* ActivePool = ActivePawnPools.Find(VehicleType);
    
    const int32 AvailableCount = AvailablePool ? AvailablePool->Num() : 0;
    const int32 ActiveCount = ActivePool ? ActivePool->Num() : 0;
    
    return AvailableCount + ActiveCount;
}

int32 UVehiclePoolManager::GetActiveCount(uint8 VehicleType) const
{
    const TArray<AVehicleBasePawn*>* ActivePool = ActivePawnPools.Find(VehicleType);
    return ActivePool ? ActivePool->Num() : 0;
}

int32 UVehiclePoolManager::GetAvailableCount(uint8 VehicleType) const
{
    const TArray<AVehicleBasePawn*>* AvailablePool = AvailablePawnPools.Find(VehicleType);
    return AvailablePool ? AvailablePool->Num() : 0;
}

int32 UVehiclePoolManager::GetTotalActiveVehicles() const
{
    int32 TotalActive = 0;
    for (const auto& ActivePoolPair : ActivePawnPools)
    {
        TotalActive += ActivePoolPair.Value.Num();
    }
    return TotalActive;
}

float UVehiclePoolManager::GetPoolMemoryUsageMB() const
{
    UpdatePoolStatistics();
    return TotalMemoryUsage;
}

AVehicleBasePawn* UVehiclePoolManager::CreateVehiclePawn(uint8 VehicleType)
{
    UWorld* World = CachedWorld.Get();
    if (!World)
    {
        return nullptr;
    }
    
    TSubclassOf<AVehicleBasePawn> PawnClass = GetVehicleClass(VehicleType);
    if (!PawnClass)
    {
        UE_LOG(LogRealGazeboBridge, Warning, TEXT("No pawn class found for vehicle type %d"), VehicleType);
        return nullptr;
    }
    
    // Spawn actor in hidden location
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.bDeferConstruction = false;
    
    const FVector HiddenLocation = FVector(0.0f, 0.0f, -100000.0f); // Far below ground
    
    AVehicleBasePawn* NewPawn = World->SpawnActor<AVehicleBasePawn>(
        PawnClass,
        HiddenLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );
    
    if (NewPawn)
    {
        // Initialize for pooling
        NewPawn->ResetForPool();
        
        UE_LOG(LogRealGazeboBridge, VeryVerbose, TEXT("Created new pooled pawn for vehicle type %d"), VehicleType);
    }
    
    return NewPawn;
}

TSubclassOf<AVehicleBasePawn> UVehiclePoolManager::GetVehicleClass(uint8 VehicleType) const
{
    // Get the bridge subsystem to access vehicle configuration
    if (const UGazeboBridgeSubsystem* BridgeSubsystem = UGazeboBridgeSubsystem::GetBridgeSubsystem(this))
    {
        if (const FBridgeVehicleConfigRow* Config = BridgeSubsystem->GetVehicleConfigInternal(VehicleType))
        {
            if (Config->VehiclePawnClass)
            {
                UE_LOG(LogRealGazeboBridge, Log, TEXT("Found vehicle class for type %d: %s"), 
                       VehicleType, *Config->VehiclePawnClass->GetName());
                return Config->VehiclePawnClass;
            }
            else
            {
                UE_LOG(LogRealGazeboBridge, Warning, TEXT("Vehicle type %d found in DataTable but no pawn class specified"), VehicleType);
            }
        }
        else
        {
            UE_LOG(LogRealGazeboBridge, Warning, TEXT("No configuration found for vehicle type %d in DataTable"), VehicleType);
        }
    }
    else
    {
        UE_LOG(LogRealGazeboBridge, Error, TEXT("Cannot access GazeboBridgeSubsystem for vehicle type lookup"));
    }
    
    // Fallback to base class if no specific configuration found
    UE_LOG(LogRealGazeboBridge, Warning, TEXT("Using fallback base class for vehicle type %d"), VehicleType);
    return AVehicleBasePawn::StaticClass();
}

void UVehiclePoolManager::InitializePawn(AVehicleBasePawn* Pawn, const FVehicleID& VehicleID)
{
    if (!Pawn)
    {
        return;
    }
    
    Pawn->InitializeForPool(VehicleID, VehicleID.VehicleType);
}

void UVehiclePoolManager::ResetPawnForPool(AVehicleBasePawn* Pawn)
{
    if (!Pawn)
    {
        return;
    }
    
    Pawn->ResetForPool();
}

void UVehiclePoolManager::ExpandPool(uint8 VehicleType, int32 ExpansionCount)
{
    PreAllocateVehicles(VehicleType, ExpansionCount);
}

void UVehiclePoolManager::ShrinkUnusedPools()
{
    if (!bAutoShrinkPools)
    {
        return;
    }
    
    const float CurrentTime = FPlatformTime::Seconds();
    
    for (auto& AvailablePoolPair : AvailablePawnPools)
    {
        const uint8 VehicleType = AvailablePoolPair.Key;
        TArray<AVehicleBasePawn*>& AvailablePool = AvailablePoolPair.Value;
        
        const float* LastUsage = LastUsageTime.Find(VehicleType);
        const bool bIsUnused = !LastUsage || (CurrentTime - *LastUsage) > UnusedActorTimeout;
        
        if (bIsUnused && AvailablePool.Num() > InitialPoolSize)
        {
            // Remove excess actors
            const int32 ActorsToRemove = AvailablePool.Num() - InitialPoolSize;
            
            for (int32 i = 0; i < ActorsToRemove; i++)
            {
                if (AvailablePool.Num() > 0)
                {
                    AVehicleBasePawn* PawnToDestroy = AvailablePool.Pop();
                    if (IsValid(PawnToDestroy))
                    {
                        PawnToTypeMap.Remove(PawnToDestroy);
                        PawnToDestroy->Destroy();
                    }
                }
            }
            
            UE_LOG(LogRealGazeboBridge, Verbose, TEXT("Shrunk unused pool for vehicle type %d by %d actors"), 
                   VehicleType, ActorsToRemove);
        }
    }
}

void UVehiclePoolManager::UpdatePoolStatistics() const
{
    float MemoryUsage = 0.0f;
    
    // Estimate memory usage (rough calculation)
    for (const auto& AvailablePoolPair : AvailablePawnPools)
    {
        MemoryUsage += AvailablePoolPair.Value.Num() * sizeof(AVehicleBasePawn);
    }
    
    for (const auto& ActivePoolPair : ActivePawnPools)
    {
        MemoryUsage += ActivePoolPair.Value.Num() * sizeof(AVehicleBasePawn);
    }
    
    TotalMemoryUsage = MemoryUsage / (1024.0f * 1024.0f); // Convert to MB
}

void UVehiclePoolManager::OnPoolCleanupTimer()
{
    ShrinkUnusedPools();
    UpdatePoolStatistics();
}

void UVehiclePoolManager::PrintPoolStatistics() const
{
    UE_LOG(LogRealGazeboBridge, Display, TEXT("=== Vehicle Pool Statistics ==="));
    
    int32 TotalAvailable = 0;
    int32 TotalActive = 0;
    
    for (const auto& PoolPair : AvailablePawnPools)
    {
        const uint8 VehicleType = PoolPair.Key;
        const int32 AvailableCount = PoolPair.Value.Num();
        const int32 ActiveCount = GetActiveCount(VehicleType);
        const int32 TotalSpawned = SpawnCount.FindRef(VehicleType);
        const int32 TotalReleased = ReleaseCount.FindRef(VehicleType);
        
        UE_LOG(LogRealGazeboBridge, Display, TEXT("Type %d: Available=%d, Active=%d, Spawned=%d, Released=%d"), 
               VehicleType, AvailableCount, ActiveCount, TotalSpawned, TotalReleased);
        
        TotalAvailable += AvailableCount;
        TotalActive += ActiveCount;
    }
    
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Total: Available=%d, Active=%d, Memory=%.2fMB"), 
           TotalAvailable, TotalActive, TotalMemoryUsage);
}

bool UVehiclePoolManager::ValidatePoolIntegrity() const
{
#if WITH_EDITOR || !UE_BUILD_SHIPPING
    bool bIntegrityValid = true;
    
    // Check that all actors in pools are valid
    for (const auto& PoolPair : AvailablePawnPools)
    {
        for (AVehicleBasePawn* Actor : PoolPair.Value)
        {
            if (!IsValid(Actor))
            {
                UE_LOG(LogRealGazeboBridge, Error, TEXT("Invalid actor found in available pool"));
                bIntegrityValid = false;
            }
        }
    }
    
    for (const auto& PoolPair : ActivePawnPools)
    {
        for (AVehicleBasePawn* Actor : PoolPair.Value)
        {
            if (!IsValid(Actor))
            {
                UE_LOG(LogRealGazeboBridge, Error, TEXT("Invalid actor found in active pool"));
                bIntegrityValid = false;
            }
        }
    }
    
    // Check that PawnToTypeMap is consistent
    for (const auto& MappingPair : PawnToTypeMap)
    {
        if (!IsValid(MappingPair.Key))
        {
            UE_LOG(LogRealGazeboBridge, Error, TEXT("Invalid actor found in type mapping"));
            bIntegrityValid = false;
        }
    }
    
    return bIntegrityValid;
#else
    return true;
#endif
}