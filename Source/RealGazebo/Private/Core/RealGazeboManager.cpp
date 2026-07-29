// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#include "Core/RealGazeboManager.h"
#include "RealGazebo.h"
#include "Engine/World.h"
#include "GazeboBridgeSubsystem.h"
#include "RealGazeboUISubsystem.h"
#include "RealGazeboStreamingSubsystem.h"
#include "Vehicles/VehiclePoolManager.h"
#include "ViewerController/RealGazeboViewerDirector.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Components/Widget.h"
#include "Blueprint/UserWidget.h"
#include "Data/RealGazeboVehicleData.h"
#include "Data/RealGazeboVehicleListItem.h"
#include "Data/VehicleTypeImageData.h"
#include "Vehicles/VehicleBasePawn.h"

ARealGazeboManager::ARealGazeboManager()
{
    // Set this actor to call Tick() every frame
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    // Initialize state
    bDidStartBridge = false;
    bDidStartUI = false;
    bDidStartRTSP = false;

    // MainWidgetClass should be set manually by user in Details panel
    MainWidgetClass = nullptr;

    // Clear subsystem references
    BridgeSubsystem = nullptr;
    UISubsystem = nullptr;
    StreamingSubsystem = nullptr;
    ViewerDirector = nullptr;
    InputSelectedVehicleItem = nullptr;

    // Initialize runtime DataTables
    RuntimeBridgeDataTable = nullptr;
    RuntimeUIDataTable = nullptr;

    // Initialize default camera presets
    CameraPresets.Empty();

    // Preset 0 - C-Track (Keyboard: 1)
    FCameraPreset CTrackPreset;
    CTrackPreset.PresetName = TEXT("C-Track");
    CTrackPreset.Location = FVector(-49985.707615f, -6242.985621f, 22005.642773f);
    CTrackPreset.Rotation = FRotator(-32.369293f, 0.734925f, 0.0f);
    CameraPresets.Add(CTrackPreset);
    
    // Preset 1 - VILS (Keyboard: 2)
    FCameraPreset VILSPreset;
    VILSPreset.PresetName = TEXT("VILS");
    VILSPreset.Location = FVector(-2492.325356f, 1164.508698f, 6482.866281f);
    VILSPreset.Rotation = FRotator(-90.0f, 179.0f, 90.0f);
    CameraPresets.Add(VILSPreset);

    // Preset 2 - Urban (Keyboard: 3)
    FCameraPreset UrbanPreset;
    UrbanPreset.PresetName = TEXT("Urban");
    UrbanPreset.Location = FVector(-31214.548424f, -17165.116186f, 14063.83629f);
    UrbanPreset.Rotation = FRotator(-90.0f, 82.0f, 90.0f);
    CameraPresets.Add(UrbanPreset);

    // Preset 3 - BeltWay Camera (Keyboard: 4)
    FCameraPreset BeltWayCamera;
    BeltWayCamera.PresetName = TEXT("BeltWay Camera");
    BeltWayCamera.Location = FVector(-10369.975632f, 6390.696626f, 32605.926715f);
    BeltWayCamera.Rotation = FRotator(-90.0f, 180.0f, 0.0f);
    CameraPresets.Add(BeltWayCamera);

    // Preset 4 - DirtRoad Camera (Keyboard: 5)
    FCameraPreset DirtRoadCamera;
    DirtRoadCamera.PresetName = TEXT("DirtRoad Camera");
    DirtRoadCamera.Location = FVector(-8113.290445f, 7729.819206f, 1092.474938f);
    DirtRoadCamera.Rotation = FRotator(-16.04874f, -18.656375f, 0.0f);
    CameraPresets.Add(DirtRoadCamera);

    // Preset 5 - Forest Camera (Keyboard: 6)
    FCameraPreset ForestCamera;
    ForestCamera.PresetName = TEXT("Forest Camera");
    ForestCamera.Location = FVector(2213.234064f, 22609.036801f, 487.442638f);
    ForestCamera.Rotation = FRotator(-2.762647f, 77.978291f, 0.0f);
    CameraPresets.Add(ForestCamera);

    // Preset 6 - Lake Camera (Keyboard: 7)
    FCameraPreset LakeCamera;
    LakeCamera.PresetName = TEXT("Lake Camera");
    LakeCamera.Location = FVector(-28342.004076f, 20093.002073f, 2293.668522f);
    LakeCamera.Rotation = FRotator(-12.077954f, 76.339822f, 0.0f);
    CameraPresets.Add(LakeCamera);
}

