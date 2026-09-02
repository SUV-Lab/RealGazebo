// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Sub-author: MinKyu Kim
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#include "GazeboBridgeSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "DataStreamProcessor.h"
#include "VehiclePoolManager.h"
#include "VehicleBasePawn.h"
#include "SpawnCommandSender.h"
#include "RealGazeboBridge.h"

UGazeboBridgeSubsystem::UGazeboBridgeSubsystem()
    : bIsBridgeActive(false)  // Atomic initialization
{
    ListenPort = 5005;
    ServerIPAddress = TEXT("");
    bAutoSpawnVehicles = true;
    ConfiguredUpdateFrequency = 60.0f;

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
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        GameInstance->GetTimerManager().ClearTimer(WindQueryRetryTimer);
    }
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

//----------------------------------------------------------
// Runtime Commands (UE -> simulation manager): spawn/despawn, world wind
//----------------------------------------------------------

USpawnCommandSender* UGazeboBridgeSubsystem::EnsureSpawnSender()
{
    if (!SpawnSender)
    {
        SpawnSender = NewObject<USpawnCommandSender>(this);
    }
    return SpawnSender;
}

FString UGazeboBridgeSubsystem::ResolveGazeboHostIP() const
{
    return !GazeboHostIP.IsEmpty() ? GazeboHostIP : LearnedGazeboHostIP;
}

void UGazeboBridgeSubsystem::NotifyGazeboHost(const FString& SenderIP)
{
    if (!SenderIP.IsEmpty() && LearnedGazeboHostIP != SenderIP)
    {
        LearnedGazeboHostIP = SenderIP;
        UE_LOG(LogRealGazeboBridge, Display,
               TEXT("Runtime commands (spawn/despawn/wind) will target simulation host %s:%d"),
               *SenderIP, SpawnCommandPort);
        // First contact with this sim: find out whether it already has wind
        // (a previous session may have left some blowing). The manager may
        // open its command port a little after its first vehicle starts
        // streaming, so keep asking until it answers.
        WindQueryAttemptsLeft = 5;
        bAwaitingWorldWindState = true;
        RetryWorldWindQuery();
    }
}

void UGazeboBridgeSubsystem::RetryWorldWindQuery()
{
    if (!bAwaitingWorldWindState || WindQueryAttemptsLeft <= 0)
    {
        return;
    }
    --WindQueryAttemptsLeft;
    RequestWorldWindState();
    if (WindQueryAttemptsLeft > 0)
    {
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            GameInstance->GetTimerManager().SetTimer(
                WindQueryRetryTimer,
                FTimerDelegate::CreateUObject(this, &UGazeboBridgeSubsystem::RetryWorldWindQuery),
                1.0f, false);
        }
    }
}

USpawnCommandSender* UGazeboBridgeSubsystem::PrepareCommandSender(const TCHAR* CommandName,
                                                                  FString& OutHostIP)
{
    OutHostIP = ResolveGazeboHostIP();
    if (OutHostIP.IsEmpty())
    {
        UE_LOG(LogRealGazeboBridge, Warning,
               TEXT("%s: simulation host unknown - set GazeboHostIP or wait for "
                    "the first incoming sim packet"), CommandName);
        return nullptr;
    }

    USpawnCommandSender* Sender = EnsureSpawnSender();
    if (!Sender->SetDestination(OutHostIP, SpawnCommandPort))
    {
        return nullptr; // SetDestination logged why
    }
    return Sender;
}

bool UGazeboBridgeSubsystem::SpawnVehicleAt(uint8 VehicleTypeCode, uint8 VehicleNum,
                                            FVector WorldLocation, FRotator WorldRotation)
{
    FString HostIP;
    USpawnCommandSender* Sender = PrepareCommandSender(TEXT("SpawnVehicleAt"), HostIP);
    if (!Sender)
    {
        return false;
    }

    const bool bSent = Sender->SendSpawn(VehicleNum, VehicleTypeCode,
                                         WorldLocation, WorldRotation.Quaternion());
    if (bSent)
    {
        UE_LOG(LogRealGazeboBridge, Display,
               TEXT("Spawn command sent: type=%d num=%d -> %s:%d"),
               VehicleTypeCode, VehicleNum, *HostIP, SpawnCommandPort);
    }
    return bSent;
}

