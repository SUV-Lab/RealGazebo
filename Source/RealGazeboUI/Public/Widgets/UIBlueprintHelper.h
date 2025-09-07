#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/EntryDataBridge.h"
#include "Core/ListManager.h"
#include "Camera/UserCameraController.h"
#include "UIBlueprintHelper.generated.h"

/**
 * UIBlueprintHelper - All-in-one Blueprint helper for CtrackEntry integration
 * Simple class that manages all RealGazebo UI functionality in one place
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API UUIBlueprintHelper : public UUserWidget
{
    GENERATED_BODY()

public:
    UUIBlueprintHelper(const FObjectInitializer& ObjectInitializer);

    /** Widget lifecycle */
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    //----------------------------------------------------------
    // Vehicle Data Functions (for CtrackEntry)
    //----------------------------------------------------------

    /** Get all vehicles as CtrackEntry data */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    TArray<FCtrackEntryData> GetVehicleEntries();

    /** Get active vehicles only */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    TArray<FCtrackEntryData> GetActiveVehicleEntries();

    /** Get vehicles by type */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    TArray<FCtrackEntryData> GetVehicleEntriesByType(const FString& VehicleType);

    /** Get vehicle count */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    int32 GetVehicleCount();

    //----------------------------------------------------------
    // Vehicle Selection Functions
    //----------------------------------------------------------

    /** Select vehicle for camera targeting */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    bool SelectVehicle(const FString& VehicleName);

    /** Get currently selected vehicle */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    FCtrackEntryData GetSelectedVehicle();

    /** Check if vehicle is selected */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    bool HasSelectedVehicle();

    /** Clear vehicle selection */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    void ClearVehicleSelection();

    //----------------------------------------------------------
    // Camera Control Functions (M/F/B keys)
    //----------------------------------------------------------

    /** Switch to manual camera (M key) */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    void SwitchToManualCamera();

    /** Switch to first person camera (F key) */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    bool SwitchToFirstPersonCamera();

    /** Switch to third person camera (B key) */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    bool SwitchToThirdPersonCamera();

    /** Get current camera mode */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    ERealGazeboCameraMode GetCurrentCameraMode();

    /** Get camera status text for display */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    FString GetCameraStatusText();

    //----------------------------------------------------------
    // Keyboard Input Handling
    //----------------------------------------------------------

    /** Handle keyboard input (M/F/B keys) */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    void HandleKeyboardInput(const FString& KeyName);

    //----------------------------------------------------------
    // Utility Functions
    //----------------------------------------------------------

    /** Refresh all data manually */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    void RefreshData();

    /** Get available vehicle types */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    TArray<FString> GetAvailableVehicleTypes();

    /** Test connection status */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    FString TestConnection();

    //----------------------------------------------------------
    // Settings
    //----------------------------------------------------------

    /** Enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Helper Settings")
    bool bEnableDebugLogging = true;

    /** Auto refresh interval */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Helper Settings")
    float AutoRefreshInterval = 0.1f;

protected:
    /** Auto refresh timer */
    void SetupAutoRefresh();

    /** Auto refresh callback */
    UFUNCTION()
    void AutoRefreshCallback();

private:
    /** Entry data bridge for CtrackEntry integration */
    UPROPERTY()
    TObjectPtr<UEntryDataBridge> EntryBridge;

    /** List manager for vehicle selection */
    UPROPERTY()
    TObjectPtr<UListManager> ListManager;

    /** Camera controller for M/F/B functionality */
    UPROPERTY()
    TObjectPtr<UUserCameraController> CameraController;

    /** Auto refresh timer handle */
    FTimerHandle AutoRefreshTimerHandle;

    /** Initialize all components */
    void InitializeComponents();

    /** Connect all components together */
    void ConnectComponents();
};