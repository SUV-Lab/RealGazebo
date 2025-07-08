// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "GazeboVehicleData.generated.h"

UENUM(BlueprintType)
enum class EGazeboVehicleType : uint8
{
    Iris = 0,
    Rover = 1,
    Boat = 2
};

USTRUCT(BlueprintType)
struct FGazeboPoseData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 VehicleNum;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    EGazeboVehicleType VehicleType;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 MessageID;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Pose Data")
    FVector Position;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Pose Data")
    FRotator Rotation;

    FGazeboPoseData()
    {
        VehicleNum = 0;
        VehicleType = EGazeboVehicleType::Iris;
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
    EGazeboVehicleType VehicleType;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Header")
    uint8 MessageID;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|RPM Data")
    TArray<float> MotorRPMs;

    FGazeboRPMData()
    {
        VehicleNum = 0;
        VehicleType = EGazeboVehicleType::Iris;
        MessageID = 2; // RPM data message ID
        MotorRPMs.Empty();
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGazeboVehicleDataReceived, const FGazeboPoseData&, VehicleData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGazeboRPMDataReceived, const FGazeboRPMData&, RPMData);