#include "RealGazeboPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GazeboVehicleManager.h"
#include "MainFreeCameraActor.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h" // Add this include for TActorIterator

ARealGazeboPlayerController::ARealGazeboPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    
    CameraManager = nullptr;
    bShowUI = true;
    
    // Enable mouse cursor and input
    bShowMouseCursor = false;
    bEnableClickEvents = false;
    bEnableMouseOverEvents = false;
}

void ARealGazeboPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    // Find camera manager in the scene
    FindCameraManager();
    
    // Set input mode to allow both game input and UI
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture);
    InputMode.SetHideCursorDuringCapture(true);
    SetInputMode(InputMode);
    
    UE_LOG(LogTemp, Warning, TEXT("RealGazeboPlayerController: Started"));
}

void ARealGazeboPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    
    // Keep keyboard camera switching (M, F, B)
    InputComponent->BindAction("RealGazebo_ManualCamera", IE_Pressed, this, &ARealGazeboPlayerController::OnManualCameraPressed);
    InputComponent->BindAction("RealGazebo_FirstPersonCamera", IE_Pressed, this, &ARealGazeboPlayerController::OnFirstPersonCameraPressed);
    InputComponent->BindAction("RealGazebo_ThirdPersonCamera", IE_Pressed, this, &ARealGazeboPlayerController::OnThirdPersonCameraPressed);
    
    // Remove N/P key bindings - vehicle selection now handled by UI dropdown only
    
    // UI toggle
    InputComponent->BindAction("RealGazebo_ToggleUI", IE_Pressed, this, &ARealGazeboPlayerController::OnToggleUIPressed);
    
    UE_LOG(LogTemp, Warning, TEXT("RealGazeboPlayerController: Input component setup complete"));
}

void ARealGazeboPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Find camera manager if we don't have one yet
    if (!CameraManager)
    {
        FindCameraManager();
    }
}

void ARealGazeboPlayerController::FindCameraManager()
{
    // Find the vehicle manager that has the camera manager component
    UWorld* World = GetWorld();
    if (!World)
        return;
        
    for (TActorIterator<AGazeboVehicleManager> ActorIterator(World); ActorIterator; ++ActorIterator)
    {
        AGazeboVehicleManager* VehicleManager = *ActorIterator;
        if (VehicleManager && IsValid(VehicleManager))
        {
            // Get the camera manager component - updated type
            CameraManager = VehicleManager->CameraManager;
            if (CameraManager)
            {
                UE_LOG(LogTemp, Warning, TEXT("RealGazeboPlayerController: Found UserCameraManager"));
                break;
            }
        }
    }
    
    if (!CameraManager)
    {
        UE_LOG(LogTemp, Error, TEXT("RealGazeboPlayerController: UserCameraManager not found in scene"));
    }
}

void ARealGazeboPlayerController::OnManualCameraPressed()
{
    if (CameraManager)
    {
        CameraManager->SwitchToMainFreeCamera();
        UE_LOG(LogTemp, Log, TEXT("RealGazeboPlayerController: Switched to Manual (Free) Camera [M]"));
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("Camera: Manual/Free [M]"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RealGazeboPlayerController: CameraManager not found"));
    }
}

void ARealGazeboPlayerController::OnFirstPersonCameraPressed()
{
    if (CameraManager)
    {
        CameraManager->SwitchToVehicleFirstPerson();
        UE_LOG(LogTemp, Log, TEXT("RealGazeboPlayerController: Switched to First Person Camera [F]"));
        
        if (GEngine)
        {
            AGazeboVehicleActor* SelectedVehicle = CameraManager->GetSelectedVehicle();
            FString VehicleName = SelectedVehicle ? SelectedVehicle->GetActorLabel() : TEXT("None");
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, 
                FString::Printf(TEXT("Camera: First Person - %s [F]"), *VehicleName));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RealGazeboPlayerController: CameraManager not found"));
    }
}

void ARealGazeboPlayerController::OnThirdPersonCameraPressed()
{
    if (CameraManager)
    {
        CameraManager->SwitchToVehicleThirdPerson();
        UE_LOG(LogTemp, Log, TEXT("RealGazeboPlayerController: Switched to Third Person Camera [B]"));
        
        if (GEngine)
        {
            AGazeboVehicleActor* SelectedVehicle = CameraManager->GetSelectedVehicle();
            FString VehicleName = SelectedVehicle ? SelectedVehicle->GetActorLabel() : TEXT("None");
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Blue, 
                FString::Printf(TEXT("Camera: Third Person - %s [B]"), *VehicleName));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RealGazeboPlayerController: CameraManager not found"));
    }
}

void ARealGazeboPlayerController::OnToggleUIPressed()
{
    bShowUI = !bShowUI;
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::White, 
            FString::Printf(TEXT("UI Display: %s [H]"), bShowUI ? TEXT("ON") : TEXT("OFF")));
    }
    
    UE_LOG(LogTemp, Log, TEXT("RealGazeboPlayerController: UI toggled %s [H]"), bShowUI ? TEXT("ON") : TEXT("OFF"));
}

