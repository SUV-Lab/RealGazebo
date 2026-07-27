// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#include "ViewerController/RealGazeboViewerDirector.h"
#include "ViewerController/RealGazeboBlueprintLib.h"
#include "ViewerController/RealGazeboCameraControllerComponent.h"
#include "Vehicles/VehicleBasePawn.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "RealGazeboUI.h"

ARealGazeboViewerDirector::ARealGazeboViewerDirector()
{
    PrimaryActorTick.bCanEverTick = false; // No longer need ticking

    // Initialize state - start with no specific mode until user input
    CurrentMode = ERealGazeboViewerMode::Manual; // Default to manual but don't use original pawn yet
    CurrentVehicle = nullptr;
    OriginalPawn = nullptr;

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

void ARealGazeboViewerDirector::BeginPlay()
{
    Super::BeginPlay();

    // Store the original UE5 DefaultPawn immediately
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && PC->GetPawn())
    {
        OriginalPawn = PC->GetPawn();
        UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Stored original UE5 DefaultPawn: %s"), *OriginalPawn->GetName());

        // Disable collision immediately so manual camera can fly through walls freely
        OriginalPawn->SetActorEnableCollision(false);
        if (UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(OriginalPawn->GetRootComponent()))
        {
            RootComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            RootComp->SetCollisionResponseToAllChannels(ECR_Ignore);
            UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Disabled DefaultPawn collision for free flight"));
        }
    }

    InitializeForBeginPlay();
}

void ARealGazeboViewerDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Restore original pawn visibility and possession if needed
    if (OriginalPawn)
    {
        OriginalPawn->SetActorHiddenInGame(false);
        SwitchPlayerPawn(OriginalPawn);
        UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Restored original pawn on EndPlay"));
        OriginalPawn = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void ARealGazeboViewerDirector::InitializeForBeginPlay()
{
    // Discover vehicles
    DiscoverVehicles();

    // Setup input bindings
    SetupInputBindings();

    // Don't start in manual mode - let user start with UE5's default pawn
    // ManualCameraPawn will be created lazily when first needed

    UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Initialized with %d vehicles"), AvailableVehicles.Num());
}

void ARealGazeboViewerDirector::SetupInputBindings()
{
    URealGazeboBlueprintLib::EnableInput(this);

    // Bind camera mode switching keys
    URealGazeboBlueprintLib::BindActionToKey(TEXT("RealGazebo_ManualCamera"), EKeys::M, this, &ARealGazeboViewerDirector::InputManualCamera);
    URealGazeboBlueprintLib::BindActionToKey(TEXT("RealGazebo_FirstPersonCamera"), EKeys::F, this, &ARealGazeboViewerDirector::InputFirstPersonCamera);
    URealGazeboBlueprintLib::BindActionToKey(TEXT("RealGazebo_ThirdPersonCamera"), EKeys::B, this, &ARealGazeboViewerDirector::InputThirdPersonCamera);

    // Bind vehicle switching keys
    URealGazeboBlueprintLib::BindActionToKey(TEXT("RealGazebo_PreviousVehicle"), EKeys::Comma, this, &ARealGazeboViewerDirector::InputPreviousVehicle);
    URealGazeboBlueprintLib::BindActionToKey(TEXT("RealGazebo_NextVehicle"), EKeys::Period, this, &ARealGazeboViewerDirector::InputNextVehicle);

    // Bind camera preset keys (1-7)
    URealGazeboBlueprintLib::BindActionToKey(TEXT("RealGazebo_CameraPreset1"), EKeys::One, this, &ARealGazeboViewerDirector::InputPreset1);
    URealGazeboBlueprintLib::BindActionToKey(TEXT("RealGazebo_CameraPreset2"), EKeys::Two, this, &ARealGazeboViewerDirector::InputPreset2);
    URealGazeboBlueprintLib::BindActionToKey(TEXT("RealGazebo_CameraPreset3"), EKeys::Three, this, &ARealGazeboViewerDirector::InputPreset3);
    URealGazeboBlueprintLib::BindActionToKey(TEXT("RealGazebo_CameraPreset4"), EKeys::Four, this, &ARealGazeboViewerDirector::InputPreset4);
    URealGazeboBlueprintLib::BindActionToKey(TEXT("RealGazebo_CameraPreset5"), EKeys::Five, this, &ARealGazeboViewerDirector::InputPreset5);
    URealGazeboBlueprintLib::BindActionToKey(TEXT("RealGazebo_CameraPreset6"), EKeys::Six, this, &ARealGazeboViewerDirector::InputPreset6);
    URealGazeboBlueprintLib::BindActionToKey(TEXT("RealGazebo_CameraPreset7"), EKeys::Seven, this, &ARealGazeboViewerDirector::InputPreset7);

    UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Input bindings setup complete"));
}

void ARealGazeboViewerDirector::DiscoverVehicles()
{
    AvailableVehicles.Empty();

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVehicleBasePawn::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (AVehicleBasePawn* Vehicle = Cast<AVehicleBasePawn>(Actor))
        {
            AvailableVehicles.Add(Vehicle);
        }
    }

    UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Discovered %d vehicles"), AvailableVehicles.Num());
}

