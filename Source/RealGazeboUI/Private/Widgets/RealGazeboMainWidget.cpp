// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#include "Widgets/RealGazeboMainWidget.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "TimerManager.h"
#include "RealGazeboUI.h"
#include "Widgets/RealGazeboVehicleEntry.h"
#include "ViewerController/RealGazeboViewerDirector.h"
#include "Vehicles/VehicleBasePawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/BlueprintGeneratedClass.h"

URealGazeboMainWidget::URealGazeboMainWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Set default values
    UpdateFrequency = 30.0f;
    MaxDisplayVehicles = 256;
    FadeDuration = 1.0f;

    // Initialize tracking variables
    LastVehicleCount = 0;
    bLastConnectionStatus = false;
    CurrentSelectedVehicleID = FVehicleID(0, 0); // Invalid ID as default
    FadeElapsedTime = 0.0f;

    // Enable ticking for real-time updates
    bIsFocusable = false;
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void URealGazeboMainWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Constructing UI
    
    // Initialize subsystem connection
    InitializeSubsystemConnection();
    
    // Setup ListView event bindings
    if (VehicleListView)
    {
        VehicleListView->OnItemSelectionChanged().AddUObject(this, &URealGazeboMainWidget::OnVehicleItemSelectionChanged);

        // Bind entry widget generation event to handle newly created/recycled widgets
        VehicleListView->OnEntryWidgetGenerated().AddUObject(this, &URealGazeboMainWidget::OnEntryWidgetGenerated);

        // ListView events bound
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("VehicleListView widget not found! Make sure it's bound in the Blueprint."));
    }

    // Initialize fade overlay to transparent and non-blocking
    if (Fade)
    {
        FLinearColor BrushColor = Fade->GetBrushColor();
        BrushColor.A = 0.0f;  // Start transparent
        Fade->SetBrushColor(BrushColor);

        // Make sure it doesn't block input - never catches mouse events
        Fade->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("Fade border not found! Make sure it's bound in the Blueprint."));
    }

    // Bind reset button
    if (Reset_Button)
    {
        Reset_Button->OnClicked.AddDynamic(this, &URealGazeboMainWidget::OnResetButtonClicked);
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("Reset_Button not found! Make sure it's bound in the Blueprint."));
    }

    // Bind sort button
    if (Sort_Button)
    {
        Sort_Button->OnClicked.AddDynamic(this, &URealGazeboMainWidget::OnSortButtonClicked);
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("Sort_Button not found! Make sure it's bound in the Blueprint."));
    }

    // Initialize sort button text
    if (!Sort_ButtonText)
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("Sort_ButtonText not found! Make sure it's bound in the Blueprint."));
    }
    UpdateSortButtonText();

    // Start regular update timer
    if (UWorld* World = GetWorld())
    {
        float UpdateInterval = 1.0f / FMath::Max(UpdateFrequency, 1.0f);
        World->GetTimerManager().SetTimer(
            UpdateTimerHandle,
            this,
            &URealGazeboMainWidget::UpdateVehicleData,
            UpdateInterval,
            true
        );
        
        // Update timer started
    }
    
    // Initial data refresh
    RefreshVehicleList();
}

void URealGazeboMainWidget::NativeDestruct()
{
    // Destructing UI

    // Clear timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UpdateTimerHandle);
    }
    
    // Clear all vehicle items
    ClearAllVehicles();
    
    Super::NativeDestruct();
}

void URealGazeboMainWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void URealGazeboMainWidget::InitializeSubsystemConnection()
{
    // Get bridge subsystem reference
    BridgeSubsystem = UGazeboBridgeSubsystem::GetBridgeSubsystem(this);
    
    if (BridgeSubsystem.IsValid())
    {
        // Connected to GazeboBridgeSubsystem
        
        // Note: Subsystem events will be handled through the update timer for now
        // Future enhancement: Add proper event handlers for OnVehicleSpawned and OnVehicleUpdated
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Error, TEXT("Failed to get GazeboBridgeSubsystem! UI will not update automatically."));
    }
}