bool UGazeboBridgeSubsystem::SetWorldWind(bool bEnable, FVector GazeboWindVelocity)
{
    // The wire carries float32: reject NaN/inf AND doubles beyond float
    // range, which would narrow to inf and be refused by the manager while
    // this call reported success.
    const double MaxFinite = static_cast<double>(TNumericLimits<float>::Max());
    if (GazeboWindVelocity.ContainsNaN()
        || FMath::Abs(GazeboWindVelocity.X) > MaxFinite
        || FMath::Abs(GazeboWindVelocity.Y) > MaxFinite
        || FMath::Abs(GazeboWindVelocity.Z) > MaxFinite)
    {
        UE_LOG(LogRealGazeboBridge, Warning,
               TEXT("SetWorldWind: wind velocity is not a finite float - command not sent"));
        return false;
    }

    FString HostIP;
    USpawnCommandSender* Sender = PrepareCommandSender(TEXT("SetWorldWind"), HostIP);
    if (!Sender)
    {
        return false;
    }

    const bool bSent = Sender->SendWindCommand(bEnable, GazeboWindVelocity);
    if (bSent)
    {
        UE_LOG(LogRealGazeboBridge, Display,
               TEXT("Wind command sent: %s (%.2f, %.2f, %.2f) m/s -> %s:%d"),
               bEnable ? TEXT("on") : TEXT("off"),
               GazeboWindVelocity.X, GazeboWindVelocity.Y, GazeboWindVelocity.Z,
               *HostIP, SpawnCommandPort);
    }
    return bSent;
}

bool UGazeboBridgeSubsystem::RequestWorldWindState()
{
    FString HostIP;
    USpawnCommandSender* Sender = PrepareCommandSender(TEXT("RequestWorldWindState"), HostIP);
    if (!Sender)
    {
        return false;
    }

    const bool bSent = Sender->SendWindQuery();
    if (bSent)
    {
        UE_LOG(LogRealGazeboBridge, Display,
               TEXT("Wind state query sent -> %s:%d"), *HostIP, SpawnCommandPort);
    }
    return bSent;
}

void UGazeboBridgeSubsystem::NotifyWorldWindState(bool bEnabled, const FVector& GazeboWindVelocity)
{
    bAwaitingWorldWindState = false;
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        GameInstance->GetTimerManager().ClearTimer(WindQueryRetryTimer);
    }
    UE_LOG(LogRealGazeboBridge, Display,
           TEXT("World wind state received: %s (%.2f, %.2f, %.2f) m/s"),
           bEnabled ? TEXT("on") : TEXT("off"),
           GazeboWindVelocity.X, GazeboWindVelocity.Y, GazeboWindVelocity.Z);
    OnWorldWindStateReceived.Broadcast(bEnabled, GazeboWindVelocity);
}

bool UGazeboBridgeSubsystem::DespawnVehicle(const FVehicleID& VehicleID)
{
    FString HostIP;
    USpawnCommandSender* Sender = PrepareCommandSender(TEXT("DespawnVehicle"), HostIP);
    if (!Sender)
    {
        return false;
    }

    const bool bSent = Sender->SendDespawn(VehicleID.VehicleNum, VehicleID.VehicleType);
    if (bSent)
    {
        UE_LOG(LogRealGazeboBridge, Display,
               TEXT("Despawn command sent: %s -> %s:%d"),
               *VehicleID.ToString(), *HostIP, SpawnCommandPort);
    }
    return bSent;
}