//----------------------------------------------------------
// Actor Lifecycle
//----------------------------------------------------------

void ARealGazeboManager::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogRealGazebo, Log, TEXT("RealGazebo Manager: Initializing [%s]"), *GetName());

    // Get subsystem references
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        BridgeSubsystem = GameInstance->GetSubsystem<UGazeboBridgeSubsystem>();
        UISubsystem = GameInstance->GetSubsystem<URealGazeboUISubsystem>();
        StreamingSubsystem = GameInstance->GetSubsystem<URealGazeboStreamingSubsystem>();

        if (!BridgeSubsystem.IsValid())
        {
            UE_LOG(LogRealGazebo, Warning, TEXT("Bridge subsystem not found - Bridge features disabled"));
        }

        if (!UISubsystem.IsValid())
        {
            UE_LOG(LogRealGazebo, Warning, TEXT("UI subsystem not found - UI features disabled"));
        }

        if (!StreamingSubsystem.IsValid())
        {
            UE_LOG(LogRealGazebo, Warning, TEXT("Streaming subsystem not found - Streaming features disabled"));
        }
    }
    else
    {
        UE_LOG(LogRealGazebo, Error, TEXT("GameInstance not available"));
        return;
    }

    // Configure subsystems with our settings
    ConfigureBridgeSubsystem();
    ConfigureUISubsystem();
    ConfigureStreamingSubsystem();

    // Auto-start bridge if enabled
    if (bAutoStartBridge)
    {
        StartBridge();
    }

    // Auto-create UI if enabled
    if (bAutoCreateUI)
    {
        InitializeCameraUI();
    }

    // Auto-start RTSP server if enabled
    if (bAutoStartRTSP)
    {
        StartRTSPServer();
    }

    // Start periodic status updates
    GetWorldTimerManager().SetTimer(StatusUpdateTimer, this, &ARealGazeboManager::UpdateStatusDisplay, 1.0f, true);
}

void ARealGazeboManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogRealGazebo, Log, TEXT("RealGazebo Manager: Shutting down"));

    // Clear timer
    GetWorldTimerManager().ClearTimer(StatusUpdateTimer);

    // Cleanup UI
    CleanupCameraUI();

    // Stop bridge if we started it
    if (bDidStartBridge)
    {
        StopBridge();
    }

    // Stop RTSP server if we started it
    if (bDidStartRTSP)
    {
        StopRTSPServer();
    }

    // Clear references
    BridgeSubsystem = nullptr;
    UISubsystem = nullptr;
    StreamingSubsystem = nullptr;
    ViewerDirector = nullptr;

    Super::EndPlay(EndPlayReason);
}

void ARealGazeboManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Tick logic can be added here if needed
}

#if WITH_EDITOR
void ARealGazeboManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Reconfigure subsystems when properties change
    if (PropertyChangedEvent.Property != nullptr && (BridgeSubsystem.IsValid() || UISubsystem.IsValid() || StreamingSubsystem.IsValid()))
    {
        ConfigureBridgeSubsystem();
        ConfigureUISubsystem();
        ConfigureStreamingSubsystem();
    }
}
#endif


void ARealGazeboManager::StartBridge()
{
    if (!BridgeSubsystem.IsValid())
    {
        UE_LOG(LogRealGazebo, Error, TEXT("Cannot start bridge - Bridge subsystem not available"));
        BridgeStatus = TEXT("Error - No Subsystem");
        return;
    }

    // Convert unified DataTable to Bridge-compatible format
    UDataTable* BridgeCompatibleTable = CreateBridgeCompatibleDataTable();
    if (!BridgeCompatibleTable)
    {
        UE_LOG(LogRealGazebo, Error, TEXT("Failed to create Bridge-compatible DataTable"));
        BridgeStatus = TEXT("Error - DataTable Conversion Failed");
        return;
    }

    // Configure bridge subsystem with the DataTable
    BridgeSubsystem->VehicleConfigTable = BridgeCompatibleTable;
    BridgeSubsystem->ListenPort = ListenPort;
    BridgeSubsystem->ServerIPAddress = ServerIPAddress;
    BridgeSubsystem->bAutoSpawnVehicles = bAutoSpawnVehicles;
    BridgeSubsystem->SetUpdateFrequency(UpdateFrequency);

    // Configure pool settings
    ConfigureBridgePoolSettings();

    // Start the bridge
    BridgeSubsystem->StartBridge();

    bDidStartBridge = true;
    BridgeStatus = TEXT("Active");
    OnBridgeStarted.Broadcast(FBridgePoseData());

    UE_LOG(LogRealGazebo, Log, TEXT("Bridge started on port %d"), ListenPort);
}

