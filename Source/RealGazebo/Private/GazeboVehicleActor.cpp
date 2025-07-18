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
    bHasServoTarget = false;
    LastUpdateTime = 0.0f;
    LastServoUpdateTime = 0.0f;
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

    if (bSmoothMovement && bHasServoTarget)
    {
        SmoothMoveServosToTarget(DeltaTime);
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

void AGazeboVehicleActor::SmoothMoveServosToTarget(float DeltaTime)
{
    bool bAllServosAtTarget = true;
    int32 NumServosToUpdate = FMath::Min3(ControllableComponents.Num(), TargetServoPositions.Num(), TargetServoRotations.Num());

    for (int32 i = 0; i < NumServosToUpdate; ++i)
    {
        if (!IsValid(ControllableComponents[i]))
        {
            continue;
        }

        USceneComponent* ServoComponent = ControllableComponents[i];
        const FVector& ServoTargetLocation = TargetServoPositions[i];
        const FRotator& ServoTargetRotation = TargetServoRotations[i];

        // Interpolate Position
        FVector NewLocation = FMath::VInterpTo(ServoComponent->GetRelativeLocation(), ServoTargetLocation, DeltaTime, InterpolationSpeed);
        ServoComponent->SetRelativeLocation(NewLocation);

        // Interpolate Rotation
        FRotator NewRotation = FMath::RInterpTo(ServoComponent->GetRelativeRotation(), ServoTargetRotation, DeltaTime, InterpolationSpeed);
        ServoComponent->SetRelativeRotation(NewRotation);

        // Check if this servo is close enough to its target
        float LocationDistance = FVector::Dist(NewLocation, ServoTargetLocation);
        float RotationDiff = FMath::Abs(FRotator::ClampAxis(NewRotation.Yaw - ServoTargetRotation.Yaw)) +
                             FMath::Abs(FRotator::ClampAxis(NewRotation.Pitch - ServoTargetRotation.Pitch)) +
                             FMath::Abs(FRotator::ClampAxis(NewRotation.Roll - ServoTargetRotation.Roll));

        if (LocationDistance > 2.0f || RotationDiff > 2.0f) // 2cm position, 2 degree rotation tolerance
        {
            bAllServosAtTarget = false;
        }
    }

    if (bAllServosAtTarget)
    {
        bHasServoTarget = false;
        // Snap to final positions for precision
        for (int32 i = 0; i < NumServosToUpdate; ++i)
        {
            if (IsValid(ControllableComponents[i]))
            {
                ControllableComponents[i]->SetRelativeLocation(TargetServoPositions[i]);
                ControllableComponents[i]->SetRelativeRotation(TargetServoRotations[i]);
            }
        }
    }
}

void AGazeboVehicleActor::UpdateVehicleRPM(const FGazeboRPMData& RPMData)
{
    // Update rotating components with RPM data
    for (int32 i = 0; i < RotatingComponents.Num() && i < RPMData.MotorRPMs.Num(); i++)
    {
        if (RotatingComponents[i] && IsValid(RotatingComponents[i]))
        {
            float radianspersecond = RPMData.MotorRPMs[i]; //Gazebo: rad/s (angular velocity) , motor_joint_[i]->GetVelocity(0) returns angular velocity in rad/s (radians per second)
            float DegreesPerSecond = ConvertRadiansToDegrees(radianspersecond); //Unreal RotationRate: degrees/s for each axis , RotatingMovementComponent->RotationRate expects degrees/second
            
            // Update rotation rate (ensure Z-axis rotation)
            FRotator NewRotationRate = FRotator(0.0f, DegreesPerSecond, 0.0f); // Z-axis spin  →  assign to Yaw
            RotatingComponents[i]->RotationRate = NewRotationRate;
        }
    }
}

void AGazeboVehicleActor::UpdateVehicleServo(const FGazeboServoData& ServoData)
{
    LastServoUpdateTime = GetWorld()->GetTimeSeconds();

    if (bSmoothMovement)
    {
        TargetServoPositions = ServoData.ServoPositions;
        TargetServoRotations = ServoData.ServoRotations;
        bHasServoTarget = true;
    }
    else
    {
        int32 NumServosToUpdate = FMath::Min3(ControllableComponents.Num(), ServoData.ServoPositions.Num(), ServoData.ServoRotations.Num());
        for (int32 i = 0; i < NumServosToUpdate; ++i)
        {
            if (IsValid(ControllableComponents[i]))
            {
                ControllableComponents[i]->SetRelativeLocation(ServoData.ServoPositions[i]);
                ControllableComponents[i]->SetRelativeRotation(ServoData.ServoRotations[i]);
            }
        }
    }
}

float AGazeboVehicleActor::ConvertRadiansToDegrees(float Radians) const
{
    // Gazebo → Unreal Conversion: degrees/s = rad/s × (180/π)
    return Radians * (180.0f / PI);
}