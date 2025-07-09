// Fill out your copyright notice in the Description page of Project Settings.

#include "GazeboVehicleManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

AGazeboVehicleManager::AGazeboVehicleManager()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f;

    // Create pose data receiver component
    PoseDataReceiver = CreateDefaultSubobject<UGazeboPoseDataReceiver>(TEXT("PoseDataReceiver"));
    
    // Create RPM data receiver component
    RPMDataReceiver = CreateDefaultSubobject<UGazeboRPMDataReceiver>(TEXT("RPMDataReceiver"));

    TotalVehiclesSpawned = 0;
}

void AGazeboVehicleManager::BeginPlay()
{
    Super::BeginPlay();

    if (PoseDataReceiver)
    {
        PoseDataReceiver->OnVehiclePoseReceived.AddDynamic(this, &AGazeboVehicleManager::OnVehiclePoseDataReceived);
    }

    if (RPMDataReceiver)
    {
        RPMDataReceiver->OnVehicleRPMReceived.AddDynamic(this, &AGazeboVehicleManager::OnVehicleRPMDataReceived);
    }

    UE_LOG(LogTemp, Warning, TEXT("GazeboVehicleManager: Started - Auto spawn: %s"), 
           bAutoSpawnVehicles ? TEXT("ON") : TEXT("OFF"));
}

void AGazeboVehicleManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (PoseDataReceiver)
    {
        PoseDataReceiver->OnVehiclePoseReceived.RemoveAll(this);
    }

    if (RPMDataReceiver)
    {
        RPMDataReceiver->OnVehicleRPMReceived.RemoveAll(this);
    }

    ClearAllVehicles();
    Super::EndPlay(EndPlayReason);
}

void AGazeboVehicleManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Periodic status update
    if (GEngine && PoseDataReceiver && PoseDataReceiver->bLogParsedData)
    {
        GEngine->AddOnScreenDebugMessage(1, 1.1f, FColor::Green,
            FString::Printf(TEXT("Gazebo Manager: %d vehicles | %d pose packets"),
                          GetActiveVehicleCount(), PoseDataReceiver->ValidPosePacketsReceived));
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

AGazeboVehicleActor* AGazeboVehicleManager::FindVehicle(uint8 VehicleNum, EGazeboVehicleType VehicleType) const
{
    FString VehicleKey = GetVehicleKey(VehicleNum, VehicleType);
    return SpawnedVehicles.FindRef(VehicleKey);
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
            
            UE_LOG(LogTemp, Warning, TEXT("GazeboVehicleManager: Spawned %s (Total: %d)"), 
                   *VehicleKey, TotalVehiclesSpawned);
        }
    }
    
    // Update vehicle pose
    if (Vehicle && IsValid(Vehicle))
    {
        Vehicle->UpdateVehiclePose(VehicleData);
    }
}

void AGazeboVehicleManager::OnVehicleRPMDataReceived(const FGazeboRPMData& RPMData)
{
    FString VehicleKey = GetVehicleKey(RPMData.VehicleNum, RPMData.VehicleType);
    
    // Find existing vehicle
    AGazeboVehicleActor* Vehicle = SpawnedVehicles.FindRef(VehicleKey);
    
    // Update vehicle RPM if it exists
    if (Vehicle && IsValid(Vehicle))
    {
        Vehicle->UpdateVehicleRPM(RPMData);
    }
}

AGazeboVehicleActor* AGazeboVehicleManager::SpawnVehicle(const FGazeboPoseData& VehicleData)
{
    TSubclassOf<AGazeboVehicleActor> VehicleClass = GetVehicleClassForType(VehicleData.VehicleType);
    
    if (!VehicleClass)
    {
        UE_LOG(LogTemp, Error, TEXT("GazeboVehicleManager: No vehicle class set for type %d"), 
               (int32)VehicleData.VehicleType);
        return nullptr;
    }

    FVector SpawnLocation = GetSpawnLocation(VehicleData);
    FRotator SpawnRotation = VehicleData.Rotation;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AGazeboVehicleActor* NewVehicle = GetWorld()->SpawnActor<AGazeboVehicleActor>(
        VehicleClass, SpawnLocation, SpawnRotation, SpawnParams);

    if (NewVehicle)
    {
        NewVehicle->VehicleNum = VehicleData.VehicleNum;
        NewVehicle->VehicleType = VehicleData.VehicleType;
        
        // Set vehicle name
        FString VehicleName = GetVehicleKey(VehicleData);
        NewVehicle->SetActorLabel(VehicleName);
    }

    return NewVehicle;
}

FString AGazeboVehicleManager::GetVehicleKey(const FGazeboPoseData& VehicleData) const
{
    return GetVehicleKey(VehicleData.VehicleNum, VehicleData.VehicleType);
}

FString AGazeboVehicleManager::GetVehicleKey(uint8 VehicleNum, EGazeboVehicleType VehicleType) const
{
    FString TypeStr;
    switch (VehicleType)
    {
    case EGazeboVehicleType::Iris: TypeStr = TEXT("iris"); break;
    case EGazeboVehicleType::Rover: TypeStr = TEXT("rover"); break;
    case EGazeboVehicleType::Boat: TypeStr = TEXT("boat"); break;
    }
    return FString::Printf(TEXT("%s_%d"), *TypeStr, VehicleNum);
}

TSubclassOf<AGazeboVehicleActor> AGazeboVehicleManager::GetVehicleClassForType(EGazeboVehicleType VehicleType) const
{
    switch (VehicleType)
    {
    case EGazeboVehicleType::Iris: return IrisVehicleClass;
    case EGazeboVehicleType::Rover: return RoverVehicleClass;
    case EGazeboVehicleType::Boat: return BoatVehicleClass;
    }
    return nullptr;
}

FVector AGazeboVehicleManager::GetSpawnLocation(const FGazeboPoseData& VehicleData) const
{
    return VehicleData.Position + SpawnOffset;
}