void ARealGazeboManager::StopBridge()
{
    if (BridgeSubsystem.IsValid())
    {
        BridgeSubsystem->StopBridge();
        bDidStartBridge = false;
        BridgeStatus = TEXT("Stopped");
        OnBridgeStopped.Broadcast(FBridgePoseData());
        UE_LOG(LogRealGazebo, Log, TEXT("Bridge stopped"));
    }
}

bool ARealGazeboManager::IsBridgeActive() const
{
    return BridgeSubsystem.IsValid() && BridgeSubsystem->IsBridgeActive();
}

void ARealGazeboManager::ClearAllVehicles()
{
    if (BridgeSubsystem.IsValid())
    {
        BridgeSubsystem->ClearAllVehicles();
        ActiveVehiclesCount = 0;
    }
}

//----------------------------------------------------------
// UI Control API
//----------------------------------------------------------

UUserWidget* ARealGazeboManager::CreateMainWidget()
{
    if (!UISubsystem.IsValid())
    {
        UE_LOG(LogRealGazebo, Warning, TEXT("UI subsystem not available - UI features disabled"));
        UIStatus = TEXT("No UI Subsystem");
        return nullptr;
    }

    if (!MainWidgetClass)
    {
        UE_LOG(LogRealGazebo, Warning, TEXT("MainWidgetClass not set - UI creation skipped"));
        UIStatus = TEXT("No Widget Class Set");
        return nullptr;
    }

    // Create widget through player controller
    if (APlayerController* PC = GetPlayerController())
    {
        UUserWidget* CreatedWidget = CreateWidget<UUserWidget>(PC, MainWidgetClass);
        if (CreatedWidget)
        {
            UE_LOG(LogRealGazebo, Log, TEXT("Main widget created successfully"));
            UIStatus = TEXT("Widget Created");
            return CreatedWidget;
        }
        else
        {
            UE_LOG(LogRealGazebo, Error, TEXT("Failed to create widget from MainWidgetClass"));
            UIStatus = TEXT("Widget Creation Failed");
            return nullptr;
        }
    }
    else
    {
        UE_LOG(LogRealGazebo, Error, TEXT("Player controller not available"));
        UIStatus = TEXT("No Player Controller");
        return nullptr;
    }
}

void ARealGazeboManager::AddMainWidgetToViewport()
{
    // This method would be called after CreateMainWidget succeeds
    // For now, just update status - actual widget management depends on the created widget
    WidgetInViewportStatus = true;
    UIStatus = TEXT("Widget in Viewport");
    UE_LOG(LogRealGazebo, Log, TEXT("Widget added to viewport (placeholder implementation)"));
}

void ARealGazeboManager::InitializeCameraUI()
{
    if (!UISubsystem.IsValid())
    {
        UE_LOG(LogRealGazebo, Error, TEXT("Cannot initialize UI - UI subsystem not available"));
        return;
    }

    // Convert unified DataTable to UI-compatible format
    UDataTable* UICompatibleTable = CreateUICompatibleDataTable();
    if (!UICompatibleTable)
    {
        UE_LOG(LogRealGazebo, Error, TEXT("Failed to create UI-compatible DataTable"));
        UIStatus = TEXT("Error - DataTable Conversion Failed");
        return;
    }

    UISubsystem->InitializeCameraUI(
        MainWidgetClass,
        UICompatibleTable,
        InitialCameraLocation,
        InitialCameraRotation,
        bAutoCreateViewerDirector,
        bAutoAddToViewport ? WidgetZOrder : -1
    );

    ViewerDirector = UISubsystem->GetViewerDirector();
    BindViewerDirectorEvents();

    // Configure camera presets on the viewer director
    if (ViewerDirector.IsValid() && CameraPresets.Num() > 0)
    {
        ViewerDirector->SetCameraPresets(CameraPresets);
        UE_LOG(LogRealGazebo, Log, TEXT("Configured %d camera presets on ViewerDirector"),
               CameraPresets.Num());
    }

    UISubsystem->SetMouseCursorAlwaysVisible(bAlwaysShowMouseCursor);

    bDidStartUI = true;
    UUserWidget* CreatedWidget = UISubsystem->GetActiveMainWidget();
    UIStatus = CreatedWidget ? TEXT("UI Active") : TEXT("UI Partial (No Widget)");

    UE_LOG(LogRealGazebo, Log, TEXT("Camera UI initialized: Widget=%s, ViewerDirector=%s"),
           CreatedWidget ? TEXT("OK") : TEXT("FAILED"),
           ViewerDirector.IsValid() ? TEXT("OK") : TEXT("FAILED"));
}

