// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "GazeboBridgeTypes.h"
#include "Data/RealGazeboVehicleData.h"
#include "ViewerController/RealGazeboViewerTypes.h"
#include "RealGazeboManager.generated.h"

// Forward declarations
class UGazeboBridgeSubsystem;
class URealGazeboUISubsystem;
class URealGazeboStreamingSubsystem;
class URealGazeboVehicleListItem;
class ARealGazeboViewerDirector;
class AVehicleBasePawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FRealGazeboVehicleListItemChangedByInput,
    URealGazeboVehicleListItem*,
    VehicleItem);

/**
 * Unified RealGazebo Manager Actor
 *
 * Combines RealGazeboBridgeManager + RealGazeboCameraUIManager into a single
 * comprehensive management solution for complete plug-and-play usage.
 *
 * This provides the OLD VERSION drag-and-drop experience while leveraging
 * the high-performance v2.0 subsystem architecture underneath.
 *
 * Key Features:
 * - Complete Bridge + UI management in one actor
 * - Drag-and-drop into level (like Old Version)
 * - Visual configuration in Details panel
 * - Plug-and-play - no project settings needed
 * - Automatic subsystem coordination
 * - Comprehensive event system
 * - Advanced performance tuning
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "RealGazebo Manager"))
class REALGAZEBO_API ARealGazeboManager : public AActor
{
    GENERATED_BODY()

public:
    ARealGazeboManager();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo",
              meta = (DisplayName = "Unified Vehicle Configuration", DisplayPriority = "1",
                     ToolTip = "Unified DataTable containing complete vehicle configurations for both Bridge operations and UI display. Uses FRealGazeboVehicleConfigRow structure."))
    TObjectPtr<UDataTable> UnifiedVehicleDataTable;

    /** Main Widget Class for UI management */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo",
              meta = (DisplayName = "Main Widget Class", DisplayPriority = "2",
                     ToolTip = "Widget class to create for the main UI. Must be set manually (e.g., WBP_RealGazeboMain)."))
    TSubclassOf<UUserWidget> MainWidgetClass;

    //----------------------------------------------------------
    // Bridge Configuration - Network & Vehicle Management
    //----------------------------------------------------------

    /** Auto-start bridge when level begins */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge",
              meta = (DisplayName = "Auto Start Bridge", DisplayPriority = "1"))
    bool bAutoStartBridge = true;

    /** Automatically spawn vehicles when data is received */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge",
              meta = (DisplayName = "Auto Spawn Vehicles", DisplayPriority = "2"))
    bool bAutoSpawnVehicles = true;

    /** UDP Listen Port */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Network",
              meta = (DisplayName = "Listen Port", DisplayPriority = "3", ClampMin = "1024", ClampMax = "65535"))
    int32 ListenPort = 5005;

    /** Server IP Address (empty = accept from all IPs, specific IP = filter for that IP only) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Network",
              meta = (DisplayName = "Server IP Address", DisplayPriority = "2",
                     ToolTip = "Leave empty to accept UDP packets from any IP address. Set specific IP (e.g. 192.168.168.100) to only accept packets from that source."))
    FString ServerIPAddress = TEXT("");

    /** Process UDP packets in batches for better performance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Network|Processing",
              meta = (DisplayName = "Enable Batch Processing", DisplayPriority = "1"))
    bool bEnableBatchProcessing = true;

    /** Number of packets to process per batch */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Network|Processing",
              meta = (DisplayName = "Batch Size", DisplayPriority = "2", ClampMin = "1", ClampMax = "1000", EditCondition = "bEnableBatchProcessing"))
    int32 BatchSize = 50;

    /** Batch processing interval in seconds (affects responsiveness) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Network|Processing",
              meta = (DisplayName = "Batch Processing Interval", DisplayPriority = "3", ClampMin = "0.001", ClampMax = "0.1", EditCondition = "bEnableBatchProcessing", AdvancedDisplay))
    float BatchProcessingInterval = 0.016f; // ~60 FPS

    /** Validate packet sizes against expected protocol */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Network",
              meta = (DisplayName = "Validate Packet Sizes", DisplayPriority = "1", AdvancedDisplay))
    bool bValidatePacketSizes = true;

    /** Update frequency in Hz */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Performance|Core",
              meta = (DisplayName = "Update Frequency", DisplayPriority = "1", ClampMin = "10.0", ClampMax = "120.0"))
    float UpdateFrequency = 60.0f;

    /** Maximum number of active vehicles allowed simultaneously
     *  Protocol supports up to 65,536 vehicles (256 types x 256 instances per type)
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Performance|Limits",
              meta = (DisplayName = "Max Active Vehicles", DisplayPriority = "1", ClampMin = "1", ClampMax = "65536",
                      ToolTip = "Maximum total active vehicles. Protocol supports up to 65,536 (256 types x 256 instances). Adjust based on performance requirements."))
    int32 MaxActiveVehicles = 65536;

    /** Maximum actors per vehicle type in the pool
     *  With protocol supporting 256 instances per type, pool can hold up to 256 per type
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Performance|Pool Management",
              meta = (DisplayName = "Max Actors Per Type", DisplayPriority = "1", ClampMin = "10", ClampMax = "256",
                      ToolTip = "Maximum pooled actors per vehicle type (protocol supports 256 instances per type)"))
    int32 MaxActorsPerType = 256;

    /** Initial pool size per vehicle type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Performance|Pool Management",
              meta = (DisplayName = "Initial Pool Size", DisplayPriority = "2", ClampMin = "5", ClampMax = "256"))
    int32 InitialPoolSize = 20;

    /** Auto-expand pools when more vehicles are needed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Performance|Pool Management",
              meta = (DisplayName = "Auto Expand Pools", DisplayPriority = "3"))
    bool bAutoExpandPools = true;

    /** Pool expansion increment size */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Performance|Pool Management",
              meta = (DisplayName = "Pool Expansion Size", DisplayPriority = "4", ClampMin = "1", ClampMax = "100", EditCondition = "bAutoExpandPools"))
    int32 PoolExpansionSize = 10;

    /** Auto-shrink unused pools to save memory */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Performance|Pool Management",
              meta = (DisplayName = "Auto Shrink Pools", DisplayPriority = "5", AdvancedDisplay))
    bool bAutoShrinkPools = false;

    /** Time before unused actors are removed from pools (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Performance|Pool Management",
              meta = (DisplayName = "Unused Actor Timeout", DisplayPriority = "6", ClampMin = "10.0", ClampMax = "300.0", EditCondition = "bAutoShrinkPools", AdvancedDisplay))
    float UnusedActorTimeout = 30.0f;

    /** Enable smooth movement interpolation for all vehicles */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Vehicles|Movement",
              meta = (DisplayName = "Default Smooth Movement", DisplayPriority = "1"))
    bool bDefaultSmoothMovement = true;

    /** Default interpolation speed for vehicle movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Bridge|Vehicles|Movement",
              meta = (DisplayName = "Default Interpolation Speed", DisplayPriority = "2", ClampMin = "1.0", ClampMax = "100.0", EditCondition = "bDefaultSmoothMovement"))
    float DefaultInterpolationSpeed = 15.0f;

    //----------------------------------------------------------
    // UI Camera Configuration - Camera & Widget Management
    //----------------------------------------------------------

    /** Auto-create and show UI when level begins */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|UI Camera",
              meta = (DisplayName = "Auto Create UI", DisplayPriority = "1"))
    bool bAutoCreateUI = true;

    /** Auto-add widget to viewport */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|UI Camera",
              meta = (DisplayName = "Auto Add to Viewport", DisplayPriority = "2"))
    bool bAutoAddToViewport = true;

    /** Auto-create viewer director for camera control */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|UI Camera",
              meta = (DisplayName = "Auto Create Viewer Director", DisplayPriority = "3"))
    bool bAutoCreateViewerDirector = true;

    /** Initial location for DefaultPawn when system starts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|UI Camera|Camera Settings",
              meta = (DisplayName = "Initial Camera Location", DisplayPriority = "1",
                     ToolTip = "Starting position for DefaultPawn camera"))
    FVector InitialCameraLocation = FVector(0.0f, 0.0f, 500.0f);

    /** Initial rotation for DefaultPawn when system starts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|UI Camera|Camera Settings",
              meta = (DisplayName = "Initial Camera Rotation", DisplayPriority = "2",
                     ToolTip = "Starting rotation for DefaultPawn camera"))
    FRotator InitialCameraRotation = FRotator(-20.0f, 0.0f, 0.0f);

    /** Predefined camera locations for quick teleportation in Manual mode */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|UI Camera|Camera Presets",
              meta = (DisplayName = "Camera Presets", DisplayPriority = "1",
                     ToolTip = "List of predefined camera locations for quick navigation"))
    TArray<FCameraPreset> CameraPresets;

    /** Z-order for the widget when added to viewport */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|UI Camera|UI Settings",
              meta = (DisplayName = "Widget Z-Order", DisplayPriority = "1", ClampMin = "0"))
    int32 WidgetZOrder = 0;

    /** Always show mouse cursor regardless of camera mode or UI state */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|UI Camera|UI Settings",
              meta = (DisplayName = "Always Show Mouse Cursor", DisplayPriority = "2",
                     ToolTip = "Keep mouse cursor visible at all times for UI interaction"))
    bool bAlwaysShowMouseCursor = true;

    //----------------------------------------------------------
    // Streaming Configuration - RTSP Video Streaming
    //----------------------------------------------------------

    /** Auto-start RTSP server when level begins */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Streaming",
              meta = (DisplayName = "Auto Start RTSP Server", DisplayPriority = "1"))
    bool bAutoStartRTSP = true;

    /** RTSP server port */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Streaming|Network",
              meta = (DisplayName = "RTSP Port", DisplayPriority = "1", ClampMin = "1024", ClampMax = "65535"))
    int32 RTSPPort = 8554;

    /** Default stream resolution (0=VGA 640x480, 1=SVGA 800x600, 2=XGA 1024x768, 3=SXGA 1280x960, 4=UXGA 1600x1200) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Streaming|Video",
              meta = (DisplayName = "Stream Resolution", DisplayPriority = "1", ClampMin = "0", ClampMax = "4",
                     ToolTip = "0=VGA 640x480, 1=SVGA 800x600, 2=XGA 1024x768 (recommended), 3=SXGA 1280x960, 4=UXGA 1600x1200"))
    int32 StreamResolution = 2;  // XGA 1024x768 default

    /** Default stream frame rate */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Streaming|Video",
              meta = (DisplayName = "Stream FPS", DisplayPriority = "2", ClampMin = "15", ClampMax = "60"))
    int32 StreamFPS = 30;

    //----------------------------------------------------------
    // Bridge Control API
    //----------------------------------------------------------

    /** Start the bridge (manual control) */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Bridge Control", meta = (DisplayName = "Start Bridge"))
    void StartBridge();

    /** Stop the bridge */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Bridge Control", meta = (DisplayName = "Stop Bridge"))
    void StopBridge();

    /** Check if bridge is running */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Bridge Control", meta = (DisplayName = "Is Bridge Active"))
    bool IsBridgeActive() const;

    /** Clear all spawned vehicles */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Bridge Control", meta = (DisplayName = "Clear All Vehicles"))
    void ClearAllVehicles();

    //----------------------------------------------------------
    // UI Control API
    //----------------------------------------------------------

    /** Create the main widget with proper configuration */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI Control", meta = (DisplayName = "Create Main Widget"))
    UUserWidget* CreateMainWidget();

    /** Add the main widget to viewport */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI Control", meta = (DisplayName = "Add Widget to Viewport"))
    void AddMainWidgetToViewport();

    /** Complete UI setup in one call - creates widget and adds to viewport */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI Control", meta = (DisplayName = "Initialize Camera UI"))
    void InitializeCameraUI();

    /** Remove widget from viewport and cleanup */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI Control", meta = (DisplayName = "Cleanup Camera UI"))
    void CleanupCameraUI();

    /** Force mouse cursor visibility state */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI Control", meta = (DisplayName = "Set Mouse Always Visible"))
    void SetMouseCursorAlwaysVisible(bool bVisible);

    //----------------------------------------------------------
    // Streaming Control API
    //----------------------------------------------------------

    /** Start RTSP streaming server (manual control) */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Streaming Control", meta = (DisplayName = "Start RTSP Server"))
    bool StartRTSPServer();

    /** Stop RTSP streaming server */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Streaming Control", meta = (DisplayName = "Stop RTSP Server"))
    void StopRTSPServer();

    /** Check if RTSP server is running */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Streaming Control", meta = (DisplayName = "Is RTSP Server Running"))
    bool IsRTSPServerRunning() const;

    /** Get number of active streams */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Streaming Control", meta = (DisplayName = "Get Active Stream Count"))
    int32 GetActiveStreamCount() const;

    /** Get number of registered cameras waiting to stream */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Streaming Control", meta = (DisplayName = "Get Registered Camera Count"))
    int32 GetRegisteredCameraCount() const;

    //----------------------------------------------------------
    // Status Information API
    //----------------------------------------------------------

    /** Get number of active vehicles */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Status", meta = (DisplayName = "Get Vehicle Count"))
    int32 GetActiveVehicleCount() const;

    /** Get network statistics */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Status", meta = (DisplayName = "Get Network Stats"))
    void GetNetworkStatistics(int32& OutValidPackets, int32& OutInvalidPackets, float& OutPacketsPerSecond) const;

    /** Get the currently created main widget (if any) */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Status", meta = (DisplayName = "Get Main Widget"))
    UUserWidget* GetMainWidget() const;

    /** Check if UI is currently active */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Status", meta = (DisplayName = "Is UI Active"))
    bool IsUIActive() const;

    /** Get the created viewer director (if any) */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Status", meta = (DisplayName = "Get Viewer Director"))
    ARealGazeboViewerDirector* GetViewerDirector() const;

    /** Validate the current setup and return status */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Status", meta = (DisplayName = "Validate Setup"))
    bool ValidateSetup();

    //----------------------------------------------------------
    // Camera Control API (from RealGazeboCameraUIManager)
    //----------------------------------------------------------

    /** Set camera mode */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Camera Control", meta = (DisplayName = "Set Camera Mode"))
    void SetCameraMode(int32 NewMode);

    /** Get current camera mode */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RealGazebo|Camera Control", meta = (DisplayName = "Get Camera Mode"))
    int32 GetCameraMode() const;

    /** Show/hide main UI widget */
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UI Control", meta = (DisplayName = "Set Main Widget Visibility"))
    void SetMainWidgetVisibility(bool bVisible);

    //----------------------------------------------------------
    // Events - Bridge Lifecycle
    //----------------------------------------------------------

    /** Called when bridge starts successfully */
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Bridge Events")
    FOnVehicleDataReceived OnBridgeStarted;

    /** Called when bridge stops */
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Bridge Events")
    FOnVehicleDataReceived OnBridgeStopped;

    /** Called when vehicle is spawned */
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Bridge Events")
    FOnVehicleDataReceived OnVehicleSpawned;

    /** Called when comma/period input changes the viewer vehicle */
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Camera Events")
    FRealGazeboVehicleListItemChangedByInput OnVehicleChangedByInput;

    //----------------------------------------------------------
    // Events - UI Lifecycle (Blueprint Implementable)
    //----------------------------------------------------------

    /** Called when UI is successfully created */
    UFUNCTION(BlueprintImplementableEvent, Category = "RealGazebo|UI Events")
    void OnUICreated();

    /** Called when UI setup fails */
    UFUNCTION(BlueprintImplementableEvent, Category = "RealGazebo|UI Events")
    void OnUISetupFailed(const FString& ErrorMessage);

    /** Called when UI is cleaned up */
    UFUNCTION(BlueprintImplementableEvent, Category = "RealGazebo|UI Events")
    void OnUICleanedUp();