void ARealGazeboViewerDirector::CycleCurrentVehicle(int32 Direction)
{
    if (Direction == 0)
    {
        return;
    }

    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    AVehicleBasePawn* PossessedVehicle = PC ? Cast<AVehicleBasePawn>(PC->GetPawn()) : nullptr;
    if (!IsValid(PossessedVehicle))
    {
        return;
    }

    int32 CurrentIndex = AvailableVehicles.IndexOfByKey(PossessedVehicle);
    if (CurrentIndex == INDEX_NONE)
    {
        DiscoverVehicles();
        CurrentIndex = AvailableVehicles.IndexOfByKey(PossessedVehicle);
    }

    const int32 VehicleCount = AvailableVehicles.Num();
    if (CurrentIndex == INDEX_NONE || VehicleCount < 2)
    {
        return;
    }

    const int32 StepDirection = Direction > 0 ? 1 : -1;
    for (int32 Step = 1; Step < VehicleCount; ++Step)
    {
        const int32 CandidateIndex = (CurrentIndex + StepDirection * Step + VehicleCount) % VehicleCount;
        AVehicleBasePawn* CandidateVehicle = AvailableVehicles[CandidateIndex];
        if (IsValid(CandidateVehicle) && CandidateVehicle != PossessedVehicle)
        {
            SetCurrentVehicle(CandidateVehicle);
            VehicleChangedByInputDelegate.Broadcast(CandidateVehicle);
            return;
        }
    }
}

void ARealGazeboViewerDirector::SetCameraMode(ERealGazeboViewerMode NewMode)
{
    if (CurrentMode == NewMode)
    {
        return;
    }

    UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Switching from %d to %d"), (int32)CurrentMode, (int32)NewMode);

    CurrentMode = NewMode;

    // Switch to new mode
    switch (CurrentMode)
    {
    case ERealGazeboViewerMode::Manual:
        SwitchToManualMode();
        break;
    case ERealGazeboViewerMode::FirstPerson:
        SwitchToFirstPersonMode();
        break;
    case ERealGazeboViewerMode::ThirdPerson:
        SwitchToThirdPersonMode();
        break;
    }
}

void ARealGazeboViewerDirector::SetCurrentVehicle(AVehicleBasePawn* Vehicle)
{
    const bool bVehicleChanged = CurrentVehicle != Vehicle;
    if (!bVehicleChanged)
    {
        APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
        const bool bNeedsPossessionRestore = IsValid(Vehicle) && PC && PC->GetPawn() != Vehicle;
        if (!bNeedsPossessionRestore)
        {
            return;
        }

        UE_LOG(LogRealGazeboUI, Log,
               TEXT("ViewerDirector: Restoring possession and camera for current vehicle %s"),
               *Vehicle->GetName());
    }

    // Disable cameras on previous vehicle if using component system
    if (bVehicleChanged && CurrentVehicle)
    {
        URealGazeboCameraControllerComponent* PreviousCameraController = FindCameraControllerForVehicle(CurrentVehicle);
        if (PreviousCameraController)
        {
            PreviousCameraController->DeactivateAllCameras();
        }
    }

    if (bVehicleChanged)
    {
        CurrentVehicle = Vehicle;
    }

    if (CurrentVehicle)
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Set current vehicle to %s"), *CurrentVehicle->GetName());

        // If we're in first or third person mode, update camera
        if (CurrentMode == ERealGazeboViewerMode::FirstPerson)
        {
            SwitchToFirstPersonMode();
        }
        else if (CurrentMode == ERealGazeboViewerMode::ThirdPerson)
        {
            SwitchToThirdPersonMode();
        }
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Cleared current vehicle"));

        // Fall back to manual mode if no vehicle
        if (CurrentMode != ERealGazeboViewerMode::Manual)
        {
            SetCameraMode(ERealGazeboViewerMode::Manual);
        }
    }
}

