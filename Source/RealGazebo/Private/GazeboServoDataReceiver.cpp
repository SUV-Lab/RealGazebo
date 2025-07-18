// Fill out your copyright notice in the Description page of Project Settings.

#include "GazeboServoDataReceiver.h"
#include "Engine/Engine.h"
#include "GazeboVehicleManager.h"

UGazeboServoDataReceiver::UGazeboServoDataReceiver()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;

    UDPReceiver = nullptr;
    ValidServoPacketsReceived = 0;
    InvalidServoPacketsReceived = 0;
}

void UGazeboServoDataReceiver::BeginPlay()
{
    Super::BeginPlay();

    UDPReceiver = NewObject<UUDPReceiver>(this);
    if (UDPReceiver)
    {
        UDPReceiver->OnDataReceived.AddDynamic(this, &UGazeboServoDataReceiver::OnUDPDataReceived);

        if (bAutoStart)
        {
            StartServoReceiver();
        }

        UE_LOG(LogTemp, Warning, TEXT("GazeboServoDataReceiver: Initialized on port %d"), ServoPort);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GazeboServoDataReceiver: Failed to create UDPReceiver"));
    }
}

void UGazeboServoDataReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UDPReceiver)
    {
        UDPReceiver->OnDataReceived.RemoveAll(this);
        StopServoReceiver();
        UDPReceiver = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void UGazeboServoDataReceiver::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UGazeboServoDataReceiver::StartServoReceiver()
{
    if (!UDPReceiver)
    {
        UE_LOG(LogTemp, Error, TEXT("GazeboServoDataReceiver: UDPReceiver is null"));
        return false;
    }

    bool bSuccess = UDPReceiver->StartListening(ServoPort);
    UE_LOG(LogTemp, Warning, TEXT("GazeboServoDataReceiver: Start receiver - %s"),
           bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
    return bSuccess;
}

void UGazeboServoDataReceiver::StopServoReceiver()
{
    if (UDPReceiver)
    {
        UDPReceiver->StopListening();
        UE_LOG(LogTemp, Warning, TEXT("GazeboServoDataReceiver: Receiver stopped"));
    }
}

bool UGazeboServoDataReceiver::IsReceiving() const
{
    return UDPReceiver ? UDPReceiver->IsListening() : false;
}

void UGazeboServoDataReceiver::OnUDPDataReceived(const FUDPData& ReceivedData)
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
                ServoStr += FString::Printf(TEXT("S%d:[x:%.1f,y:%.1f,z:%.1f,R:%.1f,P:%.1f,Y:%.1f] "),
                i, ServoData.ServoPositions[i].X, ServoData.ServoPositions[i].Y, ServoData.ServoPositions[i].Z,
                ServoData.ServoRotations[i].Roll, ServoData.ServoRotations[i].Pitch, ServoData.ServoRotations[i].Yaw);
            }

            FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(ServoData.VehicleType);
            FString VehicleName = VehicleInfo ? VehicleInfo->VehicleName : TEXT("Unknown");

            UE_LOG(LogTemp, Log, TEXT("GazeboServoDataReceiver: %s_%d - %s"),
                   *VehicleName, ServoData.VehicleNum, *ServoStr);
        }

        OnVehicleServoReceived.Broadcast(ServoData);
    }
    else
    {
        InvalidServoPacketsReceived++;
    }
}

bool UGazeboServoDataReceiver::ParseServoData(const TArray<uint8>& RawData, FGazeboServoData& OutServoData)
{
    if (RawData.Num() < 3)
    {
        return false;
    }

    // Parse header
    OutServoData.VehicleNum = RawData[0];
    OutServoData.VehicleType = RawData[1];
    OutServoData.MessageID = RawData[2];

    // Validate message ID for Servo data
    if (OutServoData.MessageID != 3)
    {
        return false;
    }

    // Validate packet size
    int32 ExpectedSize = GetExpectedPacketSize(OutServoData.VehicleType);
    if (ExpectedSize == 0 || RawData.Num() != ExpectedSize)
    {
        return false;
    }

    // Parse servo rotations
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

        // Parse Roll, Pitch, Yaw from the raw data
        float X = BytesToFloat(RawData, StartIndex);
        float Y = BytesToFloat(RawData, StartIndex + 4);
        float Z = BytesToFloat(RawData, StartIndex + 8);

        OutServoData.ServoPositions.Add(ConvertGazeboPositionToUnreal(X, Y, Z));

        float Roll = BytesToFloat(RawData, StartIndex + 12);
        float Pitch = BytesToFloat(RawData, StartIndex + 16);
        float Yaw = BytesToFloat(RawData, StartIndex + 20);

        OutServoData.ServoRotations.Add(ConvertGazeboRotationToUnreal(Roll, Pitch, Yaw));
    }

    return true;
}

float UGazeboServoDataReceiver::BytesToFloat(const TArray<uint8>& Data, int32 StartIndex)
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

int32 UGazeboServoDataReceiver::GetExpectedPacketSize(uint8 VehicleType) const
{
    FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(VehicleType);
    return VehicleInfo ? VehicleInfo->GetServoPacketSize() : 0;
}

int32 UGazeboServoDataReceiver::GetServoCount(uint8 VehicleType) const
{
    FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(VehicleType);
    return VehicleInfo ? VehicleInfo->ServoCount : 0;
}

FGazeboVehicleTableRow* UGazeboServoDataReceiver::GetVehicleInfo(uint8 VehicleType) const
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

FVector UGazeboServoDataReceiver::ConvertGazeboPositionToUnreal(float X, float Y, float Z)
{
    // Scale by 100 to convert from meters to centimeters
    // Negate Y (right-handed to left-handed coordinate system)
    return FVector(X * 100.0f, -Y * 100.0f, Z * 100.0f);
}

FRotator UGazeboServoDataReceiver::ConvertGazeboRotationToUnreal(float Roll, float Pitch, float Yaw)
{
    // Convert from radians to degrees
    // Negate pitch and yaw for coordinate system conversion
    return FRotator(-FMath::RadiansToDegrees(Pitch), 
                    -FMath::RadiansToDegrees(Yaw), 
                    FMath::RadiansToDegrees(Roll));
}