void URealGazeboMainWidget::UpdateVehicleData()
{
    if (!BridgeSubsystem.IsValid())
    {
        return;
    }
    
    double StartTime = FPlatformTime::Seconds();

    // Get current vehicle IDs from subsystem
    TArray<FVehicleID> CurrentVehicleIDs = BridgeSubsystem->GetAllVehicleIDs();

    // Sort based on current sort mode
    if (CurrentSortMode == EVehicleSortMode::ByVehicleNum)
    {
        // Sort by VehicleNum only
        CurrentVehicleIDs.Sort([](const FVehicleID& A, const FVehicleID& B)
        {
            return A.VehicleNum < B.VehicleNum;
        });
    }
    else // EVehicleSortMode::ByVehicleType
    {
        // Sort by VehicleType first, then by VehicleNum
        CurrentVehicleIDs.Sort([](const FVehicleID& A, const FVehicleID& B)
        {
            if (A.VehicleType != B.VehicleType)
            {
                return A.VehicleType < B.VehicleType;
            }
            return A.VehicleNum < B.VehicleNum;
        });
    }

    // Track changes for performance
    bool bVehicleListChanged = false;
    
    // Add new vehicles or update existing ones
    for (const FVehicleID& VehicleID : CurrentVehicleIDs)
    {
        FVehicleRuntimeData RuntimeData = BridgeSubsystem->GetVehicleData(VehicleID);

        // Skip if this vehicle is already marked as not shown in UI
        if (SkippedVehicleIDs.Contains(VehicleID))
        {
            continue;
        }

        if (VehicleItemMap.Contains(VehicleID))
        {
            // Update existing vehicle
            UpdateVehicleInList(VehicleID, RuntimeData);
        }
        else
        {
            // Add new vehicle (or mark as skipped if not shown in UI)
            URealGazeboVehicleListItem* NewItem = AddVehicleToList(VehicleID, RuntimeData);
            if (NewItem)
            {
                bVehicleListChanged = true;
            }
            else
            {
                // Vehicle was not added to UI - mark as skipped to avoid checking again
                SkippedVehicleIDs.Add(VehicleID);
            }
        }
    }
    
    // Remove vehicles that no longer exist
    TArray<FVehicleID> VehiclesToRemove;
    for (const auto& Pair : VehicleItemMap)
    {
        if (!CurrentVehicleIDs.Contains(Pair.Key))
        {
            VehiclesToRemove.Add(Pair.Key);
            bVehicleListChanged = true;
        }
    }

    for (const FVehicleID& VehicleID : VehiclesToRemove)
    {
        RemoveVehicleFromList(VehicleID);
    }

    // Also remove skipped vehicles that no longer exist
    TArray<FVehicleID> SkippedVehiclesToRemove;
    for (const FVehicleID& SkippedID : SkippedVehicleIDs)
    {
        if (!CurrentVehicleIDs.Contains(SkippedID))
        {
            SkippedVehiclesToRemove.Add(SkippedID);
        }
    }

    for (const FVehicleID& VehicleID : SkippedVehiclesToRemove)
    {
        SkippedVehicleIDs.Remove(VehicleID);
    }
    
    // Update connection status
    if (!BridgeSubsystem.IsValid())
    {
        return;
    }
    
    bool bIsConnected = BridgeSubsystem->IsBridgeActive();
    if (bIsConnected != bLastConnectionStatus)
    {
        bLastConnectionStatus = bIsConnected;
        
        FString StatusMessage = bIsConnected ? TEXT("Connected") : TEXT("Disconnected");
        OnConnectionStatusChanged(bIsConnected, StatusMessage);
    }
    
    
    if (bVehicleListChanged)
    {
        // Vehicle list updated - notify ViewerDirector to refresh its vehicle list
        if (ViewerDirector.IsValid())
        {
            ViewerDirector->RefreshVehicleList();
            UE_LOG(LogRealGazeboUI, Log, TEXT("MainWidget: Notified ViewerDirector to refresh vehicle list due to changes"));
        }
    }
}

