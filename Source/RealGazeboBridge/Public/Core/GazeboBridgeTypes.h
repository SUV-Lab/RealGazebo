// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Sub-author: MinKyu Kim
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/DataTable.h"
#include "GazeboBridgeTypes.generated.h"

// Forward declarations
class AVehicleBasePawn;

//----------------------------------------------------------
// Vehicle Identification
//
// Protocol Design:
// - VehicleNum:  uint8 (0-255) -> 256 instances per vehicle type
// - VehicleType: uint8 (0-255) -> 256 different vehicle types
// - Theoretical Maximum: 256 types x 256 instances = 65,536 unique vehicles
//
// Practical Limits:
// - MaxActiveVehicles: 256(recommended for performance)
// - MaxActorsPerType:  256 (pool supports all instances per type)
//----------------------------------------------------------

USTRUCT(BlueprintType)
struct REALGAZEBOBRIDGE_API FVehicleID
{
    GENERATED_BODY()

    /** Vehicle instance number (0-255) - Identifies individual vehicle within a type */
    UPROPERTY(BlueprintReadWrite, Category = "Bridge|Vehicle ID",
              meta = (ToolTip = "Vehicle instance number (0-255) - Up to 256 vehicles per type"))
    uint8 VehicleNum = 0;

    /** Vehicle type code (0-255) - Maps to VehicleTypeCode in FBridgeVehicleConfigRow DataTable */
    UPROPERTY(BlueprintReadWrite, Category = "Bridge|Vehicle ID",
              meta = (ToolTip = "Vehicle type code (0-255) - Supports up to 256 different vehicle types (e.g., X500, Iris, Rover)"))
    uint8 VehicleType = 0;

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

template<>
struct TStructOpsTypeTraits<FVehicleID> : public TStructOpsTypeTraitsBase2<FVehicleID>
{
    enum
    {
        WithIdenticalViaEquality = true
    };
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
// Navigation State Enum (PX4 Navigation States)
//----------------------------------------------------------

UENUM(BlueprintType)
enum class ENavigationState : uint8
{
    MANUAL = 0              UMETA(DisplayName = "Manual"),
    ALTCTL = 1              UMETA(DisplayName = "Altitude Control"),
    POSCTL = 2              UMETA(DisplayName = "Position Control"),
    AUTO_MISSION = 3        UMETA(DisplayName = "Auto Mission"),
    AUTO_LOITER = 4         UMETA(DisplayName = "Auto Loiter"),
    AUTO_RTL = 5            UMETA(DisplayName = "Auto RTL"),
    POSITION_SLOW = 6       UMETA(DisplayName = "Position Slow"),
    FREE5 = 7               UMETA(DisplayName = "Free 5"),
    ALTITUDE_CRUISE = 8     UMETA(DisplayName = "Altitude Cruise"),
    FREE3 = 9               UMETA(DisplayName = "Free 3"),
    ACRO = 10               UMETA(DisplayName = "Acro"),
    FREE2 = 11              UMETA(DisplayName = "Free 2"),
    DESCEND = 12            UMETA(DisplayName = "Descend"),
    TERMINATION = 13        UMETA(DisplayName = "Termination"),
    OFFBOARD = 14           UMETA(DisplayName = "Offboard"),
    STAB = 15               UMETA(DisplayName = "Stabilized"),
    FREE1 = 16              UMETA(DisplayName = "Free 1"),
    AUTO_TAKEOFF = 17       UMETA(DisplayName = "Auto Takeoff"),
    AUTO_LAND = 18          UMETA(DisplayName = "Auto Land"),
    AUTO_FOLLOW_TARGET = 19 UMETA(DisplayName = "Auto Follow Target"),
    AUTO_PRECLAND = 20      UMETA(DisplayName = "Precision Land"),
    ORBIT = 21              UMETA(DisplayName = "Orbit"),
    AUTO_VTOL_TAKEOFF = 22  UMETA(DisplayName = "VTOL Takeoff"),
    EXTERNAL1 = 23          UMETA(DisplayName = "External 1"),
    EXTERNAL2 = 24          UMETA(DisplayName = "External 2"),
    EXTERNAL3 = 25          UMETA(DisplayName = "External 3"),
    EXTERNAL4 = 26          UMETA(DisplayName = "External 4"),
    EXTERNAL5 = 27          UMETA(DisplayName = "External 5"),
    EXTERNAL6 = 28          UMETA(DisplayName = "External 6"),
    EXTERNAL7 = 29          UMETA(DisplayName = "External 7"),
    EXTERNAL8 = 30          UMETA(DisplayName = "External 8"),
    MAX = 31                UMETA(DisplayName = "Max")
};

USTRUCT(BlueprintType)
struct REALGAZEBOBRIDGE_API FBridgeAdditionalData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Header")
    uint8 VehicleNum = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Header")
    uint8 VehicleType = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Header")
    uint8 MessageID = 5;

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Additional Data")
    float BatteryRemaining = 1.0f; // 0.0 ~ 1.0

