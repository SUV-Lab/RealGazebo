// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/GazeboCameraManager.h"
#include "Vehicle/GazeboVehicleActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AGazeboCameraManager::AGazeboCameraManager()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create root component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // Create manual camera
    ManualCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ManualCamera"));
    ManualCamera->SetupAttachment(RootComponent);

    // Create first person camera (will be attached to vehicle)
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(RootComponent);
    FirstPersonCamera->SetActive(false);

    // Create third person camera with spring arm
    ThirdPersonArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonArm"));
    ThirdPersonArm->SetupAttachment(RootComponent);
    ThirdPersonArm->TargetArmLength = ThirdPersonArmLength;
    ThirdPersonArm->bUsePawnControlRotation = true;
    ThirdPersonArm->bInheritPitch = true;
    ThirdPersonArm->bInheritYaw = true;
    ThirdPersonArm->bInheritRoll = false;

    ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
    ThirdPersonCamera->SetupAttachment(ThirdPersonArm, USpringArmComponent::SocketName);
    ThirdPersonCamera->SetActive(false);

    // Set default camera mode
    CurrentCameraMode = EGazeboCameraMode::Manual;
}

void AGazeboCameraManager::BeginPlay()
{
    Super::BeginPlay();

    // Setup input mapping context
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (InputMappingContext)
            {
                Subsystem->AddMappingContext(InputMappingContext, 0);
            }
        }
    }

    UpdateCameraStates();
    UE_LOG(LogTemp, Log, TEXT("GazeboCameraManager: Initialized"));
}

void AGazeboCameraManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Remove input mapping context
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (InputMappingContext)
            {
                Subsystem->RemoveMappingContext(InputMappingContext);
            }
        }
    }

    Super::EndPlay(EndPlayReason);
}

void AGazeboCameraManager::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Camera switching
        if (ManualCameraAction)
        {
            EnhancedInputComponent->BindAction(ManualCameraAction, ETriggerEvent::Triggered, this, &AGazeboCameraManager::HandleManualCamera);
        }
        if (FirstPersonCameraAction)
        {
            EnhancedInputComponent->BindAction(FirstPersonCameraAction, ETriggerEvent::Triggered, this, &AGazeboCameraManager::HandleFirstPersonCamera);
        }
        if (ThirdPersonCameraAction)
        {
            EnhancedInputComponent->BindAction(ThirdPersonCameraAction, ETriggerEvent::Triggered, this, &AGazeboCameraManager::HandleThirdPersonCamera);
        }

        // Manual camera movement
        if (MoveForwardAction)
        {
            EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &AGazeboCameraManager::HandleMoveForward);
        }
        if (MoveRightAction)
        {
            EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &AGazeboCameraManager::HandleMoveRight);
        }
        if (MoveUpAction)
        {
            EnhancedInputComponent->BindAction(MoveUpAction, ETriggerEvent::Triggered, this, &AGazeboCameraManager::HandleMoveUp);
        }
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGazeboCameraManager::HandleLook);
        }
    }
}

void AGazeboCameraManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    switch (CurrentCameraMode)
    {
    case EGazeboCameraMode::Manual:
        UpdateManualCamera(DeltaTime);
        break;
    case EGazeboCameraMode::FirstPerson:
    case EGazeboCameraMode::ThirdPerson:
        UpdateVehicleAttachedCameras();
        break;
    }
}

void AGazeboCameraManager::SwitchToManualCamera()
{
    if (CurrentCameraMode == EGazeboCameraMode::Manual) return;

    CurrentCameraMode = EGazeboCameraMode::Manual;
    UpdateCameraStates();
    OnCameraModeChanged.Broadcast(CurrentCameraMode);
    UE_LOG(LogTemp, Log, TEXT("GazeboCameraManager: Switched to Manual Camera"));
}

void AGazeboCameraManager::SwitchToFirstPersonCamera()
{
    if (CurrentCameraMode == EGazeboCameraMode::FirstPerson) return;
    if (!TargetVehicle)
    {
        UE_LOG(LogTemp, Warning, TEXT("GazeboCameraManager: No target vehicle for First Person Camera"));
        return;
    }

    CurrentCameraMode = EGazeboCameraMode::FirstPerson;
    UpdateCameraStates();
    OnCameraModeChanged.Broadcast(CurrentCameraMode);
    UE_LOG(LogTemp, Log, TEXT("GazeboCameraManager: Switched to First Person Camera"));
}