int32 UGazeboBridgeSubsystem::GetNextFreeVehicleNum() const
{
    TSet<uint8> UsedNums;
    for (const TPair<FVehicleID, FVehicleRuntimeData>& Pair : VehicleDataMap)
    {
        UsedNums.Add(Pair.Key.VehicleNum);
    }
    for (int32 Num = 0; Num <= 255; ++Num)
    {
        if (!UsedNums.Contains(static_cast<uint8>(Num)))
        {
            return Num;
        }
    }
    return -1;
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

    // Initialize VehiclePool with valid world reference if not already initialized
    if (VehiclePool)
    {
        if (UWorld* World = GetWorld())
        {
            VehiclePool->InitializePool(World);
            UE_LOG(LogRealGazeboBridge, Display, TEXT("VehiclePool initialized with world: %s"), *World->GetName());
        }
        else
        {
            UE_LOG(LogRealGazeboBridge, Error, TEXT("Cannot initialize VehiclePool - no valid world"));
            return;
        }
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
    TArray<AVehicleBasePawn*> UntrackedVisualPawns;
    for (const TPair<FVehicleID, FVehicleRuntimeData>& Pair : VehicleDataMap)
    {
        AVehicleBasePawn* VisualPawn = Pair.Value.VisualPawn.Get();
        if (IsValid(VisualPawn) && (!VehiclePool || !VehiclePool->IsVehicleTracked(VisualPawn)))
        {
            UntrackedVisualPawns.AddUnique(VisualPawn);
        }
    }

    if (VehiclePool)
    {
        VehiclePool->ReleaseAllActiveVehicles();
    }

    for (AVehicleBasePawn* VisualPawn : UntrackedVisualPawns)
    {
        if (IsValid(VisualPawn))
        {
            VisualPawn->Destroy();
        }
    }

    VehicleDataMap.Empty();

    UE_LOG(LogRealGazeboBridge, Display, TEXT("All vehicles cleared"));
}

void UGazeboBridgeSubsystem::RemoveVehicle(const FVehicleID& VehicleID)
{
    // Find all vehicles with the same VehicleNum using helper function
    TArray<FVehicleID> VehiclesToRemove = FindVehiclesByNum(VehicleID.VehicleNum);

    // If no vehicles found, log and return
    if (VehiclesToRemove.Num() == 0)
    {
        UE_LOG(LogRealGazeboBridge, Warning, TEXT("No vehicles found with Num=%d for removal"),
               VehicleID.VehicleNum);
        return;
    }

    // Destroy all vehicles with the same VehicleNum
    int32 DestroyedCount = 0;
    for (const FVehicleID& IDToRemove : VehiclesToRemove)
    {
        if (FVehicleRuntimeData* VehicleData = VehicleDataMap.Find(IDToRemove))
        {
            // DESTROY the actor completely instead of releasing to pool
            // This removes it from the World Outliner
            if (AVehicleBasePawn* Pawn = VehicleData->VisualPawn.Get())
            {
                // Notify pool manager to remove tracking
                if (VehiclePool)
                {
                    VehiclePool->DestroyVehicleActor(Pawn);
                }

                // Destroy the actor (removes from World Outliner)
                Pawn->Destroy();

                UE_LOG(LogRealGazeboBridge, Display, TEXT("Vehicle actor destroyed: Type=%d, Num=%d"),
                       IDToRemove.VehicleType, IDToRemove.VehicleNum);
            }

            VehicleData->VisualPawn.Reset();

            // Remove from the vehicle data map
            VehicleDataMap.Remove(IDToRemove);
            DestroyedCount++;

            UE_LOG(LogRealGazeboBridge, Display, TEXT("Vehicle removed/destroyed: Type=%d, Num=%d"),
                   IDToRemove.VehicleType, IDToRemove.VehicleNum);
        }
    }

    UE_LOG(LogRealGazeboBridge, Display, TEXT("Destroy command (MessageID=4) completed: Num=%d, Destroyed %d vehicle(s)"),
           VehicleID.VehicleNum, DestroyedCount);
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

TArray<FVehicleID> UGazeboBridgeSubsystem::FindVehiclesByNum(uint8 VehicleNum) const
{
    TArray<FVehicleID> FoundVehicles;

    for (const auto& Pair : VehicleDataMap)
    {
        if (Pair.Key.VehicleNum == VehicleNum)
        {
            FoundVehicles.Add(Pair.Key);
        }
    }

    return FoundVehicles;
}

bool UGazeboBridgeSubsystem::RegisterVehiclePawn(const FVehicleID& VehicleID, AVehicleBasePawn* VehiclePawn)
{
    if (!IsValid(VehiclePawn))
    {
        UE_LOG(LogRealGazeboBridge, Warning,
               TEXT("RegisterVehiclePawn failed: invalid pawn for vehicle %s"),
               *VehicleID.ToString());
        return false;
    }

    FVehicleRuntimeData& RuntimeData = VehicleDataMap.FindOrAdd(VehicleID);
    if (RuntimeData.VisualPawn.IsValid() && RuntimeData.VisualPawn.Get() != VehiclePawn)
    {
        UE_LOG(LogRealGazeboBridge, Warning,
               TEXT("RegisterVehiclePawn failed: vehicle %s already has a different visual pawn"),
               *VehicleID.ToString());
        return false;
    }

    RuntimeData.Position = VehiclePawn->GetActorLocation();
    RuntimeData.Rotation = VehiclePawn->GetActorRotation().Quaternion();
    RuntimeData.LastUpdateTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    RuntimeData.VehicleType = VehicleID.VehicleType;
    RuntimeData.VisualPawn = VehiclePawn;

    VehiclePawn->InitializeForPool(VehicleID, VehicleID.VehicleType);

    if (OnVehicleSpawned.IsBound())
    {
        FBridgePoseData PoseData;
        PoseData.VehicleNum = VehicleID.VehicleNum;
        PoseData.VehicleType = VehicleID.VehicleType;
        PoseData.Position = RuntimeData.Position;
        PoseData.Rotation = RuntimeData.Rotation.Rotator();

        OnVehicleSpawned.Broadcast(PoseData);
    }

    UE_LOG(LogRealGazeboBridge, Display,
           TEXT("Registered existing vehicle pawn: %s (%s)"),
           *VehicleID.ToString(), *VehiclePawn->GetName());

    return true;
}

bool UGazeboBridgeSubsystem::UnregisterVehiclePawn(const FVehicleID& VehicleID, bool bDestroyVisualPawn)
{
    FVehicleRuntimeData* RuntimeData = VehicleDataMap.Find(VehicleID);
    if (!RuntimeData)
    {
        UE_LOG(LogRealGazeboBridge, Warning,
               TEXT("UnregisterVehiclePawn failed: vehicle %s is not registered"),
               *VehicleID.ToString());
        return false;
    }

    AVehicleBasePawn* VisualPawn = RuntimeData->VisualPawn.Get();
    if (bDestroyVisualPawn && IsValid(VisualPawn))
    {
        if (VehiclePool && VehiclePool->IsVehicleTracked(VisualPawn))
        {
            VehiclePool->DestroyVehicleActor(VisualPawn);
        }

        VisualPawn->Destroy();
    }

    VehicleDataMap.Remove(VehicleID);

    UE_LOG(LogRealGazeboBridge, Display,
           TEXT("Unregistered vehicle pawn: %s (DestroyVisualPawn=%s)"),
           *VehicleID.ToString(), bDestroyVisualPawn ? TEXT("true") : TEXT("false"));

    return true;
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
	if (!GEngine || !WorldContext)
	{
		return nullptr;
	}

    if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
    {
        // During UGameEngine startup, a Game-type World can be created before its GameInstance.
        // If called in that window the bridge can't be resolved yet, so return nullptr safely.
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UGazeboBridgeSubsystem>();
        }
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

void UGazeboBridgeSubsystem::UpdateVehicleAdditionalData(const FBridgeAdditionalData& AdditionalData)
{
    const FVehicleID VehicleID = AdditionalData.GetVehicleID();
    if (FVehicleRuntimeData* RuntimeData = VehicleDataMap.Find(VehicleID))
    {
        RuntimeData->BatteryRemaining = AdditionalData.BatteryRemaining;
        RuntimeData->NavState = AdditionalData.NavState;
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

void UGazeboBridgeSubsystem::OnAdditionalDataReceived(const FBridgeAdditionalData& AdditionalData)
{
    UpdateVehicleAdditionalData(AdditionalData);
}
