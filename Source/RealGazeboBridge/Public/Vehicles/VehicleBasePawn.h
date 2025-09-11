
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "GazeboBridgeTypes.h"
#include "VehicleBasePawn.generated.h"

/**
 * Lightweight base vehicle pawn for high-performance visualization
 * 
 * Key Features:
 * - Minimal memory footprint and fast updates
 * - Blueprint-extensible for custom vehicle types
 * - Object pooling compatible
 * - Optimized component management
 * 
 * Note: Most logic moved to subsystem for performance.
 *       This pawn is primarily for visual representation.
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOBRIDGE_API AVehicleBasePawn : public APawn
{
    GENERATED_BODY()

public:
    AVehicleBasePawn();

    //----------------------------------------------------------
    // Vehicle Identification
    //----------------------------------------------------------

    /** Vehicle identification data */
    UPROPERTY(BlueprintReadWrite, Category = "Bridge|Vehicle")
    FVehicleID VehicleID;

    /** Vehicle type code for configuration lookup */
    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Vehicle")
    uint8 VehicleType = 0;

    //----------------------------------------------------------
    // Core Components
    //----------------------------------------------------------

    /** Root scene component for transform */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bridge|Components")
    TObjectPtr<USceneComponent> RootSceneComponent;

    /** Main vehicle mesh */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bridge|Components")
    TObjectPtr<UStaticMeshComponent> VehicleMesh;

    /** Rotating components for motors/propellers/wheels */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Components")
    TArray<TObjectPtr<URotatingMovementComponent>> RotatingComponents;

    /** Controllable components for servos/fins/turrets */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Components")
    TArray<TObjectPtr<USceneComponent>> ControllableComponents;

    //----------------------------------------------------------
    // Camera Components (for RealGazeboUI integration)
    //----------------------------------------------------------

    /** First person camera component (cockpit view) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bridge|Camera Components")
    TObjectPtr<class UCameraComponent> FirstPersonCamera;

    /** Spring arm component for third person camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bridge|Camera Components")
    TObjectPtr<class USpringArmComponent> ThirdPersonSpringArm;

    /** Third person camera component (following view) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bridge|Camera Components")
    TObjectPtr<class UCameraComponent> ThirdPersonCamera;

    //----------------------------------------------------------
    // Vehicle State Updates (called by subsystem)
    //----------------------------------------------------------

    /** Update vehicle pose from runtime data */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Updates")
    void UpdateVehiclePose(const FVector& Position, const FQuat& Rotation);

    /** Update motor speeds */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Updates")
    void UpdateMotorSpeeds(const TArray<float>& MotorSpeeds);

    /** Update servo positions/rotations */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Updates") 
    void UpdateServoStates(const TArray<FVector>& ServoPositions, const TArray<FQuat>& ServoRotations);

    /** Batch update all vehicle data at once (most efficient) */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Updates")
    void UpdateVehicleData(const FVehicleRuntimeData& RuntimeData);

    //----------------------------------------------------------
    // Pool Management
    //----------------------------------------------------------

    /** Initialize for pool use with vehicle ID */
    void InitializeForPool(const FVehicleID& InVehicleID, uint8 InVehicleType);

    /** Reset to pool state (hide, stop movement, clear data) */
    void ResetForPool();

    /** Check if this pawn is currently active (not in pool) */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Pool")
    bool IsActiveVehicle() const;

    //----------------------------------------------------------
    // Performance Settings
    //----------------------------------------------------------

    /** Enable smooth movement interpolation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Performance")
    bool bSmoothMovement = true;

    /** Interpolation speed for smooth movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Performance", meta = (EditCondition = "bSmoothMovement"))
    float InterpolationSpeed = 15.0f;


    //----------------------------------------------------------
    // Blueprint Events (for customization)
    //----------------------------------------------------------

    /** Called when vehicle data is updated */
    UFUNCTION(BlueprintImplementableEvent, Category = "Bridge|Events")
    void OnVehicleDataUpdated(const FVehicleRuntimeData& VehicleData);

    /** Called when vehicle is spawned/acquired from pool */
    UFUNCTION(BlueprintImplementableEvent, Category = "Bridge|Events")
    void OnVehicleActivated(const FVehicleID& InVehicleID);

    /** Called when vehicle is returned to pool */
    UFUNCTION(BlueprintImplementableEvent, Category = "Bridge|Events")
    void OnVehicleDeactivated();

    /** Called when motor speeds change (for custom effects) */
    UFUNCTION(BlueprintImplementableEvent, Category = "Bridge|Events")
    void OnMotorSpeedsChanged(const TArray<float>& NewMotorSpeeds);

    /** Called when servo states change */
    UFUNCTION(BlueprintImplementableEvent, Category = "Bridge|Events")
    void OnServoStatesChanged(const TArray<FVector>& ServoPositions, const TArray<FQuat>& ServoRotations);

    //----------------------------------------------------------
    // Camera Functions (for RealGazeboUI integration)
    //----------------------------------------------------------

    /** Get the first person camera component */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Camera")
    UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

    /** Get the third person camera component */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Camera") 
    UCameraComponent* GetThirdPersonCamera() const { return ThirdPersonCamera; }

    /** Get the spring arm component */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Camera")
    USpringArmComponent* GetThirdPersonSpringArm() const { return ThirdPersonSpringArm; }

    /** Configure camera settings (called by UI system) */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Camera")
    void ConfigureCameraSettings(float FieldOfView = 90.0f, float SpringArmLength = 400.0f);

