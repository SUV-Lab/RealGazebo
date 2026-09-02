// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SpawnCommandSender.generated.h"

class FSocket;
class FInternetAddr;

/**
 * Minimal outbound UDP client for runtime commands to the simulation
 * manager: vehicle spawn/despawn and the world-level wind setting.
 *
 * Sends RealGazebo wire packets to the simulation manager (default port
 * 5006): the MessageID=1 pose packet doubles as the SPAWN command, a
 * header-only MessageID=4 as DESPAWN, and MessageID=6 sets the world
 * wind. Spawn coordinates are converted from Unreal world space to the
 * Gazebo frame — the exact inverse of the inbound conversion in
 * UDataStreamProcessor (both axes maps are involutions, so the same
 * component flips apply). Wind is NOT converted: it is authored in the
 * simulator's frame and goes on the wire as-is.
 *
 * Fire-and-forget by design: there is no ACK. A successful spawn shows up
 * as the vehicle's normal pose stream (which auto-spawns the visual pawn);
 * a missing stream after a timeout means the spawn failed sim-side.
 */
UCLASS()
class REALGAZEBOBRIDGE_API USpawnCommandSender : public UObject
{
    GENERATED_BODY()

public:
    /** Set/replace the manager destination. Accepts a numeric IPv4 or a
     *  host name (localhost, host.docker.internal, ...): names are resolved
     *  once and cached until the host string or port changes. Returns false
     *  when the address is invalid or cannot be resolved. */
    bool SetDestination(const FString& HostOrIP, int32 Port);

    bool HasDestination() const { return Destination.IsValid(); }

    /** Spawn command: [Num][Type][0x01] + 7 little-endian floats
     *  (gz position x,y,z in meters + gz quaternion x,y,z,w). */
    bool SendSpawn(uint8 VehicleNum, uint8 VehicleTypeCode,
                   const FVector& UnrealLocation, const FQuat& UnrealRotation);

    /** Despawn command: header-only [Num][Type][0x04]. */
    bool SendDespawn(uint8 VehicleNum, uint8 VehicleTypeCode);

    /** World wind command: [0][0][0x06] + [enable:u8] + 3 little-endian
     *  floats (wind velocity x, y, z). World-level, so the header ids are
     *  unused and sent as 0. The velocity is the Gazebo world-frame wind in
     *  m/s and is sent AS-IS — no Unreal-to-Gazebo conversion, unlike spawn.
     *  bEnable=false switches the wind off (the vector is still carried). */
    bool SendWindCommand(bool bEnable, const FVector& GazeboWindVelocity);

    /** World wind state query: [0][0][0x06] + [op=2] + 3 zero floats. The
     *  manager answers with a MessageID=6 packet (enable + vector) on the
     *  UE data port (:5005); UDataStreamProcessor turns that into
     *  UGazeboBridgeSubsystem::NotifyWorldWindState. */
    bool SendWindQuery();

    virtual void BeginDestroy() override;

private:
    bool EnsureSocket();
    bool SendPacket(const TArray<uint8>& Packet);
    static void AppendFloatLE(TArray<uint8>& Out, float Value);

    FSocket* SendSocket = nullptr;
    TSharedPtr<FInternetAddr> Destination;
    /** Host string / port that Destination was resolved from (resolution cache). */
    FString ResolvedHost;
    int32 ResolvedPort = 0;
};
