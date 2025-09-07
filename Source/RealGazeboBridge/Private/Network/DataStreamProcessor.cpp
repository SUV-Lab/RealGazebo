
#include "DataStreamProcessor.h"
#include "Engine/Engine.h"
#include "GazeboBridgeTypes.h"
#include "Core/GazeboBridgeSubsystem.h"
#include "RealGazeboBridge.h"

UDataStreamProcessor::UDataStreamProcessor()
{
    UDPReceiver = nullptr;
    
    // Initialize statistics
    TotalValidPosePackets = 0;
    TotalValidMotorPackets = 0;
    TotalValidServoPackets = 0;
    TotalInvalidPackets = 0;
    PacketsPerSecond = 0.0f;
    
    // Initialize performance tracking
    LastStatsUpdate = 0.0;
    PacketCountSinceLastUpdate = 0;
    TotalProcessingTime = 0.0f;
    ProcessedBatches = 0;
}

void UDataStreamProcessor::Initialize(UGazeboBridgeSubsystem* InBridgeSubsystem)
{
    BridgeSubsystem = InBridgeSubsystem;
    
    // Create UDP receiver
    UDPReceiver = NewObject<UUDPReceiver>(this);
    if (UDPReceiver)
    {
        UDPReceiver->OnDataReceived.AddDynamic(this, &UDataStreamProcessor::OnUDPDataReceived);
        UE_LOG(LogRealGazeboBridge, Display, TEXT("DataStreamProcessor: Initialized"));
    }
    else
    {
        UE_LOG(LogRealGazeboBridge, Error, TEXT("DataStreamProcessor: Failed to create UDPReceiver"));
    }
}

void UDataStreamProcessor::Shutdown()
{
    if (UDPReceiver)
    {
        UDPReceiver->OnDataReceived.RemoveAll(this);
        StopDataStream();
        UDPReceiver = nullptr;
    }
    
    // Clear batch processing timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BatchProcessingTimer);
        World->GetTimerManager().ClearTimer(StatsUpdateTimer);
    }
    
    BridgeSubsystem.Reset();
    
    UE_LOG(LogRealGazeboBridge, Display, TEXT("DataStreamProcessor: Shutdown complete"));
}

bool UDataStreamProcessor::StartDataStream(int32 ListenPort, const FString& ServerIP)
{
    if (!UDPReceiver)
    {
        UE_LOG(LogRealGazeboBridge, Error, TEXT("DataStreamProcessor: UDPReceiver is null"));
        return false;
    }

    bool bSuccess = UDPReceiver->StartListening(ListenPort, ServerIP);
    if (bSuccess)
    {
        // Start batch processing timer if enabled
        if (bEnableBatchProcessing)
        {
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(
                    BatchProcessingTimer,
                    this,
                    &UDataStreamProcessor::ProcessPacketBatch,
                    BatchProcessingInterval,
                    true
                );
            }
        }
        
        // Start statistics update timer
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                StatsUpdateTimer,
                this,
                &UDataStreamProcessor::UpdatePerformanceStatistics,
                StatsUpdateInterval,
                true
            );
        }
    }
    
    UE_LOG(LogRealGazeboBridge, Warning, TEXT("DataStreamProcessor: Start receiver on %s:%d - %s"), 
           *ServerIP, ListenPort, bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
    return bSuccess;
}

void UDataStreamProcessor::StopDataStream()
{
    if (UDPReceiver)
    {
        UDPReceiver->StopListening();
    }
    
    // Clear timers
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BatchProcessingTimer);
        World->GetTimerManager().ClearTimer(StatsUpdateTimer);
    }
    
    // Process any remaining packets in batch
    if (PacketBatch.Num() > 0)
    {
        ProcessPacketBatch();
    }
    
    UE_LOG(LogRealGazeboBridge, Warning, TEXT("DataStreamProcessor: Receiver stopped"));
}

bool UDataStreamProcessor::IsStreamActive() const
{
    return UDPReceiver ? UDPReceiver->IsListening() : false;
}

void UDataStreamProcessor::OnUDPDataReceived(const FUDPData& ReceivedData)
{
    if (bEnableBatchProcessing)
    {
        // Add to batch for processing
        PacketBatch.Add(ReceivedData);
        
        // Process immediately if batch is full
        if (PacketBatch.Num() >= BatchSize)
        {
            ProcessPacketBatch();
        }
    }
    else
    {
        // Process immediately
        ProcessSinglePacket(ReceivedData);
    }
    
    PacketCountSinceLastUpdate++;
}

