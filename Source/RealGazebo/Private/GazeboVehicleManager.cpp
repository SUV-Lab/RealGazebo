// Fill out your copyright notice in the Description page of Project Settings.

#include "GazeboVehicleManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GazeboServoDataReceiver.h"

AGazeboVehicleManager::AGazeboVehicleManager()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f;

    // Create pose data receiver component
    PoseDataReceiver = CreateDefaultSubobject<UGazeboPoseDataReceiver>(TEXT("PoseDataReceiver"));
    
    // Create RPM data receiver component
    RPMDataReceiver = CreateDefaultSubobject<UGazeboRPMDataReceiver>(TEXT("RPMDataReceiver"));

    // Create Servo data receiver component
    ServoDataReceiver = CreateDefaultSubobject<UGazeboServoDataReceiver>(TEXT("ServoDataReceiver"));

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

    if (ServoDataReceiver)
    {
        ServoDataReceiver->OnVehicleServoReceived.AddDynamic(this, &AGazeboVehicleManager::OnVehicleServoDataReceived);
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

    if (ServoDataReceiver)
    {
        ServoDataReceiver->OnVehicleServoReceived.RemoveAll(this);
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

    AGazeboVehicleActor* NewVehicle = GetWorld()->SpawnActor<AGazeboVehicleActor>(
        VehicleClass, SpawnLocation, SpawnRotation, SpawnParams);

    if (NewVehicle)
    {
        NewVehicle->VehicleNum = VehicleData.VehicleNum;
        NewVehicle->VehicleType = VehicleData.VehicleType;
        
        // Set vehicle name
        FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfoInternal(VehicleData.VehicleType);
        FString VehicleName = VehicleInfo ? VehicleInfo->VehicleName : TEXT("Unknown");
        NewVehicle->SetActorLabel(FString::Printf(TEXT("%s_%d"), *VehicleName, VehicleData.VehicleNum));
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