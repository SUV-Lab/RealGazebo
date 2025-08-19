// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "GazeboVehicleData.h"
#include "GazeboVehicleActor.generated.h"

UCLASS()
class REALGAZEBO_API AGazeboVehicleActor : public AActor
{
    GENERATED_BODY()

public:
    AGazeboVehicleActor();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Vehicle identification
    UPROPERTY(BlueprintReadWrite, Category = "RealGazebo|Vehicle Info")
    uint8 VehicleNum;

    UPROPERTY(BlueprintReadWrite, Category = "RealGazebo|Vehicle Info")
    uint8 VehicleType;

    // Components - Vehicle Mesh as Root
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Components")
    UStaticMeshComponent* VehicleMesh;

    // Vehicle Camera Components - renamed with "Viewer" prefix
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Viewer Cameras")
    UCameraComponent* ViewerFirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Viewer Cameras")
    USpringArmComponent* ViewerThirdPersonSpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Viewer Cameras")
    UCameraComponent* ViewerThirdPersonCamera;

    // Rotating components for motors/propellers/wheels
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Rotating Components")
    TArray<URotatingMovementComponent*> RotatingComponents;

    // Components that can be controlled by servo data (e.g., fins, turrets)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Vehicle Components")
    TArray<USceneComponent*> ControllableComponents;

    // Update functions
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle")
    void UpdateVehiclePose(const FGazeboPoseData& PoseData);

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle")
    void UpdateVehicleMotorSpeed(const FGazeboMotorSpeedData& MotorSpeedData);

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle")
    void UpdateVehicleServo(const FGazeboServoData& ServoData);

    // Camera control functions - updated names
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer Cameras")
    void SetViewerFirstPersonCameraActive(bool bActive);

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer Cameras")
    void SetViewerThirdPersonCameraActive(bool bActive);

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer Cameras")
    void DeactivateAllViewerCameras();

    UFUNCTION(BlueprintPure, Category = "RealGazebo|Viewer Cameras")
    bool IsViewerFirstPersonCameraActive() const;

    UFUNCTION(BlueprintPure, Category = "RealGazebo|Viewer Cameras")
    bool IsViewerThirdPersonCameraActive() const;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Vehicle Settings")
    bool bSmoothMovement = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Vehicle Settings", meta = (EditCondition = "bSmoothMovement"))
    float InterpolationSpeed = 10.0f;

protected:
    // Smooth movement
    FVector TargetPosition;
    FRotator TargetRotation;
    bool bHasTarget;

    // Target state for servo components
    TArray<FVector> TargetServoPositions;
    TArray<FQuat> TargetServoQuaternions;
    bool bHasServoTarget;

    virtual void SetupVehicleMesh();
    void SmoothMoveToTarget(float DeltaTime);
    void SmoothMoveServosToTarget(float DeltaTime);

private:
    // Last update time for debugging
    float LastUpdateTime;
    float LastServoUpdateTime;

    // Helper functions for motor speed conversion
    float ConvertRadiansPerSecToDegPerSec(float RadiansPerSec) const;

    // Backward compatibility functions (deprecated)
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle Cameras", meta = (DeprecatedFunction, DeprecationMessage = "Use SetViewerFirstPersonCameraActive instead"))
    void SetFirstPersonCameraActive(bool bActive) { SetViewerFirstPersonCameraActive(bActive); }

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle Cameras", meta = (DeprecatedFunction, DeprecationMessage = "Use SetViewerThirdPersonCameraActive instead"))
    void SetThirdPersonCameraActive(bool bActive) { SetViewerThirdPersonCameraActive(bActive); }

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle Cameras", meta = (DeprecatedFunction, DeprecationMessage = "Use DeactivateAllViewerCameras instead"))
    void DeactivateAllCameras() { DeactivateAllViewerCameras(); }

    UFUNCTION(BlueprintPure, Category = "RealGazebo|Vehicle Cameras", meta = (DeprecatedFunction, DeprecationMessage = "Use IsViewerFirstPersonCameraActive instead"))
    bool IsFirstPersonCameraActive() const { return IsViewerFirstPersonCameraActive(); }

    UFUNCTION(BlueprintPure, Category = "RealGazebo|Vehicle Cameras", meta = (DeprecatedFunction, DeprecationMessage = "Use IsViewerThirdPersonCameraActive instead"))
    bool IsThirdPersonCameraActive() const { return IsViewerThirdPersonCameraActive(); }
};