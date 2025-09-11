#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Engine/Texture2D.h"
#include "Data/RealGazeboVehicleListItem.h"
#include "RealGazeboVehicleEntry.generated.h"

/**
 * Individual vehicle entry widget for the ListView
 * 
 * Displays:
 * - Vehicle icon and name
 * - Real-time position
 * - Battery level (placeholder)
 * - Connection status
 * - Visual status indicators
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API URealGazeboVehicleEntry : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    URealGazeboVehicleEntry(const FObjectInitializer& ObjectInitializer);

protected:
    //----------------------------------------------------------
    // Widget Lifecycle
    //----------------------------------------------------------

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    //----------------------------------------------------------
    // IUserObjectListEntry Interface
    //----------------------------------------------------------

    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    virtual void NativeOnItemSelectionChanged(bool bIsSelectedParam) override;

    //----------------------------------------------------------
    // UI Components (Blueprint Bindable)
    //----------------------------------------------------------

    /** Drone name display, Non-folding panel */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DroneNameText;

    /** Battery percentage text, Non-folding panel */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> NonFoldingBatteryText;

    /** Vehicle ID display, Folding panel */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> VehicleID;

    /** Position display, Folding panel */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PositionText;

    /** Battery percentage text, Folding panel */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> BatteryText;

    /** Status text display, Folding panel */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

public:
    //----------------------------------------------------------
    // Configuration
    //----------------------------------------------------------

    /** Update frequency for real-time data (Hz) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float UpdateFrequency = 10.0f;


    //----------------------------------------------------------
    // Data Access
    //----------------------------------------------------------

    /** Get the associated vehicle list item */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Entry")
    URealGazeboVehicleListItem* GetVehicleListItem() const { return VehicleListItem; }

    /** Check if this entry is currently selected */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Entry")
    bool IsEntrySelected() const { return bIsSelected; }

    /** Get vehicle ID */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Entry")
    FVehicleID GetVehicleID() const;

    //----------------------------------------------------------
    // Manual Updates
    //----------------------------------------------------------

    /** Force refresh the entry display */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Entry")
    void RefreshDisplay();

    //----------------------------------------------------------
    // Events (Blueprint Implementable)
    //----------------------------------------------------------

    /** Called when the vehicle data changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnVehicleDataChanged();

    /** Called when selection state changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnSelectionStateChanged(bool bSelected);

    /** Called when vehicle status changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnStatusChanged(const FString& NewStatus, FLinearColor StatusColor);

protected:
    //----------------------------------------------------------
    // Internal Data
    //----------------------------------------------------------

    /** Reference to the vehicle list item data */
    UPROPERTY()
    TObjectPtr<URealGazeboVehicleListItem> VehicleListItem;

    /** Current selection state */
    bool bIsSelected;

    /** Timer for regular updates */
    FTimerHandle UpdateTimerHandle;


    //----------------------------------------------------------
    // Internal Methods
    //----------------------------------------------------------

    /** Update all display elements from vehicle data */
    void UpdateDisplayFromData();

    /** Update drone name display (non-folding panel) */
    void UpdateDroneNameDisplay();

    /** Update non-folding battery display */
    void UpdateNonFoldingBatteryDisplay();

    /** Update vehicle ID display */
    void UpdateVehicleIDDisplay();

    /** Update position text display */
    void UpdatePositionText();

    /** Update battery display */
    void UpdateBatteryDisplay();

    /** Update status display */
    void UpdateStatusDisplay();

    /** Format position for display */
    FString FormatPositionText(const FVector& Position) const;

    //----------------------------------------------------------
    // Event Handlers
    //----------------------------------------------------------

    /** Handle regular data updates */
    void OnUpdateTimer();

    //----------------------------------------------------------
    // Utility Methods
    //----------------------------------------------------------

    /** Start the update timer */
    void StartUpdateTimer();

    /** Stop the update timer */
    void StopUpdateTimer();

};