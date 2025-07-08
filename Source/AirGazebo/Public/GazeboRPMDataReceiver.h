// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDPReceiver.h"
#include "GazeboVehicleData.h"
#include "GazeboRPMDataReceiver.generated.h"

UCLASS(ClassGroup=(RealGazebo), meta=(BlueprintSpawnableComponent))
class REALGAZEBO_API UGazeboRPMDataReceiver : public UActorComponent
{
    GENERATED_BODY()

public:
    UGazeboRPMDataReceiver();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|RPM Receiver")
    int32 RPMPort = 5006;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|RPM Receiver")
    bool bAutoStart = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|RPM Receiver")
    bool bLogParsedData = false;

    // Control functions
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|RPM Receiver")
    bool StartRPMReceiver();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|RPM Receiver")
    void StopRPMReceiver();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|RPM Receiver")
    bool IsReceiving() const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|RPM Receiver")
    FOnGazeboRPMDataReceived OnVehicleRPMReceived;

    // Statistics
    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|RPM Receiver")
    int32 ValidRPMPacketsReceived;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|RPM Receiver")
    int32 InvalidRPMPacketsReceived;

protected:
    UPROPERTY()
    UUDPReceiver* UDPReceiver;

private:
    // Packet sizes for different vehicle types
    static const int32 BOAT_RPM_PACKET_SIZE = 11;   // 3 header + 2 motors (8 bytes)
    static const int32 ROVER_RPM_PACKET_SIZE = 19;  // 3 header + 4 motors (16 bytes)
    static const int32 IRIS_RPM_PACKET_SIZE = 19;   // 3 header + 4 motors (16 bytes)

    // Event handlers
    UFUNCTION()
    void OnUDPDataReceived(const FUDPData& ReceivedData);

    // Data parsing
    bool ParseRPMData(const TArray<uint8>& RawData, FGazeboRPMData& OutRPMData);
    float BytesToFloat(const TArray<uint8>& Data, int32 StartIndex);
    int32 GetExpectedPacketSize(EGazeboVehicleType VehicleType) const;
    int32 GetMotorCount(EGazeboVehicleType VehicleType) const;
};
