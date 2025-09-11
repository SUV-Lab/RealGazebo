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
    ExpectedIPAddress = TEXT("");
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
    ExpectedIPAddress = IPAddress; // Store for IP filtering
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
    
    // Always bind to any address (0.0.0.0) to receive from all sources
    // The IPAddress parameter is stored for reference but not used for binding
    LocalAddr->SetAnyAddress();
    
    if (IPAddress.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("UDPReceiver: Binding to any address (0.0.0.0) - will accept from all sources"));
    }
    else
    {
        // Validate the IP address format but don't use it for binding
        bool bIsValid = false;
        TSharedRef<FInternetAddr> TestAddr = SocketSubsystem->CreateInternetAddr();
        TestAddr->SetIp(*IPAddress, bIsValid);
        
        if (!bIsValid)
        {
            UE_LOG(LogTemp, Error, TEXT("UDPReceiver: Invalid IP address format: %s - will accept from all sources"), *IPAddress);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UDPReceiver: Will filter for IP: %s (binding to 0.0.0.0 for compatibility)"), *IPAddress);
        }
    }
    
    LocalAddr->SetPort(ListenPort);

    // Bind socket to address
    if (!ListenSocket->Bind(*LocalAddr))
    {
        UE_LOG(LogTemp, Error, TEXT("UDPReceiver: Failed to bind UDP socket to any address (0.0.0.0):%d"), ListenPort);
        CleanupSocket();
        return false;
    }

    // Set socket buffer size
    int32 NewSize = 0;
    ListenSocket->SetReceiveBufferSize(1024 * 1024, NewSize); // 1MB buffer
    
    UE_LOG(LogTemp, Log, TEXT("UDPReceiver: Socket receive buffer set to %d bytes (requested 1MB)"), NewSize);

    bStopRequested = false;
    bIsListening = true;

    // Start receiver thread
    ReceiverThread = FRunnableThread::Create(this, TEXT("UDPReceiverThread"), 0, TPri_Normal);
    
    if (IPAddress.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("UDPReceiver: Started listening on any address (0.0.0.0):%d - accepting from all IPs"), ListenPort);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UDPReceiver: Started listening on any address (0.0.0.0):%d - filtering for IP: %s"), ListenPort, *IPAddress);
    }
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

                    // IP filtering - check if we should accept this packet
                    bool bAcceptPacket = true;
                    if (!ExpectedIPAddress.IsEmpty())
                    {
                        // If ExpectedIPAddress is set, only accept packets from that IP
                        if (!SenderIP.Equals(ExpectedIPAddress, ESearchCase::IgnoreCase))
                        {
                            bAcceptPacket = false;
                            UE_LOG(LogTemp, Verbose, TEXT("UDPReceiver: Filtered packet from %s:%d (expecting %s)"), *SenderIP, SenderPort, *ExpectedIPAddress);
                        }
                    }

                    if (bAcceptPacket)
                    {
                        // Simple UDP packet logging  
                        UE_LOG(LogTemp, Verbose, TEXT("UDPReceiver: Received %d bytes from %s:%d"), BytesRead, *SenderIP, SenderPort);

                        // Process the received data
                        ProcessReceivedData(ReceivedData, SenderIP, SenderPort);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("UDPReceiver: Received 0 bytes from %s"), *SenderAddr->ToString(false));
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("UDPReceiver: Failed to receive data despite pending data indication"));
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

