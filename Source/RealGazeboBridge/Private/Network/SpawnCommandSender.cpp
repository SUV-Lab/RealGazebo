// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#include "SpawnCommandSender.h"
#include "RealGazeboBridge.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

namespace
{
    constexpr uint8 MSG_SPAWN = 1;   // pose packet reused as the spawn command
    constexpr uint8 MSG_DESPAWN = 4; // header-only destroy command
    constexpr uint8 MSG_WIND = 6;    // world wind: op byte + 3 floats
    // MSG_WIND op byte: 0 = wind off, 1 = wind on, 2 = query current state
    constexpr uint8 WIND_OP_QUERY = 2;
}

bool USpawnCommandSender::SetDestination(const FString& HostOrIP, int32 Port)
{
    // Called before every command: keep the outcome for this host string +
    // port, so a host name is not looked up (and a bad one not re-reported)
    // on every command. Cleared when either changes.
    if (Port == ResolvedPort && HostOrIP == ResolvedHost)
    {
        return Destination.IsValid();
    }
    ResolvedHost = HostOrIP;
    ResolvedPort = Port;
    Destination.Reset();

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        return false;
    }

    TSharedPtr<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
    bool bIsValid = false;
    Addr->SetIp(*HostOrIP, bIsValid); // numeric literal
    if (bIsValid && Addr->GetProtocolType() != FNetworkProtocolTypes::IPv4)
    {
        // the send socket is IPv4-only (NAME_DGram default); say so once here
        UE_LOG(LogRealGazeboBridge, Warning,
               TEXT("SpawnCommandSender: manager host '%s' is not an IPv4 address"), *HostOrIP);
        return false;
    }
    if (!bIsValid)
    {
        // Not a numeric address: resolve it as a host name (localhost,
        // host.docker.internal, a LAN machine name, ...). Blocking DNS
        // lookup, but only when the host string changes.
        const FAddressInfoResult Resolved = SocketSubsystem->GetAddressInfo(
            *HostOrIP, nullptr, EAddressInfoFlags::Default, NAME_None, SOCKTYPE_Datagram);
        for (const FAddressInfoResultData& Entry : Resolved.Results)
        {
            if (Entry.Address->GetProtocolType() == FNetworkProtocolTypes::IPv4)
            {
                Addr = Entry.Address->Clone();
                bIsValid = true;
                break;
            }
        }
        if (!bIsValid)
        {
            UE_LOG(LogRealGazeboBridge, Warning,
                   TEXT("SpawnCommandSender: manager host '%s' is neither an IPv4 "
                        "address nor a resolvable host name"), *HostOrIP);
            return false;
        }
        UE_LOG(LogRealGazeboBridge, Display,
               TEXT("SpawnCommandSender: manager host '%s' resolved to %s"),
               *HostOrIP, *Addr->ToString(false));
    }
    Addr->SetPort(Port);
    Destination = Addr;
    return true;
}

bool USpawnCommandSender::SendSpawn(uint8 VehicleNum, uint8 VehicleTypeCode,
                                    const FVector& UnrealLocation, const FQuat& UnrealRotation)
{
    // Exact inverse of UDataStreamProcessor::ConvertGazeboPositionToUnreal /
    // ConvertGazeboQuaternionToUnreal (both involutions): cm -> meters with
    // Y negated; quaternion components (X, -Y, Z, -W).
    const float GzX = static_cast<float>(UnrealLocation.X) / 100.0f;
    const float GzY = static_cast<float>(-UnrealLocation.Y) / 100.0f;
    const float GzZ = static_cast<float>(UnrealLocation.Z) / 100.0f;

    const float GzQx = static_cast<float>(UnrealRotation.X);
    const float GzQy = static_cast<float>(-UnrealRotation.Y);
    const float GzQz = static_cast<float>(UnrealRotation.Z);
    const float GzQw = static_cast<float>(-UnrealRotation.W);

    TArray<uint8> Packet;
    Packet.Reserve(31); // header(3) + 7 floats(28), see EXPECTED_POSE_PACKET_SIZE
    Packet.Add(VehicleNum);
    Packet.Add(VehicleTypeCode);
    Packet.Add(MSG_SPAWN);
    AppendFloatLE(Packet, GzX);
    AppendFloatLE(Packet, GzY);
    AppendFloatLE(Packet, GzZ);
    AppendFloatLE(Packet, GzQx);
    AppendFloatLE(Packet, GzQy);
    AppendFloatLE(Packet, GzQz);
    AppendFloatLE(Packet, GzQw);

    return SendPacket(Packet);
}