void ARealGazeboViewerDirector::SetCurrentVehicleByID(const FVehicleID& VehicleID)
{
    AVehicleBasePawn* FoundVehicle = nullptr;

    for (AVehicleBasePawn* Vehicle : AvailableVehicles)
    {
        if (Vehicle && Vehicle->VehicleID == VehicleID)
        {
            FoundVehicle = Vehicle;
            break;
        }
    }

    SetCurrentVehicle(FoundVehicle);
}

void ARealGazeboViewerDirector::SetInitialCameraSettings(const FVector& Location, const FRotator& Rotation)
{
    InitialCameraLocation = Location;
    InitialCameraRotation = Rotation;

    // Apply the initial position to the DefaultPawn immediately if available
    if (OriginalPawn)
    {
        FVector TargetLocation = GetActorLocation() + InitialCameraLocation;
        OriginalPawn->SetActorLocation(TargetLocation);
        OriginalPawn->SetActorRotation(InitialCameraRotation);

        // Also update the PlayerController's control rotation for camera view
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            PC->SetControlRotation(InitialCameraRotation);
            UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Updated PlayerController control rotation to match pawn rotation"));
        }

        UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Applied initial camera settings to DefaultPawn - Location: %s, Rotation: %s"),
               *TargetLocation.ToString(), *InitialCameraRotation.ToString());
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("ViewerDirector: Updated initial camera settings but DefaultPawn not available yet - Location: %s, Rotation: %s"),
               *Location.ToString(), *Rotation.ToString());
    }
}

void ARealGazeboViewerDirector::RefreshVehicleList()
{
    DiscoverVehicles();
}

void ARealGazeboViewerDirector::SwitchToManualMode()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !OriginalPawn)
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("ViewerDirector: Cannot switch to manual mode - no PlayerController or OriginalPawn"));
        return;
    }

    // Get current camera transform to position manual camera at current view
    FTransform CurrentCameraTransform = GetCurrentCameraTransform();

    // Position the original pawn at the current camera location
    OriginalPawn->SetActorLocation(CurrentCameraTransform.GetLocation());
    OriginalPawn->SetActorRotation(CurrentCameraTransform.GetRotation().Rotator());
    OriginalPawn->SetActorHiddenInGame(false); // Ensure it's visible

    // Disable collision so manual camera can fly through walls freely
    OriginalPawn->SetActorEnableCollision(false);
    if (UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(OriginalPawn->GetRootComponent()))
    {
        RootComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        RootComp->SetCollisionResponseToAllChannels(ECR_Ignore);
        UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Disabled collision for manual camera - free flight enabled"));
    }

    // Switch to the original pawn only if not already possessed
    if (PC->GetPawn() != OriginalPawn)
    {
        SwitchPlayerPawn(OriginalPawn);
        UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Switched possession to original DefaultPawn for manual camera at current view location"));
    }

    // Ensure PlayerController's control rotation matches the camera view
    PC->SetControlRotation(CurrentCameraTransform.GetRotation().Rotator());

    UE_LOG(LogRealGazeboUI, Display, TEXT("ViewerDirector: Switched to Manual Camera mode using original DefaultPawn at %s"),
           *CurrentCameraTransform.GetLocation().ToString());
}

