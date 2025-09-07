#include "Core/RealGazeboUISubsystem.h"
#include "Core/GazeboBridgeSubsystem.h"
#include "RealGazeboUI.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

URealGazeboUISubsystem::URealGazeboUISubsystem()
{
    CurrentCameraMode = ERealGazeboCameraMode::Manual;
    bUIVisible = false;
    bHasSelectedVehicle = false;
    SelectedVehicleID = FVehicleID();
    UIUpdateFrequency = 10.0f;
    bAutoHideUIInVehicleMode = false;
}

void URealGazeboUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogRealGazeboUI, Display, TEXT("RealGazeboUISubsystem: Initializing"));

    // Get reference to bridge subsystem
    BridgeSubsystem = GetGameInstance()->GetSubsystem<UGazeboBridgeSubsystem>();
    if (!BridgeSubsystem.IsValid())
    {
        UE_LOG(LogRealGazeboUI, Error, TEXT("RealGazeboUISubsystem: Failed to get BridgeSubsystem reference"));
        return;
    }

    // Create vehicle data interface
    VehicleDataInterface = NewObject<UUIVehicleDataInterface>(this, TEXT("VehicleDataInterface"));
    if (VehicleDataInterface)
    {
        VehicleDataInterface->Initialize(BridgeSubsystem.Get());
    }

    // Setup input handling
    SetupInputBindings();

    UE_LOG(LogRealGazeboUI, Display, TEXT("RealGazeboUISubsystem: Initialized successfully"));
}

void URealGazeboUISubsystem::Deinitialize()
{
    UE_LOG(LogRealGazeboUI, Display, TEXT("RealGazeboUISubsystem: Deinitializing"));

    // Clean up UI
    HideUI();


    // Clear timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UIUpdateTimerHandle);
    }

    Super::Deinitialize();
}

bool URealGazeboUISubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    // Don't create on dedicated servers
    return !IsRunningDedicatedServer();
}

void URealGazeboUISubsystem::ToggleUI()
{
    if (bUIVisible)
    {
        HideUI();
    }
    else
    {
        ShowUI();
    }
}

void URealGazeboUISubsystem::ShowUI()
{
    bUIVisible = true;
    OnUIToggled.Broadcast();
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("UI Shown"));
}

void URealGazeboUISubsystem::HideUI()
{
    bUIVisible = false;
    OnUIToggled.Broadcast();
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("UI Hidden"));
}

bool URealGazeboUISubsystem::IsUIVisible() const
{
    return bUIVisible;
}

void URealGazeboUISubsystem::SelectVehicle(const FVehicleID& VehicleID)
{
    SelectedVehicleID = VehicleID;
    bHasSelectedVehicle = true;

    // Get vehicle data for the event
    FUIVehicleData VehicleData;
    if (VehicleDataInterface && VehicleDataInterface->GetVehicleData(VehicleID, VehicleData))
    {
        OnVehicleSelected.Broadcast(VehicleData);
        UE_LOG(LogRealGazeboUI, Verbose, TEXT("Vehicle selected: %s"), *VehicleData.DisplayName);
    }
}

void URealGazeboUISubsystem::SelectVehicleByName(const FString& DisplayName)
{
    if (VehicleDataInterface)
    {
        FUIVehicleData VehicleData;
        if (VehicleDataInterface->GetVehicleDataByName(DisplayName, VehicleData))
        {
            SelectVehicle(VehicleData.VehicleID);
        }
    }
}

void URealGazeboUISubsystem::ClearVehicleSelection()
{
    bHasSelectedVehicle = false;
    SelectedVehicleID = FVehicleID();
    
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("Vehicle selection cleared"));
}

bool URealGazeboUISubsystem::GetSelectedVehicle(FUIVehicleData& OutVehicleData) const
{
    if (!bHasSelectedVehicle || !VehicleDataInterface)
    {
        return false;
    }

    return VehicleDataInterface->GetVehicleData(SelectedVehicleID, OutVehicleData);
}

bool URealGazeboUISubsystem::IsVehicleSelected() const
{
    return bHasSelectedVehicle;
}

void URealGazeboUISubsystem::SetCameraMode(ERealGazeboCameraMode NewMode)
{
    if (CurrentCameraMode == NewMode)
    {
        return;
    }

    ERealGazeboCameraMode OldMode = CurrentCameraMode;
    CurrentCameraMode = NewMode;

    HandleCameraModeChange(NewMode);
    OnCameraModeChanged.Broadcast(NewMode);

    UE_LOG(LogRealGazeboUI, Verbose, TEXT("Camera mode changed from %d to %d"), (int32)OldMode, (int32)NewMode);
}