// UI-based vehicle selection methods
void ARealGazeboPlayerController::SelectVehicleByIndex(int32 VehicleIndex)
{
    if (CameraManager)
    {
        TArray<AGazeboVehicleActor*> Vehicles = CameraManager->GetAvailableVehicles();
        if (Vehicles.IsValidIndex(VehicleIndex))
        {
            // Set the selected vehicle index directly
            CameraManager->SetSelectedVehicleIndex(VehicleIndex);
            
            // Re-apply current camera mode to the new vehicle
            EUserCameraMode CurrentMode = CameraManager->GetCurrentCameraMode();
            if (CurrentMode == EUserCameraMode::VehicleFirstPerson)
            {
                CameraManager->SwitchToVehicleFirstPerson();
            }
            else if (CurrentMode == EUserCameraMode::VehicleThirdPerson)
            {
                CameraManager->SwitchToVehicleThirdPerson();
            }
            
            AGazeboVehicleActor* SelectedVehicle = Vehicles[VehicleIndex];
            UE_LOG(LogTemp, Log, TEXT("RealGazeboPlayerController: Selected vehicle %s via UI dropdown"), 
                   *SelectedVehicle->GetActorLabel());
            
            // Show on-screen feedback
            if (GEngine)
            {
                FString VehicleName = SelectedVehicle->GetActorLabel();
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, 
                    FString::Printf(TEXT("Selected Vehicle: %s (%d/%d)"), *VehicleName, VehicleIndex + 1, Vehicles.Num()));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("RealGazeboPlayerController: Invalid vehicle index %d"), VehicleIndex);
        }
    }
}

TArray<FString> ARealGazeboPlayerController::GetAvailableVehicleNames() const
{
    TArray<FString> VehicleNames;
    if (CameraManager)
    {
        TArray<AGazeboVehicleActor*> Vehicles = CameraManager->GetAvailableVehicles();
        for (int32 i = 0; i < Vehicles.Num(); i++)
        {
            AGazeboVehicleActor* Vehicle = Vehicles[i];
            if (Vehicle)
            {
                // Format as "VehicleName (Index)" for dropdown display
                FString DisplayName = FString::Printf(TEXT("%s (%d/%d)"), 
                    *Vehicle->GetActorLabel(), i + 1, Vehicles.Num());
                VehicleNames.Add(DisplayName);
            }
        }
    }
    return VehicleNames;
}

int32 ARealGazeboPlayerController::GetCurrentVehicleIndex() const
{
    return CameraManager ? CameraManager->GetSelectedVehicleIndex() : 0;
}

// Add helper function to get current camera mode for UI
FString ARealGazeboPlayerController::GetCurrentCameraModeString() const
{
    if (!CameraManager)
        return TEXT("Unknown");
        
    EUserCameraMode CurrentMode = CameraManager->GetCurrentCameraMode();
    switch (CurrentMode)
    {
        case EUserCameraMode::MainFree:
            return TEXT("Manual/Free Camera [M]");
        case EUserCameraMode::VehicleFirstPerson:
            return TEXT("First Person Camera [F]");
        case EUserCameraMode::VehicleThirdPerson:
            return TEXT("Third Person Camera [B]");
        default:
            return TEXT("Unknown Camera");
    }
}

FString ARealGazeboPlayerController::GetCurrentVehicleName() const
{
    if (CameraManager)
    {
        AGazeboVehicleActor* SelectedVehicle = CameraManager->GetSelectedVehicle();
        if (SelectedVehicle)
        {
            return SelectedVehicle->GetActorLabel();
        }
    }
    return TEXT("No Vehicle Selected");
}