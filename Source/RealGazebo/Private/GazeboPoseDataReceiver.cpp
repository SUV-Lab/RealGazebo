// Fill out your copyright notice in the Description page of Project Settings.


#include "GazeboPoseDataReceiver.h"
#include "Engine/Engine.h"

UGazeboPoseDataReceiver::UGazeboPoseDataReceiver()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;

    UDPReceiver = nullptr;
    ValidPosePacketsReceived = 0;
    InvalidPosePacketsReceived = 0;
}

void UGazeboPoseDataReceiver::BeginPlay()
{
    Super::BeginPlay();

    UDPReceiver = NewObject<UUDPReceiver>(this);
    if (UDPReceiver)
    {
        UDPReceiver->OnDataReceived.AddDynamic(this, &UGazeboPoseDataReceiver::OnUDPDataReceived);

        if (bAutoStart)
        {
            StartPoseReceiver();
        }

        UE_LOG(LogTemp, Warning, TEXT("GazeboPoseDataReceiver: Initialized on port %d"), PosePort);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GazeboPoseDataReceiver: Failed to create UDPReceiver"));
    }
}

void UGazeboPoseDataReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UDPReceiver)
    {
        UDPReceiver->OnDataReceived.RemoveAll(this);
        StopPoseReceiver();
        UDPReceiver = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void UGazeboPoseDataReceiver::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UGazeboPoseDataReceiver::StartPoseReceiver()
{
    if (!UDPReceiver)
    {
        UE_LOG(LogTemp, Error, TEXT("GazeboPoseDataReceiver: UDPReceiver is null"));
        return false;
    }

    bool bSuccess = UDPReceiver->StartListening(PosePort);
    UE_LOG(LogTemp, Warning, TEXT("GazeboPoseDataReceiver: Start receiver - %s"), 
           bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
    return bSuccess;
}

void UGazeboPoseDataReceiver::StopPoseReceiver()
{
    if (UDPReceiver)
    {
        UDPReceiver->StopListening();
        UE_LOG(LogTemp, Warning, TEXT("GazeboPoseDataReceiver: Receiver stopped"));
    }
}

bool UGazeboPoseDataReceiver::IsReceiving() const
{
    return UDPReceiver ? UDPReceiver->IsListening() : false;
}

void UGazeboPoseDataReceiver::OnUDPDataReceived(const FUDPData& ReceivedData)
{
    if (ReceivedData.Data.Num() != EXPECTED_POSE_PACKET_SIZE)
    {
        InvalidPosePacketsReceived++;
        return;
    }

    FGazeboPoseData PoseData;
    if (ParsePoseData(ReceivedData.Data, PoseData))
    {
        ValidPosePacketsReceived++;
        
        if (bLogParsedData)
        {
            UE_LOG(LogTemp, Log, TEXT("GazeboPoseDataReceiver: Vehicle_%d (Type: %d) - Pos(%.4f,%.4f,%.4f) Rot(%.4f,%.4f,%.4f)"),
                   PoseData.VehicleNum, PoseData.VehicleType,
                   PoseData.Position.X, PoseData.Position.Y, PoseData.Position.Z,
                   PoseData.Rotation.Roll, PoseData.Rotation.Pitch, PoseData.Rotation.Yaw);
        }

        OnVehiclePoseReceived.Broadcast(PoseData);
    }
    else
    {
        InvalidPosePacketsReceived++;
    }
}

bool UGazeboPoseDataReceiver::ParsePoseData(const TArray<uint8>& RawData, FGazeboPoseData& OutPoseData)
{
    if (RawData.Num() != EXPECTED_POSE_PACKET_SIZE)
    {
        return false;
    }

    // Parse header
    OutPoseData.VehicleNum = RawData[0];
    OutPoseData.VehicleType = RawData[1];
    OutPoseData.MessageID = RawData[2];

    // Validate message ID for pose data
    if (OutPoseData.MessageID != 1)
    {
        return false;
    }

    // Parse position (bytes 3-14)
    float X = BytesToFloat(RawData, 3);
    float Y = BytesToFloat(RawData, 7);
    float Z = BytesToFloat(RawData, 11);
    OutPoseData.Position = ConvertGazeboPositionToUnreal(X, Y, Z);

    // Parse rotation (bytes 15-26)
    float Roll = BytesToFloat(RawData, 15);
    float Pitch = BytesToFloat(RawData, 19);
    float Yaw = BytesToFloat(RawData, 23);
    OutPoseData.Rotation = ConvertGazeboRotationToUnreal(Roll, Pitch, Yaw);

    return true;
}

float UGazeboPoseDataReceiver::BytesToFloat(const TArray<uint8>& Data, int32 StartIndex)
{
    if (StartIndex + 3 >= Data.Num())
    {
        return 0.0f;
    }

    union
    {
        uint8 bytes[4];
        float value;
    } converter;

    // Little-endian byte order
    converter.bytes[0] = Data[StartIndex];
    converter.bytes[1] = Data[StartIndex + 1];
    converter.bytes[2] = Data[StartIndex + 2];
    converter.bytes[3] = Data[StartIndex + 3];

    return converter.value;
}

FVector UGazeboPoseDataReceiver::ConvertGazeboPositionToUnreal(float X, float Y, float Z)
{
    // Scale by 100 to convert from meters to centimeters
    // Negate Y (right-handed to left-handed coordinate system)
    return FVector(X * 100.0f, -Y * 100.0f, Z * 100.0f);
}

FRotator UGazeboPoseDataReceiver::ConvertGazeboRotationToUnreal(float Roll, float Pitch, float Yaw)
{
    // Convert from radians to degrees
    // Negate pitch and yaw for coordinate system conversion
    return FRotator(-FMath::RadiansToDegrees(Pitch), 
                    -FMath::RadiansToDegrees(Yaw), 
                    FMath::RadiansToDegrees(Roll));
}