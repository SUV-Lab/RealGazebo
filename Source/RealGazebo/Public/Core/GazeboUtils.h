// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Vehicle/GazeboVehicleData.h"

/**
 * Utility functions for RealGazebo plugin
 */
class REALGAZEBO_API GazeboUtils
{
public:
    // Coordinate system conversions
    static FVector ConvertGazeboToUnreal(const FVector& GazeboPosition);
    static FVector ConvertUnrealToGazebo(const FVector& UnrealPosition);
    
    static FRotator ConvertGazeboToUnreal(const FRotator& GazeboRotation);
    static FRotator ConvertUnrealToGazebo(const FRotator& UnrealRotation);
    
    // Data validation
    static bool IsValidVehicleData(const FGazeboPoseData& PoseData);
    static bool IsValidMotorSpeedData(const FGazeboMotorSpeedData& MotorData);
    static bool IsValidServoData(const FGazeboServoData& ServoData);
    
    // String utilities
    static FString GetVehicleKey(uint8 VehicleNum, uint8 VehicleType);
    static bool ParseVehicleKey(const FString& Key, uint8& OutVehicleNum, uint8& OutVehicleType);
    
    // Packet size calculations
    static int32 GetExpectedPosePacketSize();
    static int32 GetExpectedMotorSpeedPacketSize(int32 MotorCount);
    static int32 GetExpectedServoPacketSize(int32 ServoCount);
    
    // Logging helpers
    static void LogVehiclePoseData(const FGazeboPoseData& PoseData);
    static void LogMotorSpeedData(const FGazeboMotorSpeedData& MotorData);
    static void LogServoData(const FGazeboServoData& ServoData);
    
    // Math utilities
    static float ClampAngle(float AngleDegrees);
    static FVector ClampVector(const FVector& Vector, float MaxMagnitude);
    
private:
    GazeboUtils() = delete; // Static class, prevent instantiation
};