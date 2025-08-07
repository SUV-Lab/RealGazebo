#include "UserCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GazeboVehicleManager.h"
#include "GameFramework/PlayerController.h"

UUserCameraManager::UUserCameraManager()
{
    PrimaryComponentTick.bCanEverTick = true; // Enable ticking for smooth transitions
    
    // Set default class
    MainFreeCameraClass = AMainFreeCameraActor::StaticClass();
    
    // Initialize state - start with no vehicle selected
    CurrentCameraMode = EUserCameraMode::MainFree;
    SelectedVehicleIndex = -1; // No vehicle selected initially
    MainFreeCamera = nullptr;

    // Initialize smooth transition variables
    bIsTransitioningMainCamera = false;
    bHasValidLastVehicleCamera = false;
    LastVehicleCameraMode = EUserCameraMode::MainFree;
    CameraTransitionSpeed = 5.0f;
    bUseSmoothMainCameraTransition = true;
}

void UUserCameraManager::BeginPlay()
{
    Super::BeginPlay();
    
    // Spawn main free camera
    SpawnMainFreeCamera();
    
    // Start with main free camera
    SwitchToMainFreeCamera();
    
    // Don't initialize vehicle selection here - vehicles haven't spawned yet
    UE_LOG(LogTemp, Warning, TEXT("UserCameraManager: Initialized - Waiting for vehicles to spawn"));
}

void UUserCameraManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Handle smooth main camera transitions
    if (bIsTransitioningMainCamera)
    {
        UpdateMainCameraTransition(DeltaTime);
    }
    
    // Update MainFreeCamera position when it's not active to track current camera
    if (MainFreeCamera && CurrentCameraMode != EUserCameraMode::MainFree)
    {
        UpdateMainFreeCameraToCurrentView();
    }
}

void UUserCameraManager::SpawnMainFreeCamera()
{
    if (!MainFreeCameraClass)
    {
        UE_LOG(LogTemp, Error, TEXT("UserCameraManager: MainFreeCameraClass is null"));
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    MainFreeCamera = GetWorld()->SpawnActor<AMainFreeCameraActor>(
        MainFreeCameraClass, MainCameraSpawnLocation, MainCameraSpawnRotation, SpawnParams);

    if (MainFreeCamera)
    {
        MainFreeCamera->SetActorLabel(TEXT("MainFreeCamera"));
        UE_LOG(LogTemp, Warning, TEXT("UserCameraManager: Spawned MainFreeCamera at (%s)"), 
               *MainCameraSpawnLocation.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UserCameraManager: Failed to spawn MainFreeCamera"));
    }
}

void UUserCameraManager::SwitchToMainFreeCamera()
{
    // Store current vehicle camera position before switching (this is now redundant since we track continuously)
    if (CurrentCameraMode != EUserCameraMode::MainFree)
    {
        UpdateLastVehicleCameraState();
    }

    DeactivateAllCameras();
    
    if (MainFreeCamera)
    {
        // MainFreeCamera is already at the correct position due to continuous tracking
        // No need to move it - it's already synchronized
        
        // Set view target with minimal blend time
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            FViewTargetTransitionParams TransitionParams;
            TransitionParams.BlendTime = 0.05f; // Very short blend for seamless transition
            TransitionParams.BlendFunction = VTBlend_Linear; // Linear for precise positioning
            TransitionParams.bLockOutgoing = true;
            
            PC->SetViewTarget(MainFreeCamera, TransitionParams);
            PC->Possess(MainFreeCamera);
        }
        
        MainFreeCamera->ActivateMainCamera();
        CurrentCameraMode = EUserCameraMode::MainFree;
        UE_LOG(LogTemp, Log, TEXT("UserCameraManager: Switched to Main Free Camera (synchronized position)"));
    }
}

void UUserCameraManager::SwitchToVehicleFirstPerson()
{
    TArray<AGazeboVehicleActor*> Vehicles = GetAvailableVehicles();
    
    // Check if a valid vehicle is selected
    if (SelectedVehicleIndex == -1 || SelectedVehicleIndex >= Vehicles.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("UserCameraManager: No vehicle selected for first person camera"));
        SwitchToMainFreeCamera();
        return;
    }
    
    AGazeboVehicleActor* SelectedVehicle = Vehicles[SelectedVehicleIndex];
    if (!SelectedVehicle)
    {
        UE_LOG(LogTemp, Error, TEXT("UserCameraManager: Selected vehicle is null at index %d"), SelectedVehicleIndex);
        SwitchToMainFreeCamera();
        return;
    }

    DeactivateAllCameras();
    SelectedVehicle->SetViewerFirstPersonCameraActive(true);
    
    // Improved camera switching with better transition
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        // First, immediately update the PlayerCameraManager's cached position
        if (SelectedVehicle->ViewerFirstPersonCamera && PC->PlayerCameraManager)
        {
            FVector CameraLoc = SelectedVehicle->ViewerFirstPersonCamera->GetComponentLocation();
            FRotator CameraRot = SelectedVehicle->ViewerFirstPersonCamera->GetComponentRotation();
            
            // Force immediate position update to prevent interpolation from wrong position
            PC->PlayerCameraManager->ViewTarget.POV.Location = CameraLoc;
            PC->PlayerCameraManager->ViewTarget.POV.Rotation = CameraRot;
            
            // Update the cached view using a different approach - create a new view info
            FMinimalViewInfo NewCacheView;
            NewCacheView.Location = CameraLoc;
            NewCacheView.Rotation = CameraRot;
            NewCacheView.FOV = 90.0f; // Default FOV
            // Note: We can't directly modify the cache, but setting ViewTarget.POV should be sufficient
        }
        
        // Set view target with proper transition
        FViewTargetTransitionParams TransitionParams;
        TransitionParams.BlendTime = 0.15f; // Slightly longer for smoother feel
        TransitionParams.BlendFunction = VTBlend_EaseOut;
        TransitionParams.bLockOutgoing = false; // Allow blending
        
        PC->SetViewTarget(SelectedVehicle, TransitionParams);
    }
    
    CurrentCameraMode = EUserCameraMode::VehicleFirstPerson;
    
    UE_LOG(LogTemp, Log, TEXT("UserCameraManager: Switched to %s Viewer First Person Camera (Index: %d)"), 
           *GetVehicleDisplayName(SelectedVehicle), SelectedVehicleIndex);
}

