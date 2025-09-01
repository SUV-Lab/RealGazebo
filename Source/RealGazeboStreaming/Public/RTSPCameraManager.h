#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTSPCameraComponent.h"
#include "RTSPCameraManager.generated.h"

USTRUCT(BlueprintType)
struct REALGAZEBOSTREAMING_API FRTSPCameraConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
    FString CameraName = TEXT("Camera");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
    FString StreamPath = TEXT("/stream");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (ClampMin = 1024, ClampMax = 65535))
    int32 StreamPort = 8554;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
    FIntPoint Resolution = FIntPoint(1920, 1080);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (ClampMin = 1, ClampMax = 60))
    float FrameRate = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
    bool bAutoStart = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCameraStreamingStatusChanged, FString, CameraName, bool, bIsStreaming);

UCLASS(ClassGroup=(Streaming), meta=(BlueprintSpawnableComponent))
class REALGAZEBOSTREAMING_API URTSPCameraManager : public UActorComponent
{
    GENERATED_BODY()

public:
    URTSPCameraManager();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "RTSP Camera Manager")
    bool AddCamera(const FString& CameraName, URTSPCameraComponent* CameraComponent, const FRTSPCameraConfig& Config);

    UFUNCTION(BlueprintCallable, Category = "RTSP Camera Manager")
    bool RemoveCamera(const FString& CameraName);

    UFUNCTION(BlueprintCallable, Category = "RTSP Camera Manager")
    bool StartCameraStreaming(const FString& CameraName);

    UFUNCTION(BlueprintCallable, Category = "RTSP Camera Manager")
    void StopCameraStreaming(const FString& CameraName);

    UFUNCTION(BlueprintCallable, Category = "RTSP Camera Manager")
    void StartAllCameras();

    UFUNCTION(BlueprintCallable, Category = "RTSP Camera Manager")
    void StopAllCameras();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTSP Camera Manager")
    bool IsCameraStreaming(const FString& CameraName) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTSP Camera Manager")
    TArray<FString> GetCameraNames() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTSP Camera Manager")
    TArray<FString> GetStreamingCameras() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTSP Camera Manager")
    FString GetCameraStreamURL(const FString& CameraName) const;

    UFUNCTION(BlueprintCallable, Category = "RTSP Camera Manager")
    TMap<FString, FString> GetAllStreamURLs() const;

    UPROPERTY(BlueprintAssignable, Category = "RTSP Camera Manager")
    FOnCameraStreamingStatusChanged OnCameraStreamingStatusChanged;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Manager Settings")
    TArray<FRTSPCameraConfig> DefaultCameraConfigs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Manager Settings")
    bool bAutoDiscoverCameras = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Manager Settings")
    int32 BasePortNumber = 8554;

private:
    struct FCameraInfo
    {
        URTSPCameraComponent* Component;
        FRTSPCameraConfig Config;
        
        FCameraInfo() : Component(nullptr) {}
        FCameraInfo(URTSPCameraComponent* InComponent, const FRTSPCameraConfig& InConfig)
            : Component(InComponent), Config(InConfig) {}
    };

    UFUNCTION()
    void OnCameraStreamingStatusChangedInternal(bool bIsStreaming);

    void AutoDiscoverCameras();
    void SetupCameraFromConfig(URTSPCameraComponent* Camera, const FRTSPCameraConfig& Config);
    int32 GetNextAvailablePort() const;

    TMap<FString, FCameraInfo> ManagedCameras;
    TMap<URTSPCameraComponent*, FString> ComponentToCameraName;
};