// Fill out your copyright notice in the Description page of Project Settings.

#include "GazeboVehicleActor.h"
#include "Engine/Engine.h"

AGazeboVehicleActor::AGazeboVehicleActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create root component
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootSceneComponent;

    // Create mesh component
    VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    VehicleMesh->SetupAttachment(RootComponent);

    // Initialize variables
    VehicleNum = 0;
    VehicleType = 0;
    bHasTarget = false;
    LastUpdateTime = 0.0f;
    TargetPosition = FVector::ZeroVector;
    TargetRotation = FRotator::ZeroRotator;
}

void AGazeboVehicleActor::BeginPlay()
{
    Super::BeginPlay();
    
    SetupVehicleMesh();
    
    UE_LOG(LogTemp, Warning, TEXT("GazeboVehicleActor: Vehicle_%d (Type: %d) spawned"), 
           VehicleNum, VehicleType);
}

void AGazeboVehicleActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bSmoothMovement && bHasTarget)
    {
        SmoothMoveToTarget(DeltaTime);
    }
}

void AGazeboVehicleActor::UpdateVehiclePose(const FGazeboPoseData& PoseData)
{
    LastUpdateTime = GetWorld()->GetTimeSeconds();

    if (bSmoothMovement)
    {
        TargetPosition = PoseData.Position;
        TargetRotation = PoseData.Rotation;
        bHasTarget = true;
    }
    else
    {
        SetActorLocation(PoseData.Position);
        SetActorRotation(PoseData.Rotation);
    }
}

void AGazeboVehicleActor::SetupVehicleMesh()
{
    // Override in child classes to set specific meshes
    if (VehicleMesh)
    {
        VehicleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void AGazeboVehicleActor::SmoothMoveToTarget(float DeltaTime)
{
    FVector CurrentLocation = GetActorLocation();
    FRotator CurrentRotation = GetActorRotation();

    // Calculate interpolation speed based on distance
    float DistanceToTarget = FVector::Dist(CurrentLocation, TargetPosition);
    float DynamicSpeed = InterpolationSpeed;
    
    // Adjust speed based on distance for more natural movement
    if (DistanceToTarget > 1000.0f) // Greater than 10 meters
    {
        DynamicSpeed = InterpolationSpeed * 2.0f;
    }
    else if (DistanceToTarget < 100.0f) // Less than 1 meter
    {
        DynamicSpeed = InterpolationSpeed * 1.0f;
    }

    // Interpolate position
    FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetPosition, DeltaTime, DynamicSpeed);
    SetActorLocation(NewLocation);

    // Interpolate rotation
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, DynamicSpeed);
    SetActorRotation(NewRotation);

    // Check if we're close enough to target
    float FinalDistance = FVector::Dist(NewLocation, TargetPosition);
    float RotationDiff = FMath::Abs(FRotator::ClampAxis(NewRotation.Yaw - TargetRotation.Yaw));
    
    if (FinalDistance < 5.0f && RotationDiff < 2.0f) // 5cm position, 2 degree rotation tolerance
    {
        bHasTarget = false;
        // Snap to final position for precision
        SetActorLocation(TargetPosition);
        SetActorRotation(TargetRotation);
    }
}

void AGazeboVehicleActor::UpdateVehicleMotorSpeed(const FGazeboMotorSpeedData& MotorSpeedData)
{
    // Update rotating components with motor speed data
    for (int32 i = 0; i < RotatingComponents.Num() && i < MotorSpeedData.MotorSpeeds_DegPerSec.Num(); i++)
    {
        if (RotatingComponents[i] && IsValid(RotatingComponents[i]))
        {
            float DegreesPerSecond = MotorSpeedData.MotorSpeeds_DegPerSec[i];
            
            // Update rotation rate (ensure Z-axis rotation for propeller spin)
            FRotator NewRotationRate = FRotator(0.0f, DegreesPerSecond, 0.0f); // Z-axis spin → assign to Yaw
            RotatingComponents[i]->RotationRate = NewRotationRate;
        }
    }
}

float AGazeboVehicleActor::ConvertRadiansPerSecToDegPerSec(float RadiansPerSec) const
{
    // Gazebo → Unreal Conversion: deg/s = rad/s × (180/π)
    return RadiansPerSec * (180.0f / PI);
}