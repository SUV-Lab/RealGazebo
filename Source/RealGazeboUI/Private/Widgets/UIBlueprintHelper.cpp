#include "Widgets/UIBlueprintHelper.h"
#include "RealGazeboUI.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

UUIBlueprintHelper::UUIBlueprintHelper(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bEnableDebugLogging = true;
    AutoRefreshInterval = 0.1f;
}

void UUIBlueprintHelper::NativeConstruct()
{
    Super::NativeConstruct();

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Display, TEXT("UIBlueprintHelper: Initializing"));
    }

    InitializeComponents();
    ConnectComponents();
    SetupAutoRefresh();

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Display, TEXT("UIBlueprintHelper: Initialization complete"));
    }
}

void UUIBlueprintHelper::NativeDestruct()
{
    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Display, TEXT("UIBlueprintHelper: Cleaning up"));
    }

    // Clear auto refresh timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoRefreshTimerHandle);
    }

    Super::NativeDestruct();
}

TArray<FCtrackEntryData> UUIBlueprintHelper::GetVehicleEntries()
{
    if (!ListManager)
    {
        return TArray<FCtrackEntryData>();
    }

    return ListManager->GetVehicleList();
}

TArray<FCtrackEntryData> UUIBlueprintHelper::GetActiveVehicleEntries()
{
    if (!ListManager)
    {
        return TArray<FCtrackEntryData>();
    }

    return ListManager->GetActiveVehicleList();
}

TArray<FCtrackEntryData> UUIBlueprintHelper::GetVehicleEntriesByType(const FString& VehicleType)
{
    if (!ListManager)
    {
        return TArray<FCtrackEntryData>();
    }

    return ListManager->GetVehicleListByType(VehicleType);
}

int32 UUIBlueprintHelper::GetVehicleCount()
{
    if (!EntryBridge)
    {
        return 0;
    }

    return EntryBridge->GetActiveVehicleCount();
}

bool UUIBlueprintHelper::SelectVehicle(const FString& VehicleName)
{
    if (!ListManager || !CameraController)
    {
        return false;
    }

    bool bSelected = ListManager->SelectVehicle(VehicleName);
    if (bSelected)
    {
        CameraController->SetSelectedVehicle(VehicleName);
        
        if (bEnableDebugLogging)
        {
            UE_LOG(LogRealGazeboUI, Log, TEXT("UIBlueprintHelper: Vehicle '%s' selected"), *VehicleName);
        }
    }

    return bSelected;
}

FCtrackEntryData UUIBlueprintHelper::GetSelectedVehicle()
{
    if (!ListManager)
    {
        return FCtrackEntryData();
    }

    return ListManager->GetSelectedVehicle();
}

bool UUIBlueprintHelper::HasSelectedVehicle()
{
    if (!ListManager)
    {
        return false;
    }

    return ListManager->HasSelectedVehicle();
}

void UUIBlueprintHelper::ClearVehicleSelection()
{
    if (ListManager)
    {
        ListManager->ClearSelection();
    }

    if (CameraController)
    {
        CameraController->SetSelectedVehicle(TEXT(""));
    }

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("UIBlueprintHelper: Vehicle selection cleared"));
    }
}

void UUIBlueprintHelper::SwitchToManualCamera()
{
    if (!CameraController)
    {
        return;
    }

    CameraController->SwitchToManualMode();

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("UIBlueprintHelper: Switched to manual camera"));
    }
}

bool UUIBlueprintHelper::SwitchToFirstPersonCamera()
{
    if (!CameraController)
    {
        return false;
    }

    bool bSuccess = CameraController->SwitchToFirstPersonMode();

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("UIBlueprintHelper: First person camera switch %s"), 
               bSuccess ? TEXT("succeeded") : TEXT("failed"));
    }

    return bSuccess;
}

bool UUIBlueprintHelper::SwitchToThirdPersonCamera()
{
    if (!CameraController)
    {
        return false;
    }

    bool bSuccess = CameraController->SwitchToThirdPersonMode();

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("UIBlueprintHelper: Third person camera switch %s"), 
               bSuccess ? TEXT("succeeded") : TEXT("failed"));
    }

    return bSuccess;
}