    UPROPERTY(BlueprintReadOnly, Category = "Bridge|Additional Data")
    ENavigationState NavState = ENavigationState::MANUAL;

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

    // Battery and Navigation State
    UPROPERTY(BlueprintReadWrite, Category = "Bridge|Status")
    float BatteryRemaining = -1.0f; // -1.0 = no data, 0.0 ~ 1.0 = actual battery level

    UPROPERTY(BlueprintReadWrite, Category = "Bridge|Status")
    ENavigationState NavState = ENavigationState::MANUAL;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Config",
              meta = (DisplayName = "Show In UI",
                      ToolTip = "If false, this object will not appear in the UI vehicle list (useful for persons, props, etc.)"))
    bool bShowInUI = true; // Default true for backward compatibility with existing vehicles

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
// Helper Function: Convert NavigationState to Display String
//----------------------------------------------------------

inline FString NavigationStateToString(ENavigationState NavState)
{
    switch (NavState)
    {
        case ENavigationState::MANUAL: return TEXT("Manual");
        case ENavigationState::ALTCTL: return TEXT("Altitude Control");
        case ENavigationState::POSCTL: return TEXT("Position Control");
        case ENavigationState::AUTO_MISSION: return TEXT("Auto Mission");
        case ENavigationState::AUTO_LOITER: return TEXT("Auto Loiter");
        case ENavigationState::AUTO_RTL: return TEXT("Auto RTL");
        case ENavigationState::POSITION_SLOW: return TEXT("Position Slow");
        case ENavigationState::ALTITUDE_CRUISE: return TEXT("Altitude Cruise");
        case ENavigationState::ACRO: return TEXT("Acro");
        case ENavigationState::DESCEND: return TEXT("Descend");
        case ENavigationState::TERMINATION: return TEXT("Termination");
        case ENavigationState::OFFBOARD: return TEXT("Offboard");
        case ENavigationState::STAB: return TEXT("Stabilized");
        case ENavigationState::AUTO_TAKEOFF: return TEXT("Auto Takeoff");
        case ENavigationState::AUTO_LAND: return TEXT("Auto Land");
        case ENavigationState::AUTO_FOLLOW_TARGET: return TEXT("Auto Follow");
        case ENavigationState::AUTO_PRECLAND: return TEXT("Precision Land");
        case ENavigationState::ORBIT: return TEXT("Orbit");
        case ENavigationState::AUTO_VTOL_TAKEOFF: return TEXT("VTOL Takeoff");
        case ENavigationState::EXTERNAL1: return TEXT("External 1");
        case ENavigationState::EXTERNAL2: return TEXT("External 2");
        case ENavigationState::EXTERNAL3: return TEXT("External 3");
        case ENavigationState::EXTERNAL4: return TEXT("External 4");
        case ENavigationState::EXTERNAL5: return TEXT("External 5");
        case ENavigationState::EXTERNAL6: return TEXT("External 6");
        case ENavigationState::EXTERNAL7: return TEXT("External 7");
        case ENavigationState::EXTERNAL8: return TEXT("External 8");
        case ENavigationState::MAX: return TEXT("Max");
        default: return TEXT("Unknown");
    }
}

//----------------------------------------------------------
// Event Delegates
//----------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVehicleDataReceived, const FBridgePoseData&, VehicleData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMotorSpeedDataReceived, const FBridgeMotorSpeedData&, MotorSpeedData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServoDataReceived, const FBridgeServoData&, ServoData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAdditionalDataReceived, const FBridgeAdditionalData&, AdditionalData);

// Legacy delegate aliases
typedef FOnVehicleDataReceived FOnGazeboVehicleDataReceived;
typedef FOnMotorSpeedDataReceived FOnGazeboMotorSpeedDataReceived;
typedef FOnServoDataReceived FOnGazeboServoDataReceived;
