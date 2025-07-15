// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/DataTable.h"
#include "GazeboVehicleData.generated.h"

USTRUCT(BlueprintType)
struct FGazeboPoseData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 VehicleNum;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 VehicleType;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 MessageID;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Pose Data")
    FVector Position;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Pose Data")
    FRotator Rotation;

    FGazeboPoseData()
    {
        VehicleNum = 0;
        VehicleType = 0;
        MessageID = 1; // Pose data message ID
        Position = FVector::ZeroVector;
        Rotation = FRotator::ZeroRotator;
    }
};

USTRUCT(BlueprintType)
struct FGazeboRPMData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 VehicleNum;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 VehicleType;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 MessageID;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|RPM Data")
    TArray<float> MotorRPMs;

    FGazeboRPMData()
    {
        VehicleNum = 0;
        VehicleType = 0;
        MessageID = 2; // RPM data message ID
        MotorRPMs.Empty();
    }
};

USTRUCT(BlueprintType, meta = (DataTable = "true"))
struct REALGAZEBO_API FGazeboVehicleTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Info", meta = (DisplayName = "Vehicle Name"))
    FString VehicleName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Info", meta = (DisplayName = "Vehicle Type Code"))
    uint8 VehicleTypeCode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Info", meta = (DisplayName = "Motor Count"))
    int32 MotorCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Info", meta = (DisplayName = "Vehicle Actor Class"))
    TSubclassOf<class AGazeboVehicleActor> VehicleActorClass;

    FGazeboVehicleTableRow()
    {
        VehicleName = TEXT("Unknown");
        VehicleTypeCode = 0;
        MotorCount = 0;
        VehicleActorClass = nullptr;
    }

    int32 GetRPMPacketSize() const
    {
        return 3 + (MotorCount * 4); // 3 header bytes + 4 bytes per motor
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGazeboVehicleDataReceived, const FGazeboPoseData&, VehicleData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGazeboRPMDataReceived, const FGazeboRPMData&, RPMData);