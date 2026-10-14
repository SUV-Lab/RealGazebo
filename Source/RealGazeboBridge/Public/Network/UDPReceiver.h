// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Sub-author: MinKyu Kim
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Engine.h"
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Containers/Queue.h"
#include <atomic>  // C++11 atomic operations for lock-free thread safety
#include "UDPReceiver.generated.h"

USTRUCT(BlueprintType)
struct FUDPData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<uint8> Data;

    UPROPERTY(BlueprintReadOnly)
    FString SenderIP;

    UPROPERTY(BlueprintReadOnly)
    int32 SenderPort;

    FUDPData()
    {
        Data.Empty();
        SenderIP = TEXT("");
        SenderPort = 0;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUDPDataReceived, const FUDPData&, ReceivedData);

UCLASS(BlueprintType, Blueprintable)
class REALGAZEBOBRIDGE_API UUDPReceiver : public UObject, public FRunnable
{
    GENERATED_BODY()

public:
    UUDPReceiver();
    virtual ~UUDPReceiver();

    // Core UDP functions
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UDP Receiver")
    bool StartListening(int32 Port, const FString& IPAddress = TEXT("127.0.0.1"));

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UDP Receiver")
    void StopListening();

    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UDP Receiver")
    bool IsListening() const;

    // Event delegate for received data
    UPROPERTY(BlueprintAssignable, Category = "RealGazebo|UDP Receiver")
    FOnUDPDataReceived OnDataReceived;

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

protected:
    // Socket and networking
    FSocket* ListenSocket;
    FRunnableThread* ReceiverThread;
    ISocketSubsystem* SocketSubsystem;

    // Thread control (atomic for lock-free thread safety)
    std::atomic<bool> bStopRequested;
    std::atomic<bool> bIsListening;
    int32 ListenPort;
    
    // IP filtering
    FString ExpectedIPAddress;
    
    // Packets from the receiver thread to the game thread (one producer, one consumer),
    // drained by at most one game-thread task at a time
    TQueue<FUDPData> ReceivedDataQueue;
    std::atomic<bool> bDrainScheduled;
    TWeakObjectPtr<UUDPReceiver> WeakThis;

    // Core functions
    void ProcessReceivedData(const TArray<uint8>& Data, const FString& SenderIP, int32 SenderPort);
    void DrainReceivedData();

private:
    // Internal cleanup
    void CleanupSocket();
    void CleanupThread();
};