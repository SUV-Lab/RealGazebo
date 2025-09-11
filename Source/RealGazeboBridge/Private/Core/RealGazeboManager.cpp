#include "Core/RealGazeboManager.h"
#include "Core/GazeboBridgeSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogRealGazeboManager, Log, All);

ARealGazeboManager::ARealGazeboManager()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f; // Update status every second

    // Create root component for visibility in editor
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // Add billboard for easy identification in editor
    if (UBillboardComponent* BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard")))
    {
        BillboardComponent->SetupAttachment(RootComponent);
        BillboardComponent->bIsEditorOnly = true;
    }

    // Add arrow to show "direction" of communication
    if (UArrowComponent* ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow")))
    {
        ArrowComponent->SetupAttachment(RootComponent);
        ArrowComponent->bIsEditorOnly = true;
        ArrowComponent->ArrowLength = 100.0f;
        ArrowComponent->ArrowSize = 2.0f;
    }
}

void ARealGazeboManager::BeginPlay()
{
    Super::BeginPlay();

    // Get reference to the subsystem
    BridgeSubsystem = UGazeboBridgeSubsystem::GetBridgeSubsystem(this);
    
    if (!BridgeSubsystem.IsValid())
    {
        UE_LOG(LogRealGazeboManager, Error, TEXT("Failed to get GazeboBridgeSubsystem! Make sure the plugin is properly loaded."));
        return;
    }

    // Validate configuration
    if (!ValidateConfiguration())
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("Configuration validation failed. Bridge will not start automatically."));
        return;
    }

    // Configure the subsystem with our settings
    ConfigureSubsystem();

    // Auto-start if enabled
    if (bAutoStart)
    {
        StartBridge();
    }

    // Start status update timer
    GetWorld()->GetTimerManager().SetTimer(StatusUpdateTimer, this, &ARealGazeboManager::UpdateStatusDisplay, 1.0f, true);
}

void ARealGazeboManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up timer
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(StatusUpdateTimer);
    }

    // Stop the bridge if we started it
    if (bDidStartSubsystem && BridgeSubsystem.IsValid())
    {
        StopBridge();
    }

    Super::EndPlay(EndPlayReason);
}

void ARealGazeboManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Tick is used for status updates via timer
}

#if WITH_EDITOR
void ARealGazeboManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Re-configure subsystem when properties change in editor
    if (BridgeSubsystem.IsValid() && PropertyChangedEvent.Property)
    {
        ConfigureSubsystem();
    }
}
#endif

void ARealGazeboManager::StartBridge()
{
    if (!BridgeSubsystem.IsValid())
    {
        UE_LOG(LogRealGazeboManager, Error, TEXT("Cannot start bridge - subsystem not available"));
        return;
    }

    if (!ValidateConfiguration())
    {
        UE_LOG(LogRealGazeboManager, Error, TEXT("Cannot start bridge - invalid configuration"));
        return;
    }

    // Configure subsystem with our settings
    ConfigureSubsystem();

    // Start the bridge
    BridgeSubsystem->StartBridge();
    bDidStartSubsystem = true;

    UE_LOG(LogRealGazeboManager, Log, TEXT("RealGazebo Bridge started - Port: %d, IP: %s"), ListenPort, *ServerIPAddress);

    // Broadcast event
    if (OnBridgeStarted.IsBound())
    {
        FBridgePoseData DummyData; // Event expects pose data, but we're using it for bridge status
        OnBridgeStarted.Broadcast(DummyData);
    }
}

void ARealGazeboManager::StopBridge()
{
    if (!BridgeSubsystem.IsValid())
    {
        return;
    }

    BridgeSubsystem->StopBridge();
    bDidStartSubsystem = false;

    UE_LOG(LogRealGazeboManager, Log, TEXT("RealGazebo Bridge stopped"));

    // Broadcast event
    if (OnBridgeStopped.IsBound())
    {
        FBridgePoseData DummyData;
        OnBridgeStopped.Broadcast(DummyData);
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
        UE_LOG(LogRealGazeboManager, Log, TEXT("All vehicles cleared"));
    }
}

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
    OutValidPackets = 0;
    OutInvalidPackets = 0;
    OutPacketsPerSecond = 0.0f;

    // This would need to be implemented in the subsystem to provide network stats
    // For now, we'll just return zeros
}