URealGazeboVehicleListItem* URealGazeboMainWidget::AddVehicleToList(const FVehicleID& VehicleID, const FVehicleRuntimeData& RuntimeData)
{
    // Check if this vehicle type should be shown in UI
    if (!ShouldShowVehicleInUI(RuntimeData.VehicleType))
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("Vehicle %s skipped - not shown in UI (bShowInUI=false)"),
               *VehicleID.ToString());
        return nullptr;
    }

    // Create new list item
    URealGazeboVehicleListItem* NewItem = NewObject<URealGazeboVehicleListItem>(this);
    if (!NewItem)
    {
        UE_LOG(LogRealGazeboUI, Error, TEXT("Failed to create vehicle list item for vehicle %s"), *VehicleID.ToString());
        return nullptr;
    }

    // Initialize item data
    NewItem->VehicleID = VehicleID;
    NewItem->VehicleTypeName = GetVehicleTypeName(RuntimeData.VehicleType);
    NewItem->VehicleName = GenerateVehicleDisplayName(VehicleID, RuntimeData.VehicleType);
    NewItem->UpdateFromRuntimeData(RuntimeData);

    // Add to map and ListView
    VehicleItemMap.Add(VehicleID, NewItem);

    if (VehicleListView)
    {
        VehicleListView->AddItem(NewItem);
    }

    UE_LOG(LogRealGazeboUI, Log, TEXT("Vehicle %s added to UI list"), *VehicleID.ToString());
    return NewItem;
}

void URealGazeboMainWidget::RemoveVehicleFromList(const FVehicleID& VehicleID)
{
    TObjectPtr<URealGazeboVehicleListItem>* ItemPtr = VehicleItemMap.Find(VehicleID);
    if (ItemPtr && *ItemPtr)
    {
        URealGazeboVehicleListItem* Item = *ItemPtr;
        
        // Remove from ListView
        if (VehicleListView)
        {
            VehicleListView->RemoveItem(Item);
        }
        
        // Remove from map
        VehicleItemMap.Remove(VehicleID);
        
        // Removed vehicle from list
    }
}

void URealGazeboMainWidget::UpdateVehicleInList(const FVehicleID& VehicleID, const FVehicleRuntimeData& RuntimeData)
{
    TObjectPtr<URealGazeboVehicleListItem>* ItemPtr = VehicleItemMap.Find(VehicleID);
    if (ItemPtr && *ItemPtr)
    {
        (*ItemPtr)->UpdateFromRuntimeData(RuntimeData);
        
        // Request ListView refresh for this item
        if (VehicleListView)
        {
            VehicleListView->RequestRefresh();
        }
    }
}


FString URealGazeboMainWidget::GetVehicleTypeName(uint8 VehicleType) const
{
    if (BridgeSubsystem.IsValid())
    {
        FBridgeVehicleConfigRow Config;
        if (BridgeSubsystem->GetVehicleConfig(VehicleType, Config))
        {
            return Config.VehicleName;
        }
    }
    
    // Fallback if not found in DataTable
    return FString::Printf(TEXT("Type_%d"), VehicleType);
}

FString URealGazeboMainWidget::GenerateVehicleDisplayName(const FVehicleID& VehicleID, uint8 VehicleType) const
{
    FString TypeName = GetVehicleTypeName(VehicleType);
    return FString::Printf(TEXT("%s_%d"), *TypeName, VehicleID.VehicleNum);
}

bool URealGazeboMainWidget::ShouldShowVehicleInUI(uint8 VehicleType) const
{
    if (!BridgeSubsystem.IsValid())
    {
        return false;
    }

    // Get vehicle configuration from DataTable
    FBridgeVehicleConfigRow Config;
    if (!BridgeSubsystem->GetVehicleConfig(VehicleType, Config))
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("Vehicle type %d not found in config table"), VehicleType);
        return false;
    }

    // Check if VehiclePawnClass is valid
    if (!Config.VehiclePawnClass)
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("Vehicle type %d has no VehiclePawnClass assigned"), VehicleType);
        return false;
    }

    // Use the bShowInUI flag from DataTable configuration
    if (!Config.bShowInUI)
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("Vehicle Type %d (%s): bShowInUI=false - NOT shown in UI"),
               VehicleType, *Config.VehicleName);
        return false;
    }

    // Show in UI if bShowInUI is true
    return true;
}