void UDataStreamProcessor::ProcessPacketBatch()
{
    if (PacketBatch.Num() == 0)
    {
        return;
    }
    
    const double StartTime = FPlatformTime::Seconds();
    
    for (const FUDPData& PacketData : PacketBatch)
    {
        ProcessSinglePacket(PacketData);
    }
    
    const double EndTime = FPlatformTime::Seconds();
    TotalProcessingTime += (EndTime - StartTime);
    ProcessedBatches++;
    
    PacketBatch.Empty();
}

void UDataStreamProcessor::ProcessSinglePacket(const FUDPData& PacketData)
{
    if (PacketData.Data.Num() < PACKET_HEADER_SIZE)
    {
        TotalInvalidPackets++;
        if (bLogPacketErrors)
        {
            LogPacketError(TEXT("Packet too small"), PacketData.Data);
        }
        return;
    }

    uint8 VehicleNum, VehicleType, MessageID;
    if (!ValidatePacketHeader(PacketData.Data, VehicleNum, VehicleType, MessageID))
    {
        TotalInvalidPackets++;
        return;
    }

    switch (MessageID)
    {
        case 1: // Pose data
        {
            FBridgePoseData PoseData;
            if (ParsePosePacket(PacketData.Data, PoseData))
            {
                TotalValidPosePackets++;
                HandlePoseData(PoseData);
            }
            else
            {
                TotalInvalidPackets++;
            }
            break;
        }
        case 2: // Motor speed data
        {
            FBridgeMotorSpeedData MotorData;
            if (ParseMotorSpeedPacket(PacketData.Data, MotorData))
            {
                TotalValidMotorPackets++;
                HandleMotorSpeedData(MotorData);
            }
            else
            {
                TotalInvalidPackets++;
            }
            break;
        }
        case 3: // Servo data
        {
            FBridgeServoData ServoData;
            if (ParseServoPacket(PacketData.Data, ServoData))
            {
                TotalValidServoPackets++;
                HandleServoData(ServoData);
            }
            else
            {
                TotalInvalidPackets++;
            }
            break;
        }
        default:
        {
            TotalInvalidPackets++;
            if (bLogPacketErrors)
            {
                UE_LOG(LogRealGazeboBridge, Warning, TEXT("Unknown message ID: %d"), MessageID);
            }
            break;
        }
    }
}

bool UDataStreamProcessor::ValidatePacketHeader(const TArray<uint8>& Data, uint8& OutVehicleNum, uint8& OutVehicleType, uint8& OutMessageID) const
{
    if (Data.Num() < PACKET_HEADER_SIZE)
    {
        return false;
    }
    
    OutVehicleNum = Data[0];
    OutVehicleType = Data[1];
    OutMessageID = Data[2];
    
    return true;
}

bool UDataStreamProcessor::ParsePosePacket(const TArray<uint8>& RawData, FBridgePoseData& OutPoseData)
{
    if (RawData.Num() != EXPECTED_POSE_PACKET_SIZE)
    {
        return false;
    }

    // Parse header
    OutPoseData.VehicleNum = RawData[0];
    OutPoseData.VehicleType = RawData[1];
    OutPoseData.MessageID = RawData[2];

    // Validate message ID
    if (OutPoseData.MessageID != 1)
    {
        return false;
    }

    // Parse position (bytes 3-14)
    float X = BytesToFloat(RawData, 3);
    float Y = BytesToFloat(RawData, 7);
    float Z = BytesToFloat(RawData, 11);
    OutPoseData.Position = ConvertGazeboPositionToUnreal(X, Y, Z);

    // Parse quaternion (bytes 15-30)
    float QuatX = BytesToFloat(RawData, 15);
    float QuatY = BytesToFloat(RawData, 19);
    float QuatZ = BytesToFloat(RawData, 23);
    float QuatW = BytesToFloat(RawData, 27);
    FQuat PoseQuat = ConvertGazeboQuaternionToUnreal(QuatX, QuatY, QuatZ, QuatW);
    OutPoseData.Rotation = PoseQuat.Rotator();

    return true;
}