void ARealGazeboManager::CleanupCameraUI()
{
    UnbindViewerDirectorEvents();

    if (UISubsystem.IsValid())
    {
        UISubsystem->CleanupCameraUI();
    }

    ViewerDirector = nullptr;
    InputSelectedVehicleItem = nullptr;
    WidgetInViewportStatus = false;
    bDidStartUI = false;
    UIStatus = TEXT("Cleaned Up");
    OnUICleanedUp();

    UE_LOG(LogRealGazebo, Log, TEXT("Camera UI cleaned up"));
}

void ARealGazeboManager::BindViewerDirectorEvents()
{
    if (!ViewerDirector.IsValid())
    {
        return;
    }

    ViewerDirector->OnVehicleChangedByInput().RemoveAll(this);
    ViewerDirector->OnVehicleChangedByInput().AddUObject(
        this,
        &ARealGazeboManager::HandleVehicleChangedByInput);
}

void ARealGazeboManager::UnbindViewerDirectorEvents()
{
    if (ViewerDirector.IsValid())
    {
        ViewerDirector->OnVehicleChangedByInput().RemoveAll(this);
    }
}

void ARealGazeboManager::HandleVehicleChangedByInput(AVehicleBasePawn* Vehicle)
{
    InputSelectedVehicleItem = nullptr;

    if (IsValid(Vehicle))
    {
        URealGazeboVehicleListItem* VehicleItem = NewObject<URealGazeboVehicleListItem>(this);
        VehicleItem->VehicleID = Vehicle->VehicleID;
        VehicleItem->VehicleTypeName = Vehicle->VehicleTypeName.IsEmpty()
            ? FString::Printf(TEXT("Type_%d"), Vehicle->VehicleType)
            : Vehicle->VehicleTypeName;
        VehicleItem->VehicleName = FString::Printf(
            TEXT("%s_%d"),
            *VehicleItem->VehicleTypeName,
            Vehicle->VehicleID.VehicleNum);
        VehicleItem->UpdateFromRuntimeData(Vehicle->GetCurrentRuntimeData());
        InputSelectedVehicleItem = VehicleItem;
    }

    OnVehicleChangedByInput.Broadcast(InputSelectedVehicleItem);
}

void ARealGazeboManager::SetMouseCursorAlwaysVisible(bool bVisible)
{
    if (APlayerController* PC = GetPlayerController())
    {
        PC->bShowMouseCursor = bVisible;
        PC->bEnableClickEvents = bVisible;
        PC->bEnableMouseOverEvents = bVisible;
    }
}

//----------------------------------------------------------
// Status Information API
//----------------------------------------------------------

int32 ARealGazeboManager::GetActiveVehicleCount() const
{
    if (BridgeSubsystem.IsValid())
    {
        return BridgeSubsystem->GetActiveVehicleCount();
    }
    return 0;
}

void ARealGazeboManager::GetNetworkStatistics(int32& OutValidPackets, int32& OutInvalidPackets, float& OutPacketsPerSecond) const
{
    // Implementation would get stats from bridge subsystem
    OutValidPackets = 0;
    OutInvalidPackets = 0;
    OutPacketsPerSecond = 0.0f;
}

UUserWidget* ARealGazeboManager::GetMainWidget() const
{
    // Implementation would return the created widget
    return nullptr;
}

bool ARealGazeboManager::IsUIActive() const
{
    return bDidStartUI && UISubsystem.IsValid();
}

ARealGazeboViewerDirector* ARealGazeboManager::GetViewerDirector() const
{
    return ViewerDirector.Get();
}

