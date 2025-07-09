#include "RTSPStreamer.h"
#include "Modules/ModuleManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "RenderTargetPool.h"
#include "EngineUtils.h"
#include "HAL/PlatformFilemanager.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/ChildActorComponent.h"
#include "RealGazebo.h"

DEFINE_LOG_CATEGORY(LogRTSPStreamer);

// RTSP 요청 핸들러 함수 전방 선언
static void ClientConnectedCallback(GstRTSPServer* Server, GstRTSPClient* Client, gpointer UserData);
static gboolean HandlePlayRequest(GstRTSPClient* Client, GstRTSPContext* Ctx, gpointer UserData);
static void ExtendedMediaConfigureCallback(GstRTSPMediaFactory* Factory, GstRTSPMedia* Media, gpointer UserData);
static void NeedDataCallback(GstElement* AppSrc, guint Length, gpointer UserData);

// FRTSPStreamerThread Implementation
FRTSPStreamerThread::FRTSPStreamerThread()
    : Thread(nullptr)
    , bStopRequested(false)
    , bServerRunning(false)
    , RTSPServer(nullptr)
    , MountPoints(nullptr)
    , MainLoop(nullptr)
{
    // 즉시 스레드 생성 및 시작
    Thread = FRunnableThread::Create(this, TEXT("RTSPStreamerThread"), 0, TPri_Normal);
    
    if (Thread)
    {
        UE_LOG(LogRTSPStreamer, Display, TEXT("RTSPStreamer 스레드가 생성되었습니다."));
    }
    else
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("RTSPStreamer 스레드 생성 실패!"));
    }
}

FRTSPStreamerThread::~FRTSPStreamerThread()
{
    if (Thread)
    {
        Thread->Kill(true);
        delete Thread;
    }
}

