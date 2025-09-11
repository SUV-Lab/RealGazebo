#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Engine/World.h"
#include "GazeboBridgeTypes.h"
#include "GazeboBridgeSubsystem.h"
#include "Data/RealGazeboVehicleListItem.h"
#include "RealGazeboMainWidget.generated.h"

// Forward declarations
class UGazeboBridgeSubsystem;

/**
 * Main RealGazebo monitoring widget with vehicle ListView
 * 
 * Features:
 * - Real-time vehicle monitoring with ListView
 * - Connection status and statistics
 * - Dynamic vehicle addition/removal
 * - Performance optimized updates
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API URealGazeboMainWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    URealGazeboMainWidget(const FObjectInitializer& ObjectInitializer);

protected:
    //----------------------------------------------------------
    // Widget Lifecycle
    //----------------------------------------------------------

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    //----------------------------------------------------------
    // UI Components (Blueprint Bindable)
    //----------------------------------------------------------

    /** Main vehicle ListView */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UListView> VehicleListView;





public:
    //----------------------------------------------------------
    // Configuration
    //----------------------------------------------------------

    /** Update frequency for vehicle data refresh (Hz) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float UpdateFrequency = 30.0f;

    /** Maximum number of vehicles to display */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    int32 MaxDisplayVehicles = 256;


    //----------------------------------------------------------
    // Vehicle Management
    //----------------------------------------------------------

    /** Get all current vehicle list items */
    UFUNCTION(BlueprintCallable, Category = "Vehicles")
    TArray<URealGazeboVehicleListItem*> GetAllVehicleItems() const;

    /** Get vehicle item by ID */
    UFUNCTION(BlueprintCallable, Category = "Vehicles")
    URealGazeboVehicleListItem* GetVehicleItem(const FVehicleID& VehicleID) const;

    /** Get selected vehicle items */
    UFUNCTION(BlueprintCallable, Category = "Vehicles")
    TArray<URealGazeboVehicleListItem*> GetSelectedVehicleItems() const;

    /** Clear all vehicles from the list */
    UFUNCTION(BlueprintCallable, Category = "Vehicles")
    void ClearAllVehicles();

    /** Refresh vehicle list from subsystem */
    UFUNCTION(BlueprintCallable, Category = "Vehicles")
    void RefreshVehicleList();

    //----------------------------------------------------------
    // Events (Blueprint Implementable)
    //----------------------------------------------------------

    /** Called when a vehicle is selected in the list */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnVehicleSelected(URealGazeboVehicleListItem* SelectedVehicle);


    /** Called when the connection status changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnConnectionStatusChanged(bool bIsConnected, const FString& StatusMessage);

protected:
    //----------------------------------------------------------
    // Internal Data
    //----------------------------------------------------------

    /** Reference to the bridge subsystem */
    TWeakObjectPtr<UGazeboBridgeSubsystem> BridgeSubsystem;

    /** Map of vehicle IDs to list items for fast lookup */
    TMap<FVehicleID, TObjectPtr<URealGazeboVehicleListItem>> VehicleItemMap;

    /** Timer handle for regular updates */
    FTimerHandle UpdateTimerHandle;

    /** Last known vehicle count for change detection */
    int32 LastVehicleCount;

    /** Last connection status */
    bool bLastConnectionStatus;

    //----------------------------------------------------------
    // Internal Methods
    //----------------------------------------------------------

    /** Initialize subsystem reference and connections */
    void InitializeSubsystemConnection();

    /** Main update loop - refresh vehicle data */
    void UpdateVehicleData();

    /** Add a new vehicle to the list */
    URealGazeboVehicleListItem* AddVehicleToList(const FVehicleID& VehicleID, const FVehicleRuntimeData& RuntimeData);

    /** Remove a vehicle from the list */
    void RemoveVehicleFromList(const FVehicleID& VehicleID);

    /** Update existing vehicle in the list */
    void UpdateVehicleInList(const FVehicleID& VehicleID, const FVehicleRuntimeData& RuntimeData);


    /** Get vehicle type name from config */
    FString GetVehicleTypeName(uint8 VehicleType) const;

    /** Generate vehicle display name */
    FString GenerateVehicleDisplayName(const FVehicleID& VehicleID, uint8 VehicleType) const;

    //----------------------------------------------------------
    // ListView Event Handlers
    //----------------------------------------------------------

    /** Handle vehicle item selection */
    UFUNCTION()
    void OnVehicleItemSelectionChanged(UObject* SelectedItem);


public:
    //----------------------------------------------------------
    // Static Utility Methods
    //----------------------------------------------------------

    /** Create and show the main widget */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI", meta = (CallInEditor = "true"))
    static URealGazeboMainWidget* CreateMainWidget(UObject* WorldContext);

    /** Get the current main widget instance (if any) */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI")
    static URealGazeboMainWidget* GetMainWidget(UObject* WorldContext);
};