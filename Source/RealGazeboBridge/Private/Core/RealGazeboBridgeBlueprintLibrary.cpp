#include "Core/RealGazeboBridgeBlueprintLibrary.h"

bool URealGazeboBridgeBlueprintLibrary::EqualEqual_VehicleIDVehicleID(
    const FVehicleID& A,
    const FVehicleID& B)
{
    return A == B;
}

bool URealGazeboBridgeBlueprintLibrary::NotEqual_VehicleIDVehicleID(
    const FVehicleID& A,
    const FVehicleID& B)
{
    return A != B;
}
