// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/GazeboUtils.h"
#include "Core/GazeboConstants.h"
#include "Engine/Engine.h"

FVector GazeboUtils::ConvertGazeboToUnreal(const FVector& GazeboPosition)
{
    // Gazebo: X-forward, Y-left, Z-up
    // Unreal: X-forward, Y-right, Z-up
    // Scale from meters to centimeters and flip Y axis
    return FVector(
        GazeboPosition.X * GazeboConstants::GAZEBO_TO_UE_SCALE,
        -GazeboPosition.Y * GazeboConstants::GAZEBO_TO_UE_SCALE,
        GazeboPosition.Z * GazeboConstants::GAZEBO_TO_UE_SCALE
    );
}

FVector GazeboUtils::ConvertUnrealToGazebo(const FVector& UnrealPosition)
{
    // Reverse conversion: scale down and flip Y axis
    return FVector(
        UnrealPosition.X / GazeboConstants::GAZEBO_TO_UE_SCALE,
        -UnrealPosition.Y / GazeboConstants::GAZEBO_TO_UE_SCALE,
        UnrealPosition.Z / GazeboConstants::GAZEBO_TO_UE_SCALE
    );
}

FRotator GazeboUtils::ConvertGazeboToUnreal(const FRotator& GazeboRotation)
{
    // Convert Gazebo RPY to Unreal YPR (Yaw, Pitch, Roll)
    return FRotator(
        GazeboRotation.Pitch,  // Pitch
        -GazeboRotation.Yaw,   // Yaw (flip for coordinate system)
        -GazeboRotation.Roll   // Roll (flip for coordinate system)
    );
}

FRotator GazeboUtils::ConvertUnrealToGazebo(const FRotator& UnrealRotation)
{
    // Reverse conversion
    return FRotator(
        UnrealRotation.Pitch,  // Pitch
        -UnrealRotation.Yaw,   // Yaw
        -UnrealRotation.Roll   // Roll
    );
}

bool GazeboUtils::IsValidVehicleData(const FGazeboPoseData& PoseData)
{
    // Check for valid vehicle numbers and reasonable position values
    if (PoseData.VehicleNum >= GazeboConstants::MAX_VEHICLES)
    {
        return false;
    }
    
    if (PoseData.MessageID != GazeboConstants::MESSAGE_ID_POSE)
    {
        return false;
    }
    
    // Check for reasonable position values (within 10km range)
    const float MaxPosition = 1000000.0f; // 10km in cm
    if (FMath::Abs(PoseData.Position.X) > MaxPosition ||
        FMath::Abs(PoseData.Position.Y) > MaxPosition ||
        FMath::Abs(PoseData.Position.Z) > MaxPosition)
    {
        return false;
    }
    
    return true;
}

bool GazeboUtils::IsValidMotorSpeedData(const FGazeboMotorSpeedData& MotorData)
{
    if (MotorData.VehicleNum >= GazeboConstants::MAX_VEHICLES)
    {
        return false;
    }
    
    if (MotorData.MessageID != GazeboConstants::MESSAGE_ID_MOTOR_SPEED)
    {
        return false;
    }
    
    if (MotorData.MotorSpeeds_DegPerSec.Num() > GazeboConstants::MAX_MOTORS_PER_VEHICLE)
    {
        return false;
    }
    
    return true;
}

bool GazeboUtils::IsValidServoData(const FGazeboServoData& ServoData)
{
    if (ServoData.VehicleNum >= GazeboConstants::MAX_VEHICLES)
    {
        return false;
    }
    
    if (ServoData.MessageID != GazeboConstants::MESSAGE_ID_SERVO)
    {
        return false;
    }
    
    if (ServoData.ServoPositions.Num() > GazeboConstants::MAX_SERVOS_PER_VEHICLE ||
        ServoData.ServoRotations.Num() > GazeboConstants::MAX_SERVOS_PER_VEHICLE)
    {
        return false;
    }
    
    if (ServoData.ServoPositions.Num() != ServoData.ServoRotations.Num())
    {
        return false;
    }
    
    return true;
}

