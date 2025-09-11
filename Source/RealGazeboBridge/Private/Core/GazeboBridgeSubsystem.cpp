
#include "GazeboBridgeSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "DataStreamProcessor.h"
#include "VehiclePoolManager.h"
#include "RealGazeboBridge.h"

UGazeboBridgeSubsystem::UGazeboBridgeSubsystem()
{
    ListenPort = 5005;
    ServerIPAddress = TEXT("");
    bAutoSpawnVehicles = true;
    ConfiguredUpdateFrequency = 60.0f;
    
    bIsBridgeActive = false;
    LastUpdateTime = 0.0f;
    FrameCounter = 0;
    AverageFrameTime = 0.0f;
    MemoryUsageMB = 0.0f;
    LastPerformanceCheck = 0.0;
}

void UGazeboBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogRealGazeboBridge, Display, TEXT("GazeboBridgeSubsystem: Initializing high-performance bridge"));
    
    // Create core components
    StreamProcessor = NewObject<UDataStreamProcessor>(this);
    if (StreamProcessor)
    {
        StreamProcessor->Initialize(this);
    }
    
    VehiclePool = NewObject<UVehiclePoolManager>(this);
    if (VehiclePool)
    {
        VehiclePool->InitializePool(GetWorld());
    }
    
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Subsystem initialized"));
}

void UGazeboBridgeSubsystem::Deinitialize()
{
    StopBridge();
    
    if (VehiclePool)
    {
        VehiclePool->ShutdownPool();
        VehiclePool = nullptr;
    }
    
    if (StreamProcessor)
    {
        StreamProcessor->Shutdown();
        StreamProcessor = nullptr;
    }
    
    VehicleDataMap.Empty();
    
    UE_LOG(LogRealGazeboBridge, Log, TEXT("GazeboBridgeSubsystem: Deinitialized"));
    
    Super::Deinitialize();
}

bool UGazeboBridgeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return !IsRunningDedicatedServer();
}

void UGazeboBridgeSubsystem::StartBridge()
{
    if (bIsBridgeActive)
    {
        UE_LOG(LogRealGazeboBridge, Warning, TEXT("Bridge already active"));
        return;
    }
    
    if (!StreamProcessor)
    {
        UE_LOG(LogRealGazeboBridge, Error, TEXT("StreamProcessor not available"));
        return;
    }
    
    // Start network processing
    if (StreamProcessor->StartDataStream(ListenPort, ServerIPAddress))
    {
        bIsBridgeActive = true;
        
        // Setup update timer for batch processing
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                UpdateTimerHandle,
                this,
                &UGazeboBridgeSubsystem::BatchUpdateVehicles,
                1.0f / ConfiguredUpdateFrequency,
                true
            );
        }
        
        UE_LOG(LogRealGazeboBridge, Display, TEXT("Bridge started on port %d"), ListenPort);
    }
    else
    {
        UE_LOG(LogRealGazeboBridge, Error, TEXT("Failed to start bridge on port %d"), ListenPort);
    }
}

void UGazeboBridgeSubsystem::StopBridge()
{
    if (!bIsBridgeActive)
    {
        return;
    }
    
    // Clear update timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UpdateTimerHandle);
    }
    
    // Stop network processing
    if (StreamProcessor)
    {
        StreamProcessor->StopDataStream();
    }
    
    // Clear all vehicles
    ClearAllVehicles();
    
    bIsBridgeActive = false;
    
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Bridge stopped"));
}

bool UGazeboBridgeSubsystem::IsBridgeActive() const
{
    return bIsBridgeActive && StreamProcessor && StreamProcessor->IsStreamActive();
}

void UGazeboBridgeSubsystem::ClearAllVehicles()
{
    if (VehiclePool)
    {
        VehiclePool->ReleaseAllActiveVehicles();
    }
    
    VehicleDataMap.Empty();
    
    UE_LOG(LogRealGazeboBridge, Display, TEXT("All vehicles cleared"));
}

int32 UGazeboBridgeSubsystem::GetTotalVehicleCount() const
{
    return VehicleDataMap.Num();
}

