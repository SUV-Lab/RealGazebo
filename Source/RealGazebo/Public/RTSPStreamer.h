#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RHI.h"
#include "RHIResources.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "GameFramework/Actor.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

// GStreamer 헤더 파일들
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h>

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

// GLib의 TRUE/FALSE 매크로 정의
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include "RTSPStreamer.generated.h"

// Foward declaration
class UCameraComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

DECLARE_LOG_CATEGORY_EXTERN(LogRTSPStreamer, Log, All);

// 버퍼 푸시 모니터링을 위한 구조체
struct FBufferPushStats
{
    uint64 TotalBytes = 0;             // 전송된 총 바이트 수
    float FirstPushTime = 0.0f;        // 첫 번째 푸시 시간
    float LastSuccessTime = 0.0f;      // 마지막 성공적인 푸시 시간
    float LastReportTime = 0.0f;       // 마지막 보고 시간
    int32 PushesSinceLastReport = 0;   // 마지막 보고 이후 푸시 횟수
    uint64 BytesSinceLastReport = 0;   // 마지막 보고 이후 전송된 바이트
};

USTRUCT(BlueprintType)
struct FRTSPStreamSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSP")
    FString StreamPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSP")
    int32 Width;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSP")
    int32 Height;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSP")
    int32 Framerate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSP")
    int32 Bitrate;

    FRTSPStreamSettings()
        : StreamPath(TEXT("")), Width(1920), Height(1080), Framerate(30), Bitrate(2000000)
    {}
};

class FRTSPStreamData
{
public:
    GstElement* AppSrc;
    GstElement* Pipeline;
    UTextureRenderTarget2D* RenderTarget;
    TArray<uint8> FrameBuffer;
    FRTSPStreamSettings Settings;
    bool bIsStreaming;
    bool bInitialFrameSent; // 첫 프레임 전송 여부 플래그
    
    // 디버깅 및 상태 추적용 변수들
    int32 SuccessfulPushes;      // 성공적으로 푸시된 프레임 수
    int32 FailedPushes;          // 실패한 프레임 푸시 횟수
    int32 ConsecutiveFailures;   // 연속적인 실패 횟수 (심각한 오류 감지용)
    float LastStateChangeAttempt; // 마지막으로 상태 변경 시도한 시간
    GstClockTime LastPts;        // 마지막으로 설정한 타임스탬프

    // 버퍼 푸시 모니터링 추가
    FBufferPushStats PushStats;

    FRTSPStreamData()
        : AppSrc(nullptr), Pipeline(nullptr), RenderTarget(nullptr), bIsStreaming(false), bInitialFrameSent(false),
          SuccessfulPushes(0), FailedPushes(0), ConsecutiveFailures(0), LastStateChangeAttempt(0.0f), LastPts(0)
    {}
};

class FRTSPStreamerThread : public FRunnable
{
public:
    FRTSPStreamerThread();
    virtual ~FRTSPStreamerThread();

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

    void AddStream(const FString& StreamPath, const FRTSPStreamSettings& Settings);
    void RemoveStream(const FString& StreamPath);
    void UpdateStreamFrame(const FString& StreamPath, const TArray<uint8>& FrameData);
    void ActivateStream(const FString& StreamPath);
    bool IsServerRunning() const { return bServerRunning; }

    // ✅ 추가된 Getter
    FCriticalSection& GetStreamMapMutex() { return StreamMapMutex; }
    TMap<FString, TSharedPtr<FRTSPStreamData>>& GetStreamMap() { return StreamMap; }

private:
    FRunnableThread* Thread;
    bool bStopRequested;
    bool bServerRunning;

    GstRTSPServer* RTSPServer;
    GstRTSPMountPoints* MountPoints;
    GMainLoop* MainLoop;

    TMap<FString, TSharedPtr<FRTSPStreamData>> StreamMap;
    FCriticalSection StreamMapMutex;

    static void MediaConfigureCallback(GstRTSPMediaFactory* Factory, GstRTSPMedia* Media, gpointer UserData);
    static void NeedDataCallback(GstElement* AppSrc, guint Length, gpointer UserData);
    static void EnoughDataCallback(GstElement* AppSrc, gpointer UserData);

    void LoadGStreamerPlugin(const FString& PluginName, const FString& PluginsDir);
};

UCLASS()
class REALGAZEBO_API ARTSPCameraActor : public AActor
{
    GENERATED_BODY()

public:
    ARTSPCameraActor();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UCameraComponent* CameraComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneCaptureComponent2D* SceneCaptureComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSP Stream Settings")
    FRTSPStreamSettings StreamSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSP Stream Settings")
    bool bAutoStart = true;

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    void StartStreaming();

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    void StopStreaming();

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    FString GetStreamURL() const;

    UFUNCTION(BlueprintCallable, Category = "RTSP Streaming")
    bool IsStreaming() const { return bIsCurrentlyStreaming; }

protected:
    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;

private:
    void CaptureFrame();

    UPROPERTY()
    UTextureRenderTarget2D* RenderTarget;

    bool bIsCurrentlyStreaming;
    TArray<uint8> FrameBuffer;
    float LastCaptureTime;
    float CaptureInterval;
};
