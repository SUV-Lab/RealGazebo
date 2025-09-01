// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GazeboGameMode.generated.h"

class AGazeboCameraManager;
class AGazeboVehicleDetectionManager;

/**
 * Custom GameMode for RealGazebo with automatic camera and vehicle setup
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API AGazeboGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AGazeboGameMode();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // Auto-spawned managers
    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Managers")
    AGazeboCameraManager* CameraManager = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Managers")
    AGazeboVehicleDetectionManager* VehicleDetectionManager = nullptr;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Settings")
    bool bAutoSpawnCameraManager = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Settings")
    bool bAutoSpawnVehicleDetectionManager = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Settings")
    bool bAutoPossessCameraManager = true;

    // Setup functions
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Setup")
    void SetupCameraManager();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Setup")
    void SetupVehicleDetectionManager();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Setup")
    void ConnectManagersSystems();

private:
    void SpawnManagers();
    void ConnectSystems();
};