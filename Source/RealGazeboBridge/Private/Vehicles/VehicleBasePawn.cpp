
// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Sub-author: MinKyu Kim
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#include "VehicleBasePawn.h"
#include "Engine/Engine.h"
#include "RealGazeboBridge.h"
#include "Core/GazeboBridgeSubsystem.h"

AVehicleBasePawn::AVehicleBasePawn()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.0f; // Tick every frame when active

    // Create root component
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootSceneComponent;

    // Create mesh component
    VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    VehicleMesh->SetupAttachment(RootComponent);


    // Initialize identification
    VehicleID = FVehicleID();
    VehicleType = 0;

    // Performance settings
    bSmoothMovement = true;
    InterpolationSpeed = 15.0f;

    // Initialize state
    TargetPosition = FVector::ZeroVector;
    TargetRotation = FQuat::Identity;
    bHasMovementTarget = false;
    bHasServoTargets = false;
    LastUpdateTime = 0.0f;
    bIsInPool = true; // Starts in pool state
}

void AVehicleBasePawn::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // editor preview shows the scaled vehicle too
    ApplyVisualScale();
}

void AVehicleBasePawn::BeginPlay()
{
    Super::BeginPlay();

    ApplyVisualScale();

    // Components attached to the root (cameras, spring arm) are not under VehicleMesh, so move them
    // to the matching spot on the scaled vehicle. Position only: a camera keeps its own FOV.
    if (HasVisualScale() && !bVisualScaleMovedRootChildren)
    {
        bVisualScaleMovedRootChildren = true;
        for (USceneComponent* Child : RootComponent->GetAttachChildren())
        {
            if (Child && Child != VehicleMesh)
            {
                Child->SetRelativeLocation(VisualScaleAnchor + VisualScale * (Child->GetRelativeLocation() - VisualScaleAnchor));
            }
        }
    }

    InitializeRotatingComponents();
    InitializeControllableComponents();
    
    // Call Blueprint setup
    SetupVehicleMesh();
    
    if (!bIsInPool)
    {
        UE_LOG(LogRealGazeboBridge, Verbose, TEXT("VehicleBasePawn: Vehicle_%s spawned and active"), *VehicleID.ToString());
    }
}

void AVehicleBasePawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsInPool)
    {
        return; // Don't process if in pool
    }

    // Perform smooth movement if enabled
    if (bSmoothMovement)
    {
        if (bHasMovementTarget)
        {
            PerformSmoothMovement(DeltaTime);
        }
        
        if (bHasServoTargets)
        {
            PerformSmoothServoMovement(DeltaTime);
        }
    }

}

void AVehicleBasePawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AVehicleBasePawn::UpdateVehiclePose(const FVector& Position, const FQuat& Rotation)
{
    LastUpdateTime = GetWorld()->GetTimeSeconds();

    if (bSmoothMovement)
    {
        TargetPosition = Position;
        TargetRotation = Rotation;
        bHasMovementTarget = true;
    }
    else
    {
        SetActorLocation(Position);
        SetActorRotation(Rotation);
    }
}

void AVehicleBasePawn::UpdateMotorSpeeds(const TArray<float>& MotorSpeeds)
{
    ApplyMotorSpeeds(MotorSpeeds);
    
    // Call Blueprint event
    OnMotorSpeedsChanged(MotorSpeeds);
}

void AVehicleBasePawn::UpdateServoStates(const TArray<FVector>& ServoPositions, const TArray<FQuat>& ServoRotations)
{
    TArray<FQuat> ScaledRotations;
    const bool bScaleSteering = bCompensateWheelKinematics && HasVisualScale();
    if (bScaleSteering)
    {
        ScaledRotations.Reserve(ServoRotations.Num());
        for (const FQuat& Rotation : ServoRotations)
        {
            ScaledRotations.Add(ScaleSteeringRotation(Rotation));
        }
    }
    const TArray<FQuat>& AppliedRotations = bScaleSteering ? ScaledRotations : ServoRotations;

    if (bSmoothMovement)
    {
        TargetServoPositions = ServoPositions;
        TargetServoRotations = AppliedRotations;
        bHasServoTargets = true;
    }
    else
    {
        ApplyServoStates(ServoPositions, AppliedRotations);
    }
    
    // Call Blueprint event
    OnServoStatesChanged(ServoPositions, ServoRotations);
}

void AVehicleBasePawn::UpdateVehicleData(const FVehicleRuntimeData& RuntimeData)
{
    // Update all data at once (most efficient)
    UpdateVehiclePose(RuntimeData.Position, RuntimeData.Rotation);
    UpdateMotorSpeeds(RuntimeData.MotorSpeeds);
    UpdateServoStates(RuntimeData.ServoPositions, RuntimeData.ServoRotations);
    
    // Call Blueprint event
    OnVehicleDataUpdated(RuntimeData);
}

