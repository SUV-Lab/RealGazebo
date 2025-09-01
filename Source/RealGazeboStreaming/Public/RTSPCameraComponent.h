#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RTSPCameraComponent.generated.h"

// Forward declaration to avoid including GStreamer headers in public header
class FRTSPStreamer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRTSPStreamingStatusChanged, bool, bIsStreaming);

UCLASS(ClassGroup=(Streaming), meta=(BlueprintSpawnableComponent))
class REALGAZEBOSTREAMING_API URTSPCameraComponent : public USceneCaptureComponent2D
{
    GENERATED_BODY()

public:
    URTSPCameraComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    bool StartRTSPStreaming();

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    void StopRTSPStreaming();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTSP Streaming")
    bool IsStreaming() const;

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    FString GetStreamURL() const;

    UPROPERTY(BlueprintAssignable, Category = "RTSP Streaming")
    FOnRTSPStreamingStatusChanged OnStreamingStatusChanged;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSP Settings")
    FString StreamPath = TEXT("/stream");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSP Settings", meta = (ClampMin = 1024, ClampMax = 65535))
    int32 StreamPort = 8554;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSP Settings", meta = (ClampMin = 1, ClampMax = 60))
    float FrameRate = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSP Settings")
    bool bAutoStartStreaming = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSP Settings")
    FIntPoint StreamResolution = FIntPoint(1920, 1080);

protected:

private:
    void InitializeRenderTarget();
    void UpdateStreamingFrame();
    void OnStreamingStatusChangedInternal(bool bIsStreaming);

    UPROPERTY()
    UTextureRenderTarget2D* StreamRenderTarget;

    TSharedPtr<FRTSPStreamer> RTSPStreamer;
    
    float LastFrameTime;
    bool bIsInitialized;
};