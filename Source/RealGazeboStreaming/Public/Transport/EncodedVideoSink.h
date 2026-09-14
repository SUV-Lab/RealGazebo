// Copyright (c) 2024-2026 SUV Lab, Chungbuk National University
// Licensed under the GNU General Public License v3.0.

#pragma once

#include "CoreMinimal.h"
#include "Encoder/HardwareEncoderWrapper.h"

/**
 * Metadata associated with one encoded video access unit.
 *
 * This deliberately contains transport-neutral sensor/platform state so one
 * encoder output can be fan-out to RTSP/WebRTC and STANAG 4609/MISB KLV
 * without rendering or encoding the frame twice.
 */
struct REALGAZEBOSTREAMING_API FEncodedVideoMetadata
{
	/** Monotonic frame identifier assigned by the streaming pipeline. */
	uint64 FrameNumber = 0;

	/** Capture/encode timestamp in microseconds. */
	uint64 TimestampUs = 0;

	/** WGS-84 platform position, when known. */
	double LatitudeDeg = 0.0;
	double LongitudeDeg = 0.0;
	double AltitudeMslMeters = 0.0;
	bool bHasWGS84Position = false;

	/** Platform attitude in degrees, when known. */
	double PlatformRollDeg = 0.0;
	double PlatformPitchDeg = 0.0;
	double PlatformHeadingDeg = 0.0;
	bool bHasPlatformAttitude = false;

	/** Sensor/camera attitude relative to the platform in degrees, when known. */
	double SensorRelativeRollDeg = 0.0;
	double SensorRelativePitchDeg = 0.0;
	double SensorRelativeYawDeg = 0.0;
	bool bHasSensorAttitude = false;

	/** Camera field of view, when known. */
	double HorizontalFovDeg = 0.0;
	double VerticalFovDeg = 0.0;
	bool bHasFieldOfView = false;
};

/**
 * Transport-neutral consumer of encoded H.264/H.265 access units.
 *
 * Implementations may packetize the same encoded stream for:
 * - RTSP/RTP (legacy RealGazebo path)
 * - browser transport (preferably WebRTC)
 * - STANAG 4609 MPEG-TS with MISB KLV metadata
 * - recording/debug sinks
 *
 * Sinks MUST NOT require a second video encode.
 */
class REALGAZEBOSTREAMING_API IEncodedVideoSink
{
public:
	virtual ~IEncodedVideoSink() = default;

	/** Called once when a stream becomes active. */
	virtual bool Start(FString& OutErrorMessage) = 0;

	/** Called during pipeline shutdown before encoder resources are released. */
	virtual void Stop() = 0;

	/**
	 * Consume encoded NAL units for one frame/access unit.
	 * Implementations must copy any data they retain asynchronously.
	 */
	virtual void PushEncodedVideo(
		const TArray<FEncodedNALUnit>& NALUnits,
		const FEncodedVideoMetadata& Metadata) = 0;

	/**
	 * Whether this sink currently wants frames.
	 *
	 * RTSP can return false when no client is connected. STANAG recorders or
	 * continuous browser gateways can return true independently of RTSP state.
	 */
	virtual bool WantsFrames() const { return true; }

	/** Human-readable sink name for logging/diagnostics. */
	virtual FString GetName() const = 0;
};
