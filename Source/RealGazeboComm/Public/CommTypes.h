#pragma once

#include "CoreMinimal.h"
#include "HAL/Event.h"
#include "Containers/LockFreeList.h"

// Delegates
DECLARE_MULTICAST_DELEGATE_FourParams(FOnUDPCommDataReceived, const TArray<uint8>&, bool, const FString&, int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnUDPCommSenderStatusChanged, bool, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnUDPCommReceiverStatusChanged, bool, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUDPCommStatisticsUpdate, const struct FUDPCommStats&);

// Message types
enum class EUDPMessageType : uint8
{
    Text = 0,
    Binary = 1,
    CompressedText = 2,
    CompressedBinary = 3
};

// Statistics structure
struct FUDPCommStats
{
    uint64 BytesSent = 0;
    uint64 BytesReceived = 0;
    uint64 MessagesSent = 0;
    uint64 MessagesReceived = 0;
    uint64 SendErrors = 0;
    uint64 ReceiveErrors = 0;
    double LastUpdateTime = 0.0;
    
    void Reset()
    {
        *this = FUDPCommStats();
        LastUpdateTime = FPlatformTime::Seconds();
    }
};

// Enhanced UDP message structure
struct FUDPMessage
{
    TArray<uint8> Data;
    FString TargetIP;
    int32 TargetPort;
    EUDPMessageType MessageType;
    bool bHighPriority;
    double Timestamp;
    
    FUDPMessage()
        : TargetIP(TEXT("127.0.0.1"))
        , TargetPort(8888)
        , MessageType(EUDPMessageType::Text)
        , bHighPriority(false)
        , Timestamp(FPlatformTime::Seconds())
    {
    }
    
    // Text message constructor
    FUDPMessage(const FString& InData, const FString& InTargetIP, int32 InTargetPort, bool bInHighPriority = false)
        : TargetIP(InTargetIP)
        , TargetPort(InTargetPort)
        , MessageType(EUDPMessageType::Text)
        , bHighPriority(bInHighPriority)
        , Timestamp(FPlatformTime::Seconds())
    {
        FTCHARToUTF8 UTF8String(*InData);
        Data.SetNumUninitialized(UTF8String.Length());
        FMemory::Memcpy(Data.GetData(), UTF8String.Get(), UTF8String.Length());
    }
    
    // Binary message constructor
    FUDPMessage(const TArray<uint8>& InData, const FString& InTargetIP, int32 InTargetPort, bool bInHighPriority = false)
        : Data(InData)
        , TargetIP(InTargetIP)
        , TargetPort(InTargetPort)
        , MessageType(EUDPMessageType::Binary)
        , bHighPriority(bInHighPriority)
        , Timestamp(FPlatformTime::Seconds())
    {
    }
    
    // Get as string (for text messages)
    FString GetAsString() const
    {
        if (MessageType == EUDPMessageType::Text || MessageType == EUDPMessageType::CompressedText)
        {
            if (Data.Num() > 0)
            {
                return FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Data.GetData())));
            }
        }
        return FString();
    }
    
    // Get data size
    int32 GetDataSize() const
    {
        return Data.Num();
    }
    
    // Check if message is expired (for cleanup)
    bool IsExpired(double MaxAge = 30.0) const
    {
        return (FPlatformTime::Seconds() - Timestamp) > MaxAge;
    }
};

// Message pool for object reuse
class FUDPMessagePool
{
private:
    TLockFreePointerListUnordered<FUDPMessage, PLATFORM_CACHE_LINE_SIZE> Pool;
    FCriticalSection PoolMutex;
    int32 MaxPoolSize;
    int32 CurrentPoolSize;
    
public:
    FUDPMessagePool(int32 InMaxPoolSize = 1000)
        : MaxPoolSize(InMaxPoolSize)
        , CurrentPoolSize(0)
    {
    }
    
    ~FUDPMessagePool()
    {
        Clear();
    }
    
    FUDPMessage* Acquire()
    {
        FUDPMessage* Message = Pool.Pop();
        if (!Message)
        {
            Message = new FUDPMessage();
        }
        else
        {
            FScopeLock Lock(&PoolMutex);
            CurrentPoolSize--;
        }
        return Message;
    }
    
    void Release(FUDPMessage* Message)
    {
        if (!Message) return;
        
        FScopeLock Lock(&PoolMutex);
        if (CurrentPoolSize < MaxPoolSize)
        {
            // Reset message for reuse
            Message->Data.Reset();
            Message->TargetIP = TEXT("127.0.0.1");
            Message->TargetPort = 8888;
            Message->MessageType = EUDPMessageType::Text;
            Message->bHighPriority = false;
            Message->Timestamp = FPlatformTime::Seconds();
            
            Pool.Push(Message);
            CurrentPoolSize++;
        }
        else
        {
            delete Message;
        }
    }
    
    void Clear()
    {
        FScopeLock Lock(&PoolMutex);
        FUDPMessage* Message;
        while ((Message = Pool.Pop()) != nullptr)
        {
            delete Message;
            CurrentPoolSize--;
        }
    }
    
    int32 GetPoolSize() const
    {
        FScopeLock Lock(const_cast<FCriticalSection*>(&PoolMutex));
        return CurrentPoolSize;
    }
};