bool UDataStreamProcessor::ParseMotorSpeedPacket(const TArray<uint8>& RawData, FBridgeMotorSpeedData& OutMotorData)
{
    if (RawData.Num() < PACKET_HEADER_SIZE)
    {
        return false;
    }

    // Parse header
    OutMotorData.VehicleNum = RawData[0];
    OutMotorData.VehicleType = RawData[1];
    OutMotorData.MessageID = RawData[2];

    // Validate message ID
    if (OutMotorData.MessageID != 2)
    {
        return false;
    }

    // Validate packet size
    int32 ExpectedSize = GetExpectedMotorSpeedPacketSize(OutMotorData.VehicleType);
    if (ExpectedSize == 0 || RawData.Num() != ExpectedSize)
    {
        return false;
    }

    // Calculate motor count
    int32 MotorCount = (RawData.Num() - PACKET_HEADER_SIZE) / 4; // 4 bytes per float
    OutMotorData.MotorSpeeds_DegPerSec.Empty();
    OutMotorData.MotorSpeeds_DegPerSec.Reserve(MotorCount);

    for (int32 i = 0; i < MotorCount; i++)
    {
        int32 StartIndex = PACKET_HEADER_SIZE + (i * 4);
        float RadiansPerSec = BytesToFloat(RawData, StartIndex);
        float DegreesPerSec = RadiansPerSec * (180.0f / PI);
        OutMotorData.MotorSpeeds_DegPerSec.Add(DegreesPerSec);
    }

    return true;
}

bool UDataStreamProcessor::ParseServoPacket(const TArray<uint8>& RawData, FBridgeServoData& OutServoData)
{
    if (RawData.Num() < PACKET_HEADER_SIZE)
    {
        return false;
    }

    // Parse header
    OutServoData.VehicleNum = RawData[0];
    OutServoData.VehicleType = RawData[1];
    OutServoData.MessageID = RawData[2];

    // Validate message ID
    if (OutServoData.MessageID != 3)
    {
        return false;
    }

    // Calculate servo count (28 bytes per servo: 3 floats position + 4 floats quaternion)
    int32 ServoCount = (RawData.Num() - PACKET_HEADER_SIZE) / 28;
    OutServoData.ServoPositions.Empty();
    OutServoData.ServoPositions.Reserve(ServoCount);
    OutServoData.ServoRotations.Empty();
    OutServoData.ServoRotations.Reserve(ServoCount);

    for (int32 i = 0; i < ServoCount; i++)
    {
        int32 StartIndex = PACKET_HEADER_SIZE + (i * 28);
        
        if (StartIndex + 27 >= RawData.Num())
        {
            return false;
        }

        // Parse position
        float X = BytesToFloat(RawData, StartIndex);
        float Y = BytesToFloat(RawData, StartIndex + 4);
        float Z = BytesToFloat(RawData, StartIndex + 8);
        OutServoData.ServoPositions.Add(ConvertGazeboPositionToUnreal(X, Y, Z));

        // Parse quaternion
        float QuatX = BytesToFloat(RawData, StartIndex + 12);
        float QuatY = BytesToFloat(RawData, StartIndex + 16);
        float QuatZ = BytesToFloat(RawData, StartIndex + 20);
        float QuatW = BytesToFloat(RawData, StartIndex + 24);
        FQuat ServoQuat = ConvertGazeboQuaternionToUnreal(QuatX, QuatY, QuatZ, QuatW);
        OutServoData.ServoRotations.Add(ServoQuat.Rotator());
    }

    return true;
}

float UDataStreamProcessor::BytesToFloat(const TArray<uint8>& Data, int32 StartIndex) const
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

FVector UDataStreamProcessor::ConvertGazeboPositionToUnreal(float X, float Y, float Z) const
{
    // Scale by 100 to convert from meters to centimeters
    // Negate Y (right-handed to left-handed coordinate system)
    return FVector(X * 100.0f, -Y * 100.0f, Z * 100.0f);
}

FRotator UDataStreamProcessor::ConvertGazeboRotationToUnreal(float Roll, float Pitch, float Yaw) const
{
    // Convert from radians to degrees
    // Negate pitch and yaw for coordinate system conversion
    return FRotator(-FMath::RadiansToDegrees(Pitch), 
                    -FMath::RadiansToDegrees(Yaw), 
                    FMath::RadiansToDegrees(Roll));
}

FQuat UDataStreamProcessor::ConvertGazeboQuaternionToUnreal(float X, float Y, float Z, float W) const
{
    // Simple Y-axis flip to convert from Gazebo's right-handed to Unreal's left-handed coordinate system
    return FQuat(X, -Y, Z, -W);
}

void UDataStreamProcessor::HandlePoseData(const FBridgePoseData& PoseData)
{
    // Forward to bridge subsystem for processing
    if (UGazeboBridgeSubsystem* Subsystem = BridgeSubsystem.Get())
    {
        Subsystem->UpdateVehicleData(PoseData);
    }
    
    // Broadcast for backward compatibility
    OnPoseDataReceived.Broadcast(PoseData);
}