int32 UGazeboBridgeSubsystem::GetActiveVehicleCount() const
{
    int32 ActiveCount = 0;
    for (const auto& VehiclePair : VehicleDataMap)
    {
        if (VehiclePair.Value.VisualPawn.IsValid())
        {
            ActiveCount++;
        }
    }
    return ActiveCount;
}

int32 UGazeboBridgeSubsystem::GetVisibleVehicleCount() const
{
    // All vehicles are always visible (like original RealGazebo)
    return GetActiveVehicleCount();
}

FVehicleRuntimeData UGazeboBridgeSubsystem::GetVehicleData(const FVehicleID& VehicleID) const
{
    if (const FVehicleRuntimeData* Data = VehicleDataMap.Find(VehicleID))
    {
        return *Data;
    }
    
    return FVehicleRuntimeData();
}

TArray<FVehicleID> UGazeboBridgeSubsystem::GetAllVehicleIDs() const
{
    TArray<FVehicleID> VehicleIDs;
    VehicleDataMap.GetKeys(VehicleIDs);
    return VehicleIDs;
}

bool UGazeboBridgeSubsystem::GetVehicleConfig(uint8 VehicleType, FBridgeVehicleConfigRow& OutConfig) const
{
    const FBridgeVehicleConfigRow* ConfigRow = GetVehicleConfigInternal(VehicleType);
    if (ConfigRow)
    {
        OutConfig = *ConfigRow;
        return true;
    }
    
    return false;
}

UGazeboBridgeSubsystem* UGazeboBridgeSubsystem::GetBridgeSubsystem(const UObject* WorldContext)
{
    if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
    {
        return World->GetGameInstance()->GetSubsystem<UGazeboBridgeSubsystem>();
    }
    
    return nullptr;
}

void UGazeboBridgeSubsystem::UpdateVehicleData(const FBridgePoseData& PoseData)
{
    const FVehicleID VehicleID = PoseData.GetVehicleID();
    FVehicleRuntimeData& RuntimeData = VehicleDataMap.FindOrAdd(VehicleID);
    
    // Update transform data
    RuntimeData.Position = PoseData.Position;
    RuntimeData.Rotation = PoseData.Rotation.Quaternion();
    RuntimeData.LastUpdateTime = GetWorld()->GetTimeSeconds();
    RuntimeData.VehicleType = PoseData.VehicleType;
    
    // Spawn visual actor if needed and within range
    if (bAutoSpawnVehicles && !RuntimeData.VisualPawn.IsValid())
    {
        SpawnVehiclePawn(VehicleID, RuntimeData);
    }
    
    // Update visual pawn if it exists
    if (AVehicleBasePawn* VisualPawn = RuntimeData.VisualPawn.Get())
    {
        VisualPawn->UpdateVehiclePose(RuntimeData.Position, RuntimeData.Rotation);
    }
    
    // Broadcast event
    OnVehicleUpdated.Broadcast(PoseData);
}

void UGazeboBridgeSubsystem::UpdateVehicleMotorData(const FBridgeMotorSpeedData& MotorData)
{
    const FVehicleID VehicleID = MotorData.GetVehicleID();
    if (FVehicleRuntimeData* RuntimeData = VehicleDataMap.Find(VehicleID))
    {
        RuntimeData->MotorSpeeds = MotorData.MotorSpeeds_DegPerSec;
        
        if (AVehicleBasePawn* VisualPawn = RuntimeData->VisualPawn.Get())
        {
            VisualPawn->UpdateMotorSpeeds(RuntimeData->MotorSpeeds);
        }
    }
}

void UGazeboBridgeSubsystem::UpdateVehicleServoData(const FBridgeServoData& ServoData)
{
    const FVehicleID VehicleID = ServoData.GetVehicleID();
    if (FVehicleRuntimeData* RuntimeData = VehicleDataMap.Find(VehicleID))
    {
        RuntimeData->ServoPositions = ServoData.ServoPositions;
        
        // Convert FRotator array to FQuat array
        RuntimeData->ServoRotations.Empty();
        for (const FRotator& Rotation : ServoData.ServoRotations)
        {
            RuntimeData->ServoRotations.Add(Rotation.Quaternion());
        }
        
        if (AVehicleBasePawn* VisualPawn = RuntimeData->VisualPawn.Get())
        {
            VisualPawn->UpdateServoStates(RuntimeData->ServoPositions, RuntimeData->ServoRotations);
        }
    }
}