void ARealGazeboManager::ConfigureSubsystem()
{
    if (!BridgeSubsystem.IsValid())
    {
        return;
    }

    // Configure basic network settings
    BridgeSubsystem->ListenPort = ListenPort;
    BridgeSubsystem->ServerIPAddress = ServerIPAddress;
    BridgeSubsystem->bAutoSpawnVehicles = bAutoSpawnVehicles;
    BridgeSubsystem->SetUpdateFrequency(UpdateFrequency);

    // Configure vehicle data table
    BridgeSubsystem->VehicleConfigTable = VehicleDataTable;

    // Configure Vehicle Pool Settings
    ConfigureVehiclePoolSettings();

    // Configure Network Processing Settings
    ConfigureNetworkProcessingSettings();

    // Configure Performance and Debug Settings
    ConfigurePerformanceAndDebugSettings();

    UE_LOG(LogRealGazeboManager, Log, TEXT("Subsystem configured - Port: %d, IP: %s, DataTable: %s"), 
           ListenPort, 
           *ServerIPAddress, 
           VehicleDataTable ? *VehicleDataTable->GetName() : TEXT("None"));
}

void ARealGazeboManager::ConfigureVehiclePoolSettings()
{
    if (!BridgeSubsystem.IsValid())
    {
        return;
    }

    // Configure VehiclePoolManager through subsystem
    if (UVehiclePoolManager* PoolManager = BridgeSubsystem->GetVehiclePoolManager())
    {
        PoolManager->MaxActorsPerType = MaxActorsPerType;
        PoolManager->InitialPoolSize = InitialPoolSize;
        PoolManager->bAutoExpandPools = bAutoExpandPools;
        PoolManager->PoolExpansionSize = PoolExpansionSize;
        PoolManager->bAutoShrinkPools = bAutoShrinkPools;
        PoolManager->UnusedActorTimeout = UnusedActorTimeout;
        
        UE_LOG(LogRealGazeboManager, Verbose, TEXT("Pool settings configured - MaxPerType: %d, InitialSize: %d"), 
               MaxActorsPerType, InitialPoolSize);
    }
}

void ARealGazeboManager::ConfigureNetworkProcessingSettings()
{
    if (!BridgeSubsystem.IsValid())
    {
        return;
    }

    // Configure DataStreamProcessor through subsystem
    if (UDataStreamProcessor* StreamProcessor = BridgeSubsystem->GetDataStreamProcessor())
    {
        StreamProcessor->bEnableBatchProcessing = bEnableBatchProcessing;
        StreamProcessor->BatchSize = BatchSize;
        StreamProcessor->BatchProcessingInterval = BatchProcessingInterval;
        StreamProcessor->bValidatePacketSizes = bValidatePacketSizes;
        
        UE_LOG(LogRealGazeboManager, Verbose, TEXT("Network processing configured - BatchSize: %d, BatchProcessing: %s"), 
               BatchSize, bEnableBatchProcessing ? TEXT("Enabled") : TEXT("Disabled"));
    }
}

void ARealGazeboManager::ConfigurePerformanceAndDebugSettings()
{
    if (!BridgeSubsystem.IsValid())
    {
        return;
    }

    // Apply performance and debug settings to subsystem
    // Note: These would need to be added as properties to the subsystem classes
    
    UE_LOG(LogRealGazeboManager, Verbose, TEXT("Performance settings configured - MaxActive: %d, UpdateRate: %.1f Hz"), 
           MaxActiveVehicles, UpdateFrequency);
}

