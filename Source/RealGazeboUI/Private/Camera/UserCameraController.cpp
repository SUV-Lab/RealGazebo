#include "Camera/UserCameraController.h"
#include "Core/RealGazeboUISubsystem.h"
#include "RealGazeboUI.h"
#include "Engine/Engine.h"

UUserCameraController::UUserCameraController()
{
    CurrentCameraMode = ERealGazeboCameraMode::Manual;
    SelectedVehicleName = TEXT("");
    ListManager = nullptr;
}

void UUserCameraController::SwitchToManualMode()
{
    CurrentCameraMode = ERealGazeboCameraMode::Manual;
    ApplyCameraModeToSubsystem();
    
    UE_LOG(LogRealGazeboUI, Log, TEXT("Camera: Switched to Manual mode"));
}

bool UUserCameraController::SwitchToFirstPersonMode()
{
    if (!IsVehicleValidForCamera())
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("Camera: Cannot switch to First Person - no valid vehicle selected"));
        return false;
    }

    CurrentCameraMode = ERealGazeboCameraMode::FirstPerson;
    ApplyCameraModeToSubsystem();
    
    UE_LOG(LogRealGazeboUI, Log, TEXT("Camera: Switched to First Person mode for vehicle '%s'"), *SelectedVehicleName);
    return true;
}

bool UUserCameraController::SwitchToThirdPersonMode()
{
    if (!IsVehicleValidForCamera())
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("Camera: Cannot switch to Third Person - no valid vehicle selected"));
        return false;
    }

    CurrentCameraMode = ERealGazeboCameraMode::ThirdPerson;
    ApplyCameraModeToSubsystem();
    
    UE_LOG(LogRealGazeboUI, Log, TEXT("Camera: Switched to Third Person mode for vehicle '%s'"), *SelectedVehicleName);
    return true;
}

ERealGazeboCameraMode UUserCameraController::GetCurrentCameraMode() const
{
    return CurrentCameraMode;
}

bool UUserCameraController::SetSelectedVehicle(const FString& VehicleName)
{
    if (!ListManager)
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("Camera: Cannot select vehicle - ListManager not set"));
        return false;
    }

    if (ListManager->SelectVehicle(VehicleName))
    {
        SelectedVehicleName = VehicleName;
        UE_LOG(LogRealGazeboUI, Log, TEXT("Camera: Selected vehicle '%s' for camera targeting"), *VehicleName);
        return true;
    }

    return false;
}

FString UUserCameraController::GetSelectedVehicleName() const
{
    return SelectedVehicleName;
}

bool UUserCameraController::RequiresVehicleSelection() const
{
    return (CurrentCameraMode == ERealGazeboCameraMode::FirstPerson || 
            CurrentCameraMode == ERealGazeboCameraMode::ThirdPerson);
}

void UUserCameraController::HandleKeyPress(const FString& KeyName)
{
    if (KeyName.Equals(TEXT("M"), ESearchCase::IgnoreCase))
    {
        SwitchToManualMode();
    }
    else if (KeyName.Equals(TEXT("F"), ESearchCase::IgnoreCase))
    {
        SwitchToFirstPersonMode();
    }
    else if (KeyName.Equals(TEXT("B"), ESearchCase::IgnoreCase))
    {
        SwitchToThirdPersonMode();
    }
}

FString UUserCameraController::GetCameraStatusText() const
{
    FString StatusText = FString::Printf(TEXT("Camera: %s"), *GetCameraModeDisplayName());

    if (RequiresVehicleSelection())
    {
        if (!SelectedVehicleName.IsEmpty())
        {
            StatusText += FString::Printf(TEXT(" | Target: %s"), *SelectedVehicleName);
        }
        else
        {
            StatusText += TEXT(" | No Target Selected");
        }
    }

    return StatusText;
}

void UUserCameraController::SetListManager(UListManager* InListManager)
{
    ListManager = InListManager;
    
    if (ListManager)
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("Camera: ListManager connected"));
    }
}

void UUserCameraController::Initialize()
{
    InitializeUISubsystem();
    
    // Set initial camera mode
    CurrentCameraMode = ERealGazeboCameraMode::Manual;
    ApplyCameraModeToSubsystem();
    
    UE_LOG(LogRealGazeboUI, Log, TEXT("Camera: UserCameraController initialized"));
}

void UUserCameraController::InitializeUISubsystem()
{
    if (UISubsystem.IsValid())
    {
        return;
    }

    UISubsystem = URealGazeboUISubsystem::GetUISubsystem(this);

    if (!UISubsystem.IsValid())
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("Camera: Could not initialize UI subsystem"));
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("Camera: UI subsystem connected"));
    }
}

void UUserCameraController::ApplyCameraModeToSubsystem()
{
    if (!UISubsystem.IsValid())
    {
        InitializeUISubsystem();
        if (!UISubsystem.IsValid())
        {
            return;
        }
    }

    UISubsystem->SetCameraMode(CurrentCameraMode);
    
    // Set target vehicle if needed
    if (RequiresVehicleSelection() && !SelectedVehicleName.IsEmpty())
    {
        UISubsystem->SetCameraTarget(SelectedVehicleName);
    }
}

bool UUserCameraController::IsVehicleValidForCamera() const
{
    if (SelectedVehicleName.IsEmpty())
    {
        return false;
    }

    if (!ListManager)
    {
        return false;
    }

    return ListManager->HasSelectedVehicle() && 
           ListManager->GetSelectedVehicleName().Equals(SelectedVehicleName);
}

FString UUserCameraController::GetCameraModeDisplayName() const
{
    switch (CurrentCameraMode)
    {
        case ERealGazeboCameraMode::Manual:
            return TEXT("Manual");
        case ERealGazeboCameraMode::FirstPerson:
            return TEXT("First Person");
        case ERealGazeboCameraMode::ThirdPerson:
            return TEXT("Third Person");
        default:
            return TEXT("Unknown");
    }
}