// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GazeboPoseDataReceiver.h"
#include "GazeboRPMDataReceiver.h"
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

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Components", meta = (ShowOnlyInnerProperties))
    UGazeboPoseDataReceiver* PoseDataReceiver;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealGazebo|Components", meta = (ShowOnlyInnerProperties))
    UGazeboRPMDataReceiver* RPMDataReceiver;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Vehicle Classes")
    TSubclassOf<AGazeboVehicleActor> IrisVehicleClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Vehicle Classes")
    TSubclassOf<AGazeboVehicleActor> RoverVehicleClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Vehicle Classes")
    TSubclassOf<AGazeboVehicleActor> BoatVehicleClass;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Spawning Settings")
    bool bAutoSpawnVehicles = true;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Spawning Settings")
    FVector SpawnOffset = FVector(0, 0, 0);

    // Control functions
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle Manager")
    void ClearAllVehicles();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle Manager")
    int32 GetActiveVehicleCount() const;

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle Manager")
    TArray<AGazeboVehicleActor*> GetAllVehicles() const;

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Vehicle Manager")
    AGazeboVehicleActor* FindVehicle(uint8 VehicleNum, EGazeboVehicleType VehicleType) const;

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
    void OnVehicleRPMDataReceived(const FGazeboRPMData& RPMData);

    // Vehicle management
    AGazeboVehicleActor* SpawnVehicle(const FGazeboPoseData& VehicleData);
    FString GetVehicleKey(const FGazeboPoseData& VehicleData) const;
    FString GetVehicleKey(uint8 VehicleNum, EGazeboVehicleType VehicleType) const;
    TSubclassOf<AGazeboVehicleActor> GetVehicleClassForType(EGazeboVehicleType VehicleType) const;
    FVector GetSpawnLocation(const FGazeboPoseData& VehicleData) const;
};