void URealGazeboMainWidget::OnVehicleItemSelectionChanged(UObject* SelectedItem)
{
    if (URealGazeboVehicleListItem* VehicleItem = Cast<URealGazeboVehicleListItem>(SelectedItem))
    {
        // Update current selection
        CurrentSelectedVehicleID = VehicleItem->VehicleID;

        // Notify ViewerDirector for camera integration
        if (ViewerDirector.IsValid())
        {
            AVehicleBasePawn* SelectedVehiclePawn = FindVehiclePawnByID(VehicleItem->VehicleID);
            if (SelectedVehiclePawn)
            {
                ViewerDirector->SetCurrentVehicle(SelectedVehiclePawn);
                UE_LOG(LogRealGazeboUI, Log, TEXT("MainWidget: Set camera target to vehicle %s (ID: %d_%d)"),
                       *SelectedVehiclePawn->GetName(),
                       VehicleItem->VehicleID.VehicleType,
                       VehicleItem->VehicleID.VehicleNum);
            }
            else
            {
                // Vehicle not found in world - clear camera target
                ViewerDirector->SetCurrentVehicle(nullptr);
                UE_LOG(LogRealGazeboUI, Warning, TEXT("MainWidget: Vehicle pawn not found for ID %d_%d, cleared camera target"),
                       VehicleItem->VehicleID.VehicleType,
                       VehicleItem->VehicleID.VehicleNum);
            }
        }
        else
        {
            UE_LOG(LogRealGazeboUI, Log, TEXT("MainWidget: No ViewerDirector available for camera integration"));
        }

        // Call Blueprint events
        OnVehicleSelected(VehicleItem);
        OnCameraTargetChanged(VehicleItem);
    }
    else
    {
        // Selection cleared
        CurrentSelectedVehicleID = FVehicleID(0, 0); // Invalid ID

        // Clear ViewerDirector target
        if (ViewerDirector.IsValid())
        {
            ViewerDirector->SetCurrentVehicle(nullptr);
            UE_LOG(LogRealGazeboUI, Log, TEXT("MainWidget: Cleared camera target (selection cleared)"));
        }

        // Notify Blueprint that camera target was cleared
        OnCameraTargetChanged(nullptr);
    }
}

void URealGazeboMainWidget::OnEntryWidgetGenerated(UUserWidget& GeneratedWidget)
{
    // This is called whenever a ListView entry widget is created or recycled
    if (URealGazeboVehicleEntry* VehicleEntry = Cast<URealGazeboVehicleEntry>(&GeneratedWidget))
    {
        // Entry widget generated/recycled

        // Configure DataTable if available
        if (VehicleTypeImageDataTable)
        {
            VehicleEntry->SetVehicleTypeImageDataTable(VehicleTypeImageDataTable);
        }

        // The widget will have its data set via NativeOnListItemObjectSet automatically.
    }
}


void URealGazeboMainWidget::ClearAllVehicles()
{
    if (VehicleListView)
    {
        VehicleListView->ClearListItems();
    }

    VehicleItemMap.Empty();
    SkippedVehicleIDs.Empty();
    // Cleared all vehicles from list
}

void URealGazeboMainWidget::RefreshVehicleList()
{
    ClearAllVehicles();
    UpdateVehicleData();
}

TArray<URealGazeboVehicleListItem*> URealGazeboMainWidget::GetAllVehicleItems() const
{
    TArray<URealGazeboVehicleListItem*> Items;
    for (const auto& Pair : VehicleItemMap)
    {
        Items.Add(Pair.Value);
    }
    return Items;
}