void AGazeboCameraManager::SwitchToThirdPersonCamera()
{
    if (CurrentCameraMode == EGazeboCameraMode::ThirdPerson) return;
    if (!TargetVehicle)
    {
        UE_LOG(LogTemp, Warning, TEXT("GazeboCameraManager: No target vehicle for Third Person Camera"));
        return;
    }

    CurrentCameraMode = EGazeboCameraMode::ThirdPerson;
    UpdateCameraStates();
    OnCameraModeChanged.Broadcast(CurrentCameraMode);
    UE_LOG(LogTemp, Log, TEXT("GazeboCameraManager: Switched to Third Person Camera"));
}

void AGazeboCameraManager::SetTargetVehicle(AGazeboVehicleActor* Vehicle)
{
    TargetVehicle = Vehicle;
    if (Vehicle)
    {
        UE_LOG(LogTemp, Log, TEXT("GazeboCameraManager: Target vehicle set"));
    }
}

// Input handlers
void AGazeboCameraManager::HandleManualCamera()
{
    SwitchToManualCamera();
}

void AGazeboCameraManager::HandleFirstPersonCamera()
{
    SwitchToFirstPersonCamera();
}

void AGazeboCameraManager::HandleThirdPersonCamera()
{
    SwitchToThirdPersonCamera();
}

void AGazeboCameraManager::HandleMoveForward(const FInputActionValue& Value)
{
    if (CurrentCameraMode == EGazeboCameraMode::Manual)
    {
        float InputValue = Value.Get<float>();
        ManualCameraVelocity += GetActorForwardVector() * InputValue * ManualCameraMoveSpeed;
    }
}

void AGazeboCameraManager::HandleMoveRight(const FInputActionValue& Value)
{
    if (CurrentCameraMode == EGazeboCameraMode::Manual)
    {
        float InputValue = Value.Get<float>();
        ManualCameraVelocity += GetActorRightVector() * InputValue * ManualCameraMoveSpeed;
    }
}

void AGazeboCameraManager::HandleMoveUp(const FInputActionValue& Value)
{
    if (CurrentCameraMode == EGazeboCameraMode::Manual)
    {
        float InputValue = Value.Get<float>();
        ManualCameraVelocity += GetActorUpVector() * InputValue * ManualCameraMoveSpeed;
    }
}

void AGazeboCameraManager::HandleLook(const FInputActionValue& Value)
{
    if (CurrentCameraMode == EGazeboCameraMode::Manual)
    {
        FVector2D LookAxisVector = Value.Get<FVector2D>();
        
        // Apply mouse sensitivity
        LookAxisVector *= MouseSensitivity;
        
        // Add to rotation
        ManualCameraRotation.Yaw += LookAxisVector.X;
        ManualCameraRotation.Pitch = FMath::Clamp(ManualCameraRotation.Pitch - LookAxisVector.Y, -89.0f, 89.0f);
    }
}

void AGazeboCameraManager::UpdateCameraStates()
{
    ManualCamera->SetActive(CurrentCameraMode == EGazeboCameraMode::Manual);
    FirstPersonCamera->SetActive(CurrentCameraMode == EGazeboCameraMode::FirstPerson);
    ThirdPersonCamera->SetActive(CurrentCameraMode == EGazeboCameraMode::ThirdPerson);
}

void AGazeboCameraManager::UpdateManualCamera(float DeltaTime)
{
    // Update manual camera rotation
    SetActorRotation(ManualCameraRotation);
    
    // Update manual camera position
    FVector NewLocation = GetActorLocation() + ManualCameraVelocity * DeltaTime;
    SetActorLocation(NewLocation);
    
    // Apply drag to velocity
    ManualCameraVelocity *= FMath::Max(0.0f, 1.0f - (5.0f * DeltaTime));
}

void AGazeboCameraManager::UpdateVehicleAttachedCameras()
{
    if (!TargetVehicle) return;

    FVector VehicleLocation = TargetVehicle->GetActorLocation();
    FRotator VehicleRotation = TargetVehicle->GetActorRotation();

    if (CurrentCameraMode == EGazeboCameraMode::FirstPerson)
    {
        // Attach first person camera to vehicle center
        FirstPersonCamera->SetWorldLocation(VehicleLocation);
        FirstPersonCamera->SetWorldRotation(VehicleRotation);
    }
    else if (CurrentCameraMode == EGazeboCameraMode::ThirdPerson)
    {
        // Position third person arm at vehicle location with offset
        FVector ArmLocation = VehicleLocation + ThirdPersonArmOffset;
        ThirdPersonArm->SetWorldLocation(ArmLocation);
        
        // Update arm length
        ThirdPersonArm->TargetArmLength = ThirdPersonArmLength;
    }
}