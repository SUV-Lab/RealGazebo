// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDPReceiver.h"
#include "GazeboVehicleData.h"
#include "GazeboPoseDataReceiver.generated.h"

UCLASS(ClassGroup=(RealGazebo), meta=(BlueprintSpawnableComponent))
class REALGAZEBO_API UGazeboPoseDataReceiver : public UActorComponent
{
    GENERATED_BODY()

public:
    UGazeboPoseDataReceiver();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Pose Receiver")
    int32 PosePort = 5005;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Pose Receiver")
    bool bAutoStart = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealGazebo|Pose Receiver")
    bool bLogParsedData = false;

    // Control functions
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Pose Receiver")
    bool StartPoseReceiver();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Pose Receiver")
    void StopPoseReceiver();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|Pose Receiver")
    bool IsReceiving() const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|Pose Receiver")
    FOnGazeboVehicleDataReceived OnVehiclePoseReceived;

    // Statistics
    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Pose Receiver")
    int32 ValidPosePacketsReceived;

    UPROPERTY(BlueprintReadOnly, Category = "RealGazebo|Pose Receiver")
    int32 InvalidPosePacketsReceived;

protected:
    UPROPERTY()
    UUDPReceiver* UDPReceiver;

private:
    static const int32 EXPECTED_POSE_PACKET_SIZE = 27; // 3 header + 6 float32 (24 bytes)

    // Event handlers
    UFUNCTION()
    void OnUDPDataReceived(const FUDPData& ReceivedData);

    // Data parsing
    bool ParsePoseData(const TArray<uint8>& RawData, FGazeboPoseData& OutPoseData);
    float BytesToFloat(const TArray<uint8>& Data, int32 StartIndex);
    FVector ConvertGazeboPositionToUnreal(float X, float Y, float Z);
    FRotator ConvertGazeboRotationToUnreal(float Roll, float Pitch, float Yaw);
};