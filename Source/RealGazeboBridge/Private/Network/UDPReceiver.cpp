// Fill out your copyright notice in the Description page of Project Settings.

#include "UDPReceiver.h"
#include "Engine/Engine.h"

UUDPReceiver::UUDPReceiver()
{
    ListenSocket = nullptr;
    ReceiverThread = nullptr;
    SocketSubsystem = nullptr;
    bStopRequested = false;
    bIsListening = false;
    ListenPort = 0;
}

UUDPReceiver::~UUDPReceiver()
{
    StopListening();
}

bool UUDPReceiver::StartListening(int32 Port, const FString& IPAddress)
{
    if (bIsListening)
    {
        UE_LOG(LogTemp, Warning, TEXT("UDPReceiver: Already listening on port %d"), ListenPort);
        return false;
    }

    ListenPort = Port;
    SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    
    if (!SocketSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("UDPReceiver: Failed to get socket subsystem"));
        return false;
    }

    // Create UDP socket
    ListenSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("UDPReceiver"), false);
    if (!ListenSocket)
    {
        UE_LOG(LogTemp, Error, TEXT("UDPReceiver: Failed to create UDP socket"));
        return false;
    }

    // Set socket to blocking for simpler implementation
    ListenSocket->SetNonBlocking(false);

    // Create address for binding
    TSharedRef<FInternetAddr> LocalAddr = SocketSubsystem->CreateInternetAddr();
    
    // Set IP address - if empty/not specified, bind to all addresses (any address)
    if (IPAddress.IsEmpty())
    {
        LocalAddr->SetAnyAddress();
        UE_LOG(LogTemp, Warning, TEXT("UDPReceiver: Binding to all addresses (any address)"));
    }
    else
    {
        bool bIsValid = false;
        LocalAddr->SetIp(*IPAddress, bIsValid);
        
        if (!bIsValid)
        {
            UE_LOG(LogTemp, Error, TEXT("UDPReceiver: Invalid IP address: %s"), *IPAddress);
            CleanupSocket();
            return false;
        }
        UE_LOG(LogTemp, Warning, TEXT("UDPReceiver: Binding to specific IP: %s"), *IPAddress);
    }
    
    LocalAddr->SetPort(ListenPort);

    // Bind socket to address
    if (!ListenSocket->Bind(*LocalAddr))
    {
        FString BindInfo = IPAddress.IsEmpty() ? TEXT("any address") : IPAddress;
        UE_LOG(LogTemp, Error, TEXT("UDPReceiver: Failed to bind UDP socket to %s:%d"), *BindInfo, ListenPort);
        CleanupSocket();
        return false;
    }

    // Set socket buffer size
    int32 NewSize = 0;
    ListenSocket->SetReceiveBufferSize(1024 * 1024, NewSize); // 1MB buffer

    bStopRequested = false;
    bIsListening = true;

    // Start receiver thread
    ReceiverThread = FRunnableThread::Create(this, TEXT("UDPReceiverThread"), 0, TPri_Normal);
    
    FString BindInfo = IPAddress.IsEmpty() ? TEXT("any address") : IPAddress;
    UE_LOG(LogTemp, Warning, TEXT("UDPReceiver: Started listening on %s:%d"), *BindInfo, ListenPort);
    return true;
}

void UUDPReceiver::StopListening()
{
    if (!bIsListening)
    {
        return;
    }

    bStopRequested = true;
    bIsListening = false;

    CleanupThread();
    CleanupSocket();

    UE_LOG(LogTemp, Warning, TEXT("UDPReceiver: Stopped listening"));
}

bool UUDPReceiver::IsListening() const
{
    return bIsListening;
}

bool UUDPReceiver::Init()
{
    return true;
}

uint32 UUDPReceiver::Run()
{
    uint8 ReceiveBuffer[1024]; // Buffer for incoming data
    
    while (!bStopRequested)
    {
        if (!ListenSocket)
        {
            break;
        }

        // Check if data is available
        bool bHasPendingData = false;
        uint32 PendingDataSize = 0;
        
        if (ListenSocket->HasPendingData(PendingDataSize))
        {
            bHasPendingData = true;
        }

        if (bHasPendingData && PendingDataSize > 0)
        {
            TSharedRef<FInternetAddr> SenderAddr = SocketSubsystem->CreateInternetAddr();
            int32 BytesRead = 0;

            // Receive data
            if (ListenSocket->RecvFrom(ReceiveBuffer, sizeof(ReceiveBuffer), BytesRead, *SenderAddr))
            {
                if (BytesRead > 0)
                {
                    // Create array from received data
                    TArray<uint8> ReceivedData;
                    ReceivedData.Append(ReceiveBuffer, BytesRead);

                    // Get sender information
                    FString SenderIP = SenderAddr->ToString(false);
                    int32 SenderPort = SenderAddr->GetPort();

                    // Process the received data
                    ProcessReceivedData(ReceivedData, SenderIP, SenderPort);
                }
            }
        }
        else
        {
            // Sleep for a short time to prevent busy waiting
            FPlatformProcess::Sleep(0.001f); // 1ms
        }
    }

    return 0;
}

void UUDPReceiver::Stop()
{
    bStopRequested = true;
}

void UUDPReceiver::Exit()
{
    // Cleanup handled in destructor
}

void UUDPReceiver::ProcessReceivedData(const TArray<uint8>& Data, const FString& SenderIP, int32 SenderPort)
{
    // Create UDP data struct
    FUDPData UDPData;
    UDPData.Data = Data;
    UDPData.SenderIP = SenderIP;
    UDPData.SenderPort = SenderPort;

    // Add to queue for main thread processing
    {
        FScopeLock Lock(&DataQueueCS);
        ReceivedDataQueue.Enqueue(UDPData);
    }

    // Trigger event on game thread
    if (IsInGameThread())
    {
        OnDataReceived.Broadcast(UDPData);
    }
    else
    {
        AsyncTask(ENamedThreads::GameThread, [this, UDPData]()
        {
            OnDataReceived.Broadcast(UDPData);
        });
    }
}

void UUDPReceiver::CleanupSocket()
{
    if (ListenSocket)
    {
        ListenSocket->Close();
        SocketSubsystem->DestroySocket(ListenSocket);
        ListenSocket = nullptr;
    }
}

void UUDPReceiver::CleanupThread()
{
    if (ReceiverThread)
    {
        ReceiverThread->WaitForCompletion();
        delete ReceiverThread;
        ReceiverThread = nullptr;
    }
}