bool FRTSPStreamerThread::Init()
{
    UE_LOG(LogRTSPStreamer, Display, TEXT("GStreamer 초기화 시작..."));
    
    // 플러그인 경로 설정
    FString PluginBaseDir = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("RealGazebo"), TEXT("Source"), TEXT("ThirdParty"), TEXT("GStreamer"), TEXT("Win64"));
    UE_LOG(LogRTSPStreamer, Display, TEXT("GStreamer 플러그인 기본 경로: %s"), *PluginBaseDir);
    
    // 필수 환경 변수 설정
    FString BinDir = FPaths::Combine(PluginBaseDir, TEXT("bin"));
    FString LibDir = FPaths::Combine(PluginBaseDir, TEXT("lib"));
    FString PluginsDir = FPaths::Combine(LibDir, TEXT("gstreamer-1.0"));
    
    // PATH 환경 변수 가져오기
    FString PathEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
    
    if (!PathEnv.Contains(BinDir))
    {
        FString NewPath = BinDir + TEXT(";") + PathEnv;
        FPlatformMisc::SetEnvironmentVar(TEXT("PATH"), *NewPath);
        UE_LOG(LogRTSPStreamer, Display, TEXT("PATH 환경 변수에 GStreamer bin 디렉토리 추가: %s"), *BinDir);
    }
    
    // GStreamer 플러그인 경로 설정
    FPlatformMisc::SetEnvironmentVar(TEXT("GST_PLUGIN_PATH"), *PluginsDir);
    UE_LOG(LogRTSPStreamer, Display, TEXT("GST_PLUGIN_PATH 설정: %s"), *PluginsDir);
    
    // 기타 GStreamer 환경 변수 설정
    FPlatformMisc::SetEnvironmentVar(TEXT("GST_PLUGIN_SYSTEM_PATH"), *PluginsDir);
    FPlatformMisc::SetEnvironmentVar(TEXT("GST_REGISTRY"), *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("gstreamer-registry.bin")));
    FPlatformMisc::SetEnvironmentVar(TEXT("GST_REGISTRY_UPDATE"), TEXT("no"));
    
    // GStreamer 디버깅 활성화
    // GST_DEBUG 환경 변수 설정
    FPlatformMisc::SetEnvironmentVar(TEXT("GST_DEBUG"), TEXT("*:5"));
    
    // 디버그 레벨 설정 (5=DEBUG, 4=INFO, 3=WARNING, 2=ERROR)
    gst_debug_set_default_threshold(GST_LEVEL_DEBUG);
    
    // RTSP 서버 관련 디버그 활성화
    gst_debug_set_threshold_for_name("rtspmedia", GST_LEVEL_DEBUG);
    gst_debug_set_threshold_for_name("rtspserver", GST_LEVEL_DEBUG);
    gst_debug_set_threshold_for_name("rtspclient", GST_LEVEL_DEBUG);
    gst_debug_set_threshold_for_name("rtsp-media-factory", GST_LEVEL_DEBUG);
    gst_debug_set_threshold_for_name("x264", GST_LEVEL_DEBUG);
    gst_debug_set_threshold_for_name("rtph264pay", GST_LEVEL_DEBUG);
    
    // 환경 변수 확인
    FString PathCheck = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
    UE_LOG(LogRTSPStreamer, Display, TEXT("현재 PATH 환경 변수: %s"), *PathCheck);
    
    FString PluginPathCheck = FPlatformMisc::GetEnvironmentVariable(TEXT("GST_PLUGIN_PATH"));
    UE_LOG(LogRTSPStreamer, Display, TEXT("현재 GST_PLUGIN_PATH: %s"), *PluginPathCheck);
    
    // 플러그인 디렉토리 존재 확인
    if (!FPaths::DirectoryExists(PluginsDir))
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("GStreamer 플러그인 디렉토리가 존재하지 않습니다: %s"), *PluginsDir);
        
        // 기본 디렉토리 구조 확인
        TArray<FString> Directories;
        IFileManager::Get().FindFiles(Directories, *PluginBaseDir, false, true);
        UE_LOG(LogRTSPStreamer, Display, TEXT("플러그인 기본 디렉토리 내용:"));
        for (const FString& Dir : Directories)
        {
            UE_LOG(LogRTSPStreamer, Display, TEXT(" - %s"), *Dir);
        }
    }
    else
    {
        UE_LOG(LogRTSPStreamer, Display, TEXT("GStreamer 플러그인 디렉토리 존재 확인됨: %s"), *PluginsDir);
        
        // 플러그인 파일 확인
        TArray<FString> PluginFiles;
        IFileManager::Get().FindFiles(PluginFiles, *(PluginsDir / TEXT("*.dll")), true, false);
        UE_LOG(LogRTSPStreamer, Display, TEXT("플러그인 디렉토리 내 DLL 파일 수: %d"), PluginFiles.Num());
        for (int32 i = 0; i < PluginFiles.Num(); i++)
        {
            UE_LOG(LogRTSPStreamer, Display, TEXT(" - %s"), *PluginFiles[i]);
        }
    }
    
    // GStreamer 버전 확인
    guint Major, Minor, Micro, Nano;
    gst_version(&Major, &Minor, &Micro, &Nano);
    UE_LOG(LogRTSPStreamer, Display, TEXT("GStreamer 버전: %d.%d.%d.%d"), Major, Minor, Micro, Nano);
    
    // 플러그인 경로 설정
    FString PluginDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("RTSPStreamer"), TEXT("Binaries"), TEXT("Win64")));
    FString GstPluginPath = FPaths::Combine(PluginDir, TEXT("gstreamer-1.0"));
    
    // 기존 GST_PLUGIN_PATH 환경 변수 가져오기
    FString ExistingPluginPath = FPlatformMisc::GetEnvironmentVariable(TEXT("GST_PLUGIN_PATH"));
    
    // 새 경로 추가
    if (!ExistingPluginPath.IsEmpty())
    {
        ExistingPluginPath += TEXT(";");
    }
    ExistingPluginPath += GstPluginPath;
    
    // 환경 변수 설정
    FPlatformMisc::SetEnvironmentVar(TEXT("GST_PLUGIN_PATH"), *ExistingPluginPath);
    UE_LOG(LogRTSPStreamer, Display, TEXT("GStreamer 플러그인 경로 설정: %s"), *ExistingPluginPath);
    
    // 시스템 GStreamer 설치 경로 확인
    FString SystemGstPath = FPlatformMisc::GetEnvironmentVariable(TEXT("GSTREAMER_1_0_ROOT_X86_64"));
    if (!SystemGstPath.IsEmpty())
    {
        FString SystemPluginPath = FPaths::Combine(SystemGstPath, TEXT("lib"), TEXT("gstreamer-1.0"));
        
        // 기존 경로에 추가
        if (!ExistingPluginPath.IsEmpty())
        {
            ExistingPluginPath += TEXT(";");
        }
        ExistingPluginPath += SystemPluginPath;
        
        // 업데이트된 환경 변수 설정
        FPlatformMisc::SetEnvironmentVar(TEXT("GST_PLUGIN_PATH"), *ExistingPluginPath);
        UE_LOG(LogRTSPStreamer, Display, TEXT("시스템 GStreamer 플러그인 경로 추가: %s"), *SystemPluginPath);
    }
    
    // GStreamer 디버깅 활성화
    FPlatformMisc::SetEnvironmentVar(TEXT("GST_DEBUG"), TEXT("*:5"));
    
    // GStreamer 초기화
    GStreamerGError* GstError = nullptr;
    gboolean InitResult = gst_init_check(nullptr, nullptr, &GstError);
    
    if (!InitResult || GstError) {
        if (GstError) {
            UE_LOG(LogRTSPStreamer, Error, TEXT("GStreamer 초기화 실패: %s"), UTF8_TO_TCHAR(GstError->message));
            g_error_free(GstError);
        } else {
            UE_LOG(LogRTSPStreamer, Error, TEXT("GStreamer 초기화 실패: 알 수 없는 오류"));
        }
        return false;
    }
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("GStreamer 초기화 성공"));
    
    // 시스템 검색 경로 확인 (직접 확인)
    UE_LOG(LogRTSPStreamer, Display, TEXT("GStreamer 플러그인 검색 경로 확인 중..."));
    
    // 대신 환경 변수로 설정된 경로 확인
    FString GstPluginPathVar = FPlatformMisc::GetEnvironmentVariable(TEXT("GST_PLUGIN_PATH"));
    UE_LOG(LogRTSPStreamer, Display, TEXT("플러그인 검색 경로 (GST_PLUGIN_PATH): %s"), *GstPluginPathVar);
    
    FString GstPluginSystemPath = FPlatformMisc::GetEnvironmentVariable(TEXT("GST_PLUGIN_SYSTEM_PATH"));
    UE_LOG(LogRTSPStreamer, Display, TEXT("플러그인 시스템 경로 (GST_PLUGIN_SYSTEM_PATH): %s"), *GstPluginSystemPath);
    
    // 플러그인 디렉토리 존재 확인
    if (FPaths::DirectoryExists(PluginsDir))
    {
        UE_LOG(LogRTSPStreamer, Display, TEXT("플러그인 디렉토리 존재: %s"), *PluginsDir);
    }
    else
    {
        UE_LOG(LogRTSPStreamer, Warning, TEXT("플러그인 디렉토리가 존재하지 않습니다: %s"), *PluginsDir);
    }
    
    // 플러그인 스캔 시도
    UE_LOG(LogRTSPStreamer, Display, TEXT("GStreamer 플러그인 스캔 시도..."));
    gst_registry_scan_path(gst_registry_get(), TCHAR_TO_UTF8(*PluginsDir));
    UE_LOG(LogRTSPStreamer, Display, TEXT("GStreamer 플러그인 스캔 완료"));
    
    // 로드된 플러그인 수 확인
    UE_LOG(LogRTSPStreamer, Display, TEXT("로드된 GStreamer 플러그인 확인 중..."));
    GList* PluginsList = gst_registry_get_plugin_list(gst_registry_get());
    int PluginCount = g_list_length(PluginsList);
    UE_LOG(LogRTSPStreamer, Display, TEXT("로드된 플러그인 수: %d"), PluginCount);
    
    if (PluginsList)
    {
        int MaxPluginsToShow = PluginCount;
        UE_LOG(LogRTSPStreamer, Display, TEXT("처음 %d개 플러그인 목록:"), MaxPluginsToShow);
        for (int i = 0; i < MaxPluginsToShow; i++)
        {
            GstPlugin* Plugin = GST_PLUGIN(g_list_nth_data(PluginsList, i));
            if (Plugin)
            {
                const gchar* PluginName = gst_plugin_get_name(Plugin);
                UE_LOG(LogRTSPStreamer, Display, TEXT(" - %s"), UTF8_TO_TCHAR(PluginName));
            }
        }
        gst_plugin_list_free(PluginsList);
    }
    
    // 수동으로 필수 플러그인 로드 시도
    UE_LOG(LogRTSPStreamer, Display, TEXT("필수 플러그인 수동 로드 시도..."));
    
    // 플러그인 파일 확인
    FString AppPluginPath = PluginsDir / TEXT("gstapp-1.0-0.dll");
    if (FPaths::FileExists(AppPluginPath))
    {
        UE_LOG(LogRTSPStreamer, Display, TEXT("app 플러그인 파일 존재: %s"), *AppPluginPath);
        
        GStreamerGError* GstPluginError = nullptr;
        GstPlugin* AppSrcPlugin = gst_plugin_load_file(TCHAR_TO_UTF8(*AppPluginPath), &GstPluginError);
        if (AppSrcPlugin)
        {
            UE_LOG(LogRTSPStreamer, Display, TEXT("app 플러그인 로드 성공"));
            gst_object_unref(AppSrcPlugin);
        }
        else
        {
            UE_LOG(LogRTSPStreamer, Error, TEXT("app 플러그인 로드 실패: %s"), 
                  GstPluginError ? UTF8_TO_TCHAR(GstPluginError->message) : TEXT("알 수 없는 오류"));
            if (GstPluginError) g_error_free(GstPluginError);
        }
    }
    else
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("app 플러그인 파일이 존재하지 않음: %s"), *AppPluginPath);
        
        // 디렉토리 내 모든 DLL 파일 확인
        TArray<FString> FoundFiles;
        IFileManager::Get().FindFiles(FoundFiles, *(PluginsDir / TEXT("*.dll")), true, false);
        UE_LOG(LogRTSPStreamer, Display, TEXT("플러그인 디렉토리 내 DLL 파일 목록:"));
        for (const FString& File : FoundFiles)
        {
            UE_LOG(LogRTSPStreamer, Display, TEXT(" - %s"), *File);
        }
    }
    
    // 테스트 파이프라인 생성
    // UE_LOG(LogRTSPStreamer, Display, TEXT("테스트 파이프라인 생성 시도..."));
    // GstElement* TestPipeline = gst_pipeline_new("test-pipeline");
    
    // if (TestPipeline) {
    //     UE_LOG(LogRTSPStreamer, Display, TEXT("테스트 파이프라인 생성 성공"));
        
    //     // 필수 요소 테스트
    //     GstElement* TestSrc = gst_element_factory_make("videotestsrc", "test-source");
    //     if (TestSrc) {
    //         UE_LOG(LogRTSPStreamer, Display, TEXT("videotestsrc 요소 생성 성공"));
    //         gst_object_unref(TestSrc);
    //     } else {
    //         UE_LOG(LogRTSPStreamer, Warning, TEXT("videotestsrc 요소 생성 실패"));
    //     }
        
    //     GstElement* TestAppSrc = gst_element_factory_make("appsrc", "test-appsrc");
    //     if (TestAppSrc) {
    //         UE_LOG(LogRTSPStreamer, Display, TEXT("appsrc 요소 생성 성공"));
    //         gst_object_unref(TestAppSrc);
    //     } else {
    //         UE_LOG(LogRTSPStreamer, Warning, TEXT("appsrc 요소 생성 실패"));
    //     }
        
    //     gst_object_unref(TestPipeline);
    // } else {
    //     UE_LOG(LogRTSPStreamer, Warning, TEXT("테스트 파이프라인 생성 실패"));
    // }
    
    // RTSP 서버 생성
    RTSPServer = gst_rtsp_server_new();
    if (!RTSPServer)
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("RTSP 서버를 생성할 수 없습니다"));
        return false;
    }
    
    // 서버 설정
    g_object_set(RTSPServer, "service", "8554", NULL);
    
    // RTSP 버전을 명시적으로 1.0으로 설정
    g_object_set(RTSPServer, "version", GST_RTSP_VERSION_1_0, NULL);
    
    // 서버 인증 완전히 비활성화
    gst_rtsp_server_set_auth(RTSPServer, NULL);
    
    // 추가 서버 파라미터 설정
    g_object_set(RTSPServer, "backlog", 20, NULL);  // 연결 대기열 증가
    g_object_set(RTSPServer, "bind-address", "0.0.0.0", NULL);  // 모든 인터페이스 바인딩
    
    // 마운트 포인트 가져오기
    MountPoints = gst_rtsp_server_get_mount_points(RTSPServer);
    if (!MountPoints)
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("마운트 포인트를 가져올 수 없습니다"));
        gst_object_unref(RTSPServer);
        RTSPServer = nullptr;
        return false;
    }
    
    // 모든 네트워크 인터페이스에 바인딩
    gst_rtsp_server_set_address(RTSPServer, "0.0.0.0");
    
    // 서버 주소 및 포트 확인 (디버깅용)
    gchar* Service = NULL;
    g_object_get(RTSPServer, "service", &Service, NULL);
    gchar* Address = NULL;
    g_object_get(RTSPServer, "address", &Address, NULL);
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("RTSP 서버 바인드 주소: %s"), Address ? UTF8_TO_TCHAR(Address) : TEXT("NULL"));
    UE_LOG(LogRTSPStreamer, Display, TEXT("RTSP 서버 포트: %s"), Service ? UTF8_TO_TCHAR(Service) : TEXT("NULL"));
    
    if (Service) g_free(Service);
    if (Address) g_free(Address);
    
    // 추가 서버 설정
    // 세션 풀 설정
    GstRTSPSessionPool* SessionPool = gst_rtsp_server_get_session_pool(RTSPServer);
    g_object_set(SessionPool, "max-sessions", 100, NULL);  // 최대 세션 증가
    g_object_set(SessionPool, "cleanup-interval", 5, NULL);  // 5초마다 정리
    g_object_set(SessionPool, "timeout", 300, NULL);  // 5분 타임아웃
    gst_object_unref(SessionPool);
    
    // 클라이언트 연결 시 디버그 로그 추가
    g_signal_connect(RTSPServer, "client-connected", G_CALLBACK(ClientConnectedCallback), this);
    
    // 메인 루프 생성
    MainLoop = g_main_loop_new(nullptr, FALSE);
    
    // 서버 포트 확인 (8554가 사용 중인지 확인)
    gchar* ServiceCheck = NULL;
    g_object_get(RTSPServer, "service", &ServiceCheck, NULL);
    
    if (ServiceCheck) {
        // 소켓 체크 로직
        UE_LOG(LogRTSPStreamer, Display, TEXT("RTSP 서버 포트 확인: %s"), UTF8_TO_TCHAR(ServiceCheck));
        
        // 무료 포트 확인 (현재 8554)
        GSocket* TestSocket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, NULL);
        if (TestSocket) {
            GInetAddress* LocalAddr = g_inet_address_new_any(G_SOCKET_FAMILY_IPV4);
            GSocketAddress* SocketAddr = g_inet_socket_address_new(LocalAddr, atoi(ServiceCheck));
            
            gboolean CanBind = g_socket_bind(TestSocket, SocketAddr, FALSE, NULL);
            if (!CanBind) {
                UE_LOG(LogRTSPStreamer, Warning, TEXT("포트 %s가 이미 사용 중입니다. 다른 포트를 사용하세요."), UTF8_TO_TCHAR(ServiceCheck));
            } else {
                UE_LOG(LogRTSPStreamer, Display, TEXT("포트 %s 사용 가능"), UTF8_TO_TCHAR(ServiceCheck));
                g_socket_close(TestSocket, NULL);
            }
            
            g_object_unref(SocketAddr);
            g_object_unref(LocalAddr);
            g_object_unref(TestSocket);
        }
        
        g_free(ServiceCheck);
    }
    
    // 서버 시작
    guint ServerID = gst_rtsp_server_attach(RTSPServer, nullptr);
    if (ServerID == 0)
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("RTSP 서버를 네트워크에 연결할 수 없습니다"));
        return false;
    }
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("RTSP 서버가 성공적으로 시작되었습니다 (ID: %u)"), ServerID);
    
    // 서버 정보 출력
    FString ServerAddress;
    
    // 로컬 IP 주소 가져오기
    bool bCanBindAll;
    TSharedPtr<FInternetAddr> LocalAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);
    if (LocalAddr.IsValid())
    {
        ServerAddress = LocalAddr->ToString(false); // 포트 없이 IP만 출력
    }
    else
    {
        ServerAddress = TEXT("localhost");
    }
    
    UE_LOG(LogRTSPStreamer, Log, TEXT("============ RTSP 서버 초기화됨 ============"));
    UE_LOG(LogRTSPStreamer, Log, TEXT("서버 주소: 0.0.0.0:8554"));
    UE_LOG(LogRTSPStreamer, Log, TEXT("로컬 접속: rtsp://localhost:8554/"));
    UE_LOG(LogRTSPStreamer, Log, TEXT("로컬 접속: rtsp://127.0.0.1:8554/"));
    UE_LOG(LogRTSPStreamer, Log, TEXT("네트워크 접속: rtsp://%s:8554/"), *ServerAddress);
    UE_LOG(LogRTSPStreamer, Log, TEXT("========================================"));
    
    return true;
}