bool ARealGazeboManager::ValidateSetup()
{
    // Validate essential data tables
    if (!ValidateVehicleDataTable())
    {
        return false;
    }

    // Validate UI configuration if enabled
    if (bAutoCreateUI && !ValidateUIConfiguration())
    {
        return false;
    }

    // Validate Bridge subsystem availability
    if (!ValidateBridgeSubsystem())
    {
        return false;
    }

    return true;
}

//----------------------------------------------------------
// Camera Control API
//----------------------------------------------------------

void ARealGazeboManager::SetCameraMode(int32 NewMode)
{
    if (ViewerDirector.IsValid())
    {
        // Implementation would set camera mode through viewer director
        UE_LOG(LogRealGazebo, Log, TEXT("Setting camera mode to: %d"), NewMode);
    }
}

int32 ARealGazeboManager::GetCameraMode() const
{
    if (ViewerDirector.IsValid())
    {
        // Implementation would get camera mode from viewer director
        return 0; // Default mode
    }
    return 0;
}

void ARealGazeboManager::SetMainWidgetVisibility(bool bVisible)
{
    if (bVisible)
    {
        if (!bDidStartUI)
        {
            InitializeCameraUI();
        }
        AddMainWidgetToViewport();
    }
    else
    {
        CleanupCameraUI();
    }
}

//----------------------------------------------------------
// Streaming Control API
//----------------------------------------------------------

bool ARealGazeboManager::StartRTSPServer()
{
    if (!StreamingSubsystem.IsValid())
    {
        UE_LOG(LogRealGazebo, Error, TEXT("Cannot start RTSP - Streaming subsystem not available"));
        StreamingStatus = TEXT("Error - No Subsystem");
        return false;
    }

    if (StreamingSubsystem->StartRTSPServer(RTSPPort))
    {
        bDidStartRTSP = true;
        StreamingStatus = TEXT("Active");
        UE_LOG(LogRealGazebo, Log, TEXT("RTSP server started on port %d"), RTSPPort);
        return true;
    }

    StreamingStatus = TEXT("Failed to Start");
    UE_LOG(LogRealGazebo, Error, TEXT("Failed to start RTSP server on port %d"), RTSPPort);
    return false;
}

void ARealGazeboManager::StopRTSPServer()
{
    if (StreamingSubsystem.IsValid())
    {
        StreamingSubsystem->StopRTSPServer();
        bDidStartRTSP = false;
        StreamingStatus = TEXT("Stopped");
        UE_LOG(LogRealGazebo, Log, TEXT("RTSP server stopped"));
    }
}

bool ARealGazeboManager::IsRTSPServerRunning() const
{
    if (StreamingSubsystem.IsValid())
    {
        return StreamingSubsystem->IsRTSPServerRunning();
    }
    return false;
}

int32 ARealGazeboManager::GetActiveStreamCount() const
{
    if (StreamingSubsystem.IsValid())
    {
        return StreamingSubsystem->GetActiveStreamCount();
    }
    return 0;
}

int32 ARealGazeboManager::GetRegisteredCameraCount() const
{
    if (StreamingSubsystem.IsValid())
    {
        return StreamingSubsystem->GetRegisteredCameraCount();
    }
    return 0;
}

//----------------------------------------------------------
// Configuration Methods
//----------------------------------------------------------

void ARealGazeboManager::ConfigureBridgeSubsystem()
{
    if (!BridgeSubsystem.IsValid())
    {
        return;
    }

    UE_LOG(LogRealGazebo, Log, TEXT("Configuring Bridge Subsystem"));

    // Configure bridge subsystem with our settings
    // Note: Actual configuration would depend on BridgeSubsystem's API
    ConfigureVehiclePoolSettings();
    ConfigureNetworkProcessingSettings();
    ConfigurePerformanceAndDebugSettings();
}

void ARealGazeboManager::ConfigureUISubsystem()
{
    if (!UISubsystem.IsValid())
    {
        return;
    }

    UE_LOG(LogRealGazebo, Log, TEXT("Configuring UI Subsystem"));

    // Configure UI subsystem with our settings
    // Note: Actual configuration would depend on UISubsystem's API
}