void UUserCameraManager::SwitchToVehicleThirdPerson()
{
    TArray<AGazeboVehicleActor*> Vehicles = GetAvailableVehicles();
    
    // Check if a valid vehicle is selected
    if (SelectedVehicleIndex == -1 || SelectedVehicleIndex >= Vehicles.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("UserCameraManager: No vehicle selected for third person camera"));
        SwitchToMainFreeCamera();
        return;
    }
    
    AGazeboVehicleActor* SelectedVehicle = Vehicles[SelectedVehicleIndex];
    if (!SelectedVehicle)
    {
        UE_LOG(LogTemp, Error, TEXT("UserCameraManager: Selected vehicle is null at index %d"), SelectedVehicleIndex);
        SwitchToMainFreeCamera();
        return;
    }

    DeactivateAllCameras();
    SelectedVehicle->SetViewerThirdPersonCameraActive(true);
    
    // Improved camera switching with better transition
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        // First, immediately update the PlayerCameraManager's cached position
        if (SelectedVehicle->ViewerThirdPersonCamera && PC->PlayerCameraManager)
        {
            FVector CameraLoc = SelectedVehicle->ViewerThirdPersonCamera->GetComponentLocation();
            FRotator CameraRot = SelectedVehicle->ViewerThirdPersonCamera->GetComponentRotation();
            
            // Force immediate position update to prevent interpolation from wrong position
            PC->PlayerCameraManager->ViewTarget.POV.Location = CameraLoc;
            PC->PlayerCameraManager->ViewTarget.POV.Rotation = CameraRot;
            
            // Update the cached view using a different approach - create a new view info
            FMinimalViewInfo NewCacheView;
            NewCacheView.Location = CameraLoc;
            NewCacheView.Rotation = CameraRot;
            NewCacheView.FOV = 90.0f; // Default FOV
            // Note: We can't directly modify the cache, but setting ViewTarget.POV should be sufficient
        }
        
        // Set view target with proper transition
        FViewTargetTransitionParams TransitionParams;
        TransitionParams.BlendTime = 0.15f; // Slightly longer for smoother feel
        TransitionParams.BlendFunction = VTBlend_EaseOut;
        TransitionParams.bLockOutgoing = false; // Allow blending
        
        PC->SetViewTarget(SelectedVehicle, TransitionParams);
    }
    
    CurrentCameraMode = EUserCameraMode::VehicleThirdPerson;
    
    UE_LOG(LogTemp, Log, TEXT("UserCameraManager: Switched to %s Viewer Third Person Camera (Index: %d)"), 
           *GetVehicleDisplayName(SelectedVehicle), SelectedVehicleIndex);
}

