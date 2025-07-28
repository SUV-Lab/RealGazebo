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
struct FGazeboMotorSpeedData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 VehicleNum;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 VehicleType;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 MessageID;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Motor Speed Data")
    TArray<float> MotorSpeeds_DegPerSec;

    FGazeboMotorSpeedData()
    {
        VehicleNum = 0;
        VehicleType = 0;
        MessageID = 2; // Motor speed data message ID
        MotorSpeeds_DegPerSec.Empty();
    }
};

USTRUCT(BlueprintType)
struct FGazeboServoData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 VehicleNum;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 VehicleType;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 MessageID;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Servo Data")
    TArray<FVector> ServoPositions;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Servo Data")
    TArray<FRotator> ServoRotations;

    FGazeboServoData()
    {
        VehicleNum = 0;
        VehicleType = 0;
        MessageID = 3; // Servo data message ID
        ServoPositions.Empty();
        ServoRotations.Empty();
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Info", meta = (DisplayName = "Servo Count"))
    int32 ServoCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Info", meta = (DisplayName = "Vehicle Actor Class"))
    TSubclassOf<class AGazeboVehicleActor> VehicleActorClass;

    FGazeboVehicleTableRow()
    {
        VehicleName = TEXT("Unknown");
        VehicleTypeCode = 0;
        MotorCount = 0;
        ServoCount = 0;
        VehicleActorClass = nullptr;
    }

    int32 GetMotorSpeedPacketSize() const
    {
        return 3 + (MotorCount * 4); // 3 header bytes + 4 bytes per motor
    }
    
    int32 GetServoPacketSize() const
    {
        // 3 header bytes + 24 bytes per servo (6 floats for XYZ + RPY * 4 bytes)
        return 3 + (ServoCount * 28);
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGazeboVehicleDataReceived, const FGazeboPoseData&, VehicleData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGazeboMotorSpeedDataReceived, const FGazeboMotorSpeedData&, MotorSpeedData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGazeboServoDataReceived, const FGazeboServoData&, ServoData);
