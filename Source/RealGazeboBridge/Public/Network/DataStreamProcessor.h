
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UDPReceiver.h"
#include "GazeboBridgeTypes.h"
#include "DataStreamProcessor.generated.h"

// Forward declarations
class UGazeboBridgeSubsystem;

/**
 * Data stream processor for PX4-Gazebo UDP communication
 * 
 * Key Features:
 * - Batch processing for network data
 * - Protocol validation and error handling  
 * - Coordinate system conversion (Gazebo -> Unreal)
 * - Real-time statistics and monitoring
 */
UCLASS()
class REALGAZEBOBRIDGE_API UDataStreamProcessor : public UObject
{
    GENERATED_BODY()

public:
    UDataStreamProcessor();

    //----------------------------------------------------------
    // Lifecycle Management
    //----------------------------------------------------------

    /** Initialize the data stream processor */
    void Initialize(UGazeboBridgeSubsystem* InBridgeSubsystem);

    /** Shutdown and cleanup */
    void Shutdown();

    //----------------------------------------------------------
    // Network Control
    //----------------------------------------------------------

    /** Start UDP data reception */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Network")
    bool StartDataStream(int32 ListenPort = 5005, const FString& ServerIP = TEXT(""));

    /** Stop UDP data reception */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Network")
    void StopDataStream();

    /** Check if data stream is active */
    UFUNCTION(BlueprintCallable, Category = "Bridge|Network")
    bool IsStreamActive() const;

    //----------------------------------------------------------
    // Configuration
    //----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Configuration")
    bool bEnableBatchProcessing = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Configuration")
    int32 BatchSize = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Configuration")
    float BatchProcessingInterval = 0.016f; // ~60 FPS

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Configuration")
    bool bLogPacketErrors = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Configuration")
    bool bValidatePacketSizes = true;

    //----------------------------------------------------------
    // Statistics and Monitoring
    //----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Bridge|Statistics")
    void GetNetworkStatistics(int32& OutValidPackets, int32& OutInvalidPackets, 
                            float& OutPacketsPerSecond, float& OutAverageProcessingTime) const;

    UFUNCTION(BlueprintCallable, Category = "Bridge|Statistics")
    void ResetStatistics();

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Statistics")
    int32 TotalValidPosePackets = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Statistics")
    int32 TotalValidMotorPackets = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Statistics") 
    int32 TotalValidServoPackets = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Statistics")
    int32 TotalInvalidPackets = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Statistics")
    float PacketsPerSecond = 0.0f;

    //----------------------------------------------------------
    // Events (for backward compatibility and debugging)
    //----------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Bridge|Events")
    FOnVehicleDataReceived OnPoseDataReceived;

    UPROPERTY(BlueprintAssignable, Category = "Bridge|Events")
    FOnMotorSpeedDataReceived OnMotorSpeedDataReceived;

    UPROPERTY(BlueprintAssignable, Category = "Bridge|Events")
    FOnServoDataReceived OnServoDataReceived;

protected:
    //----------------------------------------------------------
    // Core Components
    //----------------------------------------------------------

    UPROPERTY()
    TObjectPtr<UUDPReceiver> UDPReceiver;

    /** Reference to the bridge subsystem */
    TWeakObjectPtr<UGazeboBridgeSubsystem> BridgeSubsystem;

    //----------------------------------------------------------
    // Batch Processing
    //----------------------------------------------------------

    /** Batch of received packets waiting for processing */
    TArray<FUDPData> PacketBatch;

    /** Batch processing timer handle */
    FTimerHandle BatchProcessingTimer;

    /** Process accumulated batch of packets */
    void ProcessPacketBatch();
    
    /** Process a single packet immediately */
    void ProcessSinglePacket(const FUDPData& PacketData);

    //----------------------------------------------------------
    // Packet Parsing (Protocol Constants)
    //----------------------------------------------------------

    static constexpr int32 PACKET_HEADER_SIZE = 3; // VehicleNum + VehicleType + MessageID
    static constexpr int32 POSE_PAYLOAD_SIZE = 28; // 7 floats (position XYZ + quaternion XYZW)
    static constexpr int32 EXPECTED_POSE_PACKET_SIZE = PACKET_HEADER_SIZE + POSE_PAYLOAD_SIZE;

    //----------------------------------------------------------
    // Data Parsing Methods
    //----------------------------------------------------------

    /** Parse individual packet types */
    bool ParsePosePacket(const TArray<uint8>& RawData, FBridgePoseData& OutPoseData);
    bool ParseMotorSpeedPacket(const TArray<uint8>& RawData, FBridgeMotorSpeedData& OutMotorData);
    bool ParseServoPacket(const TArray<uint8>& RawData, FBridgeServoData& OutServoData);

    /** Low-level data conversion */
    float BytesToFloat(const TArray<uint8>& Data, int32 StartIndex) const;
    bool ValidatePacketHeader(const TArray<uint8>& Data, uint8& OutVehicleNum, uint8& OutVehicleType, uint8& OutMessageID) const;

    //----------------------------------------------------------
    // Coordinate System Conversion
    //----------------------------------------------------------

    /** Convert Gazebo coordinates to Unreal coordinates */
    FVector ConvertGazeboPositionToUnreal(float X, float Y, float Z) const;
    FRotator ConvertGazeboRotationToUnreal(float Roll, float Pitch, float Yaw) const;
    FQuat ConvertGazeboQuaternionToUnreal(float X, float Y, float Z, float W) const;

    //----------------------------------------------------------
    // Packet Size Validation
    //----------------------------------------------------------

    /** Get expected packet sizes based on vehicle configuration */
    int32 GetExpectedMotorSpeedPacketSize(uint8 VehicleType) const;
    int32 GetExpectedServoPacketSize(uint8 VehicleType) const;

    //----------------------------------------------------------
    // Performance Monitoring
    //----------------------------------------------------------

    mutable double LastStatsUpdate = 0.0;
    mutable int32 PacketCountSinceLastUpdate = 0;
    mutable float TotalProcessingTime = 0.0f;
    mutable int32 ProcessedBatches = 0;

    void UpdatePerformanceStatistics() const;

    //----------------------------------------------------------
    // Event Handlers
    //----------------------------------------------------------

    /** Main UDP data reception handler */
    UFUNCTION()
    void OnUDPDataReceived(const FUDPData& ReceivedData);

    /** Handle individual parsed packets */
    void HandlePoseData(const FBridgePoseData& PoseData);
    void HandleMotorSpeedData(const FBridgeMotorSpeedData& MotorData);
    void HandleServoData(const FBridgeServoData& ServoData);

    //----------------------------------------------------------
    // Error Handling and Logging
    //----------------------------------------------------------

    void LogPacketError(const FString& ErrorMessage, const TArray<uint8>& PacketData) const;
    void LogPacketStatistics() const;

private:
    /** Timer for periodic statistics updates */
    FTimerHandle StatsUpdateTimer;
    static constexpr float StatsUpdateInterval = 1.0f; // Update every second
};