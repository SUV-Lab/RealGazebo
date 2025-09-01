// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GazeboCameraManager.generated.h"

class AGazeboVehicleActor;
class UInputAction;
class UInputMappingContext;

UENUM(BlueprintType)
enum class EGazeboCameraMode : uint8
{
    Manual      UMETA(DisplayName = "Manual Camera"),
    FirstPerson UMETA(DisplayName = "First Person"),
    ThirdPerson UMETA(DisplayName = "Third Person")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraModeChanged, EGazeboCameraMode, NewMode);

/**
 * Camera manager for RealGazebo - handles different camera modes
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API AGazeboCameraManager : public APawn
{
    GENERATED_BODY()

public:
    AGazeboCameraManager();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

public:
    virtual void Tick(float DeltaTime) override;

    // Camera switching
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Camera")
    void SwitchToManualCamera();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Camera")
    void SwitchToFirstPersonCamera();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Camera")
    void SwitchToThirdPersonCamera();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Camera")
    void SetTargetVehicle(AGazeboVehicleActor* Vehicle);

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Camera")
    EGazeboCameraMode GetCurrentCameraMode() const { return CurrentCameraMode; }

    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Camera")
    FOnCameraModeChanged OnCameraModeChanged;

protected:
    // Input Actions
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealGazebo|Input")
    UInputMappingContext* InputMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealGazebo|Input")
    UInputAction* ManualCameraAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealGazebo|Input")
    UInputAction* FirstPersonCameraAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealGazebo|Input")
    UInputAction* ThirdPersonCameraAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealGazebo|Input")
    UInputAction* MoveForwardAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealGazebo|Input")
    UInputAction* MoveRightAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealGazebo|Input")
    UInputAction* MoveUpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealGazebo|Input")
    UInputAction* LookAction;

    // Input handlers
    void HandleManualCamera();
    void HandleFirstPersonCamera();
    void HandleThirdPersonCamera();
    void HandleMoveForward(const FInputActionValue& Value);
    void HandleMoveRight(const FInputActionValue& Value);
    void HandleMoveUp(const FInputActionValue& Value);
    void HandleLook(const FInputActionValue& Value);

    // Camera Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Camera")
    UCameraComponent* ManualCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Camera")
    UCameraComponent* FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Camera")
    USpringArmComponent* ThirdPersonArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Camera")
    UCameraComponent* ThirdPersonCamera;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Camera Settings")
    float ManualCameraMoveSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Camera Settings")
    float MouseSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Camera Settings")
    float ThirdPersonArmLength = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Camera Settings")
    FVector ThirdPersonArmOffset = FVector(0.0f, 0.0f, 50.0f);

private:
    EGazeboCameraMode CurrentCameraMode = EGazeboCameraMode::Manual;
    
    UPROPERTY()
    AGazeboVehicleActor* TargetVehicle = nullptr;

    // Manual camera movement
    FVector ManualCameraVelocity = FVector::ZeroVector;
    FRotator ManualCameraRotation = FRotator::ZeroRotator;

    void UpdateCameraStates();
    void UpdateManualCamera(float DeltaTime);
    void UpdateVehicleAttachedCameras();
};