#include "MainFreeCameraActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AMainFreeCameraActor::AMainFreeCameraActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create root component
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootSceneComponent;

    // Create main camera
    MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
    MainCamera->SetupAttachment(RootComponent);

    // Initialize state
    MouseInput = FVector2D::ZeroVector;
    bIsActive = false;
    
    // Enable input
    AutoPossessPlayer = EAutoReceiveInput::Disabled; // We'll handle possession manually
}

void AMainFreeCameraActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Start deactivated
    DeactivateMainCamera();
    
    UE_LOG(LogTemp, Warning, TEXT("MainFreeCameraActor: Spawned at (%s)"), 
           *GetActorLocation().ToString());
}

void AMainFreeCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Handle mouse look only when active
    if (bIsActive && !MouseInput.IsZero())
    {
        FRotator CurrentRotation = GetActorRotation();
        CurrentRotation.Yaw += MouseInput.X * MouseSensitivity;
        CurrentRotation.Pitch = FMath::Clamp(CurrentRotation.Pitch + MouseInput.Y * MouseSensitivity, -80.0f, 80.0f);
        SetActorRotation(CurrentRotation);
        MouseInput = FVector2D::ZeroVector;
    }
}

void AMainFreeCameraActor::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Camera movement
    PlayerInputComponent->BindAxis("MoveForward", this, &AMainFreeCameraActor::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AMainFreeCameraActor::MoveRight);
    PlayerInputComponent->BindAxis("MoveUp", this, &AMainFreeCameraActor::MoveUp);
    PlayerInputComponent->BindAxis("LookUp", this, &AMainFreeCameraActor::LookUp);
    PlayerInputComponent->BindAxis("LookRight", this, &AMainFreeCameraActor::LookRight);
}

void AMainFreeCameraActor::ActivateMainCamera()
{
    if (MainCamera)
    {
        MainCamera->SetActive(true);
        bIsActive = true;
        
        // Set this as the view target for the player controller
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            // Important: Set view target with smooth transition disabled to prevent position interpolation
            FViewTargetTransitionParams TransitionParams;
            TransitionParams.BlendTime = 0.0f; // Instant transition
            TransitionParams.BlendFunction = VTBlend_Linear;
            
            PC->SetViewTarget(this, TransitionParams);
            PC->Possess(this);
        }
        
        UE_LOG(LogTemp, Log, TEXT("MainFreeCameraActor: Main camera ACTIVATED at (%s)"), *GetActorLocation().ToString());
    }
}

void AMainFreeCameraActor::DeactivateMainCamera()
{
    if (MainCamera)
    {
        MainCamera->SetActive(false);
        bIsActive = false;
        
        // Unpossess from player controller
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            if (PC->GetPawn() == this)
            {
                PC->UnPossess();
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("MainFreeCameraActor: Main camera DEACTIVATED"));
    }
}

bool AMainFreeCameraActor::IsMainCameraActive() const
{
    return MainCamera ? MainCamera->IsActive() : false;
}

// Input handlers
void AMainFreeCameraActor::MoveForward(float Value)
{
    if (bIsActive && Value != 0.0f)
    {
        FVector Direction = GetActorForwardVector();
        AddActorWorldOffset(Direction * Value * CameraSpeed * GetWorld()->GetDeltaSeconds());
    }
}

void AMainFreeCameraActor::MoveRight(float Value)
{
    if (bIsActive && Value != 0.0f)
    {
        FVector Direction = GetActorRightVector();
        AddActorWorldOffset(Direction * Value * CameraSpeed * GetWorld()->GetDeltaSeconds());
    }
}

void AMainFreeCameraActor::MoveUp(float Value)
{
    if (bIsActive && Value != 0.0f)
    {
        FVector Direction = GetActorUpVector();
        AddActorWorldOffset(Direction * Value * CameraSpeed * GetWorld()->GetDeltaSeconds());
    }
}

void AMainFreeCameraActor::LookUp(float Value)
{
    if (bIsActive)
    {
        MouseInput.Y -= Value;
    }
}

void AMainFreeCameraActor::LookRight(float Value)
{
    if (bIsActive)
    {
        MouseInput.X += Value;
    }
}
