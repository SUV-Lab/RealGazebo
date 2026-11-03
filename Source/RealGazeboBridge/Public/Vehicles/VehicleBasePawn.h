// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Sub-author: MinKyu Kim
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

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

    /** Vehicle type name from DataTable (e.g., "x500", "iris") - used for RTSP URL generation */
    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Vehicle")
    FString VehicleTypeName;

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
    // Vehicle State Updates (called by subsystem)
    //----------------------------------------------------------

    /** Set the vehicle pose directly (placement, spawn). Drops any streamed pose samples. */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Updates")
    void UpdateVehiclePose(const FVector& Position, const FQuat& Rotation);

    /**
     * Add a streamed pose with its arrival time (FPlatformTime::Seconds()).
     * Poses arrive unevenly (several in one frame, none in the next), so the vehicle is drawn by
     * interpolating the samples at a clock that runs evenly PoseInterpolationDelay behind the newest one.
     */
    void AddPoseSample(const FVector& Position, const FQuat& Rotation, double SampleTime);

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

    /** Force pool active state (for testing/manual spawning) */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Pool")
    void SetActiveVehicle(bool bActive);

    /** Manually initialize vehicle for placed actors / streaming test */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Pool")
    void InitializeVehicleManually(int32 InVehicleNum, uint8 InVehicleType, const FString& InVehicleTypeName);

    //----------------------------------------------------------
    // Performance Settings
    //----------------------------------------------------------

    /** Enable smooth movement interpolation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Performance")
    bool bSmoothMovement = true;

    /** Interpolation speed for smooth movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Performance", meta = (EditCondition = "bSmoothMovement"))
    float InterpolationSpeed = 15.0f;

    /** Streamed poses: how far behind the newest sample the vehicle is drawn (s). Longer than the usual gap between packets. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Performance", meta = (ClampMin = "0", EditCondition = "bSmoothMovement"))
    float PoseInterpolationDelay = 0.05f;

    /** Streamed poses: no packet for this long (s) means the stream stopped; the vehicle holds its last pose */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Performance", meta = (ClampMin = "0", EditCondition = "bSmoothMovement"))
    float PoseStreamTimeout = 0.5f;

    /** Streamed poses: a sample farther than this (cm) from the previous one is a teleport, jumped to without interpolation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Performance", meta = (ClampMin = "0", EditCondition = "bSmoothMovement"))
    float PoseTeleportDistance = 200.0f;

    //----------------------------------------------------------
    // Visual Scale (Unreal-only; Gazebo physics stays 1x)
    //----------------------------------------------------------

    /**
     * Uniform scale applied to VehicleMesh (and everything attached under it) in Unreal only.
     * The actor transform keeps the exact Gazebo pose, so placement/spawn round-trips are unaffected.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bridge|Visual Scale", meta = (ClampMin = "0.01"))
    float VisualScale = 1.0f;

    /**
     * Point of the unscaled vehicle (actor-local, cm) that stays fixed when VisualScale is applied.
     * Ground vehicles: use the rear-axle ground contact so the scaled vehicle stays on the ground
     * (also on slopes) and its rear axle follows the Gazebo path.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bridge|Visual Scale")
    FVector VisualScaleAnchor = FVector::ZeroVector;

    /**
     * Wheeled vehicles: keep wheel kinematics consistent with the scaled size.
     * - Motor (wheel) rates are divided by VisualScale so wheel roll matches ground travel.
     * - Streamed controllable rotations are treated as steering: only the steering yaw is kept
     *   (Gazebo angle, not widened for the scaled wheelbase). Wheel roll in that stream would turn
     *   at the unscaled rate, so it is dropped; it must come from RotatingComponents instead.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bridge|Visual Scale")
    bool bCompensateWheelKinematics = false;

    /** Apply VisualScale / VisualScaleAnchor to VehicleMesh */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Visual Scale")
    void ApplyVisualScale();


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


protected:
    //----------------------------------------------------------
    // Pawn Lifecycle
    //----------------------------------------------------------

    virtual void OnConstruction(const FTransform& Transform) override;
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

    /** Streamed pose samples, oldest first, times strictly increasing */
    struct FPoseSample
    {
        double Time;
        FVector Location;
        FQuat Rotation;
    };
    TArray<FPoseSample> PoseSamples;
    double LastPoseSampleArrival = 0.0;

    /** Render clock for the samples */
    bool bPoseClockValid = false;
    double PoseRenderTime = 0.0;
    double PosePrevNewest = 0.0;
    double PoseSourceRate = 1.0;
    double PoseLead = 0.0;

    void ResetPoseSamples();
    void PerformSampledMovement(float DeltaTime);

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

    /** Root-attached components were already moved for VisualScale (BeginPlay runs once per pawn) */
    bool bVisualScaleMovedRootChildren = false;

    /** True when VisualScale differs from 1 */
    bool HasVisualScale() const { return !FMath::IsNearlyEqual(VisualScale, 1.0f); }

    /** Steering-only rotation (yaw about the vehicle up axis) extracted from a streamed wheel rotation */
    FQuat ScaleSteeringRotation(const FQuat& StreamedRotation) const;

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