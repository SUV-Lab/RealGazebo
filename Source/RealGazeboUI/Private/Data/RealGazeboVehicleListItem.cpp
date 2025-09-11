#include "Data/RealGazeboVehicleListItem.h"
#include "Engine/Engine.h"
#include "RealGazeboUI.h"

URealGazeboVehicleListItem::URealGazeboVehicleListItem()
{
    InitializeDefaults();
}

void URealGazeboVehicleListItem::InitializeDefaults()
{
    VehicleID = FVehicleID(0, 0);
    VehicleName = TEXT("Unknown Vehicle");
    VehicleTypeName = TEXT("Unknown");
    Position = FVector::ZeroVector;
    Rotation = FRotator::ZeroRotator;
    
    // Placeholder values for future telemetry
    BatteryPercentage = 100.0f;
    Status = TEXT("No Data");
    
    LastUpdateTime = FPlatformTime::Seconds();
}

void URealGazeboVehicleListItem::UpdateFromRuntimeData(const FVehicleRuntimeData& RuntimeData)
{
    // Update core transform data
    Position = RuntimeData.Position;
    Rotation = RuntimeData.Rotation.Rotator();
    
    // Update timing
    LastUpdateTime = FPlatformTime::Seconds();
    
    // Keep status as placeholder
    Status = TEXT("No Data");
    
    UE_LOG(LogRealGazeboUI, Verbose, TEXT("Updated vehicle %s: Position=%s, Status=%s"), 
           *VehicleName, *Position.ToString(), *Status);
}

void URealGazeboVehicleListItem::UpdateTransform(const FVector& NewPosition, const FRotator& NewRotation)
{
    Position = NewPosition;
    Rotation = NewRotation;
    LastUpdateTime = FPlatformTime::Seconds();
    
    // Keep status as placeholder
    Status = TEXT("No Data");
}


FString URealGazeboVehicleListItem::GetFormattedPosition() const
{
    // Position is in Unreal Engine coordinates (centimeters)
    return FString::Printf(TEXT("%.1f, %.1f, %.1f"), Position.X, Position.Y, Position.Z);
}

FString URealGazeboVehicleListItem::GetFormattedStatus() const
{
    if (BatteryPercentage < 100.0f)
    {
        return FString::Printf(TEXT("%s | Battery: %.0f%%"), *Status, BatteryPercentage);
    }
    return Status;
}