void AVehicleBasePawn::InitializeForPool(const FVehicleID& InVehicleID, uint8 InVehicleType)
{
    VehicleID = InVehicleID;
    VehicleType = InVehicleType;
    bIsInPool = false;
    
    // Make visible and enable ticking
    SetActorHiddenInGame(false);
    SetActorTickEnabled(true);
    
    // Reset state
    TargetPosition = GetActorLocation();
    TargetRotation = GetActorRotation().Quaternion();
    bHasMovementTarget = false;
    bHasServoTargets = false;
    
    // Clear component arrays to defaults
    TargetServoPositions.Empty();
    TargetServoRotations.Empty();
    
    // Set actor display name using vehicle name from DataTable + vehicle ID
    SetVehicleDisplayName(InVehicleID, InVehicleType);

    // Call Blueprint event
    OnVehicleActivated(InVehicleID);

    UE_LOG(LogRealGazeboBridge, VeryVerbose, TEXT("Vehicle %s activated from pool"), *InVehicleID.ToString());
}

void AVehicleBasePawn::ResetForPool()
{
    bIsInPool = true;
    
    // Hide and disable ticking to save performance
    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
    
    // Move to hidden location
    SetActorLocation(FVector(0.0f, 0.0f, -100000.0f));
    SetActorRotation(FRotator::ZeroRotator);
    
    // Reset all state
    VehicleID = FVehicleID();
    VehicleType = 0;
    VehicleTypeName = TEXT("");
    TargetPosition = FVector::ZeroVector;
    TargetRotation = FQuat::Identity;
    bHasMovementTarget = false;
    bHasServoTargets = false;
    LastUpdateTime = 0.0f;
    
    // Clear arrays
    TargetServoPositions.Empty();
    TargetServoRotations.Empty();
    
    // Stop all rotating components
    for (URotatingMovementComponent* RotatingComp : RotatingComponents)
    {
        if (RotatingComp)
        {
            RotatingComp->RotationRate = FRotator::ZeroRotator;
        }
    }

    // Call Blueprint event
    OnVehicleDeactivated();

    UE_LOG(LogRealGazeboBridge, VeryVerbose, TEXT("Vehicle reset for pool"));
}

bool AVehicleBasePawn::IsActiveVehicle() const
{
    return !bIsInPool;
}

void AVehicleBasePawn::SetActiveVehicle(bool bActive)
{
    bIsInPool = !bActive;

    SetActorHiddenInGame(!bActive);
    SetActorTickEnabled(bActive);

    UE_LOG(LogRealGazeboBridge, Log,
        TEXT("Vehicle %s Active=%s"),
        *VehicleID.ToString(),
        bActive ? TEXT("True") : TEXT("False"));
}

void AVehicleBasePawn::InitializeVehicleManually(int32 InVehicleNum, uint8 InVehicleType, const FString& InVehicleTypeName)
{
    VehicleID.VehicleType = InVehicleType;
    VehicleID.VehicleNum = InVehicleNum;

    VehicleType = InVehicleType;
    VehicleTypeName = InVehicleTypeName;

    bIsInPool = false;

    SetActorHiddenInGame(false);
    SetActorTickEnabled(true);

    TargetPosition = GetActorLocation();
    TargetRotation = GetActorRotation().Quaternion();
    bHasMovementTarget = false;
    bHasServoTargets = false;

#if WITH_EDITOR
    const FString SafeTypeName = VehicleTypeName.IsEmpty() ? TEXT("vehicle") : VehicleTypeName.ToLower();
    SetActorLabel(FString::Printf(TEXT("%s_%d"), *SafeTypeName, InVehicleNum));
#endif

    UE_LOG(LogRealGazeboBridge, Display,
        TEXT("InitializeVehicleManually: VehicleID=%s, VehicleType=%d, VehicleTypeName=%s, Active=True"),
        *VehicleID.ToString(),
        VehicleType,
        *VehicleTypeName);
}