URealGazeboVehicleListItem* URealGazeboMainWidget::GetVehicleItem(const FVehicleID& VehicleID) const
{
    if (const TObjectPtr<URealGazeboVehicleListItem>* ItemPtr = VehicleItemMap.Find(VehicleID))
    {
        return *ItemPtr;
    }
    return nullptr;
}

TArray<URealGazeboVehicleListItem*> URealGazeboMainWidget::GetSelectedVehicleItems() const
{
    TArray<URealGazeboVehicleListItem*> SelectedItems;
    
    if (VehicleListView)
    {
        TArray<UObject*> SelectedObjects;
        VehicleListView->GetSelectedItems(SelectedObjects);
        for (UObject* Object : SelectedObjects)
        {
            if (URealGazeboVehicleListItem* VehicleItem = Cast<URealGazeboVehicleListItem>(Object))
            {
                SelectedItems.Add(VehicleItem);
            }
        }
    }
    
    return SelectedItems;
}




void URealGazeboMainWidget::SetSelectedVehicle(FVehicleID VehicleID)
{
    // Check if already selected
    if (CurrentSelectedVehicleID == VehicleID)
    {
        return;
    }

    FVehicleID PreviousSelectedVehicleID = CurrentSelectedVehicleID;
    CurrentSelectedVehicleID = VehicleID;

    // Vehicle selection changed

    // Selection is now handled by ListView - no manual widget updates needed
    // Vehicle selection handled by ListView
}

void URealGazeboMainWidget::ClearVehicleSelection()
{
    if (CurrentSelectedVehicleID.VehicleNum == 0 && CurrentSelectedVehicleID.VehicleType == 0)
    {
        // Already cleared
        return;
    }

    FVehicleID PreviousSelectedVehicleID = CurrentSelectedVehicleID;
    CurrentSelectedVehicleID = FVehicleID(0, 0); // Invalid ID

    // Cleared vehicle selection

    // Clear ListView selection - this will automatically update visual feedback
    if (VehicleListView)
    {
        VehicleListView->ClearSelection();
        // Cleared ListView selection
    }
}

void URealGazeboMainWidget::SetVehicleTypeImageDataTable(UDataTable* DataTable)
{
    VehicleTypeImageDataTable = DataTable;

    // Configure all currently displayed vehicle entry widgets
    if (VehicleListView)
    {
        // Get all displayed widgets and configure them
        TArray<UUserWidget*> DisplayedEntryWidgets = VehicleListView->GetDisplayedEntryWidgets();
        for (UUserWidget* Widget : DisplayedEntryWidgets)
        {
            if (URealGazeboVehicleEntry* VehicleEntry = Cast<URealGazeboVehicleEntry>(Widget))
            {
                VehicleEntry->SetVehicleTypeImageDataTable(DataTable);
            }
        }
    }
}

void URealGazeboMainWidget::SetViewerDirector(ARealGazeboViewerDirector* InViewerDirector)
{
    ViewerDirector = InViewerDirector;

    UE_LOG(LogRealGazeboUI, Log, TEXT("MainWidget: ViewerDirector reference set for camera integration"));
}

AVehicleBasePawn* URealGazeboMainWidget::FindVehiclePawnByID(const FVehicleID& VehicleID) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // Find vehicle pawn by searching all VehicleBasePawn actors
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AVehicleBasePawn::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (AVehicleBasePawn* VehiclePawn = Cast<AVehicleBasePawn>(Actor))
        {
            if (VehiclePawn->VehicleID == VehicleID)
            {
                return VehiclePawn;
            }
        }
    }

    return nullptr;
}

//----------------------------------------------------------
// Reset Button & Fade Functionality
//----------------------------------------------------------

void URealGazeboMainWidget::OnResetButtonClicked()
{
    UE_LOG(LogRealGazeboUI, Log, TEXT("Reset button clicked - starting fade and reload"));

    // Reset fade timer
    FadeElapsedTime = 0.0f;

    // Start fade animation timer (60 FPS for smooth animation)
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            FadeTimerHandle,
            this,
            &URealGazeboMainWidget::UpdateFadeAlpha,
            1.0f / 60.0f,  // Update at 60 FPS
            true           // Loop
        );
    }
}