void ARealGazeboManager::ConfigureStreamingSubsystem()
{
    if (!StreamingSubsystem.IsValid())
    {
        return;
    }

    UE_LOG(LogRealGazebo, Log, TEXT("Configuring Streaming Subsystem"));

    // Configure streaming subsystem with our settings
    // Currently, RealGazeboStreamingSubsystem doesn't require pre-initialization configuration
    // RTSP server will be started in BeginPlay if bAutoStartRTSP is true
    // Stream resolution and FPS are configured per-camera in UVehicleCameraComponent
}

void ARealGazeboManager::ConfigureVehiclePoolSettings()
{
    UE_LOG(LogRealGazebo, Verbose, TEXT("Configuring Vehicle Pool Settings"));
    // Implementation would configure pool management settings in bridge subsystem
}

void ARealGazeboManager::ConfigureNetworkProcessingSettings()
{
    UE_LOG(LogRealGazebo, Verbose, TEXT("Configuring Network Processing Settings"));
    // Implementation would configure network settings in bridge subsystem
}

void ARealGazeboManager::ConfigurePerformanceAndDebugSettings()
{
    UE_LOG(LogRealGazebo, Verbose, TEXT("Configuring Performance and Debug Settings"));
    // Implementation would configure performance settings in bridge subsystem
}

APlayerController* ARealGazeboManager::GetPlayerController() const
{
    return UGameplayStatics::GetPlayerController(this, 0);
}

void ARealGazeboManager::ConfigureBridgePoolSettings()
{
    if (!BridgeSubsystem.IsValid())
    {
        return;
    }

    // Configure VehiclePoolManager through Bridge subsystem
    if (UVehiclePoolManager* PoolManager = BridgeSubsystem->GetVehiclePoolManager())
    {
        PoolManager->MaxActorsPerType = MaxActorsPerType;
        PoolManager->InitialPoolSize = InitialPoolSize;
        PoolManager->bAutoExpandPools = bAutoExpandPools;
        PoolManager->PoolExpansionSize = PoolExpansionSize;
        PoolManager->bAutoShrinkPools = bAutoShrinkPools;
        PoolManager->UnusedActorTimeout = UnusedActorTimeout;

        UE_LOG(LogRealGazebo, Log, TEXT("RealGazebo Manager [%s]: Configured pool settings - MaxActorsPerType: %d, InitialPoolSize: %d"),
               *GetName(), MaxActorsPerType, InitialPoolSize);
    }
}

//----------------------------------------------------------
// DataTable Conversion Helpers
//----------------------------------------------------------

UDataTable* ARealGazeboManager::CreateBridgeCompatibleDataTable()
{
    if (!UnifiedVehicleDataTable)
    {
        UE_LOG(LogRealGazebo, Warning, TEXT("Cannot create Bridge DataTable - UnifiedVehicleDataTable is null"));
        return nullptr;
    }

    RuntimeBridgeDataTable = NewObject<UDataTable>(this, UDataTable::StaticClass(), TEXT("RuntimeBridgeDataTable"));
    RuntimeBridgeDataTable->RowStruct = FBridgeVehicleConfigRow::StaticStruct();

    TArray<FName> RowNames = UnifiedVehicleDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        if (const FRealGazeboVehicleConfigRow* UnifiedRow = UnifiedVehicleDataTable->FindRow<FRealGazeboVehicleConfigRow>(RowName, TEXT("")))
        {
            RuntimeBridgeDataTable->AddRow(RowName, UnifiedRow->ToBridgeConfigRow());
        }
    }

    UE_LOG(LogRealGazebo, Verbose, TEXT("Created Bridge DataTable with %d rows"), RowNames.Num());
    return RuntimeBridgeDataTable;
}

UDataTable* ARealGazeboManager::CreateUICompatibleDataTable()
{
    if (!UnifiedVehicleDataTable)
    {
        UE_LOG(LogRealGazebo, Warning, TEXT("Cannot create UI DataTable - UnifiedVehicleDataTable is null"));
        return nullptr;
    }

    RuntimeUIDataTable = NewObject<UDataTable>(this, UDataTable::StaticClass(), TEXT("RuntimeUIDataTable"));
    RuntimeUIDataTable->RowStruct = FVehicleTypeImageRow::StaticStruct();

    TArray<FName> RowNames = UnifiedVehicleDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        if (const FRealGazeboVehicleConfigRow* UnifiedRow = UnifiedVehicleDataTable->FindRow<FRealGazeboVehicleConfigRow>(RowName, TEXT("")))
        {
            RuntimeUIDataTable->AddRow(RowName, UnifiedRow->ToVehicleTypeImageRow());
        }
    }

    UE_LOG(LogRealGazebo, Verbose, TEXT("Created UI DataTable with %d rows"), RowNames.Num());
    return RuntimeUIDataTable;
}