void UUserCameraManager::CycleToNextVehicle()
{
    TArray<AGazeboVehicleActor*> Vehicles = GetAvailableVehicles();
    if (Vehicles.Num() > 0)
    {
        SelectedVehicleIndex = (SelectedVehicleIndex + 1) % Vehicles.Num();
        
        UE_LOG(LogTemp, Log, TEXT("UserCameraManager: Selected %s (%d/%d)"), 
               *GetVehicleDisplayName(GetSelectedVehicle()), SelectedVehicleIndex + 1, Vehicles.Num());
        
        // Re-apply current camera mode to new vehicle
        if (CurrentCameraMode == EUserCameraMode::VehicleFirstPerson)
        {
            SwitchToVehicleFirstPerson();
        }
        else if (CurrentCameraMode == EUserCameraMode::VehicleThirdPerson)
        {
            SwitchToVehicleThirdPerson();
        }
    }
}

void UUserCameraManager::CycleToPreviousVehicle()
{
    TArray<AGazeboVehicleActor*> Vehicles = GetAvailableVehicles();
    if (Vehicles.Num() > 0)
    {
        SelectedVehicleIndex = (SelectedVehicleIndex - 1 + Vehicles.Num()) % Vehicles.Num();
        
        UE_LOG(LogTemp, Log, TEXT("UserCameraManager: Selected %s (%d/%d)"), 
               *GetVehicleDisplayName(GetSelectedVehicle()), SelectedVehicleIndex + 1, Vehicles.Num());
        
        // Re-apply current camera mode to new vehicle
        if (CurrentCameraMode == EUserCameraMode::VehicleFirstPerson)
        {
            SwitchToVehicleFirstPerson();
        }
        else if (CurrentCameraMode == EUserCameraMode::VehicleThirdPerson)
        {
            SwitchToVehicleThirdPerson();
        }
    }
}

void UUserCameraManager::CycleCameraMode()
{
    switch (CurrentCameraMode)
    {
        case EUserCameraMode::MainFree:
            SwitchToVehicleFirstPerson();
            break;
        case EUserCameraMode::VehicleFirstPerson:
            SwitchToVehicleThirdPerson();
            break;
        case EUserCameraMode::VehicleThirdPerson:
            SwitchToMainFreeCamera();
            break;
    }
}

void UUserCameraManager::DeactivateAllCameras()
{
    // Deactivate main free camera
    if (MainFreeCamera)
    {
        MainFreeCamera->DeactivateMainCamera();
    }
    
    // Deactivate all vehicle viewer cameras
    TArray<AGazeboVehicleActor*> AllVehicles = GetAvailableVehicles();
    for (AGazeboVehicleActor* Vehicle : AllVehicles)
    {
        if (Vehicle && IsValid(Vehicle))
        {
            Vehicle->DeactivateAllViewerCameras();
        }
    }
}

AGazeboVehicleActor* UUserCameraManager::GetSelectedVehicle() const
{
    TArray<AGazeboVehicleActor*> Vehicles = GetAvailableVehicles();
    if (SelectedVehicleIndex != -1 && Vehicles.IsValidIndex(SelectedVehicleIndex))
    {
        AGazeboVehicleActor* Vehicle = Vehicles[SelectedVehicleIndex];
        if (Vehicle && IsValid(Vehicle))
        {
            return Vehicle;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UserCameraManager: Vehicle at index %d is invalid"), SelectedVehicleIndex);
        }
    }
    return nullptr;
}

TArray<AGazeboVehicleActor*> UUserCameraManager::GetAvailableVehicles() const
{
    // Get vehicles from the vehicle manager (owner)
    if (AGazeboVehicleManager* VehicleManager = Cast<AGazeboVehicleManager>(GetOwner()))
    {
        return VehicleManager->GetAllVehicles();
    }
    return TArray<AGazeboVehicleActor*>();
}

FString UUserCameraManager::GetVehicleDisplayName(AGazeboVehicleActor* Vehicle) const
{
    if (!Vehicle)
    {
        return TEXT("Unknown");
    }
    
    // Use the actor label which contains the proper vehicle name
    return Vehicle->GetActorLabel();
}

void UUserCameraManager::InitializeVehicleSelection()
{
    // Remove automatic vehicle selection - keep at -1
    TArray<AGazeboVehicleActor*> Vehicles = GetAvailableVehicles();
    UE_LOG(LogTemp, Warning, TEXT("UserCameraManager: %d vehicles available - No automatic selection"), Vehicles.Num());
}

