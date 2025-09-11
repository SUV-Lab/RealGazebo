
#include "VehicleBasePawn.h"
#include "Engine/Engine.h"
#include "RealGazeboBridge.h"
#include "Core/GazeboBridgeSubsystem.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

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

    // Create camera components for RealGazeboUI integration
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(RootComponent);
    FirstPersonCamera->SetRelativeLocation(FVector(100.0f, 0.0f, 50.0f)); // Forward and up from center
    FirstPersonCamera->SetActive(false); // Disabled by default
    FirstPersonCamera->ComponentTags.Add(TEXT("FirstPerson")); // Tag for identification

    ThirdPersonSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonSpringArm"));
    ThirdPersonSpringArm->SetupAttachment(RootComponent);
    ThirdPersonSpringArm->TargetArmLength = 400.0f;
    ThirdPersonSpringArm->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f)); // Slight downward angle
    ThirdPersonSpringArm->bDoCollisionTest = true;
    ThirdPersonSpringArm->bUsePawnControlRotation = false; // Fixed relative to vehicle

    ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
    ThirdPersonCamera->SetupAttachment(ThirdPersonSpringArm, USpringArmComponent::SocketName);
    ThirdPersonCamera->SetActive(false); // Disabled by default
    ThirdPersonCamera->ComponentTags.Add(TEXT("ThirdPerson")); // Tag for identification

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

void AVehicleBasePawn::BeginPlay()
{
    Super::BeginPlay();
    
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
    if (bSmoothMovement)
    {
        TargetServoPositions = ServoPositions;
        TargetServoRotations = ServoRotations;
        bHasServoTargets = true;
    }
    else
    {
        ApplyServoStates(ServoPositions, ServoRotations);
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
            // Convert from degrees per second to rotation rate
            const float RotationRateDegPerSec = MotorSpeeds[i];
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

void AVehicleBasePawn::ConfigureCameraSettings(float FieldOfView, float SpringArmLength)
{
    // Configure first person camera
    if (FirstPersonCamera)
    {
        FirstPersonCamera->SetFieldOfView(FieldOfView);
    }

    // Configure third person camera and spring arm
    if (ThirdPersonCamera)
    {
        ThirdPersonCamera->SetFieldOfView(FieldOfView);
    }

    if (ThirdPersonSpringArm)
    {
        ThirdPersonSpringArm->TargetArmLength = SpringArmLength;
    }

    UE_LOG(LogRealGazeboBridge, Verbose, TEXT("Camera settings updated: FOV=%.1f, SpringArmLength=%.1f"), FieldOfView, SpringArmLength);
}

void AVehicleBasePawn::SetVehicleDisplayName(const FVehicleID& InVehicleID, uint8 InVehicleType)
{
    // Get vehicle name from DataTable via subsystem
    if (const UGazeboBridgeSubsystem* BridgeSubsystem = UGazeboBridgeSubsystem::GetBridgeSubsystem(this))
    {
        if (const FBridgeVehicleConfigRow* Config = BridgeSubsystem->GetVehicleConfigInternal(InVehicleType))
        {
            // Create display name: vehiclename_vehicleID (e.g., "iris_1", "rover_3")
            FString VehicleNameLower = Config->VehicleName.ToLower();
            FString DisplayName = FString::Printf(TEXT("%s_%d"), *VehicleNameLower, InVehicleID.VehicleNum);
            
#if WITH_EDITOR
            // Set actor label (shows in outliner)
            SetActorLabel(DisplayName);
#endif
            
            UE_LOG(LogRealGazeboBridge, Verbose, TEXT("Vehicle display name set to: %s"), *DisplayName);
        }
        else
        {
            // Fallback if no config found
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
        FString FallbackName = FString::Printf(TEXT("vehicle_%d_%d"), InVehicleType, InVehicleID.VehicleNum);
#if WITH_EDITOR
        SetActorLabel(FallbackName);
#endif
        UE_LOG(LogRealGazeboBridge, Warning, TEXT("Bridge subsystem not available, using fallback name: %s"), *FallbackName);
    }
}