#include "Core/UIVehicleDataInterface.h"
#include "Core/GazeboBridgeSubsystem.h"
#include "Vehicles/VehicleBasePawn.h"
#include "RealGazeboUI.h"
#include "Engine/World.h"

UUIVehicleDataInterface::UUIVehicleDataInterface()
{
    BridgeSubsystem = nullptr;
    LastRefreshTime = 0.0f;
}

void UUIVehicleDataInterface::Initialize(UGazeboBridgeSubsystem* InBridgeSubsystem)
{
    if (!InBridgeSubsystem)
    {
        UE_LOG(LogRealGazeboUI, Error, TEXT("UIVehicleDataInterface: Cannot initialize with null bridge subsystem"));
        return;
    }

    BridgeSubsystem = InBridgeSubsystem;
    UE_LOG(LogRealGazeboUI, Display, TEXT("UIVehicleDataInterface: Initialized successfully"));
    
    // Initial data refresh
    RefreshVehicleData();
}

TArray<FUIVehicleData> UUIVehicleDataInterface::GetAllVehicleData()
{
    RefreshVehicleData();
    return CachedVehicleData;
}

bool UUIVehicleDataInterface::GetVehicleData(const FVehicleID& VehicleID, FUIVehicleData& OutVehicleData)
{
    RefreshVehicleData();
    
    for (const FUIVehicleData& VehicleData : CachedVehicleData)
    {
        if (VehicleData.VehicleID.VehicleNum == VehicleID.VehicleNum && 
            VehicleData.VehicleID.VehicleType == VehicleID.VehicleType)
        {
            OutVehicleData = VehicleData;
            return true;
        }
    }
    
    return false;
}

bool UUIVehicleDataInterface::GetVehicleDataByName(const FString& DisplayName, FUIVehicleData& OutVehicleData)
{
    RefreshVehicleData();
    
    for (const FUIVehicleData& VehicleData : CachedVehicleData)
    {
        if (VehicleData.DisplayName.Equals(DisplayName, ESearchCase::IgnoreCase))
        {
            OutVehicleData = VehicleData;
            return true;
        }
    }
    
    return false;
}

AVehicleBasePawn* UUIVehicleDataInterface::GetVehiclePawn(const FVehicleID& VehicleID)
{
    if (!BridgeSubsystem.IsValid())
    {
        return nullptr;
    }

    FVehicleRuntimeData RuntimeData = BridgeSubsystem->GetVehicleData(VehicleID);
    return RuntimeData.VisualPawn.Get();
}

int32 UUIVehicleDataInterface::GetActiveVehicleCount() const
{
    return CachedVehicleData.Num();
}

bool UUIVehicleDataInterface::IsVehicleActive(const FVehicleID& VehicleID) const
{
    for (const FUIVehicleData& VehicleData : CachedVehicleData)
    {
        if (VehicleData.VehicleID.VehicleNum == VehicleID.VehicleNum && 
            VehicleData.VehicleID.VehicleType == VehicleID.VehicleType)
        {
            return VehicleData.bIsActive;
        }
    }
    
    return false;
}

void UUIVehicleDataInterface::RefreshVehicleData()
{
    if (!BridgeSubsystem.IsValid())
    {
        return;
    }

    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    
    // Check if we need to refresh based on interval
    if (CurrentTime - LastRefreshTime < RefreshInterval)
    {
        return;
    }

    // Get all vehicle IDs from bridge
    TArray<FVehicleID> AllVehicleIDs = BridgeSubsystem->GetAllVehicleIDs();
    
    // Clear cached data
    CachedVehicleData.Empty(AllVehicleIDs.Num());
    
    // Convert each vehicle to UI format
    for (const FVehicleID& VehicleID : AllVehicleIDs)
    {
        FVehicleRuntimeData RuntimeData = BridgeSubsystem->GetVehicleData(VehicleID);
        
        // Only include active vehicles
        if (RuntimeData.VisualPawn.IsValid())
        {
            FUIVehicleData UIData = ConvertToUIData(VehicleID, RuntimeData);
            CachedVehicleData.Add(UIData);
        }
    }
    
    LastRefreshTime = CurrentTime;
    
    UE_LOG(LogRealGazeboUI, VeryVerbose, TEXT("UIVehicleDataInterface: Refreshed %d vehicle data entries"), CachedVehicleData.Num());
}

