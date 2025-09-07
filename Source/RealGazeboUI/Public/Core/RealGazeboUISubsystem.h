#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Camera/RealGazeboCameraTypes.h"
#include "Core/UIVehicleDataInterface.h"
#include "RealGazeboUISubsystem.generated.h"

// Forward declarations
class UGazeboBridgeSubsystem;
class UUserWidget;
class AVehicleBasePawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVehicleSelected, const FUIVehicleData&, SelectedVehicle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraModeChanged, ERealGazeboCameraMode, NewCameraMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUIToggled);

/**
 * Main subsystem for RealGazebo UI functionality
 * Manages UI display, vehicle selection, and camera control
 */
UCLASS()
class REALGAZEBOUI_API URealGazeboUISubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    URealGazeboUISubsystem();

    //----------------------------------------------------------
    // Subsystem Interface
    //----------------------------------------------------------
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    //----------------------------------------------------------
    // UI Management
    //----------------------------------------------------------

    /** Show/Hide the main vehicle info UI */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI")
    void ToggleUI();

    /** Show the UI */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI")
    void ShowUI();

    /** Hide the UI */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI")
    void HideUI();

    /** Check if UI is currently visible */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI")
    bool IsUIVisible() const;

    /** Get the main UI widget */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI")
    UUserWidget* GetMainUIWidget() const { return MainUIWidget; }

    //----------------------------------------------------------
    // Vehicle Selection
    //----------------------------------------------------------

    /** Select a vehicle for camera control */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Vehicle")
    void SelectVehicle(const FVehicleID& VehicleID);

    /** Select vehicle by display name */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Vehicle")
    void SelectVehicleByName(const FString& DisplayName);

    /** Clear vehicle selection */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Vehicle")
    void ClearVehicleSelection();

    /** Get currently selected vehicle */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Vehicle")
    bool GetSelectedVehicle(FUIVehicleData& OutVehicleData) const;

    /** Check if a vehicle is selected */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Vehicle")
    bool IsVehicleSelected() const;

    //----------------------------------------------------------
    // Camera Control
    //----------------------------------------------------------

    /** Set camera mode (Manual, FirstPerson, ThirdPerson) */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Camera")
    void SetCameraMode(ERealGazeboCameraMode NewMode);

    /** Get current camera mode */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Camera")
    ERealGazeboCameraMode GetCurrentCameraMode() const;

    /** Switch to manual camera mode */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Camera")
    void SwitchToManualCamera();

    /** Switch to first person camera for selected vehicle */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Camera")
    void SwitchToFirstPersonCamera();

    /** Switch to third person camera for selected vehicle */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Camera")
    void SwitchToThirdPersonCamera();

    /** Set camera target vehicle by name */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Camera")
    void SetCameraTarget(const FString& VehicleName);

    //----------------------------------------------------------
    // Data Access
    //----------------------------------------------------------

    /** Get vehicle data interface */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Data")
    UUIVehicleDataInterface* GetVehicleDataInterface() const { return VehicleDataInterface; }

    /** Get all vehicle data for UI display */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Data")
    TArray<FUIVehicleData> GetAllVehicleData();

    /** Get single vehicle data by name */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Data")
    FUIVehicleData GetVehicleData(const FString& VehicleName);

    /** Check if vehicle is active */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Data")
    bool IsVehicleActive(const FString& VehicleName);

    /** Get active vehicle count */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI|Data")
    int32 GetActiveVehicleCount();


    //----------------------------------------------------------
    // Settings
    //----------------------------------------------------------

    /** Camera settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo UI|Camera Settings")
    FCameraSettings CameraSettings;

    /** Manual camera settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo UI|Camera Settings")
    FManualCameraSettings ManualCameraSettings;

    /** Auto-hide UI when switching to vehicle cameras */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo UI|Settings")
    bool bAutoHideUIInVehicleMode = false;

    /** UI update frequency (Hz) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo UI|Settings", meta = (ClampMin = "1.0", ClampMax = "60.0"))
    float UIUpdateFrequency = 10.0f;

    //----------------------------------------------------------
    // Events
    //----------------------------------------------------------

    /** Called when a vehicle is selected */
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo UI|Events")
    FOnVehicleSelected OnVehicleSelected;

    /** Called when camera mode changes */
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo UI|Events")
    FOnCameraModeChanged OnCameraModeChanged;

    /** Called when UI visibility toggles */
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo UI|Events")
    FOnUIToggled OnUIToggled;

    //----------------------------------------------------------
    // Static Access
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "RealGazebo UI", meta = (CallInEditor = "true"))
    static URealGazeboUISubsystem* GetUISubsystem(const UObject* WorldContext);

protected:
    //----------------------------------------------------------
    // Initialization
    //----------------------------------------------------------

    /** Initialize camera manager */
    void InitializeCameraManager();

    /** Initialize UI widgets */
    void InitializeUI();

    /** Setup input bindings */
    void SetupInputBindings();

    //----------------------------------------------------------
    // Internal Updates
    //----------------------------------------------------------

    /** Update UI display */
    UFUNCTION()
    void UpdateUI();

    /** Handle camera mode change internally */
    void HandleCameraModeChange(ERealGazeboCameraMode NewMode);

    //----------------------------------------------------------
    // Input Handlers
    //----------------------------------------------------------

    /** Handle manual camera input */
    void HandleManualCameraInput();

    /** Handle first person camera input */
    void HandleFirstPersonCameraInput();

    /** Handle third person camera input */
    void HandleThirdPersonCameraInput();

    /** Handle UI toggle input */
    void HandleUIToggleInput();

private:
    /** Reference to bridge subsystem */
    UPROPERTY()
    TWeakObjectPtr<UGazeboBridgeSubsystem> BridgeSubsystem;

    /** Vehicle data interface */
    UPROPERTY()
    TObjectPtr<UUIVehicleDataInterface> VehicleDataInterface;


    /** Main UI widget */
    UPROPERTY()
    TObjectPtr<UUserWidget> MainUIWidget;

    /** Currently selected vehicle */
    FVehicleID SelectedVehicleID;
    bool bHasSelectedVehicle;

    /** Current camera mode */
    ERealGazeboCameraMode CurrentCameraMode;

    /** UI visibility state */
    bool bUIVisible;

    /** Update timer handle */
    FTimerHandle UIUpdateTimerHandle;

    /** Input component for handling key bindings */
    UPROPERTY()
    TObjectPtr<class UInputComponent> InputComponent;
};