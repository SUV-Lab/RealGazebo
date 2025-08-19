// Fill out your copyright notice in the Description page of Project Settings.

#include "GazeboVehicleActor.h"
#include "Engine/Engine.h"

AGazeboVehicleActor::AGazeboVehicleActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create vehicle mesh as root component
    VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    RootComponent = VehicleMesh;

    // Create Viewer First Person Camera - attached to VehicleMesh
    ViewerFirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewerFirstPersonCamera"));
    ViewerFirstPersonCamera->SetupAttachment(VehicleMesh);
    ViewerFirstPersonCamera->SetActive(false); // Start inactive

    // Create Viewer Third Person Spring Arm - attached to VehicleMesh
    ViewerThirdPersonSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ViewerThirdPersonSpringArm"));
    ViewerThirdPersonSpringArm->SetupAttachment(VehicleMesh);
    ViewerThirdPersonSpringArm->bUsePawnControlRotation = false;
    ViewerThirdPersonSpringArm->bInheritPitch = false;
    ViewerThirdPersonSpringArm->bInheritYaw = false;
    ViewerThirdPersonSpringArm->bInheritRoll = false;
    ViewerThirdPersonSpringArm->bDoCollisionTest = false; // Disable collision for smooth camera

    // Create Viewer Third Person Camera - attached to spring arm
    ViewerThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewerThirdPersonCamera"));
    ViewerThirdPersonCamera->SetupAttachment(ViewerThirdPersonSpringArm, USpringArmComponent::SocketName);
    ViewerThirdPersonCamera->SetActive(false); // Start inactive

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
    
    UE_LOG(LogTemp, Warning, TEXT("GazeboVehicleActor: %s (Type: %d) spawned with viewer cameras"), 
           *GetActorLabel(), VehicleType);
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
    int32 NumServosToUpdate = FMath::Min3(ControllableComponents.Num(), TargetServoPositions.Num(), TargetServoQuaternions.Num());

    for (int32 i = 0; i < NumServosToUpdate; ++i)
    {
        if (!IsValid(ControllableComponents[i]))
        {
            continue;
        }

        USceneComponent* ServoComponent = ControllableComponents[i];
        const FVector& ServoTargetLocation = TargetServoPositions[i];
        const FQuat& ServoTargetQuaternion = TargetServoQuaternions[i];

        // Interpolate Position
        FVector NewLocation = FMath::VInterpTo(ServoComponent->GetRelativeLocation(), ServoTargetLocation, DeltaTime, InterpolationSpeed);
        ServoComponent->SetRelativeLocation(NewLocation);

        // Interpolate Rotation using quaternion SLERP (avoids gimbal lock)
        FQuat CurrentQuat = ServoComponent->GetRelativeRotation().Quaternion();
        FQuat NewQuat = FQuat::Slerp(CurrentQuat, ServoTargetQuaternion, FMath::Clamp(DeltaTime * InterpolationSpeed, 0.0f, 1.0f));
        ServoComponent->SetRelativeRotation(NewQuat.Rotator());

        // Check if this servo is close enough to its target
        float LocationDistance = FVector::Dist(NewLocation, ServoTargetLocation);
        float QuaternionDot = FMath::Abs(NewQuat | ServoTargetQuaternion); // Use dot product operator
        float RotationDiff = FMath::Acos(FMath::Clamp(QuaternionDot, 0.0f, 1.0f)) * 2.0f; // Angular difference in radians

        if (LocationDistance > 2.0f || RotationDiff > FMath::DegreesToRadians(2.0f)) // 2cm position, 2 degree rotation tolerance
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
                ControllableComponents[i]->SetRelativeRotation(TargetServoQuaternions[i].Rotator());
            }
        }
    }
}

void AGazeboVehicleActor::UpdateVehicleMotorSpeed(const FGazeboMotorSpeedData& MotorSpeedData)
{
    // Update rotating components with motor speed data (already in deg/s)
    for (int32 i = 0; i < RotatingComponents.Num() && i < MotorSpeedData.MotorSpeeds_DegPerSec.Num(); i++)
    {
        if (RotatingComponents[i] && IsValid(RotatingComponents[i]))
        {
            float DegreesPerSecond = MotorSpeedData.MotorSpeeds_DegPerSec[i]; // Already converted to deg/s in UnifiedDataReceiver
            
            // Update rotation rate (ensure Z-axis rotation for propeller spin)
            FRotator NewRotationRate = FRotator(0.0f, DegreesPerSecond, 0.0f); // Z-axis spin → assign to Yaw
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
        // Convert FRotator to FQuat for gimbal-lock-free interpolation
        TargetServoQuaternions.Empty();
        TargetServoQuaternions.Reserve(ServoData.ServoRotations.Num());
        for (const FRotator& Rotation : ServoData.ServoRotations)
        {
            TargetServoQuaternions.Add(Rotation.Quaternion());
        }
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

void AGazeboVehicleActor::SetViewerFirstPersonCameraActive(bool bActive)
{
    if (ViewerFirstPersonCamera)
    {
        ViewerFirstPersonCamera->SetActive(bActive);
        UE_LOG(LogTemp, Log, TEXT("%s: Viewer first person camera %s"), 
               *GetActorLabel(), bActive ? TEXT("ACTIVATED") : TEXT("DEACTIVATED"));
    }
}

void AGazeboVehicleActor::SetViewerThirdPersonCameraActive(bool bActive)
{
    if (ViewerThirdPersonCamera)
    {
        ViewerThirdPersonCamera->SetActive(bActive);
        UE_LOG(LogTemp, Log, TEXT("%s: Viewer third person camera %s"), 
               *GetActorLabel(), bActive ? TEXT("ACTIVATED") : TEXT("DEACTIVATED"));
    }
}

void AGazeboVehicleActor::DeactivateAllViewerCameras()
{
    if (ViewerFirstPersonCamera)
    {
        ViewerFirstPersonCamera->SetActive(false);
    }
    if (ViewerThirdPersonCamera)
    {
        ViewerThirdPersonCamera->SetActive(false);
    }
    UE_LOG(LogTemp, Log, TEXT("%s: All viewer cameras deactivated"), *GetActorLabel());
}

bool AGazeboVehicleActor::IsViewerFirstPersonCameraActive() const
{
    return ViewerFirstPersonCamera ? ViewerFirstPersonCamera->IsActive() : false;
}

bool AGazeboVehicleActor::IsViewerThirdPersonCameraActive() const
{
    return ViewerThirdPersonCamera ? ViewerThirdPersonCamera->IsActive() : false;
}

float AGazeboVehicleActor::ConvertRadiansPerSecToDegPerSec(float RadiansPerSec) const
{
    // Gazebo → Unreal Conversion: deg/s = rad/s × (180/π)
    return RadiansPerSec * (180.0f / PI);
}