//----------------------------------------------------------
// Validation Methods
//----------------------------------------------------------

bool ARealGazeboManager::ValidateVehicleDataTable() const
{
    if (!UnifiedVehicleDataTable)
    {
        UE_LOG(LogRealGazebo, Warning, TEXT("UnifiedVehicleDataTable not set - using fallback configuration"));
        return true;
    }

    if (UnifiedVehicleDataTable->GetRowStruct() != FRealGazeboVehicleConfigRow::StaticStruct())
    {
        UE_LOG(LogRealGazebo, Error, TEXT("UnifiedVehicleDataTable must use FRealGazeboVehicleConfigRow structure"));
        return false;
    }

    if (UnifiedVehicleDataTable->GetRowNames().Num() == 0)
    {
        UE_LOG(LogRealGazebo, Warning, TEXT("UnifiedVehicleDataTable is empty"));
        return true;
    }

    return true;
}

bool ARealGazeboManager::ValidateUIConfiguration() const
{
    if (!MainWidgetClass)
    {
        UE_LOG(LogRealGazebo, Error, TEXT("MainWidgetClass not set but Auto Create UI is enabled"));
        return false;
    }

    if (!UnifiedVehicleDataTable)
    {
        UE_LOG(LogRealGazebo, Warning, TEXT("UnifiedVehicleDataTable not set - UI will use default icons"));
        return true;
    }

    // Check if any rows have vehicle images configured
    TArray<FName> RowNames = UnifiedVehicleDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        if (const FRealGazeboVehicleConfigRow* Row = UnifiedVehicleDataTable->FindRow<FRealGazeboVehicleConfigRow>(RowName, TEXT("")))
        {
            if (Row->HasVehicleImage())
            {
                return true;
            }
        }
    }

    UE_LOG(LogRealGazebo, Warning, TEXT("No vehicle images configured - UI will use default icons"));
    return true;
}

bool ARealGazeboManager::ValidateBridgeSubsystem() const
{
    if (!BridgeSubsystem.IsValid())
    {
        UE_LOG(LogRealGazebo, Error, TEXT("Bridge subsystem not available"));
        return false;
    }
    return true;
}

//----------------------------------------------------------

void ARealGazeboManager::UpdateStatusDisplay()
{
    // Update active vehicle count
    ActiveVehiclesCount = GetActiveVehicleCount();

    // Update bridge status
    if (BridgeSubsystem.IsValid())
    {
        if (BridgeSubsystem->IsBridgeActive())
        {
            BridgeStatus = TEXT("Active");
        }
        else
        {
            BridgeStatus = bDidStartBridge ? TEXT("Stopped") : TEXT("Not Started");
        }
    }
    else
    {
        BridgeStatus = TEXT("No Subsystem");
    }

    // Update UI status
    if (UISubsystem.IsValid())
    {
        if (bDidStartUI)
        {
            UIStatus = TEXT("Active");

            // Update widget viewport status dynamically
            UUserWidget* MainWidget = UISubsystem->GetActiveMainWidget();
            WidgetInViewportStatus = (MainWidget != nullptr && MainWidget->IsInViewport());
        }
        else
        {
            UIStatus = TEXT("Not Started");
            WidgetInViewportStatus = false;
        }
    }
    else
    {
        UIStatus = TEXT("No Subsystem");
        WidgetInViewportStatus = false;
    }

    // Update streaming status
    if (StreamingSubsystem.IsValid())
    {
        if (StreamingSubsystem->IsRTSPServerRunning())
        {
            ActiveStreamsCount = StreamingSubsystem->GetActiveStreamCount();
            int32 RegisteredCameras = StreamingSubsystem->GetRegisteredCameraCount();

            StreamingStatus = FString::Printf(TEXT("Active (%d/%d streams)"),
                ActiveStreamsCount, RegisteredCameras);
        }
        else
        {
            StreamingStatus = bDidStartRTSP ? TEXT("Stopped") : TEXT("Not Started");
            ActiveStreamsCount = 0;
        }
    }
    else
    {
        StreamingStatus = TEXT("No Subsystem");
        ActiveStreamsCount = 0;
    }
}
