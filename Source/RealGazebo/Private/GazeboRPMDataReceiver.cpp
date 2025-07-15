// Fill out your copyright notice in the Description page of Project Settings.

#include "GazeboRPMDataReceiver.h"
#include "Engine/Engine.h"
#include "GazeboVehicleManager.h"

UGazeboRPMDataReceiver::UGazeboRPMDataReceiver()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;

    UDPReceiver = nullptr;
    ValidRPMPacketsReceived = 0;
    InvalidRPMPacketsReceived = 0;
}

void UGazeboRPMDataReceiver::BeginPlay()
{
    Super::BeginPlay();

    UDPReceiver = NewObject<UUDPReceiver>(this);
    if (UDPReceiver)
    {
        UDPReceiver->OnDataReceived.AddDynamic(this, &UGazeboRPMDataReceiver::OnUDPDataReceived);

        if (bAutoStart)
        {
            StartRPMReceiver();
        }

        UE_LOG(LogTemp, Warning, TEXT("GazeboRPMDataReceiver: Initialized on port %d"), RPMPort);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GazeboRPMDataReceiver: Failed to create UDPReceiver"));
    }
}

void UGazeboRPMDataReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UDPReceiver)
    {
        UDPReceiver->OnDataReceived.RemoveAll(this);
        StopRPMReceiver();
        UDPReceiver = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void UGazeboRPMDataReceiver::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UGazeboRPMDataReceiver::StartRPMReceiver()
{
    if (!UDPReceiver)
    {
        UE_LOG(LogTemp, Error, TEXT("GazeboRPMDataReceiver: UDPReceiver is null"));
        return false;
    }

    bool bSuccess = UDPReceiver->StartListening(RPMPort);
    UE_LOG(LogTemp, Warning, TEXT("GazeboRPMDataReceiver: Start receiver - %s"), 
           bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
    return bSuccess;
}

void UGazeboRPMDataReceiver::StopRPMReceiver()
{
    if (UDPReceiver)
    {
        UDPReceiver->StopListening();
        UE_LOG(LogTemp, Warning, TEXT("GazeboRPMDataReceiver: Receiver stopped"));
    }
}

bool UGazeboRPMDataReceiver::IsReceiving() const
{
    return UDPReceiver ? UDPReceiver->IsListening() : false;
}

void UGazeboRPMDataReceiver::OnUDPDataReceived(const FUDPData& ReceivedData)
{
    FGazeboRPMData RPMData;
    if (ParseRPMData(ReceivedData.Data, RPMData))
    {
        ValidRPMPacketsReceived++;
        
        if (bLogParsedData)
        {
            FString MotorRPMsStr;
            for (int32 i = 0; i < RPMData.MotorRPMs.Num(); i++)
            {
                MotorRPMsStr += FString::Printf(TEXT("M%d:%.1f "), i, RPMData.MotorRPMs[i]);
            }
            
            FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(RPMData.VehicleType);
            FString VehicleName = VehicleInfo ? VehicleInfo->VehicleName : TEXT("Unknown");
            
            UE_LOG(LogTemp, Log, TEXT("GazeboRPMDataReceiver: %s_%d - %s"),
                   *VehicleName, RPMData.VehicleNum, *MotorRPMsStr);
        }

        OnVehicleRPMReceived.Broadcast(RPMData);
    }
    else
    {
        InvalidRPMPacketsReceived++;
    }
}

bool UGazeboRPMDataReceiver::ParseRPMData(const TArray<uint8>& RawData, FGazeboRPMData& OutRPMData)
{
    if (RawData.Num() < 3)
    {
        return false;
    }

    // Parse header
    OutRPMData.VehicleNum = RawData[0];
    OutRPMData.VehicleType = RawData[1];
    OutRPMData.MessageID = RawData[2];

    // Validate message ID for RPM data
    if (OutRPMData.MessageID != 2)
    {
        return false;
    }

    // Validate packet size
    int32 ExpectedSize = GetExpectedPacketSize(OutRPMData.VehicleType);
    if (ExpectedSize == 0 || RawData.Num() != ExpectedSize)
    {
        return false;
    }

    // Parse motor RPMs
    int32 MotorCount = GetMotorCount(OutRPMData.VehicleType);
    OutRPMData.MotorRPMs.Empty();
    OutRPMData.MotorRPMs.Reserve(MotorCount);

    for (int32 i = 0; i < MotorCount; i++)
    {
        int32 StartIndex = 3 + (i * 4); // 3 bytes header + 4 bytes per float
        float RPM = BytesToFloat(RawData, StartIndex);
        OutRPMData.MotorRPMs.Add(RPM);
    }

    return true;
}

float UGazeboRPMDataReceiver::BytesToFloat(const TArray<uint8>& Data, int32 StartIndex)
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

int32 UGazeboRPMDataReceiver::GetExpectedPacketSize(uint8 VehicleType) const
{
    FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(VehicleType);
    return VehicleInfo ? VehicleInfo->GetRPMPacketSize() : 0;
}

int32 UGazeboRPMDataReceiver::GetMotorCount(uint8 VehicleType) const
{
    FGazeboVehicleTableRow* VehicleInfo = GetVehicleInfo(VehicleType);
    return VehicleInfo ? VehicleInfo->MotorCount : 0;
}

FGazeboVehicleTableRow* UGazeboRPMDataReceiver::GetVehicleInfo(uint8 VehicleType) const
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
