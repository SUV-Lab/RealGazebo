// Fill out your copyright notice in the Description page of Project Settings.

#include "GazeboVehicleManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UserCameraManager.h"

AGazeboVehicleManager::AGazeboVehicleManager()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f;

    // Create unified data receiver component
    UnifiedDataReceiver = CreateDefaultSubobject<UGazeboUnifiedDataReceiver>(TEXT("UnifiedDataReceiver"));

    // Create camera manager - ensure proper component creation
    CameraManager = CreateDefaultSubobject<UUserCameraManager>(TEXT("UserCameraManager"));

    TotalVehiclesSpawned = 0;
}

void AGazeboVehicleManager::BeginPlay()
{
    Super::BeginPlay();

    // Bind events to unified receiver (all properties are set directly on component)
    if (UnifiedDataReceiver)
    {
        UnifiedDataReceiver->OnVehiclePoseReceived.AddDynamic(this, &AGazeboVehicleManager::OnVehiclePoseDataReceived);
        UnifiedDataReceiver->OnVehicleMotorSpeedReceived.AddDynamic(this, &AGazeboVehicleManager::OnVehicleMotorSpeedDataReceived);
        UnifiedDataReceiver->OnVehicleServoReceived.AddDynamic(this, &AGazeboVehicleManager::OnVehicleServoDataReceived);
    }

    UE_LOG(LogTemp, Warning, TEXT("GazeboVehicleManager: Started - Auto spawn: %s"), 
           bAutoSpawnVehicles ? TEXT("ON") : TEXT("OFF"));
}

void AGazeboVehicleManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UnifiedDataReceiver)
    {
        UnifiedDataReceiver->OnVehiclePoseReceived.RemoveAll(this);
        UnifiedDataReceiver->OnVehicleMotorSpeedReceived.RemoveAll(this);
        UnifiedDataReceiver->OnVehicleServoReceived.RemoveAll(this);
    }

    ClearAllVehicles();
    Super::EndPlay(EndPlayReason);
}

void AGazeboVehicleManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Periodic status update
    if (GEngine && UnifiedDataReceiver && UnifiedDataReceiver->bLogParsedData)
    {
        GEngine->AddOnScreenDebugMessage(1, 1.1f, FColor::Green,
            FString::Printf(TEXT("Gazebo Manager: %d vehicles | P:%d M:%d S:%d packets"),
                          GetActiveVehicleCount(), 
                          UnifiedDataReceiver->ValidPosePacketsReceived,
                          UnifiedDataReceiver->ValidMotorSpeedPacketsReceived,
                          UnifiedDataReceiver->ValidServoPacketsReceived));
    }
}

void AGazeboVehicleManager::ClearAllVehicles()
{
    for (auto& VehiclePair : SpawnedVehicles)
    {
        if (VehiclePair.Value && IsValid(VehiclePair.Value))
        {
            VehiclePair.Value->Destroy();
        }
    }
    SpawnedVehicles.Empty();
    UE_LOG(LogTemp, Warning, TEXT("GazeboVehicleManager: All vehicles cleared"));
}

int32 AGazeboVehicleManager::GetActiveVehicleCount() const
{
    return SpawnedVehicles.Num();
}

TArray<AGazeboVehicleActor*> AGazeboVehicleManager::GetAllVehicles() const
{
    TArray<AGazeboVehicleActor*> Vehicles;
    SpawnedVehicles.GenerateValueArray(Vehicles);
    return Vehicles;
}

AGazeboVehicleActor* AGazeboVehicleManager::FindVehicle(uint8 VehicleNum, uint8 VehicleType) const
{
    FString VehicleKey = GetVehicleKey(VehicleNum, VehicleType);
    return SpawnedVehicles.FindRef(VehicleKey);
}

bool AGazeboVehicleManager::GetVehicleInfo(uint8 VehicleType, FGazeboVehicleTableRow& OutVehicleInfo) const
{
    FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfoInternal(VehicleType);
    if (VehicleInfo)
    {
        OutVehicleInfo = *VehicleInfo;
        return true;
    }
    return false;
}

FGazeboVehicleTableRow* AGazeboVehicleManager::GetVehicleInfoInternal(uint8 VehicleType) const
{
    if (!VehicleDataTable)
    {
        return nullptr;
    }

    TArray<FGazeboVehicleTableRow*> AllRows;
    VehicleDataTable->GetAllRows<FGazeboVehicleTableRow>(TEXT("GetVehicleInfo"), AllRows);

    for (FGazeboVehicleTableRow* Row : AllRows)
    {
        if (Row && Row->VehicleTypeCode == VehicleType)
        {
            return Row;
        }
    }

    return nullptr;
}

