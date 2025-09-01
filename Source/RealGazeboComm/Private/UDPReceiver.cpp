#include "UDPReceiver.h"
#include "RealGazeboComm.h"
#include "HAL/PlatformProcess.h"

FUDPReceiver::FUDPReceiver()
    : Thread(nullptr)
    , bStopRequested(false)
    , bIsRunning(false)
    , ThreadPriority(TPri_Normal)
    , Socket(nullptr)
    , SocketSubsystem(nullptr)
    , SocketReceiveBufferSize(65536)
    , ListenPort(8888)
    , BufferSize(65536)
    , MaxPacketSize(1472) // Standard UDP MTU size
    , NumReceiveBuffers(4)
    , CurrentBufferIndex(0)
    , LastStatsUpdate(0.0)
{
    Statistics.Reset();
}

FUDPReceiver::~FUDPReceiver()
{
    StopReceiver();
}

bool FUDPReceiver::Init()
{
    UE_LOG(LogRealGazeboComm, Log, TEXT("UDP Receiver initializing on port %d"), ListenPort);
    
    SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogRealGazeboComm, Error, TEXT("Failed to get socket subsystem"));
        BroadcastStatusChange(false, TEXT("Failed to get socket subsystem"));
        return false;
    }

    Socket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("UDPReceiver"), false);
    if (!Socket)
    {
        UE_LOG(LogRealGazeboComm, Error, TEXT("Failed to create UDP socket"));
        BroadcastStatusChange(false, TEXT("Failed to create UDP socket"));
        return false;
    }

    Socket->SetNonBlocking(true);
    Socket->SetReuseAddr(true);
    
    // Set socket buffer sizes for better performance
    Socket->SetReceiveBufferSize(SocketReceiveBufferSize, SocketReceiveBufferSize);

    // Bind to listen port
    TSharedRef<FInternetAddr> ListenAddr = SocketSubsystem->CreateInternetAddr();
    ListenAddr->SetAnyAddress();
    ListenAddr->SetPort(ListenPort);

    if (!Socket->Bind(*ListenAddr))
    {
        UE_LOG(LogRealGazeboComm, Error, TEXT("Failed to bind UDP socket to port %d"), ListenPort);
        BroadcastStatusChange(false, FString::Printf(TEXT("Failed to bind to port %d"), ListenPort));
        CleanupSocket();
        return false;
    }

    // Prepare multiple receive buffers for better performance
    ReceiveBuffers.SetNum(NumReceiveBuffers);
    for (int32 i = 0; i < NumReceiveBuffers; i++)
    {
        ReceiveBuffers[i].SetNumUninitialized(BufferSize);
    }
    
    UE_LOG(LogRealGazeboComm, Log, TEXT("UDP Receiver initialized successfully on port %d with %d buffers (%d bytes each)"), 
           ListenPort, NumReceiveBuffers, BufferSize);
    BroadcastStatusChange(true, FString::Printf(TEXT("UDP Receiver bound to port %d"), ListenPort));
    return true;
}

uint32 FUDPReceiver::Run()
{
    UE_LOG(LogRealGazeboComm, Log, TEXT("UDP Receiver thread started"));
    bIsRunning = true;
    BroadcastStatusChange(true, TEXT("UDP Receiver running"));

    const double StatsUpdateInterval = 1.0; // Update stats every second
    double LastIdleTime = FPlatformTime::Seconds();
    const double MaxIdleTime = 0.010; // 10ms max idle
    
    while (!bStopRequested)
    {
        bool bReceivedData = ProcessIncomingData();
        
        // Update statistics periodically
        double CurrentTime = FPlatformTime::Seconds();
        if (CurrentTime - LastStatsUpdate > StatsUpdateInterval)
        {
            BroadcastStatistics();
            LastStatsUpdate = CurrentTime;
        }
        
        if (!bReceivedData)
        {
            // Adaptive sleep - longer sleep if we haven't received data for a while
            double IdleDuration = CurrentTime - LastIdleTime;
            if (IdleDuration > MaxIdleTime)
            {
                FPlatformProcess::Sleep(0.005f); // 5ms sleep during idle periods
            }
            else
            {
                FPlatformProcess::Sleep(0.001f); // 1ms sleep during active periods
            }
        }
        else
        {
            LastIdleTime = CurrentTime;
        }
    }

    bIsRunning = false;
    UE_LOG(LogRealGazeboComm, Log, TEXT("UDP Receiver thread finished"));
    BroadcastStatusChange(false, TEXT("UDP Receiver stopped"));
    return 0;
}

void FUDPReceiver::Stop()
{
    bStopRequested = true;
}

void FUDPReceiver::Exit()
{
    CleanupSocket();
}

bool FUDPReceiver::StartReceiver(int32 InListenPort, int32 InBufferSize, int32 InNumBuffers)
{
    if (bIsRunning || Thread)
    {
        UE_LOG(LogRealGazeboComm, Warning, TEXT("UDP Receiver already running"));
        return true;
    }

    ListenPort = InListenPort;
    BufferSize = FMath::Max(1024, InBufferSize); // Minimum 1KB buffer
    NumReceiveBuffers = FMath::Max(1, InNumBuffers);
    SocketReceiveBufferSize = FMath::Max(BufferSize, SocketReceiveBufferSize);

    bStopRequested = false;
    Thread = FRunnableThread::Create(this, TEXT("UDPReceiverThread"), 0, ThreadPriority);
    
    if (!Thread)
    {
        UE_LOG(LogRealGazeboComm, Error, TEXT("Failed to create UDP Receiver thread"));
        BroadcastStatusChange(false, TEXT("Failed to create thread"));
        return false;
    }

    return true;
}

