// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GazeboUnifiedDataReceiver.h"
#include "GazeboVehicleActor.h"
#include "GazeboVehicleManager.generated.h"

UCLASS()
class REALGAZEBO_API AGazeboVehicleManager : public AActor
{
    GENERATED_BODY()

public:
    AGazeboVehicleManager();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void Tick(float DeltaTime) override;

    // Components ----

    // GazeboUnifiedDataReceiver 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Components")
    UGazeboUnifiedDataReceiver* UnifiedDataReceiver;

    // Configuration  -----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Vehicle Configuration")
    class UDataTable* VehicleDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Vehicle Configuration|Spawning Settings")
    bool bAutoSpawnVehicles = true;

    // Control functions ----
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle Manager")
    void ClearAllVehicles();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle Manager")
    int32 GetActiveVehicleCount() const;

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle Manager")
    TArray<AGazeboVehicleActor*> GetAllVehicles() const;

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle Manager")
    AGazeboVehicleActor* FindVehicle(uint8 VehicleNum, uint8 VehicleType) const;

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle Manager")
    bool GetVehicleInfo(uint8 VehicleType, FGazeboVehicleTableRow& OutVehicleInfo) const;

protected:
    // Vehicle tracking
    UPROPERTY()
    TMap<FString, AGazeboVehicleActor*> SpawnedVehicles;

    // Statistics
    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Statistics")
    int32 TotalVehiclesSpawned;

private:
    // Event handlers
    UFUNCTION()
    void OnVehiclePoseDataReceived(const FGazeboPoseData& VehicleData);

    UFUNCTION()
    void OnVehicleMotorSpeedDataReceived(const FGazeboMotorSpeedData& MotorSpeedData);

    UFUNCTION()
    void OnVehicleServoDataReceived(const FGazeboServoData& ServoData);

    // Vehicle management
    AGazeboVehicleActor* SpawnVehicle(const FGazeboPoseData& VehicleData);
    FString GetVehicleKey(const FGazeboPoseData& VehicleData) const;
    FString GetVehicleKey(uint8 VehicleNum, uint8 VehicleType) const;
    TSubclassOf<AGazeboVehicleActor> GetVehicleClassForType(uint8 VehicleType) const;
    FVector GetSpawnLocation(const FGazeboPoseData& VehicleData) const;

    // Helper function for internal use (returns pointer)
    FGazeboVehicleTableRow* GetVehicleInfoInternal(uint8 VehicleType) const;
};