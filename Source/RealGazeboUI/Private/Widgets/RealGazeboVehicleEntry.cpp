#include "Widgets/RealGazeboVehicleEntry.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "RealGazeboUI.h"

URealGazeboVehicleEntry::URealGazeboVehicleEntry(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Set default values
    UpdateFrequency = 10.0f;
    
    // Initialize state
    bIsSelected = false;
    VehicleListItem = nullptr;
}

void URealGazeboVehicleEntry::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("VehicleEntry: Constructing entry widget"));
    
    // Start update timer
    StartUpdateTimer();
    
    // Initial display update
    UpdateDisplayFromData();
}

void URealGazeboVehicleEntry::NativeDestruct()
{
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("VehicleEntry: Destructing entry widget"));
    
    // Stop update timer
    StopUpdateTimer();
    
    Super::NativeDestruct();
}

void URealGazeboVehicleEntry::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // Update display
    if (VehicleListItem)
    {
        UpdateDisplayFromData();
    }
}

void URealGazeboVehicleEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    // Set the vehicle list item data
    VehicleListItem = Cast<URealGazeboVehicleListItem>(ListItemObject);
    
    if (VehicleListItem)
    {
        UE_LOG(LogRealGazeboUI, Verbose, TEXT("VehicleEntry: Set list item for vehicle %s"), *VehicleListItem->VehicleName);
        
        // Update display with new data
        UpdateDisplayFromData();
        
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("VehicleEntry: Invalid list item object"));
    }
}

void URealGazeboVehicleEntry::NativeOnItemSelectionChanged(bool bIsSelectedParam)
{
    this->bIsSelected = bIsSelectedParam;
    
    // Call Blueprint event
    OnSelectionStateChanged(this->bIsSelected);
    
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("VehicleEntry: Selection changed to %s"), this->bIsSelected ? TEXT("Selected") : TEXT("Not Selected"));
}

void URealGazeboVehicleEntry::UpdateDisplayFromData()
{
    if (!VehicleListItem)
    {
        return;
    }
    
    // Update non-folding panel displays
    UpdateDroneNameDisplay();
    UpdateNonFoldingBatteryDisplay();
    
    // Update folding panel displays
    UpdateVehicleIDDisplay();
    UpdatePositionText();
    UpdateBatteryDisplay();
    UpdateStatusDisplay();
    
    // Call Blueprint event
    OnVehicleDataChanged();
}

void URealGazeboVehicleEntry::UpdateDroneNameDisplay()
{
    if (!DroneNameText || !VehicleListItem)
    {
        return;
    }
    
    // Display drone name in format: "Iris_1", "Rover_2", etc.
    FString DroneNameString = FString::Printf(TEXT("%s_%d"), 
                                            *VehicleListItem->VehicleTypeName, 
                                            VehicleListItem->VehicleID.VehicleNum);
    DroneNameText->SetText(FText::FromString(DroneNameString));
}

void URealGazeboVehicleEntry::UpdateNonFoldingBatteryDisplay()
{
    if (!NonFoldingBatteryText || !VehicleListItem)
    {
        return;
    }
    
    // Display same battery info as folding panel
    FString BatteryString = TEXT("null");
    NonFoldingBatteryText->SetText(FText::FromString(BatteryString));
}

void URealGazeboVehicleEntry::UpdateVehicleIDDisplay()
{
    if (!VehicleID || !VehicleListItem)
    {
        return;
    }
    
    // Display only vehicle number: "1", "2", "3", etc.
    FString VehicleIDString = FString::Printf(TEXT("%d"), VehicleListItem->VehicleID.VehicleNum);
    VehicleID->SetText(FText::FromString(VehicleIDString));
}

void URealGazeboVehicleEntry::UpdatePositionText()
{
    if (!PositionText || !VehicleListItem)
    {
        return;
    }
    
    // Show simplified position
    FString PositionString = FString::Printf(TEXT("%.0f, %.0f, %.0f"), 
                                           VehicleListItem->Position.X, 
                                           VehicleListItem->Position.Y, 
                                           VehicleListItem->Position.Z);
    
    PositionText->SetText(FText::FromString(PositionString));
}

void URealGazeboVehicleEntry::UpdateBatteryDisplay()
{
    if (!BatteryText || !VehicleListItem)
    {
        return;
    }
    
    // Display "null" since battery is placeholder for future implementation
    FString BatteryString = TEXT("null");
    BatteryText->SetText(FText::FromString(BatteryString));
}

void URealGazeboVehicleEntry::UpdateStatusDisplay()
{
    if (!StatusText || !VehicleListItem)
    {
        return;
    }
    
    // Display "null" since status is placeholder for future implementation
    FString FormattedStatus = TEXT("null");
    StatusText->SetText(FText::FromString(FormattedStatus));
}


FString URealGazeboVehicleEntry::FormatPositionText(const FVector& Position) const
{
    return FString::Printf(TEXT("X:%.1f Y:%.1f Z:%.1f"), Position.X, Position.Y, Position.Z);
}


void URealGazeboVehicleEntry::OnUpdateTimer()
{
    // This timer-based update is supplemented by NativeTick for responsiveness
    UpdateDisplayFromData();
}

void URealGazeboVehicleEntry::StartUpdateTimer()
{
    if (UWorld* World = GetWorld())
    {
        float UpdateInterval = 1.0f / FMath::Max(UpdateFrequency, 1.0f);
        World->GetTimerManager().SetTimer(
            UpdateTimerHandle,
            this,
            &URealGazeboVehicleEntry::OnUpdateTimer,
            UpdateInterval,
            true
        );
    }
}

void URealGazeboVehicleEntry::StopUpdateTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UpdateTimerHandle);
    }
}


FVehicleID URealGazeboVehicleEntry::GetVehicleID() const
{
    if (VehicleListItem)
    {
        return VehicleListItem->VehicleID;
    }
    return FVehicleID(0, 0);
}

void URealGazeboVehicleEntry::RefreshDisplay()
{
    UpdateDisplayFromData();
}