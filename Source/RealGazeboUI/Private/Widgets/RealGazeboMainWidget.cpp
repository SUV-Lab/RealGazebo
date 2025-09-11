#include "Widgets/RealGazeboMainWidget.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "TimerManager.h"
#include "RealGazeboUI.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

URealGazeboMainWidget::URealGazeboMainWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Set default values
    UpdateFrequency = 30.0f;
    MaxDisplayVehicles = 256;
    
    // Initialize tracking variables
    LastVehicleCount = 0;
    bLastConnectionStatus = false;
    
    // Enable ticking for real-time updates
    bIsFocusable = false;
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void URealGazeboMainWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogRealGazeboUI, Log, TEXT("RealGazeboMainWidget: Constructing UI"));
    
    // Initialize subsystem connection
    InitializeSubsystemConnection();
    
    // Setup ListView event bindings
    if (VehicleListView)
    {
        VehicleListView->OnItemSelectionChanged().AddUObject(this, &URealGazeboMainWidget::OnVehicleItemSelectionChanged);
        
        UE_LOG(LogRealGazeboUI, Verbose, TEXT("ListView events bound successfully"));
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("VehicleListView widget not found! Make sure it's bound in the Blueprint."));
    }
    
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
        
        UE_LOG(LogRealGazeboUI, Log, TEXT("Update timer started with interval: %.3f seconds"), UpdateInterval);
    }
    
    // Initial data refresh
    RefreshVehicleList();
}

void URealGazeboMainWidget::NativeDestruct()
{
    UE_LOG(LogRealGazeboUI, Log, TEXT("RealGazeboMainWidget: Destructing UI"));
    
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
    
    // Update performance stats
    // Performance monitoring removed
    if (false)
    {
    }
}

void URealGazeboMainWidget::InitializeSubsystemConnection()
{
    // Get bridge subsystem reference
    BridgeSubsystem = UGazeboBridgeSubsystem::GetBridgeSubsystem(this);
    
    if (BridgeSubsystem.IsValid())
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("Successfully connected to GazeboBridgeSubsystem"));
        
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
    
    // Track changes for performance
    bool bVehicleListChanged = false;
    
    // Add new vehicles or update existing ones
    for (const FVehicleID& VehicleID : CurrentVehicleIDs)
    {
        FVehicleRuntimeData RuntimeData = BridgeSubsystem->GetVehicleData(VehicleID);
        
        if (VehicleItemMap.Contains(VehicleID))
        {
            // Update existing vehicle
            UpdateVehicleInList(VehicleID, RuntimeData);
        }
        else
        {
            // Add new vehicle
            AddVehicleToList(VehicleID, RuntimeData);
            bVehicleListChanged = true;
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
        UE_LOG(LogRealGazeboUI, Verbose, TEXT("Vehicle list updated: %d vehicles"), CurrentVehicleIDs.Num());
    }
}

URealGazeboVehicleListItem* URealGazeboMainWidget::AddVehicleToList(const FVehicleID& VehicleID, const FVehicleRuntimeData& RuntimeData)
{
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
        
        // Auto-scroll removed
    }
    
    UE_LOG(LogRealGazeboUI, Log, TEXT("Added vehicle to list: %s"), *NewItem->VehicleName);
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
        
        UE_LOG(LogRealGazeboUI, Log, TEXT("Removed vehicle from list: %s"), *Item->VehicleName);
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

void URealGazeboMainWidget::OnVehicleItemSelectionChanged(UObject* SelectedItem)
{
    if (URealGazeboVehicleListItem* VehicleItem = Cast<URealGazeboVehicleListItem>(SelectedItem))
    {
        // Update selection state
        for (auto& Pair : VehicleItemMap)
        {
            // Selection handled in Entry widget
        }
        
        OnVehicleSelected(VehicleItem);
        UE_LOG(LogRealGazeboUI, Verbose, TEXT("Vehicle selected: %s"), *VehicleItem->VehicleName);
    }
}


void URealGazeboMainWidget::ClearAllVehicles()
{
    if (VehicleListView)
    {
        VehicleListView->ClearListItems();
    }
    
    VehicleItemMap.Empty();
    UE_LOG(LogRealGazeboUI, Log, TEXT("Cleared all vehicles from list"));
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


URealGazeboMainWidget* URealGazeboMainWidget::CreateMainWidget(UObject* WorldContext)
{
    if (!WorldContext)
    {
        UE_LOG(LogRealGazeboUI, Error, TEXT("CreateMainWidget: Invalid WorldContext"));
        return nullptr;
    }
    
    // For now, return nullptr - this will be implemented when we have Blueprint widgets
    UE_LOG(LogRealGazeboUI, Warning, TEXT("CreateMainWidget: Not implemented yet - requires Blueprint widget class"));
    return nullptr;
}

URealGazeboMainWidget* URealGazeboMainWidget::GetMainWidget(UObject* WorldContext)
{
    // For now, return nullptr - this will be implemented when we have widget management
    return nullptr;
}