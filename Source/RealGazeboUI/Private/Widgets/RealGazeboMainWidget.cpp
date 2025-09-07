#include "Widgets/RealGazeboMainWidget.h"
#include "Core/RealGazeboUISubsystem.h"
#include "RealGazeboUI.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

URealGazeboMainWidget::URealGazeboMainWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Initialize default settings
    UpdateFrequency = 10.0f;
    bAutoRefreshVehicleList = true;
    AutoRefreshInterval = 2.0f;

    // Initialize state
    SelectedVehicleID = FVehicleID();
    CurrentCameraMode = ERealGazeboCameraMode::Manual;
    bUIInitialized = false;

    // Widget simplified - child widgets removed
}

void URealGazeboMainWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: Constructing"));

    // Initialize UI subsystem reference
    InitializeUISubsystem();

    // Setup timers
    SetupUpdateTimer();

    // Initial data update
    UpdateVehicleData();

    bUIInitialized = true;

    // Call Blueprint event
    OnUIInitialized();

    UE_LOG(LogRealGazeboUI, Display, TEXT("RealGazeboMainWidget: Initialized successfully"));
}

void URealGazeboMainWidget::NativeDestruct()
{
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: Destructing"));

    // Clear timers
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UpdateTimerHandle);
        World->GetTimerManager().ClearTimer(AutoRefreshTimerHandle);
    }

    // Unbind from subsystem events
    if (UISubsystem.IsValid())
    {
        UISubsystem->OnVehicleSelected.RemoveAll(this);
        UISubsystem->OnCameraModeChanged.RemoveAll(this);
        UISubsystem->OnUIToggled.RemoveAll(this);
    }

    Super::NativeDestruct();
}

void URealGazeboMainWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Additional per-frame logic can be added here if needed
    // Currently, we rely on timer-based updates for better performance
}

void URealGazeboMainWidget::UpdateVehicleData()
{
    if (!UISubsystem.IsValid())
    {
        return;
    }

    // Get all vehicle data from subsystem
    TArray<FUIVehicleData> NewVehicleData = UISubsystem->GetAllVehicleData();

    // Check if data has changed (simple comparison by count for now)
    bool bDataChanged = (NewVehicleData.Num() != CachedVehicleData.Num());
    
    if (!bDataChanged)
    {
        // More detailed comparison could be added here
        // For now, we update anyway to ensure fresh data
        bDataChanged = true;
    }

    if (bDataChanged)
    {
        CachedVehicleData = NewVehicleData;
        
        // Call Blueprint event with new data
        OnVehicleDataUpdated(CachedVehicleData);

        UE_LOG(LogRealGazeboUI, VeryVerbose, TEXT("RealGazeboMainWidget: Vehicle data updated - %d vehicles"), CachedVehicleData.Num());
    }
}

void URealGazeboMainWidget::RefreshVehicleList()
{
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: Refreshing vehicle list"));
    
    // Force update vehicle data
    UpdateVehicleData();

    // Child widgets removed - simplified approach
}

void URealGazeboMainWidget::SetUpdateFrequency(float Frequency)
{
    UpdateFrequency = FMath::Clamp(Frequency, 1.0f, 60.0f);
    
    // Restart timer with new frequency
    SetupUpdateTimer();
    
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: Update frequency set to %.1f Hz"), UpdateFrequency);
}

void URealGazeboMainWidget::SelectVehicle(const FVehicleID& VehicleID)
{
    if (!UISubsystem.IsValid())
    {
        return;
    }

    SelectedVehicleID = VehicleID;
    UISubsystem->SelectVehicle(VehicleID);

    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: Vehicle selected - %s"), *VehicleID.ToString());
}

void URealGazeboMainWidget::SelectVehicleByName(const FString& DisplayName)
{
    if (!UISubsystem.IsValid())
    {
        return;
    }

    UISubsystem->SelectVehicleByName(DisplayName);

    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: Vehicle selected by name - %s"), *DisplayName);
}

void URealGazeboMainWidget::ClearVehicleSelection()
{
    if (!UISubsystem.IsValid())
    {
        return;
    }

    SelectedVehicleID = FVehicleID();
    UISubsystem->ClearVehicleSelection();

    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: Vehicle selection cleared"));
}

