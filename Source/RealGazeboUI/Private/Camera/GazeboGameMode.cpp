// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/GazeboGameMode.h"
#include "Camera/GazeboCameraManager.h"
#include "UI/GazeboVehicleDetectionManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AGazeboGameMode::AGazeboGameMode()
{
    // Set default pawn class to our camera manager
    DefaultPawnClass = AGazeboCameraManager::StaticClass();
}

void AGazeboGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Small delay to ensure everything is loaded
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AGazeboGameMode::SpawnManagers, 0.1f, false);
}

void AGazeboGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CameraManager = nullptr;
    VehicleDetectionManager = nullptr;
    Super::EndPlay(EndPlayReason);
}

void AGazeboGameMode::SetupCameraManager()
{
    if (!CameraManager && GetWorld())
    {
        CameraManager = GetWorld()->SpawnActor<AGazeboCameraManager>();
        if (CameraManager)
        {
            UE_LOG(LogTemp, Log, TEXT("GazeboGameMode: Camera manager created"));

            // Possess the camera manager with the first player controller
            if (bAutoPossessCameraManager)
            {
                if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
                {
                    PC->Possess(CameraManager);
                    UE_LOG(LogTemp, Log, TEXT("GazeboGameMode: Camera manager possessed"));
                }
            }
        }
    }
}

void AGazeboGameMode::SetupVehicleDetectionManager()
{
    if (!VehicleDetectionManager && GetWorld())
    {
        VehicleDetectionManager = GetWorld()->SpawnActor<AGazeboVehicleDetectionManager>();
        if (VehicleDetectionManager)
        {
            UE_LOG(LogTemp, Log, TEXT("GazeboGameMode: Vehicle detection manager created"));
        }
    }
}

void AGazeboGameMode::ConnectManagersSystems()
{
    ConnectSystems();
}

void AGazeboGameMode::SpawnManagers()
{
    if (bAutoSpawnCameraManager)
    {
        SetupCameraManager();
    }

    if (bAutoSpawnVehicleDetectionManager)
    {
        SetupVehicleDetectionManager();
    }

    // Connect systems after both are created
    ConnectSystems();
}

void AGazeboGameMode::ConnectSystems()
{
    if (CameraManager && VehicleDetectionManager)
    {
        VehicleDetectionManager->SetCameraManager(CameraManager);
        UE_LOG(LogTemp, Log, TEXT("GazeboGameMode: Systems connected"));
    }
}