void AVehicleBasePawn::PerformSmoothMovement(float DeltaTime)
{
    const FVector CurrentLocation = GetActorLocation();
    const FQuat CurrentRotation = GetActorRotation().Quaternion();

    // Calculate interpolation speed based on distance for natural movement
    float DynamicSpeed = InterpolationSpeed;
    const float DistanceToTarget = FVector::Dist(CurrentLocation, TargetPosition);
    
    if (DistanceToTarget > 1000.0f) // Greater than 10 meters
    {
        DynamicSpeed = InterpolationSpeed * 2.0f;
    }
    else if (DistanceToTarget < 10.0f) // Less than 10 cm
    {
        DynamicSpeed = InterpolationSpeed * 0.5f;
    }

    // Interpolate position and rotation
    const FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetPosition, DeltaTime, DynamicSpeed);
    const FQuat NewRotation = FMath::QInterpTo(CurrentRotation, TargetRotation, DeltaTime, DynamicSpeed);

    SetActorLocation(NewLocation);
    SetActorRotation(NewRotation);

    // Check if we've reached the target
    const float LocationTolerance = 1.0f; // 1cm
    const float RotationTolerance = 0.01f; // Small angle tolerance
    
    if (FVector::Dist(NewLocation, TargetPosition) < LocationTolerance &&
        FQuat::Error(NewRotation, TargetRotation) < RotationTolerance)
    {
        bHasMovementTarget = false;
    }
}

void AVehicleBasePawn::PerformSmoothServoMovement(float DeltaTime)
{
    if (TargetServoPositions.Num() != ControllableComponents.Num() ||
        TargetServoRotations.Num() != ControllableComponents.Num())
    {
        bHasServoTargets = false;
        return;
    }

    bool bAllReachedTarget = true;
    
    for (int32 i = 0; i < ControllableComponents.Num(); i++)
    {
        USceneComponent* Component = ControllableComponents[i];
        if (!Component)
        {
            continue;
        }
        
        const FVector CurrentPos = Component->GetRelativeLocation();
        const FQuat CurrentRot = Component->GetRelativeRotation().Quaternion();
        
        const FVector NewPos = FMath::VInterpTo(CurrentPos, TargetServoPositions[i], DeltaTime, InterpolationSpeed);
        const FQuat NewRot = FMath::QInterpTo(CurrentRot, TargetServoRotations[i], DeltaTime, InterpolationSpeed);
        
        Component->SetRelativeLocationAndRotation(NewPos, NewRot);
        
        // Check if this component reached its target
        const float PosTolerance = 0.1f; // 1mm
        const float RotTolerance = 0.001f;
        
        if (FVector::Dist(NewPos, TargetServoPositions[i]) > PosTolerance ||
            FQuat::Error(NewRot, TargetServoRotations[i]) > RotTolerance)
        {
            bAllReachedTarget = false;
        }
    }
    
    if (bAllReachedTarget)
    {
        bHasServoTargets = false;
    }
}

void AVehicleBasePawn::InitializeRotatingComponents()
{
    // This can be overridden in Blueprint or child classes
    // to set up rotating components based on vehicle configuration
}

void AVehicleBasePawn::InitializeControllableComponents()
{
    // This can be overridden in Blueprint or child classes
    // to set up controllable components based on vehicle configuration
}

void AVehicleBasePawn::ApplyMotorSpeeds(const TArray<float>& MotorSpeeds)
{
    // Apply motor speeds to rotating components
    const int32 ComponentCount = FMath::Min(RotatingComponents.Num(), MotorSpeeds.Num());
    
    for (int32 i = 0; i < ComponentCount; i++)
    {
        if (URotatingMovementComponent* RotatingComp = RotatingComponents[i])
        {
            // Gazebo (right-handed, +Z rate = CCW from above) -> Unreal (left-handed,
            // +Yaw = CW from above): the y-flip frame mapping is a reflection, so
            // angular rates about the up axis must be negated, matching the
            // quaternion conversion (X, -Y, Z, -W) used for poses.
            float RotationRateDegPerSec = -MotorSpeeds[i];
            if (bCompensateWheelKinematics && HasVisualScale())
            {
                // a wheel S times larger covers the same ground distance in 1/S turns
                RotationRateDegPerSec /= VisualScale;
            }
            RotatingComp->RotationRate = FRotator(0.0f, RotationRateDegPerSec, 0.0f);
        }
    }
}

void AVehicleBasePawn::ApplyServoStates(const TArray<FVector>& Positions, const TArray<FQuat>& Rotations)
{
    const int32 PositionCount = FMath::Min(ControllableComponents.Num(), Positions.Num());
    const int32 RotationCount = FMath::Min(ControllableComponents.Num(), Rotations.Num());
    
    for (int32 i = 0; i < FMath::Max(PositionCount, RotationCount); i++)
    {
        if (USceneComponent* Component = ControllableComponents[i])
        {
            if (i < PositionCount)
            {
                Component->SetRelativeLocation(Positions[i]);
            }
            
            if (i < RotationCount)
            {
                Component->SetRelativeRotation(Rotations[i]);
            }
        }
    }
}



