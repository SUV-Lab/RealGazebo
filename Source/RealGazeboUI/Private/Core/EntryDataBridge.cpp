#include "Core/EntryDataBridge.h"
#include "Core/RealGazeboUISubsystem.h"
#include "RealGazeboUI.h"
#include "Engine/Engine.h"

UEntryDataBridge::UEntryDataBridge()
{
    // UI subsystem will be initialized lazily when first needed
    // Cannot initialize in constructor due to world context requirements
}

FCtrackEntryData UEntryDataBridge::ConvertVehicleData(const FUIVehicleData& VehicleData)
{
    FCtrackEntryData EntryData;

    // Convert basic data
    EntryData.VehicleName = VehicleData.DisplayName;
    EntryData.VehicleID = VehicleData.VehicleID.VehicleNum; // Use VehicleNum as display ID
    EntryData.VehicleType = VehicleData.VehicleTypeName;
    
    // Convert transform data
    EntryData.Position = VehicleData.Position;
    EntryData.Rotation = VehicleData.Rotation; // Already FRotator
    
    // Set activity status
    EntryData.bIsActive = VehicleData.bIsActive;
    EntryData.StatusText = GenerateStatusText(VehicleData);

    return EntryData;
}

TArray<FCtrackEntryData> UEntryDataBridge::ConvertVehicleDataArray(const TArray<FUIVehicleData>& VehicleDataArray)
{
    TArray<FCtrackEntryData> Result;
    Result.Reserve(VehicleDataArray.Num());

    for (const FUIVehicleData& VehicleData : VehicleDataArray)
    {
        Result.Add(ConvertVehicleData(VehicleData));
    }

    return Result;
}

FCtrackEntryData UEntryDataBridge::GetVehicleAsCtrackEntry(const FString& VehicleName)
{
    if (!UISubsystem.IsValid())
    {
        InitializeUISubsystem();
        if (!UISubsystem.IsValid())
        {
            UE_LOG(LogRealGazeboUI, Warning, TEXT("EntryDataBridge: UI Subsystem not available"));
            return FCtrackEntryData();
        }
    }

    FUIVehicleData VehicleData = UISubsystem->GetVehicleData(VehicleName);
    return ConvertVehicleData(VehicleData);
}

TArray<FCtrackEntryData> UEntryDataBridge::GetAllVehiclesAsCtrackEntries()
{
    if (!UISubsystem.IsValid())
    {
        InitializeUISubsystem();
        if (!UISubsystem.IsValid())
        {
            UE_LOG(LogRealGazeboUI, Warning, TEXT("EntryDataBridge: UI Subsystem not available"));
            return TArray<FCtrackEntryData>();
        }
    }

    TArray<FUIVehicleData> AllVehicleData = UISubsystem->GetAllVehicleData();
    return ConvertVehicleDataArray(AllVehicleData);
}

bool UEntryDataBridge::IsVehicleActive(const FString& VehicleName)
{
    if (!UISubsystem.IsValid())
    {
        InitializeUISubsystem();
        if (!UISubsystem.IsValid())
        {
            return false;
        }
    }

    return UISubsystem->IsVehicleActive(VehicleName);
}

int32 UEntryDataBridge::GetActiveVehicleCount()
{
    if (!UISubsystem.IsValid())
    {
        InitializeUISubsystem();
        if (!UISubsystem.IsValid())
        {
            return 0;
        }
    }

    return UISubsystem->GetActiveVehicleCount();
}

void UEntryDataBridge::InitializeUISubsystem()
{
    if (UISubsystem.IsValid())
    {
        return;
    }

    UISubsystem = URealGazeboUISubsystem::GetUISubsystem(this);

    if (!UISubsystem.IsValid())
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("EntryDataBridge: Could not initialize UI subsystem"));
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("EntryDataBridge: UI subsystem connected"));
    }
}

FString UEntryDataBridge::GenerateStatusText(const FUIVehicleData& VehicleData)
{
    if (!VehicleData.bIsActive)
    {
        return TEXT("Inactive");
    }

    // Create status text based on position and data
    FString StatusText = TEXT("Active");
    
    if (VehicleData.Position.Z > 100.0f)
    {
        StatusText += TEXT(" (Airborne)");
    }
    else if (VehicleData.Position.Z < -50.0f)
    {
        StatusText += TEXT(" (Underwater)");
    }
    else
    {
        StatusText += TEXT(" (Surface)");
    }

    return StatusText;
}