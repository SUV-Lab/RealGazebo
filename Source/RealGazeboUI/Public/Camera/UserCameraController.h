#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Camera/RealGazeboCameraTypes.h"
#include "Core/ListManager.h"
#include "UserCameraController.generated.h"

/**
 * UserCameraController - Simple camera mode switching for RealGazebo UI
 * Handles M/F/B key functionality with vehicle selection integration
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API UUserCameraController : public UObject
{
    GENERATED_BODY()

public:
    UUserCameraController();

    /** Switch to manual camera mode (M key) */
    UFUNCTION(BlueprintCallable, Category = "Camera Controller")
    void SwitchToManualMode();

    /** Switch to first person camera mode (F key) - requires selected vehicle */
    UFUNCTION(BlueprintCallable, Category = "Camera Controller")
    bool SwitchToFirstPersonMode();

    /** Switch to third person camera mode (B key) - requires selected vehicle */
    UFUNCTION(BlueprintCallable, Category = "Camera Controller")
    bool SwitchToThirdPersonMode();

    /** Get current camera mode */
    UFUNCTION(BlueprintCallable, Category = "Camera Controller")
    ERealGazeboCameraMode GetCurrentCameraMode() const;

    /** Set selected vehicle for camera targeting */
    UFUNCTION(BlueprintCallable, Category = "Camera Controller")
    bool SetSelectedVehicle(const FString& VehicleName);

    /** Get selected vehicle name */
    UFUNCTION(BlueprintCallable, Category = "Camera Controller")
    FString GetSelectedVehicleName() const;

    /** Check if vehicle selection is required for current mode */
    UFUNCTION(BlueprintCallable, Category = "Camera Controller")
    bool RequiresVehicleSelection() const;

    /** Handle keyboard input for camera switching */
    UFUNCTION(BlueprintCallable, Category = "Camera Controller")
    void HandleKeyPress(const FString& KeyName);

    /** Get camera status text for UI display */
    UFUNCTION(BlueprintCallable, Category = "Camera Controller")
    FString GetCameraStatusText() const;

    /** Set list manager for vehicle selection */
    UFUNCTION(BlueprintCallable, Category = "Camera Controller")
    void SetListManager(UListManager* InListManager);

    /** Initialize camera controller */
    UFUNCTION(BlueprintCallable, Category = "Camera Controller")
    void Initialize();

private:
    /** Current camera mode */
    UPROPERTY()
    ERealGazeboCameraMode CurrentCameraMode;

    /** Currently selected vehicle name */
    UPROPERTY()
    FString SelectedVehicleName;

    /** List manager reference for vehicle data */
    UPROPERTY()
    TObjectPtr<UListManager> ListManager;

    /** UI subsystem reference */
    UPROPERTY()
    TWeakObjectPtr<class URealGazeboUISubsystem> UISubsystem;

    /** Initialize UI subsystem connection */
    void InitializeUISubsystem();

    /** Apply camera mode to subsystem */
    void ApplyCameraModeToSubsystem();

    /** Check if vehicle is valid for camera modes */
    bool IsVehicleValidForCamera() const;

    /** Generate camera mode name for display */
    FString GetCameraModeDisplayName() const;
};