void AGazeboVehicleManager::OnVehiclePoseDataReceived(const FGazeboPoseData& VehicleData)
{
    FString VehicleKey = GetVehicleKey(VehicleData);
    
    // Find existing vehicle or spawn new one
    AGazeboVehicleActor* Vehicle = SpawnedVehicles.FindRef(VehicleKey);
    
    if (!Vehicle && bAutoSpawnVehicles)
    {
        Vehicle = SpawnVehicle(VehicleData);
        if (Vehicle)
        {
            SpawnedVehicles.Add(VehicleKey, Vehicle);
            TotalVehiclesSpawned++;
            
            FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfoInternal(VehicleData.VehicleType);
            FString VehicleName = VehicleInfo ? VehicleInfo->VehicleName : TEXT("Unknown");
            
            UE_LOG(LogTemp, Warning, TEXT("GazeboVehicleManager: Spawned %s_%d (Total: %d)"), 
                   *VehicleName, VehicleData.VehicleNum, TotalVehiclesSpawned);

            // Notify camera manager of new vehicle
            if (CameraManager)
            {
                CameraManager->OnVehicleSpawned(Vehicle);
            }
            
            OnVehicleSpawned.Broadcast(Vehicle);
        }
    }
    
    // Update vehicle pose
    if (Vehicle && IsValid(Vehicle))
    {
        Vehicle->UpdateVehiclePose(VehicleData);
    }
}

void AGazeboVehicleManager::OnVehicleMotorSpeedDataReceived(const FGazeboMotorSpeedData& MotorSpeedData)
{
    FString VehicleKey = GetVehicleKey(MotorSpeedData.VehicleNum, MotorSpeedData.VehicleType);
    
    // Find existing vehicle
    AGazeboVehicleActor* Vehicle = SpawnedVehicles.FindRef(VehicleKey);
    
    // Update vehicle motor speed if it exists
    if (Vehicle && IsValid(Vehicle))
    {
        Vehicle->UpdateVehicleMotorSpeed(MotorSpeedData);
    }
}

void AGazeboVehicleManager::OnVehicleServoDataReceived(const FGazeboServoData& ServoData)
{
    FString VehicleKey = GetVehicleKey(ServoData.VehicleNum, ServoData.VehicleType);
    
    // Find existing vehicle
    AGazeboVehicleActor* Vehicle = SpawnedVehicles.FindRef(VehicleKey);
    
    // Update vehicle servo if it exists
    if (Vehicle && IsValid(Vehicle))
    {
        Vehicle->UpdateVehicleServo(ServoData);
    }
}

AGazeboVehicleActor* AGazeboVehicleManager::SpawnVehicle(const FGazeboPoseData& VehicleData)
{
    TSubclassOf<AGazeboVehicleActor> VehicleClass = GetVehicleClassForType(VehicleData.VehicleType);
    
    if (!VehicleClass)
    {
        UE_LOG(LogTemp, Error, TEXT("GazeboVehicleManager: No vehicle class found for type %d"), VehicleData.VehicleType);
        return nullptr;
    }

    FVector SpawnLocation = GetSpawnLocation(VehicleData);
    FRotator SpawnRotation = VehicleData.Rotation;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    // Prevent automatic BeginPlay call
    SpawnParams.bDeferConstruction = true;

    AGazeboVehicleActor* NewVehicle = GetWorld()->SpawnActorDeferred<AGazeboVehicleActor>(
        VehicleClass, FTransform(SpawnRotation, SpawnLocation), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (NewVehicle)
    {
        // Set vehicle identification BEFORE any initialization
        NewVehicle->VehicleNum = VehicleData.VehicleNum;
        NewVehicle->VehicleType = VehicleData.VehicleType;
        
        // Set vehicle name
        FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfoInternal(VehicleData.VehicleType);
        FString VehicleName = VehicleInfo ? VehicleInfo->VehicleName : TEXT("Unknown");
        NewVehicle->SetActorLabel(FString::Printf(TEXT("%s_%d"), *VehicleName, VehicleData.VehicleNum));
        
        // Finish construction and call BeginPlay
        UGameplayStatics::FinishSpawningActor(NewVehicle, FTransform(SpawnRotation, SpawnLocation));
    }

    return NewVehicle;
}

FString AGazeboVehicleManager::GetVehicleKey(const FGazeboPoseData& VehicleData) const
{
    return GetVehicleKey(VehicleData.VehicleNum, VehicleData.VehicleType);
}

FString AGazeboVehicleManager::GetVehicleKey(uint8 VehicleNum, uint8 VehicleType) const
{
    FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfoInternal(VehicleType);
    FString VehicleName = VehicleInfo ? VehicleInfo->VehicleName : TEXT("Unknown");
    return FString::Printf(TEXT("%s_%d"), *VehicleName, VehicleNum);
}

TSubclassOf<AGazeboVehicleActor> AGazeboVehicleManager::GetVehicleClassForType(uint8 VehicleType) const
{
    FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfoInternal(VehicleType);
    return VehicleInfo ? VehicleInfo->VehicleActorClass : nullptr;
}

FVector AGazeboVehicleManager::GetSpawnLocation(const FGazeboPoseData& VehicleData) const
{
    return VehicleData.Position;
}