protected:
    //----------------------------------------------------------
    // Actor Lifecycle
    //----------------------------------------------------------

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    //----------------------------------------------------------
    // Internal State
    //----------------------------------------------------------

    /** Reference to the bridge subsystem we're controlling */
    UPROPERTY()
    TWeakObjectPtr<UGazeboBridgeSubsystem> BridgeSubsystem;

    /** Reference to the UI subsystem we're controlling */
    UPROPERTY()
    TWeakObjectPtr<URealGazeboUISubsystem> UISubsystem;

    /** Reference to the streaming subsystem we're controlling */
    UPROPERTY()
    TWeakObjectPtr<URealGazeboStreamingSubsystem> StreamingSubsystem;

    /** Track if we started the bridge subsystem */
    bool bDidStartBridge = false;

    /** Track if we started the UI subsystem */
    bool bDidStartUI = false;

    /** Track if we started the streaming subsystem */
    bool bDidStartRTSP = false;

    /** Reference to created viewer director */
    UPROPERTY()
    TWeakObjectPtr<ARealGazeboViewerDirector> ViewerDirector;

    /** Vehicle list item retained for the latest keyboard-selected vehicle */
    UPROPERTY(Transient)
    TObjectPtr<URealGazeboVehicleListItem> InputSelectedVehicleItem;

    //----------------------------------------------------------
    // Status Display (Visible for Debug)
    //----------------------------------------------------------

    /** Bridge status display for debugging */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Status", meta = (DisplayName = "Bridge Status"))
    FString BridgeStatus = TEXT("Not Started");

    /** Active vehicles count for display */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Status", meta = (DisplayName = "Active Vehicles"))
    int32 ActiveVehiclesCount = 0;

    /** UI status display for debugging */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Status", meta = (DisplayName = "UI Status"))
    FString UIStatus = TEXT("Not Initialized");

    /** Widget state for display */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Status", meta = (DisplayName = "Widget In Viewport"))
    bool WidgetInViewportStatus = false;

    /** Streaming status display for debugging */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Status", meta = (DisplayName = "Streaming Status"))
    FString StreamingStatus = TEXT("Not Started");

    /** Active streams count for display */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Status", meta = (DisplayName = "Active Streams"))
    int32 ActiveStreamsCount = 0;

    //----------------------------------------------------------
    // Internal Methods
    //----------------------------------------------------------

    /** Configure the bridge subsystem with our settings */
    void ConfigureBridgeSubsystem();

    /** Configure the UI subsystem with our settings */
    void ConfigureUISubsystem();

    /** Configure the streaming subsystem with our settings */
    void ConfigureStreamingSubsystem();

    /** Configure vehicle pool management settings */
    void ConfigureVehiclePoolSettings();

    /** Configure network processing settings */
    void ConfigureNetworkProcessingSettings();

    /** Configure performance and debug settings */
    void ConfigurePerformanceAndDebugSettings();

    /** Bind keyboard vehicle-change notifications from the viewer director */
    void BindViewerDirectorEvents();

    /** Unbind keyboard vehicle-change notifications from the viewer director */
    void UnbindViewerDirectorEvents();

    /** Convert a keyboard-selected vehicle Pawn to a Blueprint vehicle item */
    void HandleVehicleChangedByInput(AVehicleBasePawn* Vehicle);

    /** Get the player controller for widget operations */
    APlayerController* GetPlayerController() const;

    /** Configure Bridge subsystem pool settings */
    void ConfigureBridgePoolSettings();

    //----------------------------------------------------------
    // DataTable Conversion Helpers
    //----------------------------------------------------------

    /** Create a Bridge-compatible DataTable from unified configuration */
    UDataTable* CreateBridgeCompatibleDataTable();

    /** Create a UI-compatible DataTable from unified configuration */
    UDataTable* CreateUICompatibleDataTable();

    //----------------------------------------------------------
    // Validation Methods
    //----------------------------------------------------------

    /** Validate vehicle data table configuration */
    bool ValidateVehicleDataTable() const;

    /** Validate UI configuration */
    bool ValidateUIConfiguration() const;

    /** Validate Bridge subsystem availability */
    bool ValidateBridgeSubsystem() const;

    /** Update status display */
    void UpdateStatusDisplay();

private:
    /** Timer for periodic status updates */
    FTimerHandle StatusUpdateTimer;

    /** Runtime-created DataTable for Bridge subsystem (converted from unified) */
    UPROPERTY()
    TObjectPtr<UDataTable> RuntimeBridgeDataTable;

    /** Runtime-created DataTable for UI subsystem (converted from unified) */
    UPROPERTY()
    TObjectPtr<UDataTable> RuntimeUIDataTable;
};
