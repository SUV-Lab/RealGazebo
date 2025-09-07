#include "Widgets/DirectWidgetSetup.h"
#include "Core/RealGazeboUISubsystem.h"
#include "RealGazeboUI.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

UDirectWidgetSetup::UDirectWidgetSetup(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Initialize settings
    UpdateFrequency = 10.0f;
    bEnableDebugLogging = true;

    // Initialize state
    bIsInitialized = false;
}

void UDirectWidgetSetup::NativeConstruct()
{
    Super::NativeConstruct();

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Display, TEXT("DirectWidgetSetup: Starting automatic setup"));
    }

    // Initialize UI subsystem
    InitializeUISubsystem();

    // Setup update timer
    SetupUpdateTimer();

    // Initial data refresh
    RefreshData();

    bIsInitialized = true;

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Display, TEXT("DirectWidgetSetup: Automatic setup completed"));
        UE_LOG(LogRealGazeboUI, Display, TEXT("DirectWidgetSetup: Found %d vehicles"), GetVehicleCount());
    }
}

void UDirectWidgetSetup::NativeDestruct()
{
    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Display, TEXT("DirectWidgetSetup: Cleaning up"));
    }

    // Clear timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UpdateTimerHandle);
    }

    Super::NativeDestruct();
}

void UDirectWidgetSetup::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Additional per-frame logic if needed
}

int32 UDirectWidgetSetup::GetVehicleCount() const
{
    return CachedVehicleData.Num();
}

TArray<FUIVehicleData> UDirectWidgetSetup::GetVehicleData() const
{
    return CachedVehicleData;
}

void UDirectWidgetSetup::RefreshData()
{
    if (!UISubsystem.IsValid())
    {
        return;
    }

    // Get fresh vehicle data
    TArray<FUIVehicleData> NewVehicleData = UISubsystem->GetAllVehicleData();
    
    // Update cached data
    CachedVehicleData = NewVehicleData;

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Verbose, TEXT("DirectWidgetSetup: Refreshed data - %d vehicles"), NewVehicleData.Num());
    }
}

void UDirectWidgetSetup::SwitchCameraMode(ERealGazeboCameraMode NewMode)
{
    if (!UISubsystem.IsValid())
    {
        return;
    }

    UISubsystem->SetCameraMode(NewMode);

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Display, TEXT("DirectWidgetSetup: Camera mode switched to %d"), (int32)NewMode);
    }
}

ERealGazeboCameraMode UDirectWidgetSetup::GetCurrentCameraMode() const
{
    if (UISubsystem.IsValid())
    {
        return UISubsystem->GetCurrentCameraMode();
    }

    return ERealGazeboCameraMode::Manual;
}

FString UDirectWidgetSetup::TestConnection() const
{
    TArray<FString> TestResults;

    // Test 1: Widget initialized
    TestResults.Add(FString::Printf(TEXT("Widget Initialized: %s"), bIsInitialized ? TEXT("Yes") : TEXT("No")));

    // Test 2: UI Subsystem
    if (UISubsystem.IsValid())
    {
        TestResults.Add(TEXT("UI Subsystem: Connected"));
    }
    else
    {
        TestResults.Add(TEXT("UI Subsystem: Not Connected"));
    }

    // Test 3: Vehicle data
    int32 VehicleCount = GetVehicleCount();
    if (VehicleCount > 0)
    {
        TestResults.Add(FString::Printf(TEXT("Vehicles Found: %d"), VehicleCount));
    }
    else
    {
        TestResults.Add(TEXT("Vehicles Found: 0 (check if simulation is running)"));
    }

    // Test 4: Camera system
    ERealGazeboCameraMode CurrentMode = GetCurrentCameraMode();
    TestResults.Add(FString::Printf(TEXT("Camera Mode: %d"), (int32)CurrentMode));

    // Test 5: Update frequency
    TestResults.Add(FString::Printf(TEXT("Update Frequency: %.1f Hz"), UpdateFrequency));

    return FString::Join(TestResults, TEXT("\n"));
}

void UDirectWidgetSetup::InitializeUISubsystem()
{
    UISubsystem = URealGazeboUISubsystem::GetUISubsystem(this);

    if (!UISubsystem.IsValid())
    {
        UE_LOG(LogRealGazeboUI, Error, TEXT("DirectWidgetSetup: Could not get UI subsystem"));
        return;
    }

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Display, TEXT("DirectWidgetSetup: UI subsystem connected"));
    }
}

void UDirectWidgetSetup::UpdateVehicleDataInternal()
{
    RefreshData();
}

void UDirectWidgetSetup::SetupUpdateTimer()
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
        &UDirectWidgetSetup::UpdateVehicleDataInternal,
        TimerInterval,
        true // Loop
    );

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Display, TEXT("DirectWidgetSetup: Update timer started - %.1f Hz"), UpdateFrequency);
    }
}