FString GazeboUtils::GetVehicleKey(uint8 VehicleNum, uint8 VehicleType)
{
    return FString::Printf(TEXT("V%d_T%d"), VehicleNum, VehicleType);
}

bool GazeboUtils::ParseVehicleKey(const FString& Key, uint8& OutVehicleNum, uint8& OutVehicleType)
{
    TArray<FString> Parts;
    if (Key.ParseIntoArray(Parts, TEXT("_")) == 2)
    {
        if (Parts[0].StartsWith(TEXT("V")) && Parts[1].StartsWith(TEXT("T")))
        {
            FString VehicleNumStr = Parts[0].RightChop(1);
            FString VehicleTypeStr = Parts[1].RightChop(1);
            
            if (VehicleNumStr.IsNumeric() && VehicleTypeStr.IsNumeric())
            {
                OutVehicleNum = FCString::Atoi(*VehicleNumStr);
                OutVehicleType = FCString::Atoi(*VehicleTypeStr);
                return true;
            }
        }
    }
    
    return false;
}

int32 GazeboUtils::GetExpectedPosePacketSize()
{
    return GazeboConstants::HEADER_SIZE + GazeboConstants::POSE_DATA_SIZE;
}

int32 GazeboUtils::GetExpectedMotorSpeedPacketSize(int32 MotorCount)
{
    return GazeboConstants::HEADER_SIZE + (MotorCount * GazeboConstants::MOTOR_SPEED_SIZE);
}

int32 GazeboUtils::GetExpectedServoPacketSize(int32 ServoCount)
{
    return GazeboConstants::HEADER_SIZE + (ServoCount * GazeboConstants::SERVO_DATA_SIZE);
}

void GazeboUtils::LogVehiclePoseData(const FGazeboPoseData& PoseData)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("[%s] Vehicle %d (Type %d): Pos(%.2f,%.2f,%.2f) Rot(%.2f,%.2f,%.2f)"),
        *GazeboConstants::LOG_CATEGORY_VEHICLE,
        PoseData.VehicleNum,
        PoseData.VehicleType,
        PoseData.Position.X, PoseData.Position.Y, PoseData.Position.Z,
        PoseData.Rotation.Pitch, PoseData.Rotation.Yaw, PoseData.Rotation.Roll
    );
}

void GazeboUtils::LogMotorSpeedData(const FGazeboMotorSpeedData& MotorData)
{
    FString SpeedsStr;
    for (int32 i = 0; i < MotorData.MotorSpeeds_DegPerSec.Num(); i++)
    {
        SpeedsStr += FString::Printf(TEXT("%.1f"), MotorData.MotorSpeeds_DegPerSec[i]);
        if (i < MotorData.MotorSpeeds_DegPerSec.Num() - 1)
        {
            SpeedsStr += TEXT(",");
        }
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("[%s] Vehicle %d (Type %d) Motor Speeds: [%s]"),
        *GazeboConstants::LOG_CATEGORY_VEHICLE,
        MotorData.VehicleNum,
        MotorData.VehicleType,
        *SpeedsStr
    );
}

void GazeboUtils::LogServoData(const FGazeboServoData& ServoData)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("[%s] Vehicle %d (Type %d) Servo Data: %d servos"),
        *GazeboConstants::LOG_CATEGORY_VEHICLE,
        ServoData.VehicleNum,
        ServoData.VehicleType,
        ServoData.ServoPositions.Num()
    );
}

float GazeboUtils::ClampAngle(float AngleDegrees)
{
    // Normalize angle to [-180, 180] range
    while (AngleDegrees > 180.0f)
    {
        AngleDegrees -= 360.0f;
    }
    while (AngleDegrees < -180.0f)
    {
        AngleDegrees += 360.0f;
    }
    return AngleDegrees;
}

FVector GazeboUtils::ClampVector(const FVector& Vector, float MaxMagnitude)
{
    if (Vector.Size() > MaxMagnitude)
    {
        return Vector.GetSafeNormal() * MaxMagnitude;
    }
    return Vector;
}