uint32 FRTSPStreamerThread::Run()
{
    bServerRunning = true;
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("============ RTSP 스트리밍 스레드 시작 ============"));
    
    if (MainLoop) {
        UE_LOG(LogRTSPStreamer, Display, TEXT("GMainLoop 이벤트 처리 시작 - RTSP 서버가 요청 수신 준비됨"));
        
        // 메인 루프 실행 (GLib 이벤트 처리 최적화)
        while (!bStopRequested && MainLoop)
        {
            g_main_context_iteration(g_main_loop_get_context(MainLoop), FALSE);
            FPlatformProcess::Sleep(0.001f); // 1ms sleep
        }
        
        UE_LOG(LogRTSPStreamer, Display, TEXT("GMainLoop 이벤트 처리 종료"));
    } else {
        UE_LOG(LogRTSPStreamer, Error, TEXT("GMainLoop가 NULL입니다 - 서버가 정상 작동하지 않을 수 있음"));
    }
    
    bServerRunning = false;
    UE_LOG(LogRTSPStreamer, Display, TEXT("RTSP 스트리밍 스레드가 종료되었습니다."));
    return 0;
}

void FRTSPStreamerThread::Stop()
{
    bStopRequested = true;
    
    if (MainLoop)
    {
        g_main_loop_quit(MainLoop);
    }
}

