#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/EntryDataBridge.h"
#include "ListManager.generated.h"

/**
 * ListManager - Simple vehicle list management for CtrackEntry UI
 * Handles vehicle selection, filtering, and list updates
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API UListManager : public UObject
{
    GENERATED_BODY()

public:
    UListManager();

    /** Get all vehicles as list entries */
    UFUNCTION(BlueprintCallable, Category = "List Manager")
    TArray<FCtrackEntryData> GetVehicleList();

    /** Get filtered vehicle list by type */
    UFUNCTION(BlueprintCallable, Category = "List Manager")
    TArray<FCtrackEntryData> GetVehicleListByType(const FString& VehicleType);

    /** Get only active vehicles */
    UFUNCTION(BlueprintCallable, Category = "List Manager")
    TArray<FCtrackEntryData> GetActiveVehicleList();

    /** Select vehicle by name */
    UFUNCTION(BlueprintCallable, Category = "List Manager")
    bool SelectVehicle(const FString& VehicleName);

    /** Select vehicle by ID */
    UFUNCTION(BlueprintCallable, Category = "List Manager")
    bool SelectVehicleByID(int32 VehicleID);

    /** Get currently selected vehicle */
    UFUNCTION(BlueprintCallable, Category = "List Manager")
    FCtrackEntryData GetSelectedVehicle();

    /** Check if a vehicle is selected */
    UFUNCTION(BlueprintCallable, Category = "List Manager")
    bool HasSelectedVehicle();

    /** Clear vehicle selection */
    UFUNCTION(BlueprintCallable, Category = "List Manager")
    void ClearSelection();

    /** Get selected vehicle name */
    UFUNCTION(BlueprintCallable, Category = "List Manager")
    FString GetSelectedVehicleName();

    /** Refresh vehicle list data */
    UFUNCTION(BlueprintCallable, Category = "List Manager")
    void RefreshVehicleList();

    /** Get vehicle types available */
    UFUNCTION(BlueprintCallable, Category = "List Manager")
    TArray<FString> GetAvailableVehicleTypes();

    /** Get vehicle count by type */
    UFUNCTION(BlueprintCallable, Category = "List Manager")
    int32 GetVehicleCountByType(const FString& VehicleType);

private:
    /** Entry data bridge for data conversion */
    UPROPERTY()
    TObjectPtr<UEntryDataBridge> EntryBridge;

    /** Currently selected vehicle name */
    UPROPERTY()
    FString SelectedVehicleName;

    /** Cached vehicle list for performance */
    UPROPERTY()
    TArray<FCtrackEntryData> CachedVehicleList;

    /** Last refresh time */
    double LastRefreshTime;

    /** Refresh interval in seconds */
    static constexpr double RefreshInterval = 0.1; // 10 Hz

    /** Initialize entry bridge */
    void InitializeEntryBridge();

    /** Check if cache needs refresh */
    bool NeedsCacheRefresh() const;

    /** Update cached vehicle list */
    void UpdateCachedVehicleList();
};