bool URealGazeboMainWidget::GetSelectedVehicleData(FUIVehicleData& OutVehicleData) const
{
    if (!UISubsystem.IsValid())
    {
        return false;
    }

    return UISubsystem->GetSelectedVehicle(OutVehicleData);
}

void URealGazeboMainWidget::SwitchCameraMode(ERealGazeboCameraMode NewMode)
{
    if (!UISubsystem.IsValid())
    {
        return;
    }

    CurrentCameraMode = NewMode;
    UISubsystem->SetCameraMode(NewMode);

    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: Camera mode switched to %d"), (int32)NewMode);
}

ERealGazeboCameraMode URealGazeboMainWidget::GetCurrentCameraMode() const
{
    if (UISubsystem.IsValid())
    {
        return UISubsystem->GetCurrentCameraMode();
    }

    return CurrentCameraMode;
}

TArray<FUIVehicleData> URealGazeboMainWidget::GetAllVehicleData() const
{
    if (UISubsystem.IsValid())
    {
        return UISubsystem->GetAllVehicleData();
    }

    return CachedVehicleData;
}

int32 URealGazeboMainWidget::GetVehicleCount() const
{
    return GetAllVehicleData().Num();
}

bool URealGazeboMainWidget::HasActiveVehicles() const
{
    return GetVehicleCount() > 0;
}

void URealGazeboMainWidget::InitializeUISubsystem()
{
    UISubsystem = URealGazeboUISubsystem::GetUISubsystem(this);
    
    if (!UISubsystem.IsValid())
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("RealGazeboMainWidget: Could not get UI subsystem"));
        return;
    }

    // Bind to subsystem events
    UISubsystem->OnVehicleSelected.AddDynamic(this, &URealGazeboMainWidget::HandleVehicleSelectionChanged);
    UISubsystem->OnCameraModeChanged.AddDynamic(this, &URealGazeboMainWidget::HandleCameraModeChanged);
    UISubsystem->OnUIToggled.AddDynamic(this, &URealGazeboMainWidget::HandleUIToggled);

    // Get initial camera mode
    CurrentCameraMode = UISubsystem->GetCurrentCameraMode();

    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: UI subsystem initialized"));
}

void URealGazeboMainWidget::SetupUpdateTimer()
{
    if (!GetWorld())
    {
        return;
    }

    // Clear existing timer
    GetWorld()->GetTimerManager().ClearTimer(UpdateTimerHandle);

    // Setup new timer
    float TimerInterval = 1.0f / UpdateFrequency;
    GetWorld()->GetTimerManager().SetTimer(
        UpdateTimerHandle,
        this,
        &URealGazeboMainWidget::HandlePeriodicUpdate,
        TimerInterval,
        true // Loop
    );

    // Setup auto-refresh timer if enabled
    if (bAutoRefreshVehicleList)
    {
        GetWorld()->GetTimerManager().ClearTimer(AutoRefreshTimerHandle);
        GetWorld()->GetTimerManager().SetTimer(
            AutoRefreshTimerHandle,
            this,
            &URealGazeboMainWidget::HandleAutoRefresh,
            AutoRefreshInterval,
            true // Loop
        );
    }

    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: Update timer setup - %.1f Hz"), UpdateFrequency);
}

void URealGazeboMainWidget::HandlePeriodicUpdate()
{
    // Update vehicle data periodically
    UpdateVehicleData();
}

void URealGazeboMainWidget::HandleAutoRefresh()
{
    // Auto-refresh vehicle list
    if (bAutoRefreshVehicleList)
    {
        RefreshVehicleList();
    }
}

void URealGazeboMainWidget::HandleVehicleSelectionChanged(const FUIVehicleData& SelectedVehicle)
{
    SelectedVehicleID = SelectedVehicle.VehicleID;
    
    // Call Blueprint event
    OnVehicleSelected(SelectedVehicle);

    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: Received vehicle selection change - %s"), *SelectedVehicle.DisplayName);
}

void URealGazeboMainWidget::HandleCameraModeChanged(ERealGazeboCameraMode NewMode)
{
    CurrentCameraMode = NewMode;
    
    // Call Blueprint event
    OnCameraModeChanged(NewMode);

    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: Received camera mode change - %d"), (int32)NewMode);
}

void URealGazeboMainWidget::HandleUIToggled()
{
    // Handle UI visibility toggle
    // Implementation depends on how the UI should respond to toggle
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("RealGazeboMainWidget: UI toggle event received"));
}