void UUserCameraManager::OnVehicleSpawned(AGazeboVehicleActor* NewVehicle)
{
    if (!NewVehicle)
        return;
        
    TArray<AGazeboVehicleActor*> Vehicles = GetAvailableVehicles();
    
    // Don't automatically select the first vehicle
    UE_LOG(LogTemp, Log, TEXT("UserCameraManager: Vehicle %s spawned - Total vehicles: %d (No automatic selection)"), 
           *GetVehicleDisplayName(NewVehicle), Vehicles.Num());
    
    // Ensure selected index is still valid after new vehicle spawn
    if (SelectedVehicleIndex >= Vehicles.Num())
    {
        SelectedVehicleIndex = -1; // Reset to no selection instead of 0
        UE_LOG(LogTemp, Log, TEXT("UserCameraManager: Reset vehicle selection to none"));
    }
}

void UUserCameraManager::SetSelectedVehicleIndex(int32 NewIndex)
{
    TArray<AGazeboVehicleActor*> Vehicles = GetAvailableVehicles();
    if (Vehicles.IsValidIndex(NewIndex))
    {
        SelectedVehicleIndex = NewIndex;
        AGazeboVehicleActor* SelectedVehicle = Vehicles[NewIndex];
        UE_LOG(LogTemp, Log, TEXT("UserCameraManager: Vehicle index set to %d (%s)"), 
               NewIndex, *GetVehicleDisplayName(SelectedVehicle));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UserCameraManager: Invalid vehicle index %d (total: %d)"), 
               NewIndex, Vehicles.Num());
    }
}

void UUserCameraManager::MoveMainFreeCameraToVehiclePosition()
{
    if (!MainFreeCamera || SelectedVehicleIndex == -1)
    {
        UE_LOG(LogTemp, Warning, TEXT("UserCameraManager: Cannot move MainFreeCamera - no camera or no vehicle selected"));
        return;
    }
    
    FVector TargetLocation = GetVehicleCameraLocation();
    FRotator TargetRotation = GetVehicleCameraRotation();
    
    if (TargetLocation.IsZero())
    {
        UE_LOG(LogTemp, Warning, TEXT("UserCameraManager: Invalid vehicle camera position"));
        return;
    }
    
    if (bUseSmoothMainCameraTransition)
    {
        StartMainCameraTransition(TargetLocation, TargetRotation);
    }
    else
    {
        // Instant positioning
        MainFreeCamera->SetActorLocation(TargetLocation);
        MainFreeCamera->SetActorRotation(TargetRotation);
    }
    
    UE_LOG(LogTemp, Log, TEXT("UserCameraManager: Moving MainFreeCamera to vehicle position"));
}

void UUserCameraManager::UpdateLastVehicleCameraState()
{
    LastVehicleCameraLocation = GetVehicleCameraLocation();
    LastVehicleCameraRotation = GetVehicleCameraRotation();
    LastVehicleCameraMode = CurrentCameraMode;
    bHasValidLastVehicleCamera = !LastVehicleCameraLocation.IsZero();
    
    UE_LOG(LogTemp, Log, TEXT("UserCameraManager: Stored last vehicle camera state: %s at %s"), 
           *UEnum::GetValueAsString(LastVehicleCameraMode), *LastVehicleCameraLocation.ToString());
}

void UUserCameraManager::StartMainCameraTransition(const FVector& TargetLocation, const FRotator& TargetRotation)
{
    if (!MainFreeCamera)
        return;
        
    MainCameraTargetLocation = TargetLocation;
    MainCameraTargetRotation = TargetRotation;
    bIsTransitioningMainCamera = true;
    
    UE_LOG(LogTemp, Log, TEXT("UserCameraManager: Starting smooth MainFreeCamera transition to %s"), 
           *TargetLocation.ToString());
}

