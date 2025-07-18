// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDPReceiver.h"
#include "GazeboVehicleData.h"
#include "GazeboServoDataReceiver.generated.h"

UCLASS(ClassGroup = (RealGazebo), meta = (BlueprintSpawnableComponent))
class REALGAZEBO_API UGazeboServoDataReceiver : public UActorComponent
{
    GENERATED_BODY()

public:
    UGazeboServoDataReceiver();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Servo Receiver")
    int32 ServoPort = 5007;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Servo Receiver")
    bool bAutoStart = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Servo Receiver")
    bool bLogParsedData = false;

    // Control functions
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Servo Receiver")
    bool StartServoReceiver();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Servo Receiver")
    void StopServoReceiver();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Servo Receiver")
    bool IsReceiving() const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Servo Receiver")
    FOnGazeboServoDataReceived OnVehicleServoReceived;

    // Statistics
    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Servo Receiver")
    int32 ValidServoPacketsReceived;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Servo Receiver")
    int32 InvalidServoPacketsReceived;

protected:
    UPROPERTY()
    UUDPReceiver* UDPReceiver;

private:
    UFUNCTION()
    void OnUDPDataReceived(const FUDPData& ReceivedData);

    bool ParseServoData(const TArray<uint8>& RawData, FGazeboServoData& OutServoData);
    
    float BytesToFloat(const TArray<uint8>& Data, int32 StartIndex);

    int32 GetExpectedPacketSize(uint8 VehicleType) const;
    int32 GetServoCount(uint8 VehicleType) const;
    
    FGazeboVehicleTableRow* GetVehicleInfo(uint8 VehicleType) const;
    FRotator ConvertGazeboRotationToUnreal(float Roll, float Pitch, float Yaw);
    FVector ConvertGazeboPositionToUnreal(float X, float Y, float Z);
}; 