FUIVehicleData UUIVehicleDataInterface::ConvertToUIData(const FVehicleID& VehicleID, const FVehicleRuntimeData& RuntimeData)
{
    FUIVehicleData UIData;
    
    UIData.VehicleID = VehicleID;
    UIData.DisplayName = GenerateDisplayName(VehicleID, VehicleID.VehicleType);
    UIData.VehicleTypeName = GetVehicleTypeName(VehicleID.VehicleType);
    UIData.Position = RuntimeData.Position;
    UIData.Rotation = RuntimeData.Rotation.Rotator();
    UIData.BatteryLevel = CalculateBatteryLevel(RuntimeData);
    UIData.Status = GenerateStatusString(RuntimeData);
    UIData.bIsActive = RuntimeData.VisualPawn.IsValid();
    UIData.bIsSelected = false; // Selection state managed by UI
    UIData.VehiclePawn = RuntimeData.VisualPawn;
    UIData.LastUpdateTime = RuntimeData.LastUpdateTime;
    
    return UIData;
}

FString UUIVehicleDataInterface::GenerateDisplayName(const FVehicleID& VehicleID, uint8 VehicleType)
{
    FString VehicleTypeName = GetVehicleTypeName(VehicleType);
    if (VehicleTypeName.IsEmpty())
    {
        VehicleTypeName = FString::Printf(TEXT("Vehicle%d"), VehicleType);
    }
    
    return FString::Printf(TEXT("%s_%d"), *VehicleTypeName.ToLower(), VehicleID.VehicleNum);
}

FString UUIVehicleDataInterface::GetVehicleTypeName(uint8 VehicleType)
{
    if (!BridgeSubsystem.IsValid())
    {
        return TEXT("Unknown");
    }

    FBridgeVehicleConfigRow ConfigRow;
    if (BridgeSubsystem->GetVehicleConfig(VehicleType, ConfigRow))
    {
        return ConfigRow.VehicleName;
    }
    
    return FString::Printf(TEXT("Type_%d"), VehicleType);
}

float UUIVehicleDataInterface::CalculateBatteryLevel(const FVehicleRuntimeData& RuntimeData)
{
    // Placeholder implementation - replace with actual battery data when available
    // For now, simulate battery based on time and vehicle activity
    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const float TimeSinceUpdate = CurrentTime - RuntimeData.LastUpdateTime;
    
    // If vehicle hasn't been updated recently, assume low battery
    if (TimeSinceUpdate > 5.0f)
    {
        return FMath::Max(0.1f, 1.0f - (TimeSinceUpdate / 30.0f));
    }
    
    // Simulate realistic battery level between 0.2 and 1.0
    return FMath::FRandRange(0.7f, 1.0f);
}

FString UUIVehicleDataInterface::GenerateStatusString(const FVehicleRuntimeData& RuntimeData)
{
    if (!RuntimeData.VisualPawn.IsValid())
    {
        return TEXT("Inactive");
    }
    
    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const float TimeSinceUpdate = CurrentTime - RuntimeData.LastUpdateTime;
    
    if (TimeSinceUpdate > 2.0f)
    {
        return TEXT("Connection Lost");
    }
    else if (TimeSinceUpdate > 1.0f)
    {
        return TEXT("Weak Signal");
    }
    else
    {
        // Check if vehicle is moving
        const float Speed = RuntimeData.Position.Size();
        if (Speed > 10.0f)
        {
            return TEXT("Moving");
        }
        else
        {
            return TEXT("Active");
        }
    }
}