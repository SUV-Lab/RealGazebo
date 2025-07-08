// Fill out your copyright notice in the Description page of Project Settings.

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
class REALGAZEBO_API UUDPReceiver : public UObject, public FRunnable
{
    GENERATED_BODY()

public:
    UUDPReceiver();
    virtual ~UUDPReceiver();

    // Core UDP functions
    UFUNCTION(BlueprintCallable, Category = "RealGazebo|UDP Receiver")
    bool StartListening(int32 Port);

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
    
    // Thread control
    bool bStopRequested;
    bool bIsListening;
    int32 ListenPort;
    
    // Data processing
    TQueue<FUDPData> ReceivedDataQueue;
    FCriticalSection DataQueueCS;

    // Core functions
    void ProcessReceivedData(const TArray<uint8>& Data, const FString& SenderIP, int32 SenderPort);

private:
    // Internal cleanup
    void CleanupSocket();
    void CleanupThread();
};