void ARealGazeboViewerDirector::SwitchToFirstPersonMode()
{
    // Auto-select first available vehicle if none selected
    if (!CurrentVehicle)
    {
        if (AvailableVehicles.Num() > 0)
        {
            SetCurrentVehicle(AvailableVehicles[0]);
            UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Auto-selected first available vehicle %s for First Person camera"), *CurrentVehicle->GetName());
        }
        else
        {
            UE_LOG(LogRealGazeboUI, Warning, TEXT("ViewerDirector: Cannot switch to First Person - no vehicles available"));
            return;
        }
    }

    // Hide original pawn when switching to vehicle mode
    if (OriginalPawn)
    {
        OriginalPawn->SetActorHiddenInGame(true);
        UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Hidden original DefaultPawn when switching to vehicle mode"));
    }

    // Switch to vehicle pawn
    SwitchPlayerPawn(CurrentVehicle);

    // Use component-based camera system
    URealGazeboCameraControllerComponent* CameraController = FindCameraControllerForVehicle(CurrentVehicle);
    if (CameraController)
    {
        CameraController->ActivateFirstPersonCamera();
        UE_LOG(LogRealGazeboUI, Display, TEXT("ViewerDirector: Switched to First Person Camera for vehicle %s"), *CurrentVehicle->GetName());
        return;
    }

    // No camera system found - fall back to manual mode
    UE_LOG(LogRealGazeboUI, Warning, TEXT("ViewerDirector: No camera controller component found for vehicle %s"), *CurrentVehicle->GetName());
    SetCameraMode(ERealGazeboViewerMode::Manual);
}

void ARealGazeboViewerDirector::SwitchToThirdPersonMode()
{
    // Auto-select first available vehicle if none selected
    if (!CurrentVehicle)
    {
        if (AvailableVehicles.Num() > 0)
        {
            SetCurrentVehicle(AvailableVehicles[0]);
            UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Auto-selected first available vehicle %s for Third Person camera"), *CurrentVehicle->GetName());
        }
        else
        {
            UE_LOG(LogRealGazeboUI, Warning, TEXT("ViewerDirector: Cannot switch to Third Person - no vehicles available"));
            return;
        }
    }

    // Hide original pawn when switching to vehicle mode
    if (OriginalPawn)
    {
        OriginalPawn->SetActorHiddenInGame(true);
        UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Hidden original DefaultPawn when switching to vehicle mode"));
    }

    // Switch to vehicle pawn
    SwitchPlayerPawn(CurrentVehicle);

    // Use component-based camera system
    URealGazeboCameraControllerComponent* CameraController = FindCameraControllerForVehicle(CurrentVehicle);
    if (CameraController)
    {
        CameraController->ActivateThirdPersonCamera();
        UE_LOG(LogRealGazeboUI, Display, TEXT("ViewerDirector: Switched to Third Person Camera for vehicle %s"), *CurrentVehicle->GetName());
        return;
    }

    // No camera system found - fall back to manual mode
    UE_LOG(LogRealGazeboUI, Warning, TEXT("ViewerDirector: No camera controller component found for vehicle %s"), *CurrentVehicle->GetName());
    SetCameraMode(ERealGazeboViewerMode::Manual);
}


void ARealGazeboViewerDirector::SwitchPlayerPawn(APawn* NewPawn)
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && NewPawn)
    {
        PC->Possess(NewPawn);
        UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Switched player pawn to %s"), *NewPawn->GetName());
    }
}

URealGazeboCameraControllerComponent* ARealGazeboViewerDirector::FindCameraControllerForVehicle(AVehicleBasePawn* Vehicle) const
{
    if (!Vehicle)
    {
        return nullptr;
    }

    // Look for camera controller component
    return Vehicle->FindComponentByClass<URealGazeboCameraControllerComponent>();
}

FTransform ARealGazeboViewerDirector::GetCurrentCameraTransform() const
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        // Fallback to default transform
        return FTransform(InitialCameraRotation, GetActorLocation() + InitialCameraLocation, FVector::OneVector);
    }

    // Get current camera transform from player controller
    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

    return FTransform(CameraRotation, CameraLocation, FVector::OneVector);
}

//----------------------------------------------------------
// Camera Preset Control
//----------------------------------------------------------

