// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DefaultPawn.h"
#include "ViewerController/RealGazeboViewerTypes.h"
#include "GazeboBridgeTypes.h"
#include "RealGazeboViewerDirector.generated.h"

// Forward declarations
class AVehicleBasePawn;
class URealGazeboCameraControllerComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FRealGazeboViewerVehicleChangedByInput, AVehicleBasePawn*);

/**
 * Central camera director for RealGazebo viewer system
 * Simplified approach using UE5's DefaultPawn for manual camera
 *
 * Features:
 * - Manual camera using DefaultPawn with built-in flying movement (M key)
 * - First person vehicle camera (F key)
 * - Third person chase camera (B key)
 * - Pawn switching between DefaultPawn and vehicles
 * - Integration with RealGazebo vehicle selection UI
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API ARealGazeboViewerDirector : public AActor
{
    GENERATED_BODY()

public:
    ARealGazeboViewerDirector();

    //----------------------------------------------------------
    // Actor Lifecycle
    //----------------------------------------------------------

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    //----------------------------------------------------------
    // Camera Control
    //----------------------------------------------------------

    /** Get current camera mode */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Viewer")
    ERealGazeboViewerMode GetCurrentMode() const { return CurrentMode; }

    /** Set camera mode */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer")
    void SetCameraMode(ERealGazeboViewerMode NewMode);

    /** Get the currently followed vehicle */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Viewer")
    AVehicleBasePawn* GetCurrentVehicle() const { return CurrentVehicle; }

    /** Native event fired when the current vehicle changes through keyboard input */
    FRealGazeboViewerVehicleChangedByInput& OnVehicleChangedByInput() { return VehicleChangedByInputDelegate; }

    /** Set the vehicle to follow (for first/third person cameras) */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer")
    void SetCurrentVehicle(AVehicleBasePawn* Vehicle);

    /** Set current vehicle by ID */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer")
    void SetCurrentVehicleByID(const FVehicleID& VehicleID);

    /** Configure initial camera settings */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer")
    void SetInitialCameraSettings(const FVector& Location, const FRotator& Rotation);

    /** Refresh the available vehicles list (called by UI when vehicle list changes) */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer")
    void RefreshVehicleList();

    //----------------------------------------------------------
    // Camera Preset Control
    //----------------------------------------------------------

    /** Apply a camera preset by index (teleports manual camera) */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer")
    void ApplyCameraPreset(int32 PresetIndex);

    /** Apply a camera preset by name (teleports manual camera) */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer")
    bool ApplyCameraPresetByName(const FString& PresetName);

    /** Get all available camera presets */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Viewer")
    TArray<FCameraPreset> GetCameraPresets() const { return CameraPresets; }

    /** Add a camera preset at runtime */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer")
    void AddCameraPreset(const FCameraPreset& NewPreset);

    /** Remove a camera preset by index */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer")
    void RemoveCameraPreset(int32 PresetIndex);

    /** Set all camera presets (used during initialization) */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Viewer")
    void SetCameraPresets(const TArray<FCameraPreset>& NewPresets);

    //----------------------------------------------------------
    // Manual Camera Control
    //----------------------------------------------------------

    /** Get the original pawn used for manual camera mode */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Viewer")
    APawn* GetOriginalPawn() const { return OriginalPawn; }

    //----------------------------------------------------------
    // Input Handlers
    //----------------------------------------------------------

private:
    /** Input handler for manual camera mode (M key) */
    void InputManualCamera();

    /** Input handler for first person camera mode (F key) */
    void InputFirstPersonCamera();

    /** Input handler for third person camera mode (B key) */
    void InputThirdPersonCamera();

    /** Input handler for selecting the previous vehicle (< key) */
    void InputPreviousVehicle();

    /** Input handler for selecting the next vehicle (> key) */
    void InputNextVehicle();

    /** Input handler for camera preset 1 (1 key - C-Track) */
    void InputPreset1();

    /** Input handler for camera preset 2 (2 key - VILS) */
    void InputPreset2();

    /** Input handler for camera preset 3 (3 key - Urban) */
    void InputPreset3();

    /** Input handler for camera preset 4 (4 key - BeltWay Camera) */
    void InputPreset4();

    /** Input handler for camera preset 5 (5 key - DirtRoad Camera) */
    void InputPreset5();

    /** Input handler for camera preset 6 (6 key - Forest Camera) */
    void InputPreset6();

    /** Input handler for camera preset 7 (7 key - Lake Camera) */
    void InputPreset7();

    //----------------------------------------------------------
    // Configuration
    //----------------------------------------------------------

protected:

    /** Initial location for manual camera pawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Camera Settings")
    FVector InitialCameraLocation = FVector(0.0f, 0.0f, 500.0f);

    /** Initial rotation for manual camera pawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Camera Settings")
    FRotator InitialCameraRotation = FRotator(-20.0f, 0.0f, 0.0f);

    /** Camera presets for quick teleportation in Manual mode */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Camera Presets")
    TArray<FCameraPreset> CameraPresets;

    //----------------------------------------------------------
    // Internal State
    //----------------------------------------------------------

private:
    /** Current camera mode */
    ERealGazeboViewerMode CurrentMode;

    /** Currently followed vehicle */
    UPROPERTY()
    AVehicleBasePawn* CurrentVehicle;


    /** All available vehicles for camera switching */
    UPROPERTY()
    TArray<AVehicleBasePawn*> AvailableVehicles;

    /** Store original pawn when switching to manual camera */
    UPROPERTY()
    APawn* OriginalPawn;

    /** Notifies UI integrations after comma/period vehicle switching */
    FRealGazeboViewerVehicleChangedByInput VehicleChangedByInputDelegate;

    //----------------------------------------------------------
    // Internal Methods
    //----------------------------------------------------------

    /** Initialize camera director for begin play */
    void InitializeForBeginPlay();

    /** Setup input bindings */
    void SetupInputBindings();

    /** Discover all vehicles in the level */
    void DiscoverVehicles();

    /** Select a vehicle relative to the currently possessed vehicle */
    void CycleCurrentVehicle(int32 Direction);

    /** Find camera controller component for a specific vehicle */
    URealGazeboCameraControllerComponent* FindCameraControllerForVehicle(AVehicleBasePawn* Vehicle) const;

    /** Get current camera transform from player controller */
    FTransform GetCurrentCameraTransform() const;


    /** Switch to manual camera mode using DefaultPawn */
    void SwitchToManualMode();

    /** Switch to first person camera mode */
    void SwitchToFirstPersonMode();

    /** Switch to third person camera mode */
    void SwitchToThirdPersonMode();

    /** Switch pawn possession for the player controller */
    void SwitchPlayerPawn(APawn* NewPawn);
};