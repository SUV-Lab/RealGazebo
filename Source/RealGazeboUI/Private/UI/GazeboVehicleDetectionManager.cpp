// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/GazeboVehicleDetectionManager.h"
#include "Vehicle/GazeboVehicleActor.h"
#include "Camera/GazeboCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AGazeboVehicleDetectionManager::AGazeboVehicleDetectionManager()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f; // 10 times per second

    // Set default vehicle class to detect
    VehicleClassToDetect = AGazeboVehicleActor::StaticClass();
}

void AGazeboVehicleDetectionManager::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoDetectOnBeginPlay)
    {
        DetectAllVehiclesInLevel();
        InitializeAllVehicles();
    }

    UE_LOG(LogTemp, Log, TEXT("GazeboVehicleDetectionManager: Started"));
}

void AGazeboVehicleDetectionManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DetectedVehicles.Empty();
    Super::EndPlay(EndPlayReason);
}

void AGazeboVehicleDetectionManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bContinuousDetection)
    {
        DetectionTimer += DeltaTime;
        if (DetectionTimer >= DetectionInterval)
        {
            UpdateDetectedVehicles();
            DetectionTimer = 0.0f;
        }
    }
}

void AGazeboVehicleDetectionManager::DetectAllVehiclesInLevel()
{
    if (!GetWorld()) return;

    UE_LOG(LogTemp, Log, TEXT("GazeboVehicleDetectionManager: Detecting vehicles in level..."));

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), VehicleClassToDetect, FoundActors);

    int32 NewVehicles = 0;
    for (AActor* Actor : FoundActors)
    {
        if (AGazeboVehicleActor* VehicleActor = Cast<AGazeboVehicleActor>(Actor))
        {
            if (!IsVehicleAlreadyDetected(VehicleActor))
            {
                AddVehicle(VehicleActor);
                NewVehicles++;
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("GazeboVehicleDetectionManager: Found %d vehicles (%d new)"), 
           DetectedVehicles.Num(), NewVehicles);
}

TArray<AGazeboVehicleActor*> AGazeboVehicleDetectionManager::GetAllVehicleActors() const
{
    TArray<AGazeboVehicleActor*> VehicleActors;
    for (const FDetectedVehicleInfo& VehicleInfo : DetectedVehicles)
    {
        if (VehicleInfo.VehicleActor && IsValid(VehicleInfo.VehicleActor))
        {
            VehicleActors.Add(VehicleInfo.VehicleActor);
        }
    }
    return VehicleActors;
}

AGazeboVehicleActor* AGazeboVehicleDetectionManager::GetVehicleByID(int32 VehicleID) const
{
    for (const FDetectedVehicleInfo& VehicleInfo : DetectedVehicles)
    {
        if (VehicleInfo.VehicleID == VehicleID && IsValid(VehicleInfo.VehicleActor))
        {
            return VehicleInfo.VehicleActor;
        }
    }
    return nullptr;
}

AGazeboVehicleActor* AGazeboVehicleDetectionManager::GetVehicleByIndex(int32 Index) const
{
    if (DetectedVehicles.IsValidIndex(Index) && IsValid(DetectedVehicles[Index].VehicleActor))
    {
        return DetectedVehicles[Index].VehicleActor;
    }
    return nullptr;
}

void AGazeboVehicleDetectionManager::InitializeAllVehicles()
{
    for (FDetectedVehicleInfo& VehicleInfo : DetectedVehicles)
    {
        if (VehicleInfo.VehicleActor && IsValid(VehicleInfo.VehicleActor))
        {
            // Set vehicle ID if not already set
            if (VehicleInfo.VehicleActor->VehicleNum == 0)
            {
                VehicleInfo.VehicleActor->VehicleNum = VehicleInfo.VehicleID;
            }
            
            VehicleInfo.bIsActive = true;
            UE_LOG(LogTemp, Log, TEXT("GazeboVehicleDetectionManager: Initialized vehicle %s (ID: %d)"), 
                   *VehicleInfo.VehicleName, VehicleInfo.VehicleID);
        }
    }
}

void AGazeboVehicleDetectionManager::SetCameraManager(AGazeboCameraManager* NewCameraManager)
{
    this->CameraManager = NewCameraManager;
    if (NewCameraManager)
    {
        UE_LOG(LogTemp, Log, TEXT("GazeboVehicleDetectionManager: Camera manager set"));
    }
}

void AGazeboVehicleDetectionManager::SetTargetVehicleByIndex(int32 Index)
{
    AGazeboVehicleActor* TargetVehicle = GetVehicleByIndex(Index);
    if (TargetVehicle && CameraManager)
    {
        CameraManager->SetTargetVehicle(TargetVehicle);
        UE_LOG(LogTemp, Log, TEXT("GazeboVehicleDetectionManager: Set target vehicle by index %d"), Index);
    }
    else if (!CameraManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("GazeboVehicleDetectionManager: No camera manager set"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GazeboVehicleDetectionManager: Invalid vehicle index %d"), Index);
    }
}

void AGazeboVehicleDetectionManager::SetTargetVehicleByID(int32 VehicleID)
{
    AGazeboVehicleActor* TargetVehicle = GetVehicleByID(VehicleID);
    if (TargetVehicle && CameraManager)
    {
        CameraManager->SetTargetVehicle(TargetVehicle);
        UE_LOG(LogTemp, Log, TEXT("GazeboVehicleDetectionManager: Set target vehicle by ID %d"), VehicleID);
    }
    else if (!CameraManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("GazeboVehicleDetectionManager: No camera manager set"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GazeboVehicleDetectionManager: Vehicle with ID %d not found"), VehicleID);
    }
}

void AGazeboVehicleDetectionManager::UpdateDetectedVehicles()
{
    if (!GetWorld()) return;

    // Check for new vehicles
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), VehicleClassToDetect, FoundActors);

    // Add new vehicles
    for (AActor* Actor : FoundActors)
    {
        if (AGazeboVehicleActor* VehicleActor = Cast<AGazeboVehicleActor>(Actor))
        {
            if (!IsVehicleAlreadyDetected(VehicleActor))
            {
                AddVehicle(VehicleActor);
            }
        }
    }

    // Remove invalid vehicles
    for (int32 i = DetectedVehicles.Num() - 1; i >= 0; i--)
    {
        if (!IsValid(DetectedVehicles[i].VehicleActor))
        {
            UE_LOG(LogTemp, Log, TEXT("GazeboVehicleDetectionManager: Removing invalid vehicle %s"), 
                   *DetectedVehicles[i].VehicleName);
            
            OnVehicleRemoved.Broadcast(DetectedVehicles[i].VehicleActor);
            DetectedVehicles.RemoveAt(i);
        }
    }
}

void AGazeboVehicleDetectionManager::AddVehicle(AGazeboVehicleActor* Vehicle)
{
    if (!Vehicle) return;

    int32 VehicleID = GetNextAvailableVehicleID();
    FString VehicleName = FString::Printf(TEXT("Vehicle_%d"), VehicleID);

    // Try to get a better name from the actor
    if (!Vehicle->GetName().IsEmpty())
    {
        VehicleName = Vehicle->GetName();
    }

    FDetectedVehicleInfo NewVehicleInfo(Vehicle, VehicleID, VehicleName);
    DetectedVehicles.Add(NewVehicleInfo);

    OnVehicleDetected.Broadcast(Vehicle);

    UE_LOG(LogTemp, Log, TEXT("GazeboVehicleDetectionManager: Added vehicle %s (ID: %d)"), 
           *VehicleName, VehicleID);
}

void AGazeboVehicleDetectionManager::RemoveVehicle(AGazeboVehicleActor* Vehicle)
{
    for (int32 i = 0; i < DetectedVehicles.Num(); i++)
    {
        if (DetectedVehicles[i].VehicleActor == Vehicle)
        {
            OnVehicleRemoved.Broadcast(Vehicle);
            DetectedVehicles.RemoveAt(i);
            break;
        }
    }
}

bool AGazeboVehicleDetectionManager::IsVehicleAlreadyDetected(AGazeboVehicleActor* Vehicle) const
{
    for (const FDetectedVehicleInfo& VehicleInfo : DetectedVehicles)
    {
        if (VehicleInfo.VehicleActor == Vehicle)
        {
            return true;
        }
    }
    return false;
}

int32 AGazeboVehicleDetectionManager::GetNextAvailableVehicleID() const
{
    int32 NextID = 1;
    TArray<int32> UsedIDs;

    // Collect all used IDs
    for (const FDetectedVehicleInfo& VehicleInfo : DetectedVehicles)
    {
        UsedIDs.Add(VehicleInfo.VehicleID);
    }

    // Find the first available ID
    while (UsedIDs.Contains(NextID))
    {
        NextID++;
    }

    return NextID;
}