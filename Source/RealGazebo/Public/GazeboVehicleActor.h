// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
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

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Components")
    UStaticMeshComponent* VehicleMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Components")
    USceneComponent* RootSceneComponent;

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
    void UpdateVehicleRPM(const FGazeboRPMData& RPMData);

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle")
    void UpdateVehicleServo(const FGazeboServoData& ServoData);

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
    TArray<FRotator> TargetServoRotations;
    bool bHasServoTarget;

    virtual void SetupVehicleMesh();
    void SmoothMoveToTarget(float DeltaTime);
    void SmoothMoveServosToTarget(float DeltaTime);

private:
    // Last update time for debugging
    float LastUpdateTime;
    float LastServoUpdateTime;

    // Helper functions for RPM conversion
    float ConvertRadiansToDegrees(float Radians) const;
};