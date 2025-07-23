// Fill out your copyright notice in the Description page of Project Settings.

#include "GazeboUnifiedDataReceiver.h"
#include "Engine/Engine.h"
#include "GazeboVehicleManager.h"

UGazeboUnifiedDataReceiver::UGazeboUnifiedDataReceiver()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;

    UDPReceiver = nullptr;
    ValidPosePacketsReceived = 0;
    InvalidPosePacketsReceived = 0;
    ValidMotorSpeedPacketsReceived = 0;
    InvalidMotorSpeedPacketsReceived = 0;
    ValidServoPacketsReceived = 0;
    InvalidServoPacketsReceived = 0;
}

void UGazeboUnifiedDataReceiver::BeginPlay()
{
    Super::BeginPlay();

    UDPReceiver = NewObject<UUDPReceiver>(this);
    if (UDPReceiver)
    {
        UDPReceiver->OnDataReceived.AddDynamic(this, &UGazeboUnifiedDataReceiver::OnUDPDataReceived);

        if (bAutoStart)
        {
            StartUnifiedDataReceiver();
        }

        UE_LOG(LogTemp, Warning, TEXT("GazeboUnifiedDataReceiver: Initialized on %s:%d"), *ServerIPAddress, ListenPort);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GazeboUnifiedDataReceiver: Failed to create UDPReceiver"));
    }
}

void UGazeboUnifiedDataReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UDPReceiver)
    {
        UDPReceiver->OnDataReceived.RemoveAll(this);
        StopUnifiedDataReceiver();
        UDPReceiver = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void UGazeboUnifiedDataReceiver::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UGazeboUnifiedDataReceiver::StartUnifiedDataReceiver()
{
    if (!UDPReceiver)
    {
        UE_LOG(LogTemp, Error, TEXT("GazeboUnifiedDataReceiver: UDPReceiver is null"));
        return false;
    }

    bool bSuccess = UDPReceiver->StartListening(ListenPort);
    UE_LOG(LogTemp, Warning, TEXT("GazeboUnifiedDataReceiver: Start receiver on %s:%d - %s"), 
           *ServerIPAddress, ListenPort, bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
    return bSuccess;
}

void UGazeboUnifiedDataReceiver::StopUnifiedDataReceiver()
{
    if (UDPReceiver)
    {
        UDPReceiver->StopListening();
        UE_LOG(LogTemp, Warning, TEXT("GazeboUnifiedDataReceiver: Receiver stopped"));
    }
}

bool UGazeboUnifiedDataReceiver::IsReceiving() const
{
    return UDPReceiver ? UDPReceiver->IsListening() : false;
}

void UGazeboUnifiedDataReceiver::OnUDPDataReceived(const FUDPData& ReceivedData)
{
    if (ReceivedData.Data.Num() < 3)
    {
        InvalidPosePacketsReceived++;
        return;
    }

    // Check message ID to determine data type
    uint8 MessageID = ReceivedData.Data[2];
    
    if (MessageID == 1) // Pose data (6DOF)
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
                FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(PoseData.VehicleType);
                FString VehicleName = VehicleInfo ? VehicleInfo->VehicleName : TEXT("Unknown");
                
                UE_LOG(LogTemp, Log, TEXT("GazeboUnifiedDataReceiver: %s_%d - Pos(%.2f,%.2f,%.2f) Rot(%.2f,%.2f,%.2f)"),
                       *VehicleName, PoseData.VehicleNum,
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
    else if (MessageID == 2) // Motor speed data (DegPerSec)
    {
        FGazeboMotorSpeedData MotorSpeedData;
        if (ParseMotorSpeedData(ReceivedData.Data, MotorSpeedData))
        {
            ValidMotorSpeedPacketsReceived++;
            
            if (bLogParsedData)
            {
                FString MotorSpeedsStr;
                for (int32 i = 0; i < MotorSpeedData.MotorSpeeds_DegPerSec.Num(); i++)
                {
                    MotorSpeedsStr += FString::Printf(TEXT("M%d:%.1f°/s "), i, MotorSpeedData.MotorSpeeds_DegPerSec[i]);
                }
                
                FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(MotorSpeedData.VehicleType);
                FString VehicleName = VehicleInfo ? VehicleInfo->VehicleName : TEXT("Unknown");
                
                UE_LOG(LogTemp, Log, TEXT("GazeboUnifiedDataReceiver: %s_%d - %s"),
                       *VehicleName, MotorSpeedData.VehicleNum, *MotorSpeedsStr);
            }

            OnVehicleMotorSpeedReceived.Broadcast(MotorSpeedData);
        }
        else
        {
            InvalidMotorSpeedPacketsReceived++;
        }
    }
    else if (MessageID == 3) // Servo data
    {
        FGazeboServoData ServoData;
        if (ParseServoData(ReceivedData.Data, ServoData))
        {
            ValidServoPacketsReceived++;
            
            if (bLogParsedData)
            {
                FString ServoStr;
                for (int32 i = 0; i < ServoData.ServoRotations.Num(); i++)
                {
                    ServoStr += FString::Printf(TEXT("S%d:[P:%.2f,%.2f,%.2f R:%.1f,%.1f,%.1f] "),
                        i, ServoData.ServoPositions[i].X, ServoData.ServoPositions[i].Y, ServoData.ServoPositions[i].Z,
                        ServoData.ServoRotations[i].Roll, ServoData.ServoRotations[i].Pitch, ServoData.ServoRotations[i].Yaw);
                }
                
                FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(ServoData.VehicleType);
                FString VehicleName = VehicleInfo ? VehicleInfo->VehicleName : TEXT("Unknown");
                
                UE_LOG(LogTemp, Log, TEXT("GazeboUnifiedDataReceiver: %s_%d - %s"),
                       *VehicleName, ServoData.VehicleNum, *ServoStr);
            }

            OnVehicleServoReceived.Broadcast(ServoData);
        }
        else
        {
            InvalidServoPacketsReceived++;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GazeboUnifiedDataReceiver: Unknown message ID: %d"), MessageID);
        InvalidPosePacketsReceived++;
    }
}

bool UGazeboUnifiedDataReceiver::ParsePoseData(const TArray<uint8>& RawData, FGazeboPoseData& OutPoseData)
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

bool UGazeboUnifiedDataReceiver::ParseMotorSpeedData(const TArray<uint8>& RawData, FGazeboMotorSpeedData& OutMotorSpeedData)
{
    if (RawData.Num() < 3)
    {
        return false;
    }

    // Parse header
    OutMotorSpeedData.VehicleNum = RawData[0];
    OutMotorSpeedData.VehicleType = RawData[1];
    OutMotorSpeedData.MessageID = RawData[2];

    // Validate message ID for motor speed data
    if (OutMotorSpeedData.MessageID != 2)
    {
        return false;
    }

    // Validate packet size
    int32 ExpectedSize = GetExpectedMotorSpeedPacketSize(OutMotorSpeedData.VehicleType);
    if (ExpectedSize == 0 || RawData.Num() != ExpectedSize)
    {
        return false;
    }

    // Parse motor speeds (convert from rad/s to deg/s)
    int32 MotorCount = GetMotorCount(OutMotorSpeedData.VehicleType);
    OutMotorSpeedData.MotorSpeeds_DegPerSec.Empty();
    OutMotorSpeedData.MotorSpeeds_DegPerSec.Reserve(MotorCount);

    for (int32 i = 0; i < MotorCount; i++)
    {
        int32 StartIndex = 3 + (i * 4); // 3 bytes header + 4 bytes per float
        float RadiansPerSec = BytesToFloat(RawData, StartIndex);
        float DegreesPerSec = RadiansPerSec * (180.0f / PI); // Convert rad/s to deg/s
        OutMotorSpeedData.MotorSpeeds_DegPerSec.Add(DegreesPerSec);
    }

    return true;
}

bool UGazeboUnifiedDataReceiver::ParseServoData(const TArray<uint8>& RawData, FGazeboServoData& OutServoData)
{
    if (RawData.Num() < 3)
    {
        return false;
    }

    // Parse header
    OutServoData.VehicleNum = RawData[0];
    OutServoData.VehicleType = RawData[1];
    OutServoData.MessageID = RawData[2];

    // Validate message ID for servo data
    if (OutServoData.MessageID != 3)
    {
        return false;
    }

    // Validate packet size
    int32 ExpectedSize = GetExpectedServoPacketSize(OutServoData.VehicleType);
    if (ExpectedSize == 0 || RawData.Num() != ExpectedSize)
    {
        return false;
    }

    // Parse servo positions and rotations
    int32 ServoCount = GetServoCount(OutServoData.VehicleType);
    OutServoData.ServoPositions.Empty();
    OutServoData.ServoPositions.Reserve(ServoCount);
    OutServoData.ServoRotations.Empty();
    OutServoData.ServoRotations.Reserve(ServoCount);

    for (int32 i = 0; i < ServoCount; i++)
    {
        int32 StartIndex = 3 + (i * 24); // 3-byte header + 24 bytes per servo (6 floats)
        
        if (StartIndex + 23 >= RawData.Num())
        {
            return false;
        }

        // Parse position (X, Y, Z)
        float X = BytesToFloat(RawData, StartIndex);
        float Y = BytesToFloat(RawData, StartIndex + 4);
        float Z = BytesToFloat(RawData, StartIndex + 8);
        OutServoData.ServoPositions.Add(ConvertGazeboPositionToUnreal(X, Y, Z));

        // Parse rotation (Roll, Pitch, Yaw)
        float Roll = BytesToFloat(RawData, StartIndex + 12);
        float Pitch = BytesToFloat(RawData, StartIndex + 16);
        float Yaw = BytesToFloat(RawData, StartIndex + 20);
        OutServoData.ServoRotations.Add(ConvertGazeboRotationToUnreal(Roll, Pitch, Yaw));
    }

    return true;
}

float UGazeboUnifiedDataReceiver::BytesToFloat(const TArray<uint8>& Data, int32 StartIndex)
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

FVector UGazeboUnifiedDataReceiver::ConvertGazeboPositionToUnreal(float X, float Y, float Z)
{
    // Scale by 100 to convert from meters to centimeters
    // Negate Y (right-handed to left-handed coordinate system)
    return FVector(X * 100.0f, -Y * 100.0f, Z * 100.0f);
}

FRotator UGazeboUnifiedDataReceiver::ConvertGazeboRotationToUnreal(float Roll, float Pitch, float Yaw)
{
    // Convert from radians to degrees
    // Negate pitch and yaw for coordinate system conversion
    return FRotator(-FMath::RadiansToDegrees(Pitch), 
                    -FMath::RadiansToDegrees(Yaw), 
                    FMath::RadiansToDegrees(Roll));
}

int32 UGazeboUnifiedDataReceiver::GetExpectedMotorSpeedPacketSize(uint8 VehicleType) const
{
    FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(VehicleType);
    return VehicleInfo ? VehicleInfo->GetMotorSpeedPacketSize() : 0;
}

int32 UGazeboUnifiedDataReceiver::GetMotorCount(uint8 VehicleType) const
{
    FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(VehicleType);
    return VehicleInfo ? VehicleInfo->MotorCount : 0;
}

int32 UGazeboUnifiedDataReceiver::GetExpectedServoPacketSize(uint8 VehicleType) const
{
    FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(VehicleType);
    return VehicleInfo ? VehicleInfo->GetServoPacketSize() : 0;
}

int32 UGazeboUnifiedDataReceiver::GetServoCount(uint8 VehicleType) const
{
    FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(VehicleType);
    return VehicleInfo ? VehicleInfo->ServoCount : 0;
}

FGazeboVehicleTableRow* UGazeboUnifiedDataReceiver::GetVehicleInfo(uint8 VehicleType) const
{
    // Get VehicleDataTable from the owner (GazeboVehicleManager)
    UDataTable* VehicleDataTable = nullptr;
    if (AActor* Owner = GetOwner())
    {
        if (AGazeboVehicleManager* Manager = Cast<AGazeboVehicleManager>(Owner))
        {
            VehicleDataTable = Manager->VehicleDataTable;
        }
    }

    if (!VehicleDataTable)
    {
        return nullptr;
    }

    TArray<FGazeboVehicleTableRow*> AllRows;
    VehicleDataTable->GetAllRows<FGazeboVehicleTableRow>(TEXT("GetVehicleInfo"), AllRows);

    for (FGazeboVehicleTableRow* Row : AllRows)
    {
        if (Row && Row->VehicleTypeCode == VehicleType)
        {
            return Row;
        }
    }

    return nullptr;
}