void AVehicleBasePawn::ApplyVisualScale()
{
    if (!VehicleMesh || !HasVisualScale())
    {
        return;
    }

    // Scale about VisualScaleAnchor: the anchor point keeps its unscaled actor-local position.
    // The offset is actor-local, so it follows the vehicle attitude (slopes included).
    VehicleMesh->SetRelativeScale3D(FVector(VisualScale));
    VehicleMesh->SetRelativeLocation((1.0f - VisualScale) * VisualScaleAnchor);
}

FQuat AVehicleBasePawn::ScaleSteeringRotation(const FQuat& StreamedRotation) const
{
    // The streamed wheel link rotation is yaw(steer) * roll about the axle (Y). The axle direction
    // does not depend on the roll, so the steering yaw is read from where the Y axis ends up.
    const FVector Axle = StreamedRotation.RotateVector(FVector::RightVector);
    const float SteerRad = FMath::Atan2(-Axle.X, Axle.Y);

    // Keep the Gazebo steering angle as is: the no-slip angle for the scaled wheelbase,
    // atan(S * tan(steer)), is exact but looks unnatural (30 deg -> 66 deg at S = 4).
    return FQuat(FVector::UpVector, SteerRad);
}

float AVehicleBasePawn::ConvertRadiansPerSecToDegPerSec(float RadiansPerSec) const
{
    return FMath::RadiansToDegrees(RadiansPerSec);
}

bool AVehicleBasePawn::ValidateComponentConfiguration() const
{
    // This can be overridden to validate that the vehicle has the correct
    // number of rotating and controllable components for its type
    return true;
}

FVehicleRuntimeData AVehicleBasePawn::GetCurrentRuntimeData() const
{
    FVehicleRuntimeData RuntimeData;
    
    RuntimeData.Position = GetActorLocation();
    RuntimeData.Rotation = GetActorRotation().Quaternion();
    RuntimeData.LastUpdateTime = LastUpdateTime;
    RuntimeData.VehicleType = VehicleType;
    
    // Note: Motor speeds and servo data would need to be tracked separately
    // as they're not directly stored in the pawn
    
    return RuntimeData;
}

void AVehicleBasePawn::PrintVehicleStatus() const
{
    UE_LOG(LogRealGazeboBridge, Display, TEXT("=== Vehicle Status: %s ==="), *VehicleID.ToString());
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Type: %d, Active: %s"), 
           VehicleType, bIsInPool ? TEXT("No") : TEXT("Yes"));
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Position: %s"), 
           *GetActorLocation().ToString());
    UE_LOG(LogRealGazeboBridge, Display, TEXT("Rotating Components: %d, Controllable Components: %d"), 
           RotatingComponents.Num(), ControllableComponents.Num());
}


void AVehicleBasePawn::SetVehicleDisplayName(const FVehicleID& InVehicleID, uint8 InVehicleType)
{
    // Get vehicle name from DataTable via subsystem
    if (const UGazeboBridgeSubsystem* BridgeSubsystem = UGazeboBridgeSubsystem::GetBridgeSubsystem(this))
    {
        if (const FBridgeVehicleConfigRow* Config = BridgeSubsystem->GetVehicleConfigInternal(InVehicleType))
        {
            // Store vehicle type name for RTSP URL generation
            VehicleTypeName = Config->VehicleName;

            // Create display name: vehiclename_vehicleID (e.g., "iris_1", "rover_3")
            FString VehicleNameLower = Config->VehicleName.ToLower();
            FString DisplayName = FString::Printf(TEXT("%s_%d"), *VehicleNameLower, InVehicleID.VehicleNum);

#if WITH_EDITOR
            // Set actor label (shows in outliner)
            SetActorLabel(DisplayName);
#endif

            UE_LOG(LogRealGazeboBridge, Verbose, TEXT("Vehicle display name set to: %s (VehicleTypeName: %s)"), *DisplayName, *VehicleTypeName);
        }
        else
        {
            // Fallback if no config found
            VehicleTypeName = TEXT("");
            FString FallbackName = FString::Printf(TEXT("vehicle_%d_%d"), InVehicleType, InVehicleID.VehicleNum);
#if WITH_EDITOR
            SetActorLabel(FallbackName);
#endif
            UE_LOG(LogRealGazeboBridge, Warning, TEXT("No vehicle config found for type %d, using fallback name: %s"), InVehicleType, *FallbackName);
        }
    }
    else
    {
        // Fallback if subsystem not available
        VehicleTypeName = TEXT("");
        FString FallbackName = FString::Printf(TEXT("vehicle_%d_%d"), InVehicleType, InVehicleID.VehicleNum);
#if WITH_EDITOR
        SetActorLabel(FallbackName);
#endif
        UE_LOG(LogRealGazeboBridge, Warning, TEXT("Bridge subsystem not available, using fallback name: %s"), *FallbackName);
    }
}