bool ARealGazeboManager::ValidateConfiguration() const
{
    // Check port range
    if (ListenPort < 1024 || ListenPort > 65535)
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("Invalid port number: %d. Should be between 1024-65535"), ListenPort);
        return false;
    }

    // Check if DataTable is set
    if (!VehicleDataTable)
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("No Vehicle Data Table specified. Please assign a DataTable."));
        return false;
    }

    // Validate DataTable format
    if (!IsValidVehicleDataTable(VehicleDataTable))
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("Invalid Vehicle Data Table format."));
        return false;
    }

    // Validate pool configuration
    if (!ValidatePoolConfiguration())
    {
        return false;
    }

    // Validate network processing configuration
    if (!ValidateNetworkConfiguration())
    {
        return false;
    }

    // Validate performance configuration
    if (!ValidatePerformanceConfiguration())
    {
        return false;
    }

    return true;
}

void ARealGazeboManager::UpdateStatusDisplay()
{
    if (IsBridgeActive())
    {
        ActiveVehiclesCount = GetActiveVehicleCount();
        BridgeStatus = FString::Printf(TEXT("Active - Port: %d | Vehicles: %d"), ListenPort, ActiveVehiclesCount);
    }
    else
    {
        BridgeStatus = TEXT("Inactive");
        ActiveVehiclesCount = 0;
    }
}

bool ARealGazeboManager::IsValidVehicleDataTable(UDataTable* DataTable) const
{
    if (!DataTable)
    {
        return false;
    }

    // Check if DataTable uses the correct row structure
    if (DataTable->GetRowStruct() != FBridgeVehicleConfigRow::StaticStruct())
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("DataTable must use FBridgeVehicleConfigRow structure"));
        return false;
    }

    // Check if DataTable has at least one row
    if (DataTable->GetRowNames().Num() == 0)
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("DataTable is empty - no vehicle configurations found"));
        return false;
    }

    return true;
}

bool ARealGazeboManager::ValidatePoolConfiguration() const
{
    // Validate pool size limits
    if (MaxActorsPerType < 10 || MaxActorsPerType > 1000)
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("Invalid MaxActorsPerType: %d. Should be between 10-1000"), MaxActorsPerType);
        return false;
    }

    if (InitialPoolSize < 5 || InitialPoolSize > 100 || InitialPoolSize > MaxActorsPerType)
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("Invalid InitialPoolSize: %d. Should be between 5-100 and not exceed MaxActorsPerType"), InitialPoolSize);
        return false;
    }

    if (PoolExpansionSize < 1 || PoolExpansionSize > 50)
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("Invalid PoolExpansionSize: %d. Should be between 1-50"), PoolExpansionSize);
        return false;
    }

    if (UnusedActorTimeout < 10.0f || UnusedActorTimeout > 300.0f)
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("Invalid UnusedActorTimeout: %.1f. Should be between 10-300 seconds"), UnusedActorTimeout);
        return false;
    }

    return true;
}

bool ARealGazeboManager::ValidateNetworkConfiguration() const
{
    // Validate batch processing settings
    if (BatchSize < 1 || BatchSize > 100)
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("Invalid BatchSize: %d. Should be between 1-100"), BatchSize);
        return false;
    }

    if (BatchProcessingInterval < 0.001f || BatchProcessingInterval > 0.1f)
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("Invalid BatchProcessingInterval: %.4f. Should be between 0.001-0.1 seconds"), BatchProcessingInterval);
        return false;
    }

    return true;
}

bool ARealGazeboManager::ValidatePerformanceConfiguration() const
{
    // Validate performance limits
    if (MaxActiveVehicles < 50 || MaxActiveVehicles > 2000)
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("Invalid MaxActiveVehicles: %d. Should be between 50-2000"), MaxActiveVehicles);
        return false;
    }

    if (UpdateFrequency < 10.0f || UpdateFrequency > 120.0f)
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("Invalid UpdateFrequency: %.1f. Should be between 10-120 Hz"), UpdateFrequency);
        return false;
    }

    if (DefaultInterpolationSpeed < 1.0f || DefaultInterpolationSpeed > 100.0f)
    {
        UE_LOG(LogRealGazeboManager, Warning, TEXT("Invalid DefaultInterpolationSpeed: %.1f. Should be between 1-100"), DefaultInterpolationSpeed);
        return false;
    }


    return true;
}