#pragma once

#include "CoreMinimal.h"
#include "Core/GazeboBridgeTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RealGazeboBridgeBlueprintLibrary.generated.h"

/** Blueprint utilities for RealGazebo Bridge data types. */
UCLASS()
class REALGAZEBOBRIDGE_API URealGazeboBridgeBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Returns whether two vehicle IDs are equal. */
    UFUNCTION(
        BlueprintPure,
        Category = "RealGazebo|Vehicle ID",
        meta = (
            DisplayName = "Equal (Vehicle ID)",
            CompactNodeTitle = "==",
            ScriptOperator = "==",
            Keywords = "== equal",
            BlueprintThreadSafe))
    static bool EqualEqual_VehicleIDVehicleID(
        const FVehicleID& A,
        const FVehicleID& B);

    /** Returns whether two vehicle IDs are different. */
    UFUNCTION(
        BlueprintPure,
        Category = "RealGazebo|Vehicle ID",
        meta = (
            DisplayName = "Not Equal (Vehicle ID)",
            CompactNodeTitle = "!=",
            ScriptOperator = "!=",
            Keywords = "!= not equal",
            BlueprintThreadSafe))
    static bool NotEqual_VehicleIDVehicleID(
        const FVehicleID& A,
        const FVehicleID& B);
};
