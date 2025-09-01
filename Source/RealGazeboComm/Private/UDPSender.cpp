#include "UDPSender.h"
#include "RealGazeboComm.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformFileManager.h"

FUDPSender::FUDPSender()
    : Thread(nullptr)
    , bStopRequested(false)
    , bIsRunning(false)
    , WakeupEvent(nullptr)
    , ThreadPriority(TPri_Normal)
    , Socket(nullptr)
    , SocketSubsystem(nullptr)
    , SocketSendBufferSize(65536)
    , MessagePool(1000)
    , MaxQueueSize(10000)
    , MaxBatchSize(10)
    , bBatchingEnabled(true)
    , LastStatsUpdate(0.0)
{
    WakeupEvent = FPlatformProcess::GetSynchEventFromPool(false);
    Statistics.Reset();
}

FUDPSender::~FUDPSender()
{
    StopSender();
    
    if (WakeupEvent)
    {
        FPlatformProcess::ReturnSynchEventToPool(WakeupEvent);
        WakeupEvent = nullptr;
    }
    
    CleanupQueues();
}

bool FUDPSender::Init()
{
    UE_LOG(LogRealGazeboComm, Log, TEXT("UDP Sender initializing"));
    
    SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogRealGazeboComm, Error, TEXT("Failed to get socket subsystem"));
        BroadcastStatusChange(false, TEXT("Failed to get socket subsystem"));
        return false;
    }

    Socket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("UDPSender"), false);
    if (!Socket)
    {
        UE_LOG(LogRealGazeboComm, Error, TEXT("Failed to create UDP socket"));
        BroadcastStatusChange(false, TEXT("Failed to create UDP socket"));
        return false;
    }

    Socket->SetNonBlocking(true);
    Socket->SetReuseAddr(true);
    
    // Set socket buffer sizes for better performance
    Socket->SetSendBufferSize(SocketSendBufferSize, SocketSendBufferSize);
    
    UE_LOG(LogRealGazeboComm, Log, TEXT("UDP Sender initialized successfully with %d byte buffer"), SocketSendBufferSize);
    BroadcastStatusChange(true, TEXT("UDP Sender initialized"));
    return true;
}

uint32 FUDPSender::Run()
{
    UE_LOG(LogRealGazeboComm, Log, TEXT("UDP Sender thread started"));
    bIsRunning = true;
    BroadcastStatusChange(true, TEXT("UDP Sender running"));

    const double StatsUpdateInterval = 1.0; // Update stats every second
    
    while (!bStopRequested)
    {
        bool bProcessedMessages = ProcessMessages();
        
        // Update statistics periodically
        double CurrentTime = FPlatformTime::Seconds();
        if (CurrentTime - LastStatsUpdate > StatsUpdateInterval)
        {
            BroadcastStatistics();
            LastStatsUpdate = CurrentTime;
        }
        
        if (!bProcessedMessages)
        {
            // Wait for new messages or timeout after 10ms
            WakeupEvent->Wait(10);
        }
    }

    bIsRunning = false;
    UE_LOG(LogRealGazeboComm, Log, TEXT("UDP Sender thread finished"));
    BroadcastStatusChange(false, TEXT("UDP Sender stopped"));
    return 0;
}

void FUDPSender::Stop()
{
    bStopRequested = true;
}

void FUDPSender::Exit()
{
    CleanupSocket();
}

bool FUDPSender::StartSender(int32 SendBufferSize, int32 InMaxBatchSize)
{
    if (bIsRunning || Thread)
    {
        UE_LOG(LogRealGazeboComm, Warning, TEXT("UDP Sender already running"));
        return true;
    }

    SocketSendBufferSize = FMath::Max(4096, SendBufferSize);
    MaxBatchSize = FMath::Max(1, InMaxBatchSize);
    
    bStopRequested = false;
    Thread = FRunnableThread::Create(this, TEXT("UDPSenderThread"), 0, ThreadPriority);
    
    if (!Thread)
    {
        UE_LOG(LogRealGazeboComm, Error, TEXT("Failed to create UDP Sender thread"));
        BroadcastStatusChange(false, TEXT("Failed to create thread"));
        return false;
    }

    return true;
}

void FUDPSender::StopSender()
{
    if (Thread)
    {
        bStopRequested = true;
        
        // Wake up the thread so it can exit quickly
        if (WakeupEvent)
        {
            WakeupEvent->Trigger();
        }
        
        Thread->WaitForCompletion();
        delete Thread;
        Thread = nullptr;
    }
    
    CleanupSocket();
}

bool FUDPSender::IsRunning() const
{
    return bIsRunning;
}

void FUDPSender::QueueMessage(const FString& Message, const FString& TargetIP, int32 TargetPort, bool bHighPriority)
{
    if (!bIsRunning)
    {
        UE_LOG(LogRealGazeboComm, Warning, TEXT("Attempting to queue message while UDP Sender is not running"));
        return;
    }
    
    // Check queue size limit
    if (GetTotalQueueSize() >= MaxQueueSize)
    {
        UE_LOG(LogRealGazeboComm, Warning, TEXT("Message queue full, dropping message"));
        return;
    }

    FUDPMessage* UDPMessage = MessagePool.Acquire();
    *UDPMessage = FUDPMessage(Message, TargetIP, TargetPort, bHighPriority);
    
    if (bHighPriority)
    {
        HighPriorityQueue.Push(UDPMessage);
    }
    else
    {
        NormalPriorityQueue.Push(UDPMessage);
    }
    
    // Wake up the thread
    if (WakeupEvent)
    {
        WakeupEvent->Trigger();
    }
    
    UE_LOG(LogRealGazeboComm, VeryVerbose, TEXT("Message queued for %s:%d (Priority: %s)"), 
           *TargetIP, TargetPort, bHighPriority ? TEXT("High") : TEXT("Normal"));
}

void FUDPSender::QueueMessage(const FUDPMessage& Message)
{
    if (!bIsRunning)
    {
        UE_LOG(LogRealGazeboComm, Warning, TEXT("Attempting to queue message while UDP Sender is not running"));
        return;
    }
    
    // Check queue size limit
    if (GetTotalQueueSize() >= MaxQueueSize)
    {
        UE_LOG(LogRealGazeboComm, Warning, TEXT("Message queue full, dropping message"));
        return;
    }

    FUDPMessage* UDPMessage = MessagePool.Acquire();
    *UDPMessage = Message;
    
    if (Message.bHighPriority)
    {
        HighPriorityQueue.Push(UDPMessage);
    }
    else
    {
        NormalPriorityQueue.Push(UDPMessage);
    }
    
    // Wake up the thread
    if (WakeupEvent)
    {
        WakeupEvent->Trigger();
    }
    
    UE_LOG(LogRealGazeboComm, VeryVerbose, TEXT("Message queued for %s:%d"), *Message.TargetIP, Message.TargetPort);
}

void FUDPSender::QueueBinaryMessage(const TArray<uint8>& Data, const FString& TargetIP, int32 TargetPort, bool bHighPriority)
{
    if (!bIsRunning)
    {
        UE_LOG(LogRealGazeboComm, Warning, TEXT("Attempting to queue binary message while UDP Sender is not running"));
        return;
    }
    
    // Check queue size limit
    if (GetTotalQueueSize() >= MaxQueueSize)
    {
        UE_LOG(LogRealGazeboComm, Warning, TEXT("Message queue full, dropping binary message"));
        return;
    }

    FUDPMessage* UDPMessage = MessagePool.Acquire();
    *UDPMessage = FUDPMessage(Data, TargetIP, TargetPort, bHighPriority);
    
    if (bHighPriority)
    {
        HighPriorityQueue.Push(UDPMessage);
    }
    else
    {
        NormalPriorityQueue.Push(UDPMessage);
    }
    
    // Wake up the thread
    if (WakeupEvent)
    {
        WakeupEvent->Trigger();
    }
    
    UE_LOG(LogRealGazeboComm, VeryVerbose, TEXT("Binary message queued for %s:%d (%d bytes)"), 
           *TargetIP, TargetPort, Data.Num());
}

void FUDPSender::QueueMessages(const TArray<FUDPMessage>& Messages)
{
    if (!bIsRunning)
    {
        UE_LOG(LogRealGazeboComm, Warning, TEXT("Attempting to queue messages while UDP Sender is not running"));
        return;
    }
    
    for (const FUDPMessage& Message : Messages)
    {
        QueueMessage(Message);
    }
}

void FUDPSender::CleanupSocket()
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

bool FUDPSender::ProcessMessage(const FUDPMessage& Message)
{
    if (!Socket)
    {
        UE_LOG(LogRealGazeboComm, Error, TEXT("No socket available for sending"));
        return false;
    }

    FIPv4Address IPv4Address;
    if (!FIPv4Address::Parse(Message.TargetIP, IPv4Address))
    {
        UE_LOG(LogRealGazeboComm, Error, TEXT("Invalid target IP address: %s"), *Message.TargetIP);
        return false;
    }

    TSharedRef<FInternetAddr> TargetAddr = SocketSubsystem->CreateInternetAddr();
    TargetAddr->SetIp(IPv4Address.Value);
    TargetAddr->SetPort(Message.TargetPort);

    // Use the binary data directly
    const uint8* Data = Message.Data.GetData();
    int32 DataSize = Message.Data.Num();

    int32 BytesSent = 0;
    bool bSuccess = Socket->SendTo(Data, DataSize, BytesSent, *TargetAddr);
    
    if (bSuccess && BytesSent == DataSize)
    {
        UE_LOG(LogRealGazeboComm, VeryVerbose, TEXT("Message sent successfully to %s:%d (%d bytes)"), 
               *Message.TargetIP, Message.TargetPort, BytesSent);
        return true;
    }
    else
    {
        UE_LOG(LogRealGazeboComm, Warning, TEXT("Failed to send message to %s:%d (sent %d/%d bytes)"), 
               *Message.TargetIP, Message.TargetPort, BytesSent, DataSize);
        return false;
    }
}

// Configuration methods
void FUDPSender::SetThreadPriority(EThreadPriority Priority)
{
    ThreadPriority = Priority;
    if (Thread)
    {
        Thread->SetThreadPriority(Priority);
    }
}

// Statistics methods
FUDPCommStats FUDPSender::GetStatistics() const
{
    FScopeLock Lock(&StatsMutex);
    return Statistics;
}

void FUDPSender::ResetStatistics()
{
    FScopeLock Lock(&StatsMutex);
    Statistics.Reset();
}

// Optimized message processing
bool FUDPSender::ProcessMessages()
{
    CurrentBatch.Reset();
    bool bProcessedAny = false;
    
    // Process high priority messages first
    FUDPMessage* Message;
    while ((Message = HighPriorityQueue.Pop()) != nullptr)
    {
        if (bBatchingEnabled && CurrentBatch.Num() < MaxBatchSize)
        {
            CurrentBatch.Add(Message);
        }
        else
        {
            SendSingleMessage(*Message);
            MessagePool.Release(Message);
        }
        bProcessedAny = true;
    }
    
    // Process normal priority messages
    while ((Message = NormalPriorityQueue.Pop()) != nullptr && 
           (!bBatchingEnabled || CurrentBatch.Num() < MaxBatchSize))
    {
        if (bBatchingEnabled)
        {
            CurrentBatch.Add(Message);
        }
        else
        {
            SendSingleMessage(*Message);
            MessagePool.Release(Message);
        }
        bProcessedAny = true;
    }
    
    // Send batched messages
    if (bBatchingEnabled && CurrentBatch.Num() > 0)
    {
        ProcessBatchedMessages();
    }
    
    return bProcessedAny;
}

bool FUDPSender::ProcessBatchedMessages()
{
    bool bAllSuccessful = true;
    
    for (FUDPMessage* Message : CurrentBatch)
    {
        if (!SendSingleMessage(*Message))
        {
            bAllSuccessful = false;
        }
        MessagePool.Release(Message);
    }
    
    CurrentBatch.Reset();
    return bAllSuccessful;
}

bool FUDPSender::SendSingleMessage(const FUDPMessage& Message)
{
    if (!Socket)
    {
        UE_LOG(LogRealGazeboComm, Error, TEXT("No socket available for sending"));
        UpdateStatistics(0, false);
        return false;
    }

    TSharedPtr<FInternetAddr> TargetAddr = GetCachedAddress(Message.TargetIP, Message.TargetPort);
    if (!TargetAddr.IsValid())
    {
        UE_LOG(LogRealGazeboComm, Error, TEXT("Invalid target address: %s:%d"), *Message.TargetIP, Message.TargetPort);
        UpdateStatistics(0, false);
        return false;
    }

    int32 BytesSent = 0;
    bool bSuccess = Socket->SendTo(Message.Data.GetData(), Message.Data.Num(), BytesSent, *TargetAddr);
    
    UpdateStatistics(BytesSent, bSuccess && BytesSent == Message.Data.Num());
    
    if (bSuccess && BytesSent == Message.Data.Num())
    {
        UE_LOG(LogRealGazeboComm, VeryVerbose, TEXT("Message sent successfully to %s:%d (%d bytes)"), 
               *Message.TargetIP, Message.TargetPort, BytesSent);
        return true;
    }
    else
    {
        UE_LOG(LogRealGazeboComm, Warning, TEXT("Failed to send message to %s:%d (sent %d/%d bytes)"), 
               *Message.TargetIP, Message.TargetPort, BytesSent, Message.Data.Num());
        return false;
    }
}

TSharedPtr<FInternetAddr> FUDPSender::GetCachedAddress(const FString& IP, int32 Port)
{
    FString AddressKey = FString::Printf(TEXT("%s:%d"), *IP, Port);
    
    {
        FScopeLock Lock(&AddressCacheMutex);
        if (TSharedPtr<FInternetAddr>* Found = AddressCache.Find(AddressKey))
        {
            return *Found;
        }
    }
    
    // Create new address
    FIPv4Address IPv4Address;
    if (!FIPv4Address::Parse(IP, IPv4Address))
    {
        return nullptr;
    }
    
    TSharedRef<FInternetAddr> NewAddrRef = SocketSubsystem->CreateInternetAddr();
    TSharedPtr<FInternetAddr> NewAddr = NewAddrRef;
    NewAddr->SetIp(IPv4Address.Value);
    NewAddr->SetPort(Port);
    
    // Cache it
    {
        FScopeLock Lock(&AddressCacheMutex);
        AddressCache.Add(AddressKey, NewAddr);
    }
    
    return NewAddr;
}

void FUDPSender::UpdateStatistics(int32 BytesSent, bool bSuccess)
{
    FScopeLock Lock(&StatsMutex);
    
    if (bSuccess)
    {
        Statistics.BytesSent += BytesSent;
        Statistics.MessagesSent++;
    }
    else
    {
        Statistics.SendErrors++;
    }
}

void FUDPSender::BroadcastStatistics()
{
    FUDPCommStats CurrentStats = GetStatistics();
    
    AsyncTask(ENamedThreads::GameThread, [this, CurrentStats]()
    {
        OnStatisticsUpdate.Broadcast(CurrentStats);
    });
}

void FUDPSender::CleanupQueues()
{
    FUDPMessage* Message;
    while ((Message = HighPriorityQueue.Pop()) != nullptr)
    {
        MessagePool.Release(Message);
    }
    
    while ((Message = NormalPriorityQueue.Pop()) != nullptr)
    {
        MessagePool.Release(Message);
    }
    
    AddressCache.Empty();
}

int32 FUDPSender::GetTotalQueueSize() const
{
    // This is an approximation since lock-free structures don't provide exact counts
    return MessagePool.GetPoolSize(); // Rough estimate based on pool usage
}

void FUDPSender::BroadcastStatusChange(bool bRunning, const FString& StatusMessage)
{
    // Execute on game thread for delegate broadcast
    AsyncTask(ENamedThreads::GameThread, [this, bRunning, StatusMessage]()
    {
        OnStatusChanged.Broadcast(bRunning, StatusMessage);
    });
}