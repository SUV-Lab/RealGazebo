// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#include "ViewerController/RealGazeboThirdPersonCameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Components/StaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "RealGazeboUI.h"

URealGazeboThirdPersonCameraComponent::URealGazeboThirdPersonCameraComponent()
{
    // Ticks only while active: places the arm pivot/rotation (no vehicle roll/pitch), then runs spring arm lag
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;

    // Set component tag for discovery by camera controller
    ComponentTags.Add(FName("ThirdPersonCamera"));

    // Configure spring arm settings
    ConfigureSpringArmSettings();

    // Create internal camera component
    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    CameraComponent->SetupAttachment(this);
    CameraComponent->bUsePawnControlRotation = false;

    // Start inactive
    bCameraActive = false;

    UE_LOG(LogRealGazeboUI, Log, TEXT("ThirdPersonCameraComponent: Constructor completed"));
}

void URealGazeboThirdPersonCameraComponent::BeginPlay()
{
    Super::BeginPlay();

    // Configure camera and spring arm settings
    ConfigureCameraSettings();

    // Place the camera after the vehicle pose update of this frame
    if (AActor* Owner = GetOwner())
    {
        PrimaryComponentTick.AddPrerequisite(Owner, Owner->PrimaryActorTick);
    }

    UE_LOG(LogRealGazeboUI, Log, TEXT("ThirdPersonCameraComponent: Initialized on %s"), GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
}

void URealGazeboThirdPersonCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Deactivate if currently active
    if (bCameraActive)
    {
        DeactivateCamera();
    }

    Super::EndPlay(EndPlayReason);
}


void URealGazeboThirdPersonCameraComponent::ActivateCamera()
{
    if (!CameraComponent)
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("ThirdPersonCameraComponent: No internal camera component available"));
        return;
    }

    // Take over placement from the attachment: the vehicle's roll/pitch must not reach the camera
    CaptureChaseBase();
    SetUsingAbsoluteLocation(true);
    SetUsingAbsoluteRotation(true);
    UpdateChaseTransform();
    UpdateDesiredArmLocation(bDoCollisionTest, false, false, 0.0f); // snap, no lag from a stale position
    SetComponentTickEnabled(true);

    // Activate the internal camera component
    CameraComponent->SetActive(true);
    bCameraActive = true;

    // Set as view target using the internal camera
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
            {
                // Set view target to the pawn, but the active camera will be our internal camera component
                PC->SetViewTargetWithBlend(OwnerPawn, 0.5f);
            }
        }
    }

    // Fire Blueprint event
    OnCameraActivated();

    UE_LOG(LogRealGazeboUI, Log, TEXT("ThirdPersonCameraComponent: Camera activated on %s"), GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
}

void URealGazeboThirdPersonCameraComponent::DeactivateCamera()
{
    if (CameraComponent)
    {
        // Deactivate the internal camera component
        CameraComponent->SetActive(false);
    }

    bCameraActive = false;
    SetComponentTickEnabled(false);

    // Hand placement back to the attachment
    if (bChaseBaseCaptured && IsUsingAbsoluteLocation())
    {
        SetUsingAbsoluteLocation(false);
        SetUsingAbsoluteRotation(false);
        SetRelativeLocationAndRotation(BaseRelativeLocation, BaseRelativeRotation);
        TargetArmLength = BaseConfiguredArmLength;
    }

    // Fire Blueprint event
    OnCameraDeactivated();

    UE_LOG(LogRealGazeboUI, Log, TEXT("ThirdPersonCameraComponent: Camera deactivated on %s"), GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
}

void URealGazeboThirdPersonCameraComponent::ConfigureForVehicleType(float ArmLength, const FVector& ArmOffset)
{
    // Configure spring arm settings (inherited from USpringArmComponent)
    TargetArmLength = ArmLength;
    SetRelativeLocation(ArmOffset);

    UE_LOG(LogRealGazeboUI, Log, TEXT("ThirdPersonCameraComponent: Configured for vehicle type (Length: %.1f, Offset: %s)"),
           ArmLength, *ArmOffset.ToString());
}

void URealGazeboThirdPersonCameraComponent::ConfigureCameraSettings()
{
    if (CameraComponent)
    {
        // Configure internal camera settings
        CameraComponent->SetFieldOfView(CameraFieldOfView);

        UE_LOG(LogRealGazeboUI, Log, TEXT("ThirdPersonCameraComponent: Camera settings configured (FOV: %.1f)"), CameraFieldOfView);
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("ThirdPersonCameraComponent: No internal camera component to configure"));
    }
}

void URealGazeboThirdPersonCameraComponent::ConfigureSpringArmSettings()
{
    // Configure spring arm settings (inherited from USpringArmComponent)
    SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
    SetRelativeRotation(FRotator(-20.0f, 0.0f, 0.0f));
    TargetArmLength = 400.0f;
    bEnableCameraLag = true;
    bEnableCameraRotationLag = true;
    CameraLagSpeed = 10.0f;
    CameraRotationLagSpeed = 10.0f;
    bDoCollisionTest = false;  // Disable collision so camera can move freely through walls
    ProbeSize = 0.0f;         // No collision probe sphere

    // Spring arm inheritance settings for vehicle following
    bInheritPitch = true;
    bInheritYaw = true;
    bInheritRoll = false; // Usually don't want roll for chase cameras

    UE_LOG(LogRealGazeboUI, Log, TEXT("ThirdPersonCameraComponent: Spring arm settings configured"));
}
//----------------------------------------------------------
// Stabilized chase view
//----------------------------------------------------------

void URealGazeboThirdPersonCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    if (bCameraActive)
    {
        UpdateChaseTransform();
    }

    // Spring arm: camera lag / rotation lag towards the transform set above
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void URealGazeboThirdPersonCameraComponent::CaptureChaseBase()
{
    const AActor* Owner = GetOwner();
    if (bChaseBaseCaptured || !Owner)
    {
        return;
    }

    // Blueprint placement relative to the actor, independent of the vehicle tilt at this moment
    const FTransform ActorTransform = Owner->GetActorTransform();
    BasePivotActorLocal = ActorTransform.InverseTransformPosition(GetComponentLocation()) * ActorTransform.GetScale3D();
    BaseArmRotation = (ActorTransform.GetRotation().Inverse() * GetComponentQuat()).Rotator();
    BaseRelativeLocation = GetRelativeLocation();
    BaseRelativeRotation = GetRelativeRotation();
    BaseConfiguredArmLength = TargetArmLength;
    BaseArmLength = TargetArmLength;

    LastVehicleYaw = Owner->GetActorRotation().Yaw;
    bChaseBaseCaptured = true;
}

float URealGazeboThirdPersonCameraComponent::GetVehicleYaw()
{
    if (const AActor* Owner = GetOwner())
    {
        // Heading of the forward axis projected on the ground; roll and pitch drop out
        const FVector Forward = Owner->GetActorForwardVector();
        if (FVector2D(Forward.X, Forward.Y).SizeSquared() > KINDA_SMALL_NUMBER)
        {
            LastVehicleYaw = FMath::RadiansToDegrees(FMath::Atan2(Forward.Y, Forward.X));
        }
    }
    return LastVehicleYaw;
}

void URealGazeboThirdPersonCameraComponent::UpdateChaseTransform()
{
    const AActor* Owner = GetOwner();
    if (!Owner || !bChaseBaseCaptured)
    {
        return;
    }

    const float VehicleYaw = GetVehicleYaw();
    const bool bWorldFixed = HeadingMode == ERealGazeboChaseHeadingMode::WorldFixed;

    // Pivot: the configured point on the vehicle, rotated by heading only
    const FVector Pivot = Owner->GetActorLocation() + FRotator(0.0f, VehicleYaw, 0.0f).RotateVector(BasePivotActorLocal);

    const float Yaw = bWorldFixed ? WorldFixedYaw : VehicleYaw + BaseArmRotation.Yaw;
    const float Pitch = FMath::Clamp(BaseArmRotation.Pitch + (bWorldFixed ? OrbitPitchOffset : 0.0f), MinChasePitch, MaxChasePitch);
    TargetArmLength = BaseArmLength * (bWorldFixed ? ArmLengthScale : 1.0f);

    SetWorldLocationAndRotation(Pivot, FRotator(Pitch, Yaw, 0.0f));
}

void URealGazeboThirdPersonCameraComponent::SetHeadingMode(ERealGazeboChaseHeadingMode NewMode)
{
    if (HeadingMode == NewMode)
    {
        return;
    }

    if (NewMode == ERealGazeboChaseHeadingMode::WorldFixed)
    {
        // Freeze the direction currently on screen
        WorldFixedYaw = CameraComponent ? CameraComponent->GetComponentRotation().Yaw : GetComponentRotation().Yaw;
    }
    OrbitPitchOffset = 0.0f;
    ArmLengthScale = 1.0f;
    HeadingMode = NewMode;

    UE_LOG(LogRealGazeboUI, Log, TEXT("ThirdPersonCameraComponent: Heading mode %s on %s"),
           NewMode == ERealGazeboChaseHeadingMode::WorldFixed ? TEXT("WorldFixed") : TEXT("FollowHeading"),
           GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
}

void URealGazeboThirdPersonCameraComponent::AddWorldFixedOrbit(float DeltaYaw, float DeltaPitch, float ArmLengthMultiplier)
{
    if (HeadingMode != ERealGazeboChaseHeadingMode::WorldFixed)
    {
        return;
    }

    WorldFixedYaw = FRotator::NormalizeAxis(WorldFixedYaw + DeltaYaw);
    const float Pitch = FMath::Clamp(BaseArmRotation.Pitch + OrbitPitchOffset + DeltaPitch, MinChasePitch, MaxChasePitch);
    OrbitPitchOffset = Pitch - BaseArmRotation.Pitch;
    ArmLengthScale = FMath::Clamp(ArmLengthScale * ArmLengthMultiplier, MinArmLengthScale, MaxArmLengthScale);
}

void URealGazeboThirdPersonCameraComponent::CopyChaseStateFrom(const URealGazeboThirdPersonCameraComponent& Other)
{
    // Mode and world direction carry over; pitch/zoom are per vehicle type, so start from this camera's own setup
    HeadingMode = Other.HeadingMode;
    WorldFixedYaw = Other.WorldFixedYaw;
    OrbitPitchOffset = 0.0f;
    ArmLengthScale = 1.0f;
}