void FRTSPStreamerThread::Exit()
{
    // 모든 스트림 정리
    {
        FScopeLock Lock(&StreamMapMutex);
        for (auto& StreamPair : StreamMap)
        {
            if (StreamPair.Value->AppSrc)
            {
                gst_element_set_state(StreamPair.Value->AppSrc, GST_STATE_NULL);
                gst_object_unref(StreamPair.Value->AppSrc);
            }
            
            if (StreamPair.Value->Pipeline)
            {
                gst_element_set_state(StreamPair.Value->Pipeline, GST_STATE_NULL);
                gst_object_unref(StreamPair.Value->Pipeline);
            }
        }
        StreamMap.Empty();
    }
    
    // RTSP 서버 정리
    if (MountPoints)
    {
        gst_object_unref(MountPoints);
        MountPoints = nullptr;
    }
    
    if (RTSPServer)
    {
        gst_object_unref(RTSPServer);
        RTSPServer = nullptr;
    }
    
    if (MainLoop)
    {
        g_main_loop_unref(MainLoop);
        MainLoop = nullptr;
    }
}

void FRTSPStreamerThread::AddStream(const FString& StreamPath, const FRTSPStreamSettings& Settings)
{
    // 동기화 확보
    FScopeLock Lock(&StreamMapMutex);
    UE_LOG(LogRTSPStreamer, Display, TEXT("[%s] AddStream in thread called."), *StreamPath);

    // 마운트 포인트 정규화 (앞에 '/'가 있는지 확인)
    FString MountPath = StreamPath;
    if (!MountPath.StartsWith("/"))
    {
        MountPath = "/" + MountPath;
    }

    // 이미 해당 마운트 포인트가 존재하는지 확인
    if (StreamMap.Contains(StreamPath))
    {
        UE_LOG(LogRTSPStreamer, Warning, TEXT("[%s] Stream path already exists."), *StreamPath);
        return;
    }
    UE_LOG(LogRTSPStreamer, Display, TEXT("[%s] Stream path does not exist, proceeding to add."), *StreamPath);

    // 새로운 스트림 데이터 생성
    TSharedPtr<FRTSPStreamData> StreamData = MakeShared<FRTSPStreamData>();
    StreamData->Settings = Settings;
    
    // 미디어 팩토리 생성
    GstRTSPMediaFactory* Factory = gst_rtsp_media_factory_new();

    // 수동으로 필수 요소 확인 (플러그인 로드 확인용)
    UE_LOG(LogRTSPStreamer, Display, TEXT("필수 요소 가용성 다시 확인..."));
    
    
    // 다른 H.264 인코더 옵션 확인
    GstElementFactory* OpenH264EncFactory = gst_element_factory_find("openh264enc");
    bool bHasOpenH264 = (OpenH264EncFactory != nullptr);
    UE_LOG(LogRTSPStreamer, Display, TEXT("openh264enc 팩토리: %s"), bHasOpenH264 ? TEXT("찾음") : TEXT("없음"));
    if (OpenH264EncFactory) gst_object_unref(OpenH264EncFactory);
    
    // 파이프라인 문자열 최적화 - 호환성 개선
    FString PipelineStr;
    
    // 사용 가능한 H.264 인코더 확인
    bool bHasH264Encoder = false;
    const char* H264EncoderElements[] = {"openh264enc"};
    FString H264EncoderName;
    
    for (const char* EncoderName : H264EncoderElements)
    {
        GstElementFactory* EncFactory = gst_element_factory_find(EncoderName);
        if (EncFactory)
        {
            H264EncoderName = UTF8_TO_TCHAR(EncoderName);
            bHasH264Encoder = true;
            gst_object_unref(EncFactory);
            UE_LOG(LogRTSPStreamer, Display, TEXT("H.264 인코더 발견: %s"), *H264EncoderName);
            break;
        }
    }
    
    FString EncoderOptions = FString::Printf(TEXT("rate-control=bitrate complexity=low bitrate=%d gop-size=%d usage-type=camera"),
                                                Settings.Bitrate, Settings.Framerate);
    
    // H.264 파이프라인 생성 - BGR 입력 명시
    PipelineStr = FString::Printf(TEXT("( appsrc name=source is-live=true format=3 do-timestamp=true ! "
                                        "video/x-raw,format=BGR,width=%d,height=%d,framerate=%d/1 ! "
                                        "queue max-size-buffers=2 leaky=downstream ! "
                                        "videoconvert ! video/x-raw,format=I420 ! "
                                        "%s %s ! "
                                        "h264parse config-interval=-1 ! "
                                        "rtph264pay name=pay0 pt=96 )"),
                                    Settings.Width, Settings.Height, Settings.Framerate,
                                    *H264EncoderName, *EncoderOptions);
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("H.264 인코더 사용 중: %s"), *H264EncoderName);
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("선택된 RTSP 파이프라인: %s"), *PipelineStr);
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("파이프라인 설정 중 - Factory: %p"), Factory);
    gst_rtsp_media_factory_set_launch(Factory, TCHAR_TO_UTF8(*PipelineStr));
    UE_LOG(LogRTSPStreamer, Display, TEXT("파이프라인 설정 완료"));
    
    // 추가 RTSP 설정
    gst_rtsp_media_factory_set_shared(Factory, TRUE);
    gst_rtsp_media_factory_set_latency(Factory, 0);
    
    // 프로토콜 설정 최적화 - TCP 우선
    gst_rtsp_media_factory_set_transport_mode(Factory, GST_RTSP_TRANSPORT_MODE_PLAY);
    gst_rtsp_media_factory_set_protocols(Factory, GST_RTSP_LOWER_TRANS_TCP);
    
    // RTSP 서버가 SDP 정보를 자동으로 생성하도록 설정
    g_object_set(Factory, "enable-rtcp", FALSE, NULL);  // RTCP 비활성화하여 단순화
    
    // 인증 완전히 비활성화
    g_object_set(Factory, "auth", NULL, NULL);
    
    // 미디어 팩토리 준비와 구성에 대한 콜백 설정
    g_signal_connect(Factory, "media-configure", G_CALLBACK(ExtendedMediaConfigureCallback), StreamData.Get());

    // 미디어 팩토리를 마운트 포인트에 등록
    gst_rtsp_mount_points_add_factory(MountPoints, TCHAR_TO_UTF8(*MountPath), Factory);
    
    // 스트림 맵에 추가
    StreamMap.Add(StreamPath, StreamData);
    UE_LOG(LogRTSPStreamer, Display, TEXT("[%s] Added to StreamMap. Map size is now %d."), *StreamPath, StreamMap.Num());

    // 전체 URL 로깅 (로컬 및 네트워크 접근용)
    UE_LOG(LogRTSPStreamer, Display, TEXT("====== RTSP 스트림 추가됨 ======"));
    UE_LOG(LogRTSPStreamer, Display, TEXT("Stream path: %s"), *StreamPath);
    UE_LOG(LogRTSPStreamer, Display, TEXT("Mount path: %s"), *MountPath);
    UE_LOG(LogRTSPStreamer, Display, TEXT("로컬 접속 URL: rtsp://localhost:8554%s"), *MountPath);
    
    // 정확한 테스트 URL 표시
    UE_LOG(LogRTSPStreamer, Display, TEXT("VLC 테스트 명령어: vlc --rtsp-tcp rtsp://localhost:8554%s"), *MountPath);
    UE_LOG(LogRTSPStreamer, Display, TEXT("FFplay 테스트 명령어: ffplay -rtsp_transport tcp rtsp://localhost:8554%s"), *MountPath);
    
    // IP 주소로 접근 가능한 URL 표시
    bool bCanBindAll;
    TSharedPtr<FInternetAddr> LocalAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);
    if (LocalAddr.IsValid())
    {
        FString IPAddress = LocalAddr->ToString(false);  // 포트 제외
        UE_LOG(LogRTSPStreamer, Display, TEXT("Access from other computers: rtsp://%s:8554%s"), *IPAddress, *MountPath);
    }
}