ERealGazeboCameraMode URealGazeboUISubsystem::GetCurrentCameraMode() const
{
    return CurrentCameraMode;
}

void URealGazeboUISubsystem::SwitchToManualCamera()
{
    SetCameraMode(ERealGazeboCameraMode::Manual);
}

void URealGazeboUISubsystem::SwitchToFirstPersonCamera()
{
    if (!bHasSelectedVehicle)
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("Cannot switch to first person camera: No vehicle selected"));
        return;
    }

    SetCameraMode(ERealGazeboCameraMode::FirstPerson);
}

void URealGazeboUISubsystem::SwitchToThirdPersonCamera()
{
    if (!bHasSelectedVehicle)
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("Cannot switch to third person camera: No vehicle selected"));
        return;
    }

    SetCameraMode(ERealGazeboCameraMode::ThirdPerson);
}

TArray<FUIVehicleData> URealGazeboUISubsystem::GetAllVehicleData()
{
    if (VehicleDataInterface)
    {
        return VehicleDataInterface->GetAllVehicleData();
    }

    return TArray<FUIVehicleData>();
}

FUIVehicleData URealGazeboUISubsystem::GetVehicleData(const FString& VehicleName)
{
    if (VehicleDataInterface)
    {
        FUIVehicleData VehicleData;
        if (VehicleDataInterface->GetVehicleDataByName(VehicleName, VehicleData))
        {
            return VehicleData;
        }
    }

    return FUIVehicleData();
}

bool URealGazeboUISubsystem::IsVehicleActive(const FString& VehicleName)
{
    FUIVehicleData VehicleData = GetVehicleData(VehicleName);
    return !VehicleName.IsEmpty() && VehicleData.bIsActive;
}

int32 URealGazeboUISubsystem::GetActiveVehicleCount()
{
    TArray<FUIVehicleData> AllVehicles = GetAllVehicleData();
    int32 ActiveCount = 0;
    
    for (const FUIVehicleData& VehicleData : AllVehicles)
    {
        if (VehicleData.bIsActive)
        {
            ActiveCount++;
        }
    }

    return ActiveCount;
}

void URealGazeboUISubsystem::SetCameraTarget(const FString& VehicleName)
{
    SelectVehicleByName(VehicleName);
    UE_LOG(LogRealGazeboUI, Log, TEXT("Camera target set to vehicle: %s"), *VehicleName);
}

URealGazeboUISubsystem* URealGazeboUISubsystem::GetUISubsystem(const UObject* WorldContext)
{
    if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
    {
        return World->GetGameInstance()->GetSubsystem<URealGazeboUISubsystem>();
    }
    
    return nullptr;
}

void URealGazeboUISubsystem::InitializeCameraManager()
{
    // Camera manager initialization - will be implemented later
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("Camera manager initialization - to be implemented"));
}

void URealGazeboUISubsystem::InitializeUI()
{
    // UI initialization - will be implemented later
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("UI initialization - to be implemented"));
}

void URealGazeboUISubsystem::SetupInputBindings()
{
    UE_LOG(LogRealGazeboUI, Display, TEXT("RealGazeboUISubsystem: Input setup simplified - UI classes handle their own input"));
    
    // Input handling simplified - individual UI classes handle their own input needs
    // No global input handler needed anymore
}

void URealGazeboUISubsystem::UpdateUI()
{
    // UI update logic - will be implemented later
    if (VehicleDataInterface)
    {
        VehicleDataInterface->RefreshVehicleData();
    }
}

void URealGazeboUISubsystem::HandleCameraModeChange(ERealGazeboCameraMode NewMode)
{
    // Handle camera mode changes - will be implemented with camera manager
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("Handling camera mode change to: %d"), (int32)NewMode);
}

void URealGazeboUISubsystem::HandleManualCameraInput()
{
    UE_LOG(LogRealGazeboUI, VeryVerbose, TEXT("Manual camera input handled"));
}

void URealGazeboUISubsystem::HandleFirstPersonCameraInput()
{
    UE_LOG(LogRealGazeboUI, VeryVerbose, TEXT("First person camera input handled"));
}

void URealGazeboUISubsystem::HandleThirdPersonCameraInput()
{
    UE_LOG(LogRealGazeboUI, VeryVerbose, TEXT("Third person camera input handled"));
}

void URealGazeboUISubsystem::HandleUIToggleInput()
{
    ToggleUI();
}