void ARealGazeboViewerDirector::ApplyCameraPreset(int32 PresetIndex)
{
    // Validate preset index
    if (!CameraPresets.IsValidIndex(PresetIndex))
    {
        UE_LOG(LogRealGazeboUI, Warning, TEXT("ViewerDirector: Invalid preset index: %d (Available: %d)"),
               PresetIndex, CameraPresets.Num());
        return;
    }

    const FCameraPreset& Preset = CameraPresets[PresetIndex];

    // Must be in Manual mode to apply presets
    if (CurrentMode != ERealGazeboViewerMode::Manual)
    {
        UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Switching to Manual mode to apply preset '%s'"),
               *Preset.PresetName);
        SetCameraMode(ERealGazeboViewerMode::Manual);
    }

    // Apply preset to OriginalPawn (DefaultPawn used for manual camera)
    if (OriginalPawn)
    {
        OriginalPawn->SetActorLocation(Preset.Location);
        OriginalPawn->SetActorRotation(Preset.Rotation);

        // Update PlayerController's control rotation for smooth camera view
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            PC->SetControlRotation(Preset.Rotation);
        }

        UE_LOG(LogRealGazeboUI, Display, TEXT("ViewerDirector: Applied camera preset '%s' - Location: %s, Rotation: %s"),
               *Preset.PresetName, *Preset.Location.ToString(), *Preset.Rotation.ToString());
    }
    else
    {
        UE_LOG(LogRealGazeboUI, Error, TEXT("ViewerDirector: Cannot apply preset - OriginalPawn not available"));
    }
}

bool ARealGazeboViewerDirector::ApplyCameraPresetByName(const FString& PresetName)
{
    for (int32 i = 0; i < CameraPresets.Num(); ++i)
    {
        if (CameraPresets[i].PresetName.Equals(PresetName, ESearchCase::IgnoreCase))
        {
            ApplyCameraPreset(i);
            return true;
        }
    }

    UE_LOG(LogRealGazeboUI, Warning, TEXT("ViewerDirector: Camera preset '%s' not found"), *PresetName);
    return false;
}

void ARealGazeboViewerDirector::AddCameraPreset(const FCameraPreset& NewPreset)
{
    CameraPresets.Add(NewPreset);
    UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Added camera preset '%s'"), *NewPreset.PresetName);
}

void ARealGazeboViewerDirector::RemoveCameraPreset(int32 PresetIndex)
{
    if (CameraPresets.IsValidIndex(PresetIndex))
    {
        FString PresetName = CameraPresets[PresetIndex].PresetName;
        CameraPresets.RemoveAt(PresetIndex);
        UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Removed camera preset '%s'"), *PresetName);
    }
}

void ARealGazeboViewerDirector::SetCameraPresets(const TArray<FCameraPreset>& NewPresets)
{
    CameraPresets = NewPresets;
    UE_LOG(LogRealGazeboUI, Log, TEXT("ViewerDirector: Set %d camera presets"), CameraPresets.Num());
}

//----------------------------------------------------------
// Input Handlers
//----------------------------------------------------------

void ARealGazeboViewerDirector::InputManualCamera()
{
    SetCameraMode(ERealGazeboViewerMode::Manual);
}

void ARealGazeboViewerDirector::InputFirstPersonCamera()
{
    SetCameraMode(ERealGazeboViewerMode::FirstPerson);
}

void ARealGazeboViewerDirector::InputThirdPersonCamera()
{
    SetCameraMode(ERealGazeboViewerMode::ThirdPerson);
}

void ARealGazeboViewerDirector::InputPreviousVehicle()
{
    CycleCurrentVehicle(-1);
}

void ARealGazeboViewerDirector::InputNextVehicle()
{
    CycleCurrentVehicle(1);
}

void ARealGazeboViewerDirector::InputPreset1()
{
    ApplyCameraPresetByName("C-Track");
}

void ARealGazeboViewerDirector::InputPreset2()
{
    ApplyCameraPresetByName("VILS");
}

void ARealGazeboViewerDirector::InputPreset3()
{
    ApplyCameraPresetByName("Urban");
}

void ARealGazeboViewerDirector::InputPreset4()
{
    ApplyCameraPresetByName("BeltWay Camera");
}

void ARealGazeboViewerDirector::InputPreset5()
{
    ApplyCameraPresetByName("DirtRoad Camera");
}

void ARealGazeboViewerDirector::InputPreset6()
{
    ApplyCameraPresetByName("Forest Camera");
}

void ARealGazeboViewerDirector::InputPreset7()
{
    ApplyCameraPresetByName("Lake Camera");
}
