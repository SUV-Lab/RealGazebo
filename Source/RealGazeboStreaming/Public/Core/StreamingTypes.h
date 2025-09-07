#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GazeboBridgeTypes.h"
#include "StreamingTypes.generated.h"

// Forward declarations
class UVehicleCameraStreamComponent;
class FAdvancedRTSPStreamer;

//----------------------------------------------------------
// Camera Configuration Data
//----------------------------------------------------------

USTRUCT(BlueprintType)
struct REALGAZEBOSTREAMING_API FCameraStreamConfig
{
    GENERATED_BODY()

    /** Unique camera identifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
    FString CameraName = TEXT("Camera_01");

    /** RTSP stream path (e.g., /vehicle1/camera1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
    FString StreamPath = TEXT("/stream");

    /** RTSP server port */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (ClampMin = 1024, ClampMax = 65535))
    int32 StreamPort = 8554;

    /** Target streaming frame rate */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (ClampMin = 1.0f, ClampMax = 120.0f))
    float FrameRate = 30.0f;

    /** Stream resolution */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
    FIntPoint StreamResolution = FIntPoint(1920, 1080);

    /** Video codec (H.264, H.265) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
    FString VideoCodec = TEXT("H264");

    /** Bitrate in kbps */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (ClampMin = 100, ClampMax = 50000))
    int32 Bitrate = 5000;

    /** Auto-start streaming */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
    bool bAutoStart = false;

    FCameraStreamConfig()
    {
        CameraName = TEXT("Camera_01");
        StreamPath = TEXT("/stream");
        StreamPort = 8554;
        FrameRate = 30.0f;
        StreamResolution = FIntPoint(1920, 1080);
        VideoCodec = TEXT("H264");
        Bitrate = 5000;
        bAutoStart = false;
    }
};

//----------------------------------------------------------
// Vehicle Camera Configuration (DataTable Integration)
//----------------------------------------------------------

USTRUCT(BlueprintType, meta = (DataTable = "true"))
struct REALGAZEBOSTREAMING_API FVehicleCameraConfigRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Vehicle type this camera config applies to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Config")
    uint8 VehicleType = 0;

    /** Array of camera configurations for this vehicle type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Config")
    TArray<FCameraStreamConfig> CameraConfigs;

    /** Enable multi-camera streaming for this vehicle type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Config")
    bool bEnableStreaming = true;
};

//----------------------------------------------------------
// Runtime Streaming Data
//----------------------------------------------------------

USTRUCT(BlueprintType)
struct REALGAZEBOSTREAMING_API FStreamingRuntimeData
{
    GENERATED_BODY()

    /** Associated vehicle ID */
    UPROPERTY(BlueprintReadOnly, Category = "Runtime Data")
    FVehicleID VehicleID;

    /** Camera name */
    UPROPERTY(BlueprintReadOnly, Category = "Runtime Data")
    FString CameraName;

    /** Current streaming status */
    UPROPERTY(BlueprintReadOnly, Category = "Runtime Data")
    bool bIsStreaming = false;

    /** Stream URL (rtsp://ip:port/path) */
    UPROPERTY(BlueprintReadOnly, Category = "Runtime Data")
    FString StreamURL;

    /** Current FPS */
    UPROPERTY(BlueprintReadOnly, Category = "Runtime Data")
    float CurrentFPS = 0.0f;

    /** Performance metrics */
    UPROPERTY(BlueprintReadOnly, Category = "Runtime Data")
    float LastFrameTime = 0.0f;

    /** Weak reference to camera component */
    TWeakObjectPtr<UVehicleCameraStreamComponent> CameraComponent;

    /** Weak reference to streamer */
    TWeakPtr<FAdvancedRTSPStreamer> StreamerInstance;

    FStreamingRuntimeData()
    {
        VehicleID = FVehicleID();
        CameraName = TEXT("");
        bIsStreaming = false;
        StreamURL = TEXT("");
        CurrentFPS = 0.0f;
        LastFrameTime = 0.0f;
    }

    /** Generate unique camera ID for this stream */
    FString GetCameraID() const
    {
        return FString::Printf(TEXT("%s_%s"), *VehicleID.ToString(), *CameraName);
    }
};

//----------------------------------------------------------
// Event Delegates
//----------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStreamingStatusChanged, const FString&, CameraID, bool, bIsStreaming);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMultiCameraStreamingUpdate, const FVehicleID&, VehicleID, int32, ActiveStreams, int32, TotalCameras);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStreamingError, const FString&, CameraID, const FString&, ErrorMessage);

//----------------------------------------------------------
// Performance Tracking
//----------------------------------------------------------

USTRUCT(BlueprintType)
struct REALGAZEBOSTREAMING_API FStreamingPerformanceStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    int32 TotalActiveCameras = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    int32 TotalActiveStreams = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float AverageFrameRate = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float TotalMemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    int32 DroppedFrames = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float NetworkBandwidthMbps = 0.0f;
};

//----------------------------------------------------------
// Integration with RealGazebo Types
//----------------------------------------------------------

/** Stream ID combining vehicle and camera info */
USTRUCT(BlueprintType)
struct REALGAZEBOSTREAMING_API FStreamID
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Stream ID")
    FVehicleID VehicleID;

    UPROPERTY(BlueprintReadWrite, Category = "Stream ID")
    FString CameraName;

    FStreamID() = default;
    FStreamID(const FVehicleID& InVehicleID, const FString& InCameraName)
        : VehicleID(InVehicleID), CameraName(InCameraName) {}

    FString ToString() const
    {
        return FString::Printf(TEXT("%s_%s"), *VehicleID.ToString(), *CameraName);
    }

    bool operator==(const FStreamID& Other) const
    {
        return VehicleID == Other.VehicleID && CameraName == Other.CameraName;
    }

    friend uint32 GetTypeHash(const FStreamID& StreamID)
    {
        return HashCombine(GetTypeHash(StreamID.VehicleID), GetTypeHash(StreamID.CameraName));
    }
};