void UUserCameraManager::UpdateMainCameraTransition(float DeltaTime)
{
    if (!MainFreeCamera || !bIsTransitioningMainCamera)
        return;
    
    FVector CurrentLocation = MainFreeCamera->GetActorLocation();
    FRotator CurrentRotation = MainFreeCamera->GetActorRotation();
    
    // Smooth interpolation
    FVector NewLocation = FMath::VInterpTo(CurrentLocation, MainCameraTargetLocation, DeltaTime, CameraTransitionSpeed);
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, MainCameraTargetRotation, DeltaTime, CameraTransitionSpeed);
    
    MainFreeCamera->SetActorLocation(NewLocation);
    MainFreeCamera->SetActorRotation(NewRotation);
    
    // Check if transition is complete
    float DistanceToTarget = FVector::Dist(NewLocation, MainCameraTargetLocation);
    float RotationDiff = FMath::Abs(FRotator::ClampAxis(NewRotation.Yaw - MainCameraTargetRotation.Yaw));
    
    if (DistanceToTarget < 10.0f && RotationDiff < 1.0f) // 10cm and 1 degree tolerance
    {
        // Snap to final position
        MainFreeCamera->SetActorLocation(MainCameraTargetLocation);
        MainFreeCamera->SetActorRotation(MainCameraTargetRotation);
        bIsTransitioningMainCamera = false;
        
        UE_LOG(LogTemp, Log, TEXT("UserCameraManager: MainFreeCamera transition completed"));
    }
}

FVector UUserCameraManager::GetVehicleCameraLocation() const
{
    AGazeboVehicleActor* SelectedVehicle = GetSelectedVehicle();
    if (!SelectedVehicle)
        return FVector::ZeroVector;
    
    switch (CurrentCameraMode)
    {
        case EUserCameraMode::VehicleFirstPerson:
            if (SelectedVehicle->ViewerFirstPersonCamera)
                return SelectedVehicle->ViewerFirstPersonCamera->GetComponentLocation();
            break;
            
        case EUserCameraMode::VehicleThirdPerson:
            if (SelectedVehicle->ViewerThirdPersonCamera)
                return SelectedVehicle->ViewerThirdPersonCamera->GetComponentLocation();
            break;
            
        default:
            break;
    }
    
    return FVector::ZeroVector;
}

FRotator UUserCameraManager::GetVehicleCameraRotation() const
{
    AGazeboVehicleActor* SelectedVehicle = GetSelectedVehicle();
    if (!SelectedVehicle)
        return FRotator::ZeroRotator;
    
    switch (CurrentCameraMode)
    {
        case EUserCameraMode::VehicleFirstPerson:
            if (SelectedVehicle->ViewerFirstPersonCamera)
                return SelectedVehicle->ViewerFirstPersonCamera->GetComponentRotation();
            break;
            
        case EUserCameraMode::VehicleThirdPerson:
            if (SelectedVehicle->ViewerThirdPersonCamera)
                return SelectedVehicle->ViewerThirdPersonCamera->GetComponentRotation();
            break;
            
        default:
            break;
    }
    
    return FRotator::ZeroRotator;
}

void UUserCameraManager::UpdateMainFreeCameraToCurrentView()
{
    if (!MainFreeCamera)
        return;
    
    FVector CurrentCameraLocation;
    FRotator CurrentCameraRotation;
    bool bValidLocation = false;
    
    // Get current active camera position and rotation
    switch (CurrentCameraMode)
    {
        case EUserCameraMode::VehicleFirstPerson:
        {
            AGazeboVehicleActor* SelectedVehicle = GetSelectedVehicle();
            if (SelectedVehicle && SelectedVehicle->ViewerFirstPersonCamera)
            {
                CurrentCameraLocation = SelectedVehicle->ViewerFirstPersonCamera->GetComponentLocation();
                CurrentCameraRotation = SelectedVehicle->ViewerFirstPersonCamera->GetComponentRotation();
                bValidLocation = true;
            }
            break;
        }
        
        case EUserCameraMode::VehicleThirdPerson:
        {
            AGazeboVehicleActor* SelectedVehicle = GetSelectedVehicle();
            if (SelectedVehicle && SelectedVehicle->ViewerThirdPersonCamera)
            {
                CurrentCameraLocation = SelectedVehicle->ViewerThirdPersonCamera->GetComponentLocation();
                CurrentCameraRotation = SelectedVehicle->ViewerThirdPersonCamera->GetComponentRotation();
                bValidLocation = true;
            }
            break;
        }
        
        default:
            // For MainFree mode, don't update (this function shouldn't be called in that case anyway)
            break;
    }
    
    // Update MainFreeCamera position if we have a valid location
    if (bValidLocation)
    {
        MainFreeCamera->SetActorLocation(CurrentCameraLocation);
        MainFreeCamera->SetActorRotation(CurrentCameraRotation);
        
        // Optional: Log for debugging (uncomment if needed)
        // UE_LOG(LogTemp, VeryVerbose, TEXT("UserCameraManager: MainFreeCamera synced to (%s, %s)"), 
        //        *CurrentCameraLocation.ToString(), *CurrentCameraRotation.ToString());
    }
}