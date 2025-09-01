#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RTSPCameraManager.h"
#include "RTSPStreamingSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGlobalStreamingStatusChanged, FString, CameraName, bool, bIsStreaming);

UCLASS(BlueprintType)
class REALGAZEBOSTREAMING_API URTSPStreamingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    static URTSPStreamingSubsystem* Get(const UObject* WorldContext);

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    bool RegisterCameraManager(URTSPCameraManager* CameraManager);

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    void UnregisterCameraManager(URTSPCameraManager* CameraManager);

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    TArray<FString> GetAllCameraNames() const;

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    TArray<FString> GetAllStreamingCameras() const;

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    bool StartCameraStreaming(const FString& CameraName);

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    void StopCameraStreaming(const FString& CameraName);

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    void StartAllCameras();

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    void StopAllCameras();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTSP Streaming")
    bool IsCameraStreaming(const FString& CameraName) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTSP Streaming")
    FString GetCameraStreamURL(const FString& CameraName) const;

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    TMap<FString, FString> GetAllStreamURLs() const;

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    void PrintStreamURLsToLog() const;

    UPROPERTY(BlueprintAssignable, Category = "RTSP Streaming")
    FOnGlobalStreamingStatusChanged OnGlobalStreamingStatusChanged;

private:
    UFUNCTION()
    void OnCameraManagerStreamingStatusChanged(FString CameraName, bool bIsStreaming);

    TArray<URTSPCameraManager*> RegisteredManagers;
};