ERealGazeboCameraMode UUIBlueprintHelper::GetCurrentCameraMode()
{
    if (!CameraController)
    {
        return ERealGazeboCameraMode::Manual;
    }

    return CameraController->GetCurrentCameraMode();
}

FString UUIBlueprintHelper::GetCameraStatusText()
{
    if (!CameraController)
    {
        return TEXT("Camera: Not Available");
    }

    return CameraController->GetCameraStatusText();
}

void UUIBlueprintHelper::HandleKeyboardInput(const FString& KeyName)
{
    if (!CameraController)
    {
        return;
    }

    CameraController->HandleKeyPress(KeyName);

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("UIBlueprintHelper: Handled key press '%s'"), *KeyName);
    }
}

void UUIBlueprintHelper::RefreshData()
{
    if (ListManager)
    {
        ListManager->RefreshVehicleList();
    }

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Verbose, TEXT("UIBlueprintHelper: Data refreshed"));
    }
}

TArray<FString> UUIBlueprintHelper::GetAvailableVehicleTypes()
{
    if (!ListManager)
    {
        return TArray<FString>();
    }

    return ListManager->GetAvailableVehicleTypes();
}

FString UUIBlueprintHelper::TestConnection()
{
    TArray<FString> TestResults;

    // Test EntryBridge
    if (EntryBridge)
    {
        TestResults.Add(FString::Printf(TEXT("EntryBridge: Connected (%d vehicles)"), 
                                      EntryBridge->GetActiveVehicleCount()));
    }
    else
    {
        TestResults.Add(TEXT("EntryBridge: Not Connected"));
    }

    // Test ListManager
    if (ListManager)
    {
        TestResults.Add(FString::Printf(TEXT("ListManager: Connected (%d vehicles)"), 
                                      ListManager->GetActiveVehicleList().Num()));
    }
    else
    {
        TestResults.Add(TEXT("ListManager: Not Connected"));
    }

    // Test CameraController
    if (CameraController)
    {
        TestResults.Add(FString::Printf(TEXT("CameraController: %s"), 
                                      *CameraController->GetCameraStatusText()));
    }
    else
    {
        TestResults.Add(TEXT("CameraController: Not Connected"));
    }

    return FString::Join(TestResults, TEXT("\n"));
}

void UUIBlueprintHelper::SetupAutoRefresh()
{
    if (!GetWorld() || AutoRefreshInterval <= 0.0f)
    {
        return;
    }

    // Clear existing timer
    GetWorld()->GetTimerManager().ClearTimer(AutoRefreshTimerHandle);

    // Setup new timer
    GetWorld()->GetTimerManager().SetTimer(
        AutoRefreshTimerHandle,
        this,
        &UUIBlueprintHelper::AutoRefreshCallback,
        AutoRefreshInterval,
        true // Loop
    );

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("UIBlueprintHelper: Auto refresh timer started - %.1f Hz"), 
               1.0f / AutoRefreshInterval);
    }
}

void UUIBlueprintHelper::AutoRefreshCallback()
{
    RefreshData();
}

void UUIBlueprintHelper::InitializeComponents()
{
    // Create EntryBridge with proper naming
    EntryBridge = NewObject<UEntryDataBridge>(this, TEXT("UIHelper_EntryBridge"));
    if (!EntryBridge)
    {
        UE_LOG(LogRealGazeboUI, Error, TEXT("UIBlueprintHelper: Failed to create EntryBridge"));
    }

    // Create ListManager with proper naming
    ListManager = NewObject<UListManager>(this, TEXT("UIHelper_ListManager"));
    if (!ListManager)
    {
        UE_LOG(LogRealGazeboUI, Error, TEXT("UIBlueprintHelper: Failed to create ListManager"));
    }

    // Create CameraController with proper naming
    CameraController = NewObject<UUserCameraController>(this, TEXT("UIHelper_CameraController"));
    if (!CameraController)
    {
        UE_LOG(LogRealGazeboUI, Error, TEXT("UIBlueprintHelper: Failed to create CameraController"));
    }

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("UIBlueprintHelper: Components created"));
    }
}

void UUIBlueprintHelper::ConnectComponents()
{
    // Connect CameraController to ListManager
    if (CameraController && ListManager)
    {
        CameraController->SetListManager(ListManager);
        CameraController->Initialize();
    }

    if (bEnableDebugLogging)
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("UIBlueprintHelper: Components connected"));
    }
}