void URealGazeboMainWidget::UpdateFadeAlpha()
{
    FadeElapsedTime += (1.0f / 60.0f);
    float Alpha = FMath::Clamp(FadeElapsedTime / FadeDuration, 0.0f, 1.0f);

    // Update border alpha
    if (Fade)
    {
        FLinearColor BrushColor = Fade->GetBrushColor();
        BrushColor.A = Alpha;
        Fade->SetBrushColor(BrushColor);
    }

    // When fade completes (fully opaque), reload level
    if (Alpha >= 1.0f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(FadeTimerHandle);
        }

        ReloadCurrentLevel();
    }
}

void URealGazeboMainWidget::ReloadCurrentLevel()
{
    UE_LOG(LogRealGazeboUI, Log, TEXT("Reloading current level"));

    // Get current level name and reload it
    FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);
    UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName));

    // Note: After reload, NativeConstruct will be called again
    // and Fade border will be reset to alpha 0 (transparent)
}

//----------------------------------------------------------
// Sorting Functionality
//----------------------------------------------------------

void URealGazeboMainWidget::OnSortButtonClicked()
{
    ToggleSortMode();
}

void URealGazeboMainWidget::ToggleSortMode()
{
    // Toggle between sort modes
    if (CurrentSortMode == EVehicleSortMode::ByVehicleNum)
    {
        CurrentSortMode = EVehicleSortMode::ByVehicleType;
        UE_LOG(LogRealGazeboUI, Log, TEXT("Sort mode changed to: By Vehicle Type"));
    }
    else
    {
        CurrentSortMode = EVehicleSortMode::ByVehicleNum;
        UE_LOG(LogRealGazeboUI, Log, TEXT("Sort mode changed to: By Vehicle Number"));
    }

    // Update button text
    UpdateSortButtonText();

    // Re-sort the vehicle list immediately
    SortVehicleList();
}

void URealGazeboMainWidget::UpdateSortButtonText()
{
    if (!Sort_ButtonText)
    {
        return;
    }

    FText ButtonText;
    if (CurrentSortMode == EVehicleSortMode::ByVehicleNum)
    {
        ButtonText = FText::FromString(TEXT("SortBy: ID"));
    }
    else // EVehicleSortMode::ByVehicleType
    {
        ButtonText = FText::FromString(TEXT("SortBy: Vehicle"));
    }

    Sort_ButtonText->SetText(ButtonText);
}

void URealGazeboMainWidget::SortVehicleList()
{
    if (!VehicleListView)
    {
        return;
    }

    // Clear and re-add all items in sorted order
    TArray<URealGazeboVehicleListItem*> AllItems = GetAllVehicleItems();

    // Sort items based on current mode
    if (CurrentSortMode == EVehicleSortMode::ByVehicleNum)
    {
        AllItems.Sort([](const URealGazeboVehicleListItem& A, const URealGazeboVehicleListItem& B)
        {
            return A.VehicleID.VehicleNum < B.VehicleID.VehicleNum;
        });
    }
    else // EVehicleSortMode::ByVehicleType
    {
        AllItems.Sort([](const URealGazeboVehicleListItem& A, const URealGazeboVehicleListItem& B)
        {
            if (A.VehicleID.VehicleType != B.VehicleID.VehicleType)
            {
                return A.VehicleID.VehicleType < B.VehicleID.VehicleType;
            }
            return A.VehicleID.VehicleNum < B.VehicleID.VehicleNum;
        });
    }

    // Clear ListView and re-add in sorted order
    VehicleListView->ClearListItems();
    for (URealGazeboVehicleListItem* Item : AllItems)
    {
        VehicleListView->AddItem(Item);
    }

    UE_LOG(LogRealGazeboUI, Log, TEXT("Vehicle list re-sorted with %d items"), AllItems.Num());
}

