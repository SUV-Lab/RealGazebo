// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "GazeboVehicleDetectionManager.generated.h"

class AGazeboVehicleActor;
class UGazeboVehicleInfoWidget;
class AGazeboCameraManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVehicleDetected, AGazeboVehicleActor*, Vehicle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVehicleRemoved, AGazeboVehicleActor*, Vehicle);

USTRUCT(BlueprintType)
struct FDetectedVehicleInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AGazeboVehicleActor* VehicleActor = nullptr;

    UPROPERTY(BlueprintReadOnly)
    int32 VehicleID = -1;

    UPROPERTY(BlueprintReadOnly)
    FString VehicleName = TEXT("Unknown");

    UPROPERTY(BlueprintReadOnly)
    bool bIsActive = false;

    FDetectedVehicleInfo()
    {
        VehicleActor = nullptr;
        VehicleID = -1;
        VehicleName = TEXT("Unknown");
        bIsActive = false;
    }

    FDetectedVehicleInfo(AGazeboVehicleActor* Actor, int32 ID, const FString& Name)
    {
        VehicleActor = Actor;
        VehicleID = ID;
        VehicleName = Name;
        bIsActive = true;
    }
};

/**
 * Manager for automatically detecting and tracking vehicles in the level
 */
UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOUI_API AGazeboVehicleDetectionManager : public AActor
{
    GENERATED_BODY()

public:
    AGazeboVehicleDetectionManager();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void Tick(float DeltaTime) override;

    // Vehicle Detection
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Detection")
    void DetectAllVehiclesInLevel();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Detection")
    TArray<AGazeboVehicleActor*> GetAllVehicleActors() const;

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Detection")
    TArray<FDetectedVehicleInfo> GetDetectedVehicles() const { return DetectedVehicles; }

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Detection")
    AGazeboVehicleActor* GetVehicleByID(int32 VehicleID) const;

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Detection")
    AGazeboVehicleActor* GetVehicleByIndex(int32 Index) const;

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Detection")
    int32 GetVehicleCount() const { return DetectedVehicles.Num(); }

    // Vehicle Management
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Management")
    void InitializeAllVehicles();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Management")
    void SetCameraManager(AGazeboCameraManager* NewCameraManager);

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Management")
    void SetTargetVehicleByIndex(int32 Index);

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Management")
    void SetTargetVehicleByID(int32 VehicleID);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Events")
    FOnVehicleDetected OnVehicleDetected;

    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Events")
    FOnVehicleRemoved OnVehicleRemoved;

protected:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Settings")
    float DetectionInterval = 1.0f; // Seconds between detection scans

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Settings")
    bool bAutoDetectOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Settings")
    bool bContinuousDetection = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Settings")
    TSubclassOf<AGazeboVehicleActor> VehicleClassToDetect;

private:
    UPROPERTY()
    TArray<FDetectedVehicleInfo> DetectedVehicles;

    UPROPERTY()
    AGazeboCameraManager* CameraManager = nullptr;

    float DetectionTimer = 0.0f;

    // Helper functions
    void UpdateDetectedVehicles();
    void AddVehicle(AGazeboVehicleActor* Vehicle);
    void RemoveVehicle(AGazeboVehicleActor* Vehicle);
    bool IsVehicleAlreadyDetected(AGazeboVehicleActor* Vehicle) const;
    int32 GetNextAvailableVehicleID() const;
};