#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/UIVehicleDataInterface.h"
#include "Camera/RealGazeboCameraTypes.h"
#include "RealGazeboMainWidget.generated.h"

// Forward declarations
class URealGazeboUISubsystem;

/**
 * Main UI widget for RealGazebo vehicle information display
 * Base class for S2R_MainWidget Blueprint
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API URealGazeboMainWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    URealGazeboMainWidget(const FObjectInitializer& ObjectInitializer);

    //----------------------------------------------------------
    // Widget Interface
    //----------------------------------------------------------

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    //----------------------------------------------------------
    // UI Management
    //----------------------------------------------------------

    /** Update all vehicle data displayed in the UI */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI")
    void UpdateVehicleData();

    /** Refresh the vehicle list */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI")
    void RefreshVehicleList();

    /** Set UI update frequency */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI")
    void SetUpdateFrequency(float Frequency);

    /** Get current update frequency */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI")
    float GetUpdateFrequency() const { return UpdateFrequency; }

    //----------------------------------------------------------
    // Vehicle Selection
    //----------------------------------------------------------

    /** Select a vehicle by ID */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Vehicle")
    void SelectVehicle(const FVehicleID& VehicleID);

    /** Select vehicle by display name */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Vehicle")
    void SelectVehicleByName(const FString& DisplayName);

    /** Clear vehicle selection */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Vehicle")
    void ClearVehicleSelection();

    /** Get currently selected vehicle data */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Vehicle")
    bool GetSelectedVehicleData(FUIVehicleData& OutVehicleData) const;

    //----------------------------------------------------------
    // Camera Control Integration
    //----------------------------------------------------------

    /** Switch camera mode */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Camera")
    void SwitchCameraMode(ERealGazeboCameraMode NewMode);

    /** Get current camera mode */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Camera")
    ERealGazeboCameraMode GetCurrentCameraMode() const;

    //----------------------------------------------------------
    // Settings
    //----------------------------------------------------------

    /** UI update frequency in Hz */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo UI|Settings", meta = (ClampMin = "1.0", ClampMax = "60.0"))
    float UpdateFrequency = 10.0f;

    /** Auto-refresh vehicle list */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo UI|Settings")
    bool bAutoRefreshVehicleList = true;

    /** Auto-refresh interval (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo UI|Settings", meta = (EditCondition = "bAutoRefreshVehicleList", ClampMin = "0.5", ClampMax = "10.0"))
    float AutoRefreshInterval = 2.0f;

    //----------------------------------------------------------
    // Widget References (Set in Blueprint)
    //----------------------------------------------------------


    //----------------------------------------------------------
    // Blueprint Events
    //----------------------------------------------------------

    /** Called when vehicle data is updated */
    UFUNCTION(BlueprintImplementableEvent, Category = "RealGazebo UI|Events")
    void OnVehicleDataUpdated(const TArray<FUIVehicleData>& VehicleData);

    /** Called when a vehicle is selected */
    UFUNCTION(BlueprintImplementableEvent, Category = "RealGazebo UI|Events")
    void OnVehicleSelected(const FUIVehicleData& SelectedVehicle);

    /** Called when vehicle selection is cleared */
    UFUNCTION(BlueprintImplementableEvent, Category = "RealGazebo UI|Events")
    void OnVehicleSelectionCleared();

    /** Called when camera mode changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "RealGazebo UI|Events")
    void OnCameraModeChanged(ERealGazeboCameraMode NewMode);

    /** Called when UI is initialized */
    UFUNCTION(BlueprintImplementableEvent, Category = "RealGazebo UI|Events")
    void OnUIInitialized();

    //----------------------------------------------------------
    // Data Access
    //----------------------------------------------------------

    /** Get all current vehicle data */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Data")
    TArray<FUIVehicleData> GetAllVehicleData() const;

    /** Get vehicle count */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Data")
    int32 GetVehicleCount() const;

    /** Check if any vehicles are active */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Data")
    bool HasActiveVehicles() const;

protected:
    //----------------------------------------------------------
    // Internal Updates
    //----------------------------------------------------------

    /** Initialize UI subsystem reference */
    void InitializeUISubsystem();

    /** Setup update timer */
    void SetupUpdateTimer();

    /** Handle periodic updates */
    UFUNCTION()
    void HandlePeriodicUpdate();

    /** Handle auto refresh */
    UFUNCTION()
    void HandleAutoRefresh();

    //----------------------------------------------------------
    // Event Handlers
    //----------------------------------------------------------

    /** Handle vehicle selection from subsystem */
    UFUNCTION()
    void HandleVehicleSelectionChanged(const FUIVehicleData& SelectedVehicle);

    /** Handle camera mode change from subsystem */
    UFUNCTION()
    void HandleCameraModeChanged(ERealGazeboCameraMode NewMode);

    /** Handle UI toggle from subsystem */
    UFUNCTION()
    void HandleUIToggled();

private:
    /** Reference to UI subsystem */
    UPROPERTY()
    TWeakObjectPtr<URealGazeboUISubsystem> UISubsystem;

    /** Update timer handle */
    FTimerHandle UpdateTimerHandle;

    /** Auto refresh timer handle */
    FTimerHandle AutoRefreshTimerHandle;

    /** Current vehicle data cache */
    UPROPERTY()
    TArray<FUIVehicleData> CachedVehicleData;

    /** Currently selected vehicle ID */
    FVehicleID SelectedVehicleID;

    /** Current camera mode */
    ERealGazeboCameraMode CurrentCameraMode;

    /** UI initialized flag */
    bool bUIInitialized;
};