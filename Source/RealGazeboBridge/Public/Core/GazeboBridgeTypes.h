
#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/DataTable.h"
#include "GazeboBridgeTypes.generated.h"

// Forward declarations
class AVehicleBasePawn;

//----------------------------------------------------------
// Vehicle Identification
//----------------------------------------------------------

USTRUCT(BlueprintType)
struct REALGAZEBOBRIDGE_API FVehicleID
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Bridge|Vehicle ID")
    uint8 VehicleNum = 0; // 0-255 unique vehicle IDs

    UPROPERTY(BlueprintReadWrite, Category = "Bridge|Vehicle ID")
    uint8 VehicleType = 0; // Vehicle type code (0-255) matching VehicleTypeCode in FBridgeVehicleConfigRow (e.g., 0=Iris, 1=Rover)

    FVehicleID() = default;
    FVehicleID(uint8 InVehicleNum, uint8 InVehicleType) 
        : VehicleNum(InVehicleNum), VehicleType(InVehicleType) {}

    FString ToString() const
    {
        return FString::Printf(TEXT("%d_%d"), VehicleType, VehicleNum);
    }

    bool operator==(const FVehicleID& Other) const
    {
        return VehicleNum == Other.VehicleNum && VehicleType == Other.VehicleType;
    }

    bool operator!=(const FVehicleID& Other) const
    {
        return !(*this == Other);
    }

    friend uint32 GetTypeHash(const FVehicleID& VehicleID)
    {
        return HashCombine(GetTypeHash(VehicleID.VehicleNum), GetTypeHash(VehicleID.VehicleType));
    }
};

//----------------------------------------------------------
// Network Data Structures (maintain compatibility)
//----------------------------------------------------------

USTRUCT(BlueprintType)
struct REALGAZEBOBRIDGE_API FBridgePoseData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Header")
    uint8 VehicleNum = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Header")
    uint8 VehicleType = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Header")
    uint8 MessageID = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Pose Data")
    FVector Position = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Pose Data")
    FRotator Rotation = FRotator::ZeroRotator;

    FVehicleID GetVehicleID() const { return FVehicleID(VehicleNum, VehicleType); }
};

USTRUCT(BlueprintType)
struct REALGAZEBOBRIDGE_API FBridgeMotorSpeedData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Header")
    uint8 VehicleNum = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Header")
    uint8 VehicleType = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Header")
    uint8 MessageID = 2;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Motor Speed Data")
    TArray<float> MotorSpeeds_DegPerSec;

    FVehicleID GetVehicleID() const { return FVehicleID(VehicleNum, VehicleType); }
};

USTRUCT(BlueprintType)
struct REALGAZEBOBRIDGE_API FBridgeServoData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Header")
    uint8 VehicleNum = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Header")
    uint8 VehicleType = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Header")
    uint8 MessageID = 3;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Servo Data")
    TArray<FVector> ServoPositions;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Servo Data")
    TArray<FRotator> ServoRotations;

    FVehicleID GetVehicleID() const { return FVehicleID(VehicleNum, VehicleType); }
};

//----------------------------------------------------------
// Optimized Runtime Data (for subsystem storage)
//----------------------------------------------------------

USTRUCT(BlueprintType)
struct REALGAZEBOBRIDGE_API FVehicleRuntimeData
{
    GENERATED_BODY()

    // Core transform data
    UPROPERTY(BlueprintReadWrite, Category = "Bridge|Runtime")
    FVector Position = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite, Category = "Bridge|Runtime")
    FQuat Rotation = FQuat::Identity;

    UPROPERTY(BlueprintReadWrite, Category = "Bridge|Runtime")
    TArray<float> MotorSpeeds;

    UPROPERTY(BlueprintReadWrite, Category = "Bridge|Runtime")
    TArray<FVector> ServoPositions;

    UPROPERTY(BlueprintReadWrite, Category = "Bridge|Runtime")
    TArray<FQuat> ServoRotations;

    // Performance tracking
    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Performance")
    float LastUpdateTime = 0.0f;


    // Visual representation (weak reference to avoid circular dependencies)
    TWeakObjectPtr<AVehicleBasePawn> VisualPawn;

    // Vehicle configuration reference
    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Configuration")
    uint8 VehicleType = 0;
};

//----------------------------------------------------------
// DataTable Configuration (maintains backward compatibility)
//----------------------------------------------------------

USTRUCT(BlueprintType, meta = (DataTable = "true"))
struct REALGAZEBOBRIDGE_API FBridgeVehicleConfigRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Config", meta = (DisplayName = "Vehicle Name"))
    FString VehicleName = TEXT("Unknown"); // Descriptive name for the vehicle type

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Config", meta = (DisplayName = "Vehicle Type Code"))
    uint8 VehicleTypeCode = 0; // Unique code to identify vehicle type (0-255) for network messages

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Config", meta = (DisplayName = "Motor Count"))
    int32 MotorCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Config", meta = (DisplayName = "Servo Count"))
    int32 ServoCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Config", meta = (DisplayName = "Vehicle Pawn Class"))
    TSubclassOf<AVehicleBasePawn> VehiclePawnClass;

    // Performance settings for packet calculation

    int32 GetMotorSpeedPacketSize() const
    {
        return 3 + (MotorCount * 4); // 3 header bytes + 4 bytes per motor
    }
    
    int32 GetServoPacketSize() const
    {
        // 3 header bytes + 28 bytes per servo (6 floats for XYZ + RPY * 4 bytes + padding)
        return 3 + (ServoCount * 28);
    }
};

//----------------------------------------------------------
// Legacy Type Aliases (for backward compatibility)
//----------------------------------------------------------

typedef FBridgePoseData FGazeboPoseData;
typedef FBridgeMotorSpeedData FGazeboMotorSpeedData;
typedef FBridgeServoData FGazeboServoData;
typedef FBridgeVehicleConfigRow FGazeboVehicleTableRow;

//----------------------------------------------------------
// Event Delegates
//----------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVehicleDataReceived, const FBridgePoseData&, VehicleData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMotorSpeedDataReceived, const FBridgeMotorSpeedData&, MotorSpeedData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServoDataReceived, const FBridgeServoData&, ServoData);

// Legacy delegate aliases
typedef FOnVehicleDataReceived FOnGazeboVehicleDataReceived;
typedef FOnMotorSpeedDataReceived FOnGazeboMotorSpeedDataReceived;
typedef FOnServoDataReceived FOnGazeboServoDataReceived;