void FRTSPStreamerThread::RemoveStream(const FString& StreamPath)
{
    FScopeLock Lock(&StreamMapMutex);
    
    if (!StreamMap.Contains(StreamPath))
    {
        return;
    }
    
    // 마운트 포인트 정규화
    FString MountPath = StreamPath;
    if (!MountPath.StartsWith("/"))
    {
        MountPath = "/" + MountPath;
    }
    
    // 스트림 데이터 가져오기
    TSharedPtr<FRTSPStreamData> StreamData = StreamMap[StreamPath];
    if (StreamData.IsValid())
    {
        // AppSrc 정리
        if (StreamData->AppSrc)
        {
            UE_LOG(LogRTSPStreamer, Display, TEXT("AppSrc 상태를 NULL로 변경 중"));
            gst_element_set_state(StreamData->AppSrc, GST_STATE_NULL);
            gst_object_unref(StreamData->AppSrc);
            StreamData->AppSrc = nullptr;
        }
        
        // 파이프라인 정리
        if (StreamData->Pipeline)
        {
            UE_LOG(LogRTSPStreamer, Display, TEXT("파이프라인 상태를 NULL로 변경 중"));
            gst_element_set_state(StreamData->Pipeline, GST_STATE_NULL);
            gst_object_unref(StreamData->Pipeline);
            StreamData->Pipeline = nullptr;
        }
    }
    
    // 미디어 팩토리 제거
    gst_rtsp_mount_points_remove_factory(MountPoints, TCHAR_TO_UTF8(*MountPath));
    
    // 스트림 데이터 제거
    StreamMap.Remove(StreamPath);
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("Stream removed: %s"), *StreamPath);
}

void FRTSPStreamerThread::ActivateStream(const FString& StreamPath)
{
    FScopeLock Lock(&StreamMapMutex);
    if (StreamMap.Contains(StreamPath))
    {
        TSharedPtr<FRTSPStreamData> StreamData = StreamMap[StreamPath];
        if (StreamData.IsValid())
        {
            UE_LOG(LogRTSPStreamer, Display, TEXT("스트리밍 활성화: %s"), *StreamPath);
            StreamData->bIsStreaming = true;
        }
    }
    else
    {
        UE_LOG(LogRTSPStreamer, Warning, TEXT("ActivateStream: 스트림 경로 '%s'를 찾을 수 없음"), *StreamPath);
    }
}

void FRTSPStreamerThread::UpdateStreamFrame(const FString& StreamPath, const TArray<uint8>& FrameData)
{
    if (!bServerRunning)
    {
        UE_LOG(LogRTSPStreamer, Warning, TEXT("UpdateStreamFrame: 서버가 실행되지 않았습니다"));
        return;
    }
    
    if (FrameData.Num() == 0)
    {
        UE_LOG(LogRTSPStreamer, Warning, TEXT("UpdateStreamFrame: 빈 프레임 데이터"));
        return;
    }
    
    FScopeLock Lock(&StreamMapMutex);
    
    if (!StreamMap.Contains(StreamPath))
    {
        UE_LOG(LogRTSPStreamer, Warning, TEXT("UpdateStreamFrame: 등록되지 않은 스트림 %s"), *StreamPath);
        return;
    }
    
    TSharedPtr<FRTSPStreamData> StreamData = StreamMap[StreamPath];
    if (!StreamData.IsValid() || !StreamData->AppSrc || !StreamData->bIsStreaming)
    {
        return;
    }

    // AppSrc 상태 확인 - 주기적으로 체크
    bool bShouldCheckState = (StreamData->SuccessfulPushes < 10) || 
                            (StreamData->SuccessfulPushes % 50 == 0) || 
                            (StreamData->FailedPushes > 0 && StreamData->FailedPushes % 5 == 0);
    
    if (bShouldCheckState)
    {
        GstState CurrentState, PendingState;
        gst_element_get_state(StreamData->AppSrc, &CurrentState, &PendingState, 0);
        
        if (CurrentState != GST_STATE_PLAYING) 
        {
            float CurrentTime = FPlatformTime::Seconds();
            if (!StreamData->LastStateChangeAttempt || (CurrentTime - StreamData->LastStateChangeAttempt) > 2.0f)
            {
                UE_LOG(LogRTSPStreamer, Display, TEXT("[%s] AppSrc 상태가 PLAYING이 아님 (현재: %d), 상태 변경 시도"), 
                       *StreamPath, (int)CurrentState);
                
                gst_element_set_state(StreamData->AppSrc, GST_STATE_PLAYING);
                
                if (StreamData->Pipeline) {
                    gst_element_set_state(StreamData->Pipeline, GST_STATE_PLAYING);
                }
                
                StreamData->LastStateChangeAttempt = CurrentTime;
            }
        }
    }
    
    try
    {
        GstBuffer* Buffer = gst_buffer_new_allocate(nullptr, FrameData.Num(), nullptr);
        if (!Buffer)
        {
            UE_LOG(LogRTSPStreamer, Error, TEXT("UpdateStreamFrame: 버퍼 할당 실패"));
            return;
        }
        
        GstMapInfo MapInfo;
        if (!gst_buffer_map(Buffer, &MapInfo, GST_MAP_WRITE))
        {
            UE_LOG(LogRTSPStreamer, Error, TEXT("UpdateStreamFrame: 버퍼 매핑 실패"));
            gst_buffer_unref(Buffer);
            return;
        }
        
        FMemory::Memcpy(MapInfo.data, FrameData.GetData(), FrameData.Num());
        gst_buffer_unmap(Buffer, &MapInfo);
        
        // 타임스탬프는 appsrc의 do-timestamp=true 설정에 맡깁니다.

        GstFlowReturn Ret = gst_app_src_push_buffer(GST_APP_SRC(StreamData->AppSrc), Buffer);
        
        if (Ret == GST_FLOW_OK)
        {
            StreamData->SuccessfulPushes++;
            StreamData->ConsecutiveFailures = 0;
            
            if (StreamData->SuccessfulPushes <= 5 || StreamData->SuccessfulPushes % 30 == 0)
            {
                UE_LOG(LogRTSPStreamer, Display, TEXT("[%s] 성공적으로 버퍼를 푸시했습니다 (#%d, %d 바이트)"), 
                       *StreamPath, StreamData->SuccessfulPushes, FrameData.Num());
            }
        }
        else
        {
            StreamData->FailedPushes++;
            StreamData->ConsecutiveFailures++;
            
            if (StreamData->FailedPushes <= 10 || StreamData->FailedPushes % 5 == 0)
            {
                UE_LOG(LogRTSPStreamer, Warning, TEXT("[%s] 버퍼 푸시 실패 (code: %d)"), *StreamPath, Ret);
            }
        }
    }
    catch (...)
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("UpdateStreamFrame: 예외 발생"));
    }
}