void UGazeboBridgeSubsystem::BatchUpdateVehicles()
{
    if (!bIsBridgeActive)
    {
        return;
    }
    
    // All vehicles are always visible (like original RealGazebo)
    // No LOD system needed for simulation bridge
    
    // Performance tracking
    ++FrameCounter;
    const double CurrentTime = FPlatformTime::Seconds();
    
    if (CurrentTime - LastPerformanceCheck > 1.0) // Update stats every second
    {
        AverageFrameTime = (CurrentTime - LastPerformanceCheck) / FrameCounter;
        FrameCounter = 0;
        LastPerformanceCheck = CurrentTime;
        
        // Update memory usage estimate
        MemoryUsageMB = VehicleDataMap.Num() * sizeof(FVehicleRuntimeData) / (1024.0f * 1024.0f);
    }
}

void UGazeboBridgeSubsystem::GetPerformanceStats(int32& OutTotalVehicles, int32& OutVisibleVehicles, 
                                               float& OutAverageUpdateTime, float& OutMemoryUsageMB) const
{
    OutTotalVehicles = GetTotalVehicleCount();
    OutVisibleVehicles = GetVisibleVehicleCount();
    OutAverageUpdateTime = AverageFrameTime * 1000.0f; // Convert to milliseconds
    OutMemoryUsageMB = MemoryUsageMB;
}

const FBridgeVehicleConfigRow* UGazeboBridgeSubsystem::GetVehicleConfigInternal(uint8 VehicleType) const
{
    if (!VehicleConfigTable)
    {
        return nullptr;
    }
    
    TArray<FBridgeVehicleConfigRow*> AllRows;
    VehicleConfigTable->GetAllRows<FBridgeVehicleConfigRow>(TEXT("GetVehicleConfig"), AllRows);
    
    for (FBridgeVehicleConfigRow* Row : AllRows)
    {
        if (Row && Row->VehicleTypeCode == VehicleType)
        {
            return Row;
        }
    }
    
    return nullptr;
}

void UGazeboBridgeSubsystem::SpawnVehiclePawn(const FVehicleID& VehicleID, FVehicleRuntimeData& VehicleData)
{
    if (!VehiclePool)
    {
        return;
    }
    
    AVehicleBasePawn* NewActor = VehiclePool->AcquireVehicle(VehicleID.VehicleType, VehicleID);
    if (NewActor)
    {
        VehicleData.VisualPawn = NewActor;
        NewActor->InitializeForPool(VehicleID, VehicleID.VehicleType);
        
        UE_LOG(LogRealGazeboBridge, VeryVerbose, TEXT("Spawned vehicle: %s"), *VehicleID.ToString());
        
        // Broadcast spawn event
        if (OnVehicleSpawned.IsBound())
        {
            FBridgePoseData PoseData;
            PoseData.VehicleNum = VehicleID.VehicleNum;
            PoseData.VehicleType = VehicleID.VehicleType;
            PoseData.Position = VehicleData.Position;
            PoseData.Rotation = VehicleData.Rotation.Rotator();
            
            OnVehicleSpawned.Broadcast(PoseData);
        }
    }
}

void UGazeboBridgeSubsystem::ReleaseVehiclePawn(FVehicleRuntimeData& VehicleData)
{
    if (AVehicleBasePawn* Pawn = VehicleData.VisualPawn.Get())
    {
        if (VehiclePool)
        {
            VehiclePool->ReleaseVehicle(Pawn);
        }
        VehicleData.VisualPawn.Reset();
    }
}


// Event handlers for network data
void UGazeboBridgeSubsystem::OnPoseDataReceived(const FBridgePoseData& PoseData)
{
    UpdateVehicleData(PoseData);
}

void UGazeboBridgeSubsystem::OnMotorSpeedDataReceived(const FBridgeMotorSpeedData& MotorData)
{
    UpdateVehicleMotorData(MotorData);
}

void UGazeboBridgeSubsystem::OnServoDataReceived(const FBridgeServoData& ServoData)
{
    UpdateVehicleServoData(ServoData);
}