void FUDPReceiver::StopReceiver()
{
    if (Thread)
    {
        bStopRequested = true;
        Thread->WaitForCompletion();
        delete Thread;
        Thread = nullptr;
    }
    
    CleanupSocket();
}

bool FUDPReceiver::IsRunning() const
{
    return bIsRunning;
}

void FUDPReceiver::CleanupSocket()
{
    if (Socket)
    {
        Socket->Close();
        if (SocketSubsystem)
        {
            SocketSubsystem->DestroySocket(Socket);
        }
        Socket = nullptr;
    }
    SocketSubsystem = nullptr;
}

bool FUDPReceiver::ProcessIncomingData()
{
    if (!Socket)
    {
        return false;
    }

    uint32 PendingDataSize = 0;
    if (!Socket->HasPendingData(PendingDataSize) || PendingDataSize == 0)
    {
        return false;
    }

    TSharedRef<FInternetAddr> SenderAddr = SocketSubsystem->CreateInternetAddr();
    TArray<uint8>& CurrentBuffer = GetNextReceiveBuffer();
    int32 BytesRead = 0;

    bool bSuccess = Socket->RecvFrom(CurrentBuffer.GetData(), FMath::Min(CurrentBuffer.Num(), (int32)PendingDataSize), BytesRead, *SenderAddr);
    
    if (bSuccess && BytesRead > 0)
    {
        // Create a copy of the received data with exact size
        TArray<uint8> ReceivedData;
        ReceivedData.SetNumUninitialized(BytesRead);
        FMemory::Memcpy(ReceivedData.GetData(), CurrentBuffer.GetData(), BytesRead);
        
        // Get sender info (with caching for performance)
        uint32 SenderAddressValue = 0;
        SenderAddr->GetIp(SenderAddressValue);
        FString SenderIP = GetCachedAddressString(SenderAddressValue);
        int32 SenderPort = SenderAddr->GetPort();
        
        // Detect if this is binary or text data (simple heuristic)
        bool bIsBinary = false;
        for (int32 i = 0; i < FMath::Min(BytesRead, 64); i++)
        {
            uint8 Byte = ReceivedData[i];
            if (Byte < 32 && Byte != 9 && Byte != 10 && Byte != 13)
            {
                bIsBinary = true;
                break;
            }
        }
        
        UpdateStatistics(BytesRead, true);
        
        UE_LOG(LogRealGazeboComm, VeryVerbose, TEXT("Received %d bytes from %s:%d (Binary: %s)"), 
               BytesRead, *SenderIP, SenderPort, bIsBinary ? TEXT("Yes") : TEXT("No"));
        
        BroadcastDataReceived(ReceivedData, bIsBinary, SenderIP, SenderPort);
        return true;
    }
    else
    {
        UpdateStatistics(0, false);
    }

    return false;
}

// Configuration methods
void FUDPReceiver::SetThreadPriority(EThreadPriority Priority)
{
    ThreadPriority = Priority;
    if (Thread)
    {
        Thread->SetThreadPriority(Priority);
    }
}

// Statistics methods
FUDPCommStats FUDPReceiver::GetStatistics() const
{
    FScopeLock Lock(&StatsMutex);
    return Statistics;
}

void FUDPReceiver::ResetStatistics()
{
    FScopeLock Lock(&StatsMutex);
    Statistics.Reset();
}

// Optimized helper methods
TArray<uint8>& FUDPReceiver::GetNextReceiveBuffer()
{
    CurrentBufferIndex = (CurrentBufferIndex + 1) % NumReceiveBuffers;
    return ReceiveBuffers[CurrentBufferIndex];
}

FString FUDPReceiver::GetCachedAddressString(uint32 AddressValue)
{
    {
        FScopeLock Lock(&AddressCacheMutex);
        if (FString* Found = AddressCache.Find(AddressValue))
        {
            return *Found;
        }
    }
    
    // Create new address string
    FIPv4Address IPv4Address(AddressValue);
    FString AddressString = IPv4Address.ToString();
    
    {
        FScopeLock Lock(&AddressCacheMutex);
        AddressCache.Add(AddressValue, AddressString);
    }
    
    return AddressString;
}

void FUDPReceiver::UpdateStatistics(int32 BytesReceived, bool bSuccess)
{
    FScopeLock Lock(&StatsMutex);
    
    if (bSuccess && BytesReceived > 0)
    {
        Statistics.BytesReceived += BytesReceived;
        Statistics.MessagesReceived++;
    }
    else if (!bSuccess)
    {
        Statistics.ReceiveErrors++;
    }
}

void FUDPReceiver::BroadcastStatistics()
{
    FUDPCommStats CurrentStats = GetStatistics();
    
    AsyncTask(ENamedThreads::GameThread, [this, CurrentStats]()
    {
        OnStatisticsUpdate.Broadcast(CurrentStats);
    });
}

void FUDPReceiver::BroadcastDataReceived(const TArray<uint8>& Data, bool bIsBinary, const FString& SenderIP, int32 SenderPort)
{
    // Execute on game thread for delegate broadcast
    AsyncTask(ENamedThreads::GameThread, [this, Data, bIsBinary, SenderIP, SenderPort]()
    {
        OnDataReceived.Broadcast(Data, bIsBinary, SenderIP, SenderPort);
    });
}

void FUDPReceiver::BroadcastStatusChange(bool bRunning, const FString& StatusMessage)
{
    // Execute on game thread for delegate broadcast
    AsyncTask(ENamedThreads::GameThread, [this, bRunning, StatusMessage]()
    {
        OnStatusChanged.Broadcast(bRunning, StatusMessage);
    });
}