#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/Event.h"
#include "CommTypes.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"

class REALGAZEBOCOMM_API FUDPReceiver : public FRunnable
{
public:
    FUDPReceiver();
    virtual ~FUDPReceiver();

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

    // Public interface
    bool StartReceiver(int32 ListenPort = 8888, int32 BufferSize = 65536, int32 NumBuffers = 4);
    void StopReceiver();
    bool IsRunning() const;
    
    // Configuration
    int32 GetListenPort() const { return ListenPort; }
    int32 GetBufferSize() const { return BufferSize; }
    void SetThreadPriority(EThreadPriority Priority);
    void SetMaxPacketSize(int32 MaxSize) { MaxPacketSize = MaxSize; }
    
    // Statistics
    FUDPCommStats GetStatistics() const;
    void ResetStatistics();

    // Events
    FOnUDPCommDataReceived OnDataReceived;
    FOnUDPCommReceiverStatusChanged OnStatusChanged;
    FOnUDPCommStatisticsUpdate OnStatisticsUpdate;

private:
    // Thread management
    FRunnableThread* Thread;
    FThreadSafeBool bStopRequested;
    FThreadSafeBool bIsRunning;
    EThreadPriority ThreadPriority;

    // Socket
    FSocket* Socket;
    ISocketSubsystem* SocketSubsystem;
    int32 SocketReceiveBufferSize;

    // Configuration
    int32 ListenPort;
    int32 BufferSize;
    int32 MaxPacketSize;
    int32 NumReceiveBuffers;
    
    // Receive buffers - multiple buffers for better performance
    TArray<TArray<uint8>> ReceiveBuffers;
    int32 CurrentBufferIndex;
    
    // Address cache for sender info
    TMap<uint32, FString> AddressCache;
    FCriticalSection AddressCacheMutex;
    
    // Statistics
    mutable FCriticalSection StatsMutex;
    FUDPCommStats Statistics;
    double LastStatsUpdate;

    // Private methods
    void CleanupSocket();
    bool ProcessIncomingData();
    void BroadcastDataReceived(const TArray<uint8>& Data, bool bIsBinary, const FString& SenderIP, int32 SenderPort);
    void BroadcastStatusChange(bool bRunning, const FString& StatusMessage);
    void BroadcastStatistics();
    void UpdateStatistics(int32 BytesReceived, bool bSuccess);
    FString GetCachedAddressString(uint32 AddressValue);
    TArray<uint8>& GetNextReceiveBuffer();
};