bool USpawnCommandSender::SendDespawn(uint8 VehicleNum, uint8 VehicleTypeCode)
{
    TArray<uint8> Packet;
    Packet.Reserve(3);
    Packet.Add(VehicleNum);
    Packet.Add(VehicleTypeCode);
    Packet.Add(MSG_DESPAWN);

    return SendPacket(Packet);
}

bool USpawnCommandSender::SendWindCommand(bool bEnable, const FVector& GazeboWindVelocity)
{
    TArray<uint8> Packet;
    Packet.Reserve(16); // header(3) + enable(1) + 3 floats(12)
    Packet.Add(0);      // VehicleNum: unused, world-level command
    Packet.Add(0);      // VehicleType: unused
    Packet.Add(MSG_WIND);
    Packet.Add(bEnable ? 1 : 0);
    AppendFloatLE(Packet, static_cast<float>(GazeboWindVelocity.X));
    AppendFloatLE(Packet, static_cast<float>(GazeboWindVelocity.Y));
    AppendFloatLE(Packet, static_cast<float>(GazeboWindVelocity.Z));

    return SendPacket(Packet);
}

bool USpawnCommandSender::SendWindQuery()
{
    TArray<uint8> Packet;
    Packet.Reserve(16); // same layout as the wind command, vector unused
    Packet.Add(0);
    Packet.Add(0);
    Packet.Add(MSG_WIND);
    Packet.Add(WIND_OP_QUERY);
    AppendFloatLE(Packet, 0.0f);
    AppendFloatLE(Packet, 0.0f);
    AppendFloatLE(Packet, 0.0f);

    return SendPacket(Packet);
}

bool USpawnCommandSender::EnsureSocket()
{
    if (SendSocket)
    {
        return true;
    }
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        return false;
    }
    SendSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("RealGazeboSpawnSender"), false);
    return SendSocket != nullptr;
}

bool USpawnCommandSender::SendPacket(const TArray<uint8>& Packet)
{
    if (!Destination.IsValid())
    {
        UE_LOG(LogRealGazeboBridge, Warning,
               TEXT("SpawnCommandSender: no destination set"));
        return false;
    }
    if (!EnsureSocket())
    {
        return false;
    }

    int32 BytesSent = 0;
    const bool bSent = SendSocket->SendTo(Packet.GetData(), Packet.Num(), BytesSent, *Destination);
    if (!bSent || BytesSent != Packet.Num())
    {
        // UDP cannot tell us whether the manager received anything, but a
        // local send failure (unreachable host, dead socket, oversized
        // datagram) is known right here - say so instead of a silent false.
        // Only a failed SendTo leaves a meaningful socket error behind; a
        // short write did not fail any syscall, so do not print a stale one.
        FString Why = TEXT("short write");
        if (!bSent)
        {
            ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
            Why = SocketSubsystem
                ? SocketSubsystem->GetSocketError(SocketSubsystem->GetLastErrorCode())
                : TEXT("no socket subsystem");
        }
        UE_LOG(LogRealGazeboBridge, Warning,
               TEXT("SpawnCommandSender: send to %s failed (%d/%d bytes, %s)"),
               *Destination->ToString(true), BytesSent, Packet.Num(), *Why);
        return false;
    }
    return true;
}

void USpawnCommandSender::AppendFloatLE(TArray<uint8>& Out, float Value)
{
    // Raw IEEE-754 little-endian bytes, mirroring the receiver's BytesToFloat
    union
    {
        float value;
        uint8 bytes[4];
    } Converter;
    Converter.value = Value;
    Out.Append(Converter.bytes, 4);
}

void USpawnCommandSender::BeginDestroy()
{
    if (SendSocket)
    {
        SendSocket->Close();
        if (ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
        {
            SocketSubsystem->DestroySocket(SendSocket);
        }
        SendSocket = nullptr;
    }
    Super::BeginDestroy();
}