void UDataStreamProcessor::HandleMotorSpeedData(const FBridgeMotorSpeedData& MotorData)
{
    // Forward to bridge subsystem for processing
    if (UGazeboBridgeSubsystem* Subsystem = BridgeSubsystem.Get())
    {
        Subsystem->UpdateVehicleMotorData(MotorData);
    }
    
    // Broadcast for backward compatibility
    OnMotorSpeedDataReceived.Broadcast(MotorData);
}

void UDataStreamProcessor::HandleServoData(const FBridgeServoData& ServoData)
{
    // Forward to bridge subsystem for processing
    if (UGazeboBridgeSubsystem* Subsystem = BridgeSubsystem.Get())
    {
        Subsystem->UpdateVehicleServoData(ServoData);
    }
    
    // Broadcast for backward compatibility
    OnServoDataReceived.Broadcast(ServoData);
}

int32 UDataStreamProcessor::GetExpectedMotorSpeedPacketSize(uint8 VehicleType) const
{
    if (UGazeboBridgeSubsystem* Subsystem = BridgeSubsystem.Get())
    {
        if (const FBridgeVehicleConfigRow* Config = Subsystem->GetVehicleConfigInternal(VehicleType))
        {
            return PACKET_HEADER_SIZE + (Config->MotorCount * 4); // 4 bytes per float
        }
    }
    return 0;
}

int32 UDataStreamProcessor::GetExpectedServoPacketSize(uint8 VehicleType) const
{
    if (UGazeboBridgeSubsystem* Subsystem = BridgeSubsystem.Get())
    {
        if (const FBridgeVehicleConfigRow* Config = Subsystem->GetVehicleConfigInternal(VehicleType))
        {
            return PACKET_HEADER_SIZE + (Config->ServoCount * 28); // 28 bytes per servo
        }
    }
    return 0;
}

void UDataStreamProcessor::GetNetworkStatistics(int32& OutValidPackets, int32& OutInvalidPackets, 
                                              float& OutPacketsPerSecond, float& OutAverageProcessingTime) const
{
    OutValidPackets = TotalValidPosePackets + TotalValidMotorPackets + TotalValidServoPackets;
    OutInvalidPackets = TotalInvalidPackets;
    OutPacketsPerSecond = PacketsPerSecond;
    OutAverageProcessingTime = ProcessedBatches > 0 ? (TotalProcessingTime / ProcessedBatches) : 0.0f;
}

void UDataStreamProcessor::ResetStatistics()
{
    TotalValidPosePackets = 0;
    TotalValidMotorPackets = 0;
    TotalValidServoPackets = 0;
    TotalInvalidPackets = 0;
    PacketsPerSecond = 0.0f;
    
    LastStatsUpdate = FPlatformTime::Seconds();
    PacketCountSinceLastUpdate = 0;
    TotalProcessingTime = 0.0f;
    ProcessedBatches = 0;
}

void UDataStreamProcessor::UpdatePerformanceStatistics() const
{
    const double CurrentTime = FPlatformTime::Seconds();
    const double TimeDelta = CurrentTime - LastStatsUpdate;
    
    if (TimeDelta > 0.0)
    {
        const_cast<UDataStreamProcessor*>(this)->PacketsPerSecond = PacketCountSinceLastUpdate / TimeDelta;
        const_cast<UDataStreamProcessor*>(this)->PacketCountSinceLastUpdate = 0;
        const_cast<UDataStreamProcessor*>(this)->LastStatsUpdate = CurrentTime;
    }
}

void UDataStreamProcessor::LogPacketError(const FString& ErrorMessage, const TArray<uint8>& PacketData) const
{
    FString DataHex;
    for (int32 i = 0; i < FMath::Min(PacketData.Num(), 16); i++) // Log first 16 bytes
    {
        DataHex += FString::Printf(TEXT("%02X "), PacketData[i]);
    }
    
    UE_LOG(LogRealGazeboBridge, Warning, TEXT("DataStreamProcessor Error: %s. Data: %s"), 
           *ErrorMessage, *DataHex);
}

void UDataStreamProcessor::LogPacketStatistics() const
{
    const int32 TotalValid = TotalValidPosePackets + TotalValidMotorPackets + TotalValidServoPackets;
    const int32 TotalProcessed = TotalValid + TotalInvalidPackets;
    const float SuccessRate = TotalProcessed > 0 ? (float(TotalValid) / TotalProcessed * 100.0f) : 0.0f;
    
    UE_LOG(LogRealGazeboBridge, Log, TEXT("DataStreamProcessor Stats: Valid=%d, Invalid=%d, Rate=%.1f%%, PPS=%.1f"), 
           TotalValid, TotalInvalidPackets, SuccessRate, PacketsPerSecond);
}