// RTSP 미디어 구성 콜백 구현
static void ExtendedMediaConfigureCallback(GstRTSPMediaFactory* Factory, GstRTSPMedia* Media, gpointer UserData)
{
    // 미디어가 유효한지 확인
    if (!Media) {
        UE_LOG(LogRTSPStreamer, Error, TEXT("미디어가 NULL입니다"));
        return;
    }
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("RTSP 미디어 구성 중"));
    
    // 가장 기본적인 미디어 설정만 적용
    g_object_set(Media, "latency", 0, NULL);
    g_object_set(Media, "enable-rtcp", FALSE, NULL);
    
    // 파이프라인 요소 가져오기
    GstElement* Element = gst_rtsp_media_get_element(Media);
    if (!Element) {
        UE_LOG(LogRTSPStreamer, Error, TEXT("미디어 요소를 가져올 수 없습니다"));
        return;
    }
    
    // pay0 요소 설정
    GstElement* Pay = gst_bin_get_by_name_recurse_up(GST_BIN(Element), "pay0");
    if (Pay) {
        g_object_set(Pay, "pt", 96, NULL);
        g_object_set(Pay, "config-interval", 1, NULL);
        UE_LOG(LogRTSPStreamer, Display, TEXT("pay0 요소 설정 완료"));
        g_object_unref(Pay);
    } else {
        UE_LOG(LogRTSPStreamer, Warning, TEXT("pay0 요소를 찾을 수 없습니다"));
    }
    
    // 사용자 데이터 확인
    FRTSPStreamData* StreamData = static_cast<FRTSPStreamData*>(UserData);
    if (!StreamData) {
        UE_LOG(LogRTSPStreamer, Error, TEXT("스트림 데이터가 없습니다"));
        g_object_unref(Element);
        return;
    }
    
    // source 요소 찾기
    StreamData->AppSrc = gst_bin_get_by_name_recurse_up(GST_BIN(Element), "source");
    if (StreamData->AppSrc) {
        UE_LOG(LogRTSPStreamer, Display, TEXT("AppSrc 요소를 찾았습니다"));
        
        // AppSrc 설정 - BGR 형식 사용
        GstCaps* Caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "BGR",
            "width", G_TYPE_INT, StreamData->Settings.Width,
            "height", G_TYPE_INT, StreamData->Settings.Height,
            "framerate", GST_TYPE_FRACTION, StreamData->Settings.Framerate, 1,
            nullptr);
        
        gst_app_src_set_caps(GST_APP_SRC(StreamData->AppSrc), Caps);
        gst_caps_unref(Caps);
        
        // AppSrc 상세 설정 - 버퍼 제어 매개변수 추가
        g_object_set(StreamData->AppSrc,
            "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
            "format", GST_FORMAT_TIME,
            "is-live", TRUE,
            "do-timestamp", TRUE,
            "min-latency", 0,
            "max-latency", 100000000, // 100ms 최대 지연
            "max-bytes", 0, // 무제한 버퍼 (필요시 조절)
            "block", FALSE, // 블로킹 비활성화
            "emit-signals", TRUE, // 시그널 활성화
            nullptr);
        
        // 시그널 연결
        g_signal_connect(StreamData->AppSrc, "need-data", G_CALLBACK(NeedDataCallback), StreamData);
        
        UE_LOG(LogRTSPStreamer, Display, TEXT("AppSrc 설정 완료 - 스트림 경로: %s"), *StreamData->Settings.StreamPath);
    } else {
        UE_LOG(LogRTSPStreamer, Error, TEXT("AppSrc 요소를 찾을 수 없습니다"));
    }
    
    // 파이프라인 참조 저장
    StreamData->Pipeline = Element;
    gst_object_ref(Element);
    
    // 파이프라인을 PLAYING 상태로 설정 (개선된 오류 처리)
    UE_LOG(LogRTSPStreamer, Display, TEXT("파이프라인을 PLAYING 상태로 전환 시도 중..."));
    
    // 현재 상태 확인
    GstState CurrentState, PendingState;
    GstStateChangeReturn CurrentStateRet = gst_element_get_state(Element, &CurrentState, &PendingState, 0);
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("현재 파이프라인 상태: %d, 대기 상태: %d, 반환 값: %d"), 
           (int)CurrentState, (int)PendingState, (int)CurrentStateRet);
    
    // 상태 변경 시도
    GstStateChangeReturn Ret = gst_element_set_state(Element, GST_STATE_PLAYING);
    
    switch (Ret) {
        case GST_STATE_CHANGE_SUCCESS:
            UE_LOG(LogRTSPStreamer, Display, TEXT("파이프라인이 즉시 PLAYING 상태로 변경되었습니다"));
            break;
            
        case GST_STATE_CHANGE_ASYNC:
            UE_LOG(LogRTSPStreamer, Display, TEXT("파이프라인 상태 변경이 비동기적으로 진행 중입니다"));
            
            // 상태 변경 완료까지 약간 대기 (최대 100ms)
            Ret = gst_element_get_state(Element, &CurrentState, &PendingState, 100 * GST_MSECOND);
            
            if (Ret == GST_STATE_CHANGE_SUCCESS) {
                UE_LOG(LogRTSPStreamer, Display, TEXT("파이프라인이 성공적으로 상태 %d로 변경되었습니다"), (int)CurrentState);
            } else {
                UE_LOG(LogRTSPStreamer, Warning, TEXT("파이프라인이 여전히 상태 변경 중입니다 (현재: %d, 대기: %d)"),
                      (int)CurrentState, (int)PendingState);
            }
            break;
            
        case GST_STATE_CHANGE_FAILURE:
            UE_LOG(LogRTSPStreamer, Error, TEXT("파이프라인 상태를 PLAYING으로 변경하지 못했습니다"));
            break;
            
        case GST_STATE_CHANGE_NO_PREROLL:
            UE_LOG(LogRTSPStreamer, Display, TEXT("파이프라인이 LIVE 상태입니다 - 정상입니다"));
            break;
            
        default:
            UE_LOG(LogRTSPStreamer, Warning, TEXT("알 수 없는 상태 변경 반환 값: %d"), (int)Ret);
            break;
    }
    
    g_object_unref(Element);
    UE_LOG(LogRTSPStreamer, Display, TEXT("RTSP 미디어 구성 완료 - 스트림 경로: %s"), *StreamData->Settings.StreamPath);
}

// RTSP 요청 핸들러 함수 구현
static void ClientConnectedCallback(GstRTSPServer* Server, GstRTSPClient* Client, gpointer UserData)
{
    const gchar* ClientIP = "알 수 없음";
    GstRTSPConnection* Conn = gst_rtsp_client_get_connection(Client);
    if (Conn)
    {
        ClientIP = gst_rtsp_connection_get_ip(Conn);
    }
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("RTSP 클라이언트 연결됨: %s"), UTF8_TO_TCHAR(ClientIP));
    
    // 클라이언트 타임아웃 증가
    g_object_set(Client, "session-timeout", 300, NULL);  // 5분 타임아웃
    
    // 연결 유지 옵션 활성화
    g_object_set(Client, "tcp-keepalive", TRUE, NULL);
    
    // PLAY 요청 시 스트림 활성화를 위한 핸들러 연결
    g_signal_connect(Client, "play-request", G_CALLBACK(HandlePlayRequest), UserData);
}

