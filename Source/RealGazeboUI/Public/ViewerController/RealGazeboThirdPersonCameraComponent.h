// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "RealGazeboThirdPersonCameraComponent.generated.h"

/** How the chase camera picks its direction. Vehicle roll and pitch are never inherited. */
UENUM(BlueprintType)
enum class ERealGazeboChaseHeadingMode : uint8
{
    /** Stays behind the vehicle, following its heading (yaw) only */
    FollowHeading UMETA(DisplayName = "Follow Heading"),

    /** Follows the vehicle position, camera direction fixed in the world (inertial frame) */
    WorldFixed UMETA(DisplayName = "World Fixed")
};

/**
 * Third Person Camera Component for RealGazebo vehicles
 *
 * Visual spring arm component with integrated camera for chase camera functionality.
 * This IS a spring arm component with built-in camera - appears as single component in Blueprint.
 * Users can see and position this in Blueprint editor as one integrated component.
 *
 * Features:
 * - Visual spring arm component with integrated camera in Blueprint viewport
 * - Built-in chase camera functionality with all spring arm features
 * - User-configurable camera and spring arm settings
 * - Tag-based discovery by camera controller
 * - Blueprint events for activation/deactivation
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Camera), meta=(BlueprintSpawnableComponent, DisplayName="RealGazebo Third Person Camera"))
class REALGAZEBOUI_API URealGazeboThirdPersonCameraComponent : public USpringArmComponent
{
    GENERATED_BODY()

public:
    URealGazeboThirdPersonCameraComponent();

    //----------------------------------------------------------
    // Component Lifecycle
    //----------------------------------------------------------

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    //----------------------------------------------------------
    // Stabilized chase view
    //----------------------------------------------------------

    /** Current heading mode */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Third Person Camera|Chase")
    ERealGazeboChaseHeadingMode GetHeadingMode() const { return HeadingMode; }

    /**
     * Set the heading mode. Entering WorldFixed keeps the current camera direction,
     * so the view does not jump. Orbit offsets are reset.
     */
    UFUNCTION(BlueprintCallable, Category = "Third Person Camera|Chase")
    void SetHeadingMode(ERealGazeboChaseHeadingMode NewMode);

    /** WorldFixed only: orbit around the vehicle (degrees) and zoom (multiplier on arm length) */
    UFUNCTION(BlueprintCallable, Category = "Third Person Camera|Chase")
    void AddWorldFixedOrbit(float DeltaYaw, float DeltaPitch, float ArmLengthMultiplier = 1.0f);

    /** Copy heading mode and WorldFixed direction from another chase camera (used when switching vehicles) */
    void CopyChaseStateFrom(const URealGazeboThirdPersonCameraComponent& Other);

    /** Lowest / highest camera pitch (degrees, negative looks down) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person Camera|Chase")
    float MinChasePitch = -85.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person Camera|Chase")
    float MaxChasePitch = -5.0f;

    /** Arm length zoom range as a multiple of the configured arm length */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person Camera|Chase")
    float MinArmLengthScale = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person Camera|Chase")
    float MaxArmLengthScale = 5.0f;

    //----------------------------------------------------------
    // Camera Settings (User Configurable)
    //----------------------------------------------------------

    /** Field of view for the camera */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (ClampMin = "5.0", ClampMax = "170.0"))
    float CameraFieldOfView = 90.0f;

    //----------------------------------------------------------
    // Spring Arm Settings (User Configurable)
    //----------------------------------------------------------
    // Note: Spring arm settings are inherited from USpringArmComponent
    // Additional settings can be configured here

    /** Configure spring arm for different vehicle types */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Configuration")
    bool bAutoConfigureForVehicle = true;

    //----------------------------------------------------------
    // Camera Control
    //----------------------------------------------------------

    /** Activate this camera (called by camera controller) */
    UFUNCTION(BlueprintCallable, Category = "Third Person Camera")
    void ActivateCamera();

    /** Deactivate this camera (called by camera controller) */
    UFUNCTION(BlueprintCallable, Category = "Third Person Camera")
    void DeactivateCamera();

    /** Check if camera is currently active */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Third Person Camera")
    bool IsCameraActive() const { return bCameraActive; }

    /** Get the internal camera component */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Third Person Camera")
    UCameraComponent* GetCameraComponent() const { return CameraComponent; }

    //----------------------------------------------------------
    // Spring Arm Control
    //----------------------------------------------------------

    /** Configure spring arm settings for different vehicle types */
    UFUNCTION(BlueprintCallable, Category = "Third Person Camera")
    void ConfigureForVehicleType(float ArmLength = 400.0f, const FVector& ArmOffset = FVector(0.0f, 0.0f, 50.0f));

    //----------------------------------------------------------
    // Blueprint Events
    //----------------------------------------------------------

    /** Called when camera is activated */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnCameraActivated();

    /** Called when camera is deactivated */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnCameraDeactivated();

protected:
    //----------------------------------------------------------
    // Components
    //----------------------------------------------------------

    /** Internal camera component attached to spring arm */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCameraComponent> CameraComponent;

    //----------------------------------------------------------
    // Internal State
    //----------------------------------------------------------

    /** Whether this camera is currently active */
    UPROPERTY(BlueprintReadOnly, Category = "Runtime")
    bool bCameraActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Runtime")
    ERealGazeboChaseHeadingMode HeadingMode = ERealGazeboChaseHeadingMode::FollowHeading;

private:
    /** Blueprint-configured placement, captured on first activation (actor-local, unscaled by tilt) */
    bool bChaseBaseCaptured = false;
    FVector BasePivotActorLocal = FVector::ZeroVector;
    FRotator BaseArmRotation = FRotator::ZeroRotator;
    float BaseArmLength = 400.0f;
    FVector BaseRelativeLocation = FVector::ZeroVector;
    FRotator BaseRelativeRotation = FRotator::ZeroRotator;
    float BaseConfiguredArmLength = 400.0f;

    /** WorldFixed view state */
    float WorldFixedYaw = 0.0f;
    float OrbitPitchOffset = 0.0f;
    float ArmLengthScale = 1.0f;

    /** Last valid vehicle heading (kept when the vehicle points straight up/down) */
    float LastVehicleYaw = 0.0f;

    void CaptureChaseBase();
    float GetVehicleYaw();
    void UpdateChaseTransform();

    //----------------------------------------------------------
    // Internal Methods
    //----------------------------------------------------------

    /** Configure camera settings */
    void ConfigureCameraSettings();

    /** Configure spring arm settings */
    void ConfigureSpringArmSettings();
};