protected:
    //----------------------------------------------------------
    // Pawn Lifecycle
    //----------------------------------------------------------

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Setup vehicle-specific mesh and components (override in blueprints) */
    UFUNCTION(BlueprintImplementableEvent, Category = "Bridge|Setup")
    void SetupVehicleMesh();

    //----------------------------------------------------------
    // Movement and Interpolation
    //----------------------------------------------------------

    /** Target transform for smooth movement */
    FVector TargetPosition = FVector::ZeroVector;
    FQuat TargetRotation = FQuat::Identity;
    bool bHasMovementTarget = false;

    /** Target states for servo components */
    TArray<FVector> TargetServoPositions;
    TArray<FQuat> TargetServoRotations;
    bool bHasServoTargets = false;

    /** Smooth interpolation methods */
    void PerformSmoothMovement(float DeltaTime);
    void PerformSmoothServoMovement(float DeltaTime);

    //----------------------------------------------------------
    // Component Management
    //----------------------------------------------------------

    /** Initialize rotating components */
    void InitializeRotatingComponents();

    /** Initialize controllable servo components */
    void InitializeControllableComponents();

    /** Apply motor speeds to rotating components */
    void ApplyMotorSpeeds(const TArray<float>& MotorSpeeds);

    /** Apply servo states to controllable components */
    void ApplyServoStates(const TArray<FVector>& Positions, const TArray<FQuat>& Rotations);

    //----------------------------------------------------------
    // Performance Optimization
    //----------------------------------------------------------

    /** Current visibility state */

    /** Last update time for performance tracking */
    float LastUpdateTime = 0.0f;

    /** Pool state tracking */
    bool bIsInPool = true; // Starts in pool state




    //----------------------------------------------------------
    // Utility Methods
    //----------------------------------------------------------

    /** Convert motor speed from rad/s to deg/s */
    float ConvertRadiansPerSecToDegPerSec(float RadiansPerSec) const;

    /** Validate component arrays match configuration */
    bool ValidateComponentConfiguration() const;

public:
    //----------------------------------------------------------
    // Debug and Development
    //----------------------------------------------------------

    /** Get current vehicle runtime data for debugging */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Debug")
    FVehicleRuntimeData GetCurrentRuntimeData() const;

    /** Print vehicle status to log */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Debug")
    void PrintVehicleStatus() const;

private:
    /** Set vehicle display name based on DataTable name + vehicle ID */
    void SetVehicleDisplayName(const FVehicleID& InVehicleID, uint8 InVehicleType);
};