static gboolean HandlePlayRequest(GstRTSPClient* Client, GstRTSPContext* Ctx, gpointer UserData)
{
    FString Uri;
    if (Ctx && Ctx->uri) {
        gchar* UriStr = g_strdup(gst_rtsp_url_get_request_uri(Ctx->uri));
        Uri = UriStr ? UTF8_TO_TCHAR(UriStr) : TEXT("NULL");
        g_free(UriStr);
    } else {
        Uri = TEXT("미상");
    }
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("RTSP PLAY 요청 받음: %s"), *Uri);

    FRTSPStreamerThread* StreamerThread = static_cast<FRTSPStreamerThread*>(UserData);
    if (StreamerThread && Ctx && Ctx->uri && Ctx->uri->abspath)
    {
        FString MountPath = UTF8_TO_TCHAR(Ctx->uri->abspath);
        // StreamMap의 키는 맨 앞의 '/'가 없습니다.
        if (MountPath.StartsWith(TEXT("/")))
        {
            MountPath.RightChopInline(1);
        }
        StreamerThread->ActivateStream(MountPath);
    }

    UE_LOG(LogRTSPStreamer, Display, TEXT("스트리밍 시작됨: %s"), *Uri);
    
    return FALSE; // 기본 처리 계속
}

// 추가 핸들러 함수
static gboolean HandleTeardownRequest(GstRTSPClient* Client, GstRTSPContext* Ctx, gpointer UserData)
{
    FString Uri;
    if (Ctx && Ctx->uri) {
        gchar* UriStr = g_strdup(gst_rtsp_url_get_request_uri(Ctx->uri));
        Uri = UriStr ? UTF8_TO_TCHAR(UriStr) : TEXT("NULL");
        g_free(UriStr);
    } else {
        Uri = TEXT("미상");
    }
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("RTSP TEARDOWN 요청 받음: %s"), *Uri);
    return FALSE; // 기본 처리 계속
}

// ARTSPCameraActor Implementation
ARTSPCameraActor::ARTSPCameraActor()
    : bIsCurrentlyStreaming(false)
    , LastCaptureTime(0.0f)
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Camera Component 생성
    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    RootComponent = CameraComponent;
    
    // Scene Capture Component 생성 및 부착
    SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
    SceneCaptureComponent->SetupAttachment(CameraComponent);
    
    // 씬 캡처 설정 - 경고 방지
    SceneCaptureComponent->bCaptureEveryFrame = false;
    SceneCaptureComponent->bCaptureOnMovement = false;
    SceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
    
    // 기본 설정 - StreamPath는 사용하는 곳에서 명시적으로 설정하도록 제거
    StreamSettings.Width = 1920;
    StreamSettings.Height = 1080;
    StreamSettings.Framerate = 30;
    StreamSettings.Bitrate = 4000000; // 4Mbps
}

void ARTSPCameraActor::BeginPlay()
{
    Super::BeginPlay();

    // Render Target 생성
    RenderTarget = NewObject<UTextureRenderTarget2D>();
    RenderTarget->InitAutoFormat(StreamSettings.Width, StreamSettings.Height);
    RenderTarget->UpdateResourceImmediate(true);
    
    // RenderTarget 설정 검증
    UE_LOG(LogRTSPStreamer, Display, TEXT("RenderTarget 생성: %d x %d, 리소스 상태: %s"), 
           RenderTarget->SizeX, RenderTarget->SizeY, 
           RenderTarget->Resource ? TEXT("있음") : TEXT("없음"));
    
    // SceneCaptureComponent 설정
    SceneCaptureComponent->TextureTarget = RenderTarget;
    SceneCaptureComponent->CaptureSource = SCS_FinalColorLDR;
    
    // 캡처 설정 최적화
    SceneCaptureComponent->bCaptureEveryFrame = false;
    SceneCaptureComponent->bCaptureOnMovement = false;
    
    // 캡처 품질 설정
    SceneCaptureComponent->ShowFlags.SetMotionBlur(false); // 모션 블러 제거로 성능 향상
    SceneCaptureComponent->ShowFlags.SetAntiAliasing(true); // 안티앨리어싱 유지
    
    // 추가 품질 설정
    SceneCaptureComponent->ShowFlags.SetPostProcessing(true); // 포스트 프로세싱 활성화
    SceneCaptureComponent->ShowFlags.SetHMDDistortion(false); // VR 왜곡 비활성화
    
    CaptureInterval = 1.0f / StreamSettings.Framerate;
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("SceneCapture 설정 완료 - 프레임 간격: %f초 (목표 %d FPS)"), 
           CaptureInterval, StreamSettings.Framerate);
    
    // 자동 시작 지원
    if (bAutoStart)
    {
        StartStreaming();
    }
}

void ARTSPCameraActor::BeginDestroy()
{
    StopStreaming();
    Super::BeginDestroy();
}

void ARTSPCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (CameraComponent)
    {
        SceneCaptureComponent->FOVAngle = CameraComponent->FieldOfView;
    }

    if (!bIsCurrentlyStreaming || !RenderTarget || !SceneCaptureComponent)
        return;

    // 모듈 및 스트리머 스레드 유효성 검사
    if (!FModuleManager::Get().IsModuleLoaded("RealGazebo"))
        return;

    // AppSrc가 준비되었는지 확인
    bool bReadyToStream = false;
    
    try
    {
        FRealGazeboModule& Module = FRealGazeboModule::Get();

        if (!Module.IsServerRunning() || !Module.GetStreamerThread().IsValid())
            return;

        const TUniquePtr<FRTSPStreamerThread>& ThreadPtr = Module.GetStreamerThread();
        if (!ThreadPtr.IsValid() || !ThreadPtr->IsServerRunning())
            return;

        FScopeLock Lock(&ThreadPtr->GetStreamMapMutex());
        auto& Map = ThreadPtr->GetStreamMap();
        if (Map.Contains(StreamSettings.StreamPath))
        {
            const TSharedPtr<FRTSPStreamData>& Data = Map[StreamSettings.StreamPath];
            if (Data.IsValid())
            {
                // AppSrc가 설정되었고, PLAY 요청을 받아 bIsStreaming이 true일 때만 캡처 준비
                bReadyToStream = (Data->AppSrc != nullptr && Data->bIsStreaming);
            }
        }

        // AppSrc가 준비된 경우에만 프레임 캡처
        if (bReadyToStream)
        {
            LastCaptureTime += DeltaTime;
            
            // 프레임 캡처 간격이 경과했는지 확인
            if (LastCaptureTime >= CaptureInterval)
            {
                // 디버깅용 로그 (1초에 한 번만 출력)
                static float LastLogTime = 0.0f;
                float CurrentTime = FPlatformTime::Seconds();
                
                if (CurrentTime - LastLogTime > 1.0f)
                {
                    UE_LOG(LogRTSPStreamer, Display, TEXT("프레임 캡처 시도 중 - 간격: %f, 경과 시간: %f"), 
                           CaptureInterval, LastCaptureTime);
                    LastLogTime = CurrentTime;
                }
                
                // 프레임 캡처 실행
                CaptureFrame();
                LastCaptureTime = 0.0f;
            }
        }
        else
        {
            // 준비되지 않은 경우 로그 출력
            static float LastNotReadyLogTime = 0.0f;
            float CurrentTime = FPlatformTime::Seconds();
            if (CurrentTime - LastNotReadyLogTime > 3.0f) // 3초마다 로그 출력
            {
                UE_LOG(LogRTSPStreamer, Warning, TEXT("스트리밍 준비되지 않음 - Path: %s, 스트리밍 상태: %s"), 
                       *StreamSettings.StreamPath, 
                       Map.Contains(StreamSettings.StreamPath) ? TEXT("등록됨") : TEXT("미등록"));
                
                if (Map.Contains(StreamSettings.StreamPath))
                {
                    const TSharedPtr<FRTSPStreamData>& Data = Map[StreamSettings.StreamPath];
                    UE_LOG(LogRTSPStreamer, Warning, TEXT("AppSrc: %s, 스트리밍 상태: %s"), 
                           Data->AppSrc ? TEXT("있음") : TEXT("없음"),
                           Data->bIsStreaming ? TEXT("스트리밍 중") : TEXT("스트리밍 안 함"));
                }
                
                LastNotReadyLogTime = CurrentTime;
            }
        }
    }
    catch (...)
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("Tick: 예외 발생"));
    }
}

void ARTSPCameraActor::StartStreaming()
{
    if (bIsCurrentlyStreaming)
    {
        UE_LOG(LogRTSPStreamer, Display, TEXT("[%s] Already streaming."), *StreamSettings.StreamPath);
        return;
    }

    if (StreamSettings.StreamPath.IsEmpty()){
        FString Path;
        FString ActorNamePart;

        // If this actor was created by a Child Actor Component, its name in the editor is the component's name.
        if (UChildActorComponent* ChildActorComp = Cast<UChildActorComponent>(GetParentComponent()))
        {
            ActorNamePart = ChildActorComp->GetName();
        }
        else
        {
            ActorNamePart = GetName();
        }

        if (GetParentActor())
        {
            FString ParentName = GetParentActor()->GetName();
            int32 StartIndex = ParentName.Find(TEXT("_C_UAID"));
            if (StartIndex != INDEX_NONE)
            {
                ParentName = ParentName.Left(StartIndex);
            }
            Path = ParentName + TEXT("_") + ActorNamePart;
        }
        else
        {
            Path = ActorNamePart;
        }

        StreamSettings.StreamPath = Path;
        UE_LOG(LogRTSPStreamer, Display, TEXT("StreamPath was empty, automatically set to: %s"), *StreamSettings.StreamPath);
    }
    

    
    UE_LOG(LogRTSPStreamer, Display, TEXT("[%s] Starting RTSP streaming."), *StreamSettings.StreamPath);
    
    // 모듈이 유효한지 확인
    auto& RTSPModule = FRealGazeboModule::Get();
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("[%s] Calling RegisterStream."), *StreamSettings.StreamPath);
    // 스트림 등록
    RTSPModule.RegisterStream(StreamSettings.StreamPath, StreamSettings);
    
    // 렌더 타겟 설정 확인
    if (!RenderTarget)
    {
        UE_LOG(LogRTSPStreamer, Warning, TEXT("유효한 RenderTarget이 없습니다. 테스트 패턴을 사용합니다."));
        
        // RenderTarget 생성 시도
        RenderTarget = NewObject<UTextureRenderTarget2D>();
        RenderTarget->InitAutoFormat(StreamSettings.Width, StreamSettings.Height);
        RenderTarget->UpdateResourceImmediate(true);
        
        SceneCaptureComponent->TextureTarget = RenderTarget;
    }
    
    // 스트리밍 시작
    bIsCurrentlyStreaming = true;
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("[%s] Stream is now active. URL: rtsp://localhost:8554/%s"), *StreamSettings.StreamPath, *StreamSettings.StreamPath);
}

void ARTSPCameraActor::StopStreaming()
{
    if (!bIsCurrentlyStreaming)
    {
        UE_LOG(LogRTSPStreamer, Display, TEXT("StopStreaming: 스트리밍 중이 아닙니다"));
        return;
    }
    
    UE_LOG(LogRTSPStreamer, Display, TEXT("스트리밍 중지 중: %s"), *StreamSettings.StreamPath);
    
    // 모듈 상태 확인
    if (!FModuleManager::Get().IsModuleLoaded("RealGazebo"))
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("StopStreaming: RTSPStreamer 모듈이 로드되지 않았습니다"));
        bIsCurrentlyStreaming = false;
        return;
    }
    
    FRealGazeboModule& Module = FRealGazeboModule::Get();
    Module.UnregisterStream(StreamSettings.StreamPath);
    
    bIsCurrentlyStreaming = false;
    UE_LOG(LogRTSPStreamer, Log, TEXT("스트리밍 중지됨: %s"), *StreamSettings.StreamPath);
}

FString ARTSPCameraActor::GetStreamURL() const
{
    FString Path = StreamSettings.StreamPath;
    if (Path.IsEmpty())
    {
        Path = GetName();
    }
    return FString::Printf(TEXT("rtsp://localhost:8554/%s"), *Path);
}

void ARTSPCameraActor::CaptureFrame()
{
    if (!RenderTarget || !SceneCaptureComponent)
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("CaptureFrame 실패: RenderTarget 또는 SceneCaptureComponent 없음"));
        return;
    }

    // 명시적으로 캡처 수행
    SceneCaptureComponent->CaptureScene();

    
    // 게임 스레드에서 렌더 타겟 처리
    if (IsInGameThread())
    {
        // 렌더 타겟 리소스 가져오기
        FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
        if (!RTResource)
        {
            UE_LOG(LogRTSPStreamer, Error, TEXT("RenderTargetResource 없음"));
            return;
        }
        
        // 이미지 크기 확인
        const FIntPoint Size = RTResource->GetSizeXY();
        
        // 텍스처 데이터 읽기
        TArray<FColor> SurfaceData;
        FReadSurfaceDataFlags ReadDataFlags;
        ReadDataFlags.SetLinearToGamma(false);
        
        // 안전하게 픽셀 읽기 시도
        if (RTResource->ReadPixels(SurfaceData, ReadDataFlags))
        {
            int32 NumPixels = SurfaceData.Num();
            
            // SurfaceData가 비어있지 않은지 확인
            if (NumPixels > 0)
            {
                // 버퍼 크기 확인 및 설정
                int32 ExpectedBufferSize = NumPixels * 3; // RGB
                if (FrameBuffer.Num() != ExpectedBufferSize)
                {
                    FrameBuffer.SetNum(ExpectedBufferSize);
                }
                
                // BGR 형식으로 변환 (GStreamer가 기대하는 방식)
                for (int32 i = 0; i < NumPixels; ++i)
                {
                    const int32 Index = i * 3;
                    FrameBuffer[Index + 0] = SurfaceData[i].B; // B
                    FrameBuffer[Index + 1] = SurfaceData[i].G; // G
                    FrameBuffer[Index + 2] = SurfaceData[i].R; // R
                }
                
                // 스트림에 프레임 전송
                FRealGazeboModule& Module = FRealGazeboModule::Get();

                if (Module.IsServerRunning() && !FrameBuffer.IsEmpty())
                {
                    Module.UpdateStream(StreamSettings.StreamPath, FrameBuffer);
                }
            }
            else
            {
                UE_LOG(LogRTSPStreamer, Warning, TEXT("읽은 픽셀 데이터가 비어있습니다"));
            }
        }
        else
        {
            UE_LOG(LogRTSPStreamer, Warning, TEXT("픽셀 데이터를 읽는데 실패했습니다"));
        }
    }
    else
    {
        UE_LOG(LogRTSPStreamer, Warning, TEXT("게임 스레드에서 실행되지 않았습니다"));
    }
}


// NeedDataCallback 및 EnoughDataCallback 구현
static void NeedDataCallback(GstElement* AppSrc, guint Length, gpointer UserData)
{
    FRTSPStreamData* StreamData = static_cast<FRTSPStreamData*>(UserData);
    if (StreamData)
    {
        // 스트리밍 상태 설정
        StreamData->bIsStreaming = true;
    }
    else
    {
        UE_LOG(LogRTSPStreamer, Error, TEXT("NeedDataCallback: UserData가 NULL입니다"));
    }
}