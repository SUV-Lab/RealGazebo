// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDPReceiver.h"
#include "Vehicle/GazeboVehicleData.h"
#include "GazeboUnifiedDataReceiver.generated.h"

UCLASS(ClassGroup=(RealGazebo), meta=(BlueprintSpawnableComponent))
class REALGAZEBO_API UGazeboUnifiedDataReceiver : public UActorComponent
{
    GENERATED_BODY()

public:
    UGazeboUnifiedDataReceiver();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Unified Data Receiver")
    int32 ListenPort = 5005;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Unified Data Receiver")
    FString ServerIPAddress = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Unified Data Receiver")
    bool bAutoStart = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Unified Data Receiver")
    bool bLogParsedData = false;

    // Control functions
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Unified Data Receiver")
    bool StartUnifiedDataReceiver();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Unified Data Receiver")
    void StopUnifiedDataReceiver();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Unified Data Receiver")
    bool IsReceiving() const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Unified Data Receiver")
    FOnGazeboVehicleDataReceived OnVehiclePoseReceived;

    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Unified Data Receiver")
    FOnGazeboMotorSpeedDataReceived OnVehicleMotorSpeedReceived;

    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Unified Data Receiver")
    FOnGazeboServoDataReceived OnVehicleServoReceived;
    // Statistics
    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Unified Data Receiver")
    int32 ValidPosePacketsReceived;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Unified Data Receiver")
    int32 InvalidPosePacketsReceived;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Unified Data Receiver")
    int32 ValidMotorSpeedPacketsReceived;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Unified Data Receiver")
    int32 InvalidMotorSpeedPacketsReceived;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Unified Data Receiver")
    int32 ValidServoPacketsReceived;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Unified Data Receiver")
    int32 InvalidServoPacketsReceived;
protected:
    TUniquePtr<FUDPReceiver> UDPReceiver;

private:
    static const int32 EXPECTED_POSE_PACKET_SIZE = 31; // 3 header + 7 float32 (28 bytes) - pos(3) + quat(4)

    // Event handlers
    void OnUDPDataReceived(const TArray<uint8>& Data, bool bSuccess, const FString& ErrorMessage, int32 BytesReceived);

    // Data parsing
    bool ParsePoseData(const TArray<uint8>& RawData, FGazeboPoseData& OutPoseData);
    bool ParseMotorSpeedData(const TArray<uint8>& RawData, FGazeboMotorSpeedData& OutMotorSpeedData);
    bool ParseServoData(const TArray<uint8>& RawData, FGazeboServoData& OutServoData);
    float BytesToFloat(const TArray<uint8>& Data, int32 StartIndex);
    FVector ConvertGazeboPositionToUnreal(float X, float Y, float Z);
    FRotator ConvertGazeboRotationToUnreal(float Roll, float Pitch, float Yaw);
    FQuat ConvertGazeboQuaternionToUnreal(float X, float Y, float Z, float W);
    FQuat ConvertGazeboQuaternionToUnreal(float Roll, float Pitch, float Yaw);
    
    // Helper functions
    int32 GetExpectedMotorSpeedPacketSize(uint8 VehicleType) const;
    int32 GetExpectedServoPacketSize(uint8 VehicleType) const;
    int32 GetMotorCount(uint8 VehicleType) const;
    int32 GetServoCount(uint8 VehicleType) const;
    FGazeboVehicleTableRow* GetVehicleInfo(uint8 VehicleType) const;
};
