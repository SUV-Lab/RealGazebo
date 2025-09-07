#include "Core/ListManager.h"
#include "RealGazeboUI.h"
#include "Engine/Engine.h"
#include "HAL/PlatformProcess.h"

UListManager::UListManager()
{
    SelectedVehicleName = TEXT("");
    LastRefreshTime = 0.0;
    
    // Initialize entry bridge
    InitializeEntryBridge();
}

TArray<FCtrackEntryData> UListManager::GetVehicleList()
{
    if (NeedsCacheRefresh())
    {
        UpdateCachedVehicleList();
    }

    return CachedVehicleList;
}

TArray<FCtrackEntryData> UListManager::GetVehicleListByType(const FString& VehicleType)
{
    TArray<FCtrackEntryData> AllVehicles = GetVehicleList();
    TArray<FCtrackEntryData> FilteredVehicles;

    for (const FCtrackEntryData& VehicleData : AllVehicles)
    {
        if (VehicleData.VehicleType.Equals(VehicleType, ESearchCase::IgnoreCase))
        {
            FilteredVehicles.Add(VehicleData);
        }
    }

    return FilteredVehicles;
}

TArray<FCtrackEntryData> UListManager::GetActiveVehicleList()
{
    TArray<FCtrackEntryData> AllVehicles = GetVehicleList();
    TArray<FCtrackEntryData> ActiveVehicles;

    for (const FCtrackEntryData& VehicleData : AllVehicles)
    {
        if (VehicleData.bIsActive)
        {
            ActiveVehicles.Add(VehicleData);
        }
    }

    return ActiveVehicles;
}

bool UListManager::SelectVehicle(const FString& VehicleName)
{
    if (!EntryBridge)
    {
        InitializeEntryBridge();
        if (!EntryBridge)
        {
            UE_LOG(LogRealGazeboUI, Warning, TEXT("ListManager: Cannot select vehicle - EntryBridge not available"));
            return false;
        }
    }

    // Check if vehicle exists
    if (EntryBridge->IsVehicleActive(VehicleName))
    {
        SelectedVehicleName = VehicleName;
        UE_LOG(LogRealGazeboUI, Log, TEXT("ListManager: Selected vehicle '%s'"), *VehicleName);
        return true;
    }

    UE_LOG(LogRealGazeboUI, Warning, TEXT("ListManager: Vehicle '%s' not found or inactive"), *VehicleName);
    return false;
}

bool UListManager::SelectVehicleByID(int32 VehicleID)
{
    TArray<FCtrackEntryData> AllVehicles = GetVehicleList();
    
    for (const FCtrackEntryData& VehicleData : AllVehicles)
    {
        if (VehicleData.VehicleID == VehicleID)
        {
            return SelectVehicle(VehicleData.VehicleName);
        }
    }

    UE_LOG(LogRealGazeboUI, Warning, TEXT("ListManager: Vehicle with ID %d not found"), VehicleID);
    return false;
}

FCtrackEntryData UListManager::GetSelectedVehicle()
{
    if (SelectedVehicleName.IsEmpty() || !EntryBridge)
    {
        return FCtrackEntryData();
    }

    return EntryBridge->GetVehicleAsCtrackEntry(SelectedVehicleName);
}

bool UListManager::HasSelectedVehicle()
{
    return !SelectedVehicleName.IsEmpty();
}

void UListManager::ClearSelection()
{
    SelectedVehicleName = TEXT("");
    UE_LOG(LogRealGazeboUI, Log, TEXT("ListManager: Selection cleared"));
}

FString UListManager::GetSelectedVehicleName()
{
    return SelectedVehicleName;
}

void UListManager::RefreshVehicleList()
{
    UpdateCachedVehicleList();
}

TArray<FString> UListManager::GetAvailableVehicleTypes()
{
    TArray<FCtrackEntryData> AllVehicles = GetVehicleList();
    TArray<FString> VehicleTypes;

    for (const FCtrackEntryData& VehicleData : AllVehicles)
    {
        VehicleTypes.AddUnique(VehicleData.VehicleType);
    }

    return VehicleTypes;
}

int32 UListManager::GetVehicleCountByType(const FString& VehicleType)
{
    return GetVehicleListByType(VehicleType).Num();
}

void UListManager::InitializeEntryBridge()
{
    if (EntryBridge)
    {
        return;
    }

    EntryBridge = NewObject<UEntryDataBridge>(this, TEXT("EntryDataBridge"));
    
    if (!EntryBridge)
    {
        UE_LOG(LogRealGazeboUI, Error, TEXT("ListManager: Failed to create EntryDataBridge"));
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("ListManager: EntryDataBridge initialized"));
    }
}

bool UListManager::NeedsCacheRefresh() const
{
    double CurrentTime = FPlatformTime::Seconds();
    return (CurrentTime - LastRefreshTime) >= RefreshInterval;
}

void UListManager::UpdateCachedVehicleList()
{
    if (!EntryBridge)
    {
        InitializeEntryBridge();
        if (!EntryBridge)
        {
            return;
        }
    }

    CachedVehicleList = EntryBridge->GetAllVehiclesAsCtrackEntries();
    LastRefreshTime = FPlatformTime::Seconds();

    UE_LOG(LogRealGazeboUI, Verbose, TEXT("ListManager: Updated vehicle list cache - %d vehicles"), CachedVehicleList.Num());
}