#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/Event.h"
#include "Containers/Queue.h"
#include "Containers/LockFreeList.h"
#include "CommTypes.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"

class REALGAZEBOCOMM_API FUDPSender : public FRunnable
{
public:
    FUDPSender();
    virtual ~FUDPSender();

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

    // Public interface
    bool StartSender(int32 SendBufferSize = 65536, int32 MaxBatchSize = 10);
    void StopSender();
    bool IsRunning() const;
    
    // Message queuing (legacy string support)
    void QueueMessage(const FString& Message, const FString& TargetIP, int32 TargetPort, bool bHighPriority = false);
    
    // Enhanced message queuing
    void QueueMessage(const FUDPMessage& Message);
    void QueueBinaryMessage(const TArray<uint8>& Data, const FString& TargetIP, int32 TargetPort, bool bHighPriority = false);
    
    // Batch message sending
    void QueueMessages(const TArray<FUDPMessage>& Messages);
    
    // Configuration
    void SetThreadPriority(EThreadPriority Priority);
    void SetMaxQueueSize(int32 MaxSize) { MaxQueueSize = MaxSize; }
    void SetBatchingEnabled(bool bEnabled) { bBatchingEnabled = bEnabled; }
    
    // Statistics
    FUDPCommStats GetStatistics() const;
    void ResetStatistics();

    // Events
    FOnUDPCommSenderStatusChanged OnStatusChanged;
    FOnUDPCommStatisticsUpdate OnStatisticsUpdate;

private:
    // Thread management
    FRunnableThread* Thread;
    FThreadSafeBool bStopRequested;
    FThreadSafeBool bIsRunning;
    FEvent* WakeupEvent;
    EThreadPriority ThreadPriority;

    // Socket
    FSocket* Socket;
    ISocketSubsystem* SocketSubsystem;
    int32 SocketSendBufferSize;

    // Message queue - using lock-free structures for better performance
    TLockFreePointerListUnordered<FUDPMessage, PLATFORM_CACHE_LINE_SIZE> HighPriorityQueue;
    TLockFreePointerListUnordered<FUDPMessage, PLATFORM_CACHE_LINE_SIZE> NormalPriorityQueue;
    FUDPMessagePool MessagePool;
    
    // Configuration
    int32 MaxQueueSize;
    int32 MaxBatchSize;
    bool bBatchingEnabled;
    
    // Batching
    TArray<FUDPMessage*> CurrentBatch;
    TMap<FString, TSharedPtr<FInternetAddr>> AddressCache;
    FCriticalSection AddressCacheMutex;
    
    // Statistics
    mutable FCriticalSection StatsMutex;
    FUDPCommStats Statistics;
    double LastStatsUpdate;

    // Private methods
    void CleanupSocket();
    bool ProcessMessages();
    bool ProcessMessage(const FUDPMessage& Message);
    bool ProcessBatchedMessages();
    bool SendSingleMessage(const FUDPMessage& Message);
    TSharedPtr<FInternetAddr> GetCachedAddress(const FString& IP, int32 Port);
    void UpdateStatistics(int32 BytesSent, bool bSuccess);
    void BroadcastStatistics();
    void BroadcastStatusChange(bool bRunning, const FString& StatusMessage);
    void CleanupQueues();
    int32 GetTotalQueueSize() const;
};