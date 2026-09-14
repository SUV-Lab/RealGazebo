// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#pragma once

#include "CoreMinimal.h"
#include "StreamingTypes.h"

// UE 5.1: Must include full header to access nested enums
// (FVideoEncoder::H264Profile, RateControlMode, MultipassMode)
#if !PLATFORM_MAC
#include "VideoEncoder.h"
#endif

/**
 * FEncoderConfig
 *
 * Converts RealGazebo streaming configuration to AVEncoder API parameters.
 * Enforces zero-copy GPU encoding with hardware-only acceleration.
 *
 * Key Features:
 * - Automatic bitrate calculation based on resolution/FPS
 * - UltraLowLatency preset mapping
 * - Baseline H.264 profile enforcement
 * - CBR (Constant Bitrate) rate control
 * - GOP size optimization for low latency
 */
struct FEncoderConfig
{
	//----------------------------------------------------------
	// Video Parameters
	//----------------------------------------------------------

	/** Target resolution width */
	int32 Width = 1024;

	/** Target resolution height */
	int32 Height = 768;

	/** Target frame rate */
	int32 FrameRate = 30;

	/** Target bitrate in bits per second (auto-calculated from resolution + FPS) */
	uint64 Bitrate = 2000000; // 2 Mbps default

	/** GOP (Group of Pictures) size - keyframe interval */
	int32 GOPSize = 15;

	//----------------------------------------------------------
	// Encoder Settings (Hardcoded for Ultra-Low-Latency)
	//----------------------------------------------------------

	/** Encoder preset (always UltraLowLatency) */
	EEncoderPreset Preset = EEncoderPreset::UltraLowLatency;

	/** H.264 profile (always Baseline for maximum compatibility) */
	ERealGazeboH264Profile Profile = ERealGazeboH264Profile::Baseline;

	/** Rate control mode (always CBR for consistent latency) */
	bool bConstantBitrate = true;

	/** Zero-copy GPU encoding (always enabled) */
	bool bZeroCopy = true;

	/** Multipass encoding (disabled for low latency) */
	bool bMultipass = false;

	/** Number of B-frames (0 for ultra-low latency) */
	int32 NumBFrames = 0;

	/** Number of reference frames (1 for ultra-low latency) */
	int32 NumRefFrames = 1;

	//----------------------------------------------------------
	// Construction & Conversion
	//----------------------------------------------------------

	/** Default constructor */
	FEncoderConfig() = default;

	/** Construct from FStreamConfig with auto-bitrate calculation */
	explicit FEncoderConfig(const FStreamConfig& StreamConfig)
	{
		// Get resolution dimensions
		StreamConfig.GetResolution(Width, Height);

		// Get frame rate
		FrameRate = StreamConfig.GetFrameRateValue();

		// Calculate optimal bitrate (convert kbps to bps)
		Bitrate = static_cast<uint64>(StreamConfig.GetBitrate()) * 1000;

		// Set GOP size (auto-calculated as FPS/2)
		GOPSize = StreamConfig.GetGOPSize();

		// Encoder settings (always hardcoded for ultra-low latency)
		Preset = StreamConfig.Preset;
		Profile = StreamConfig.Profile;
		bZeroCopy = StreamConfig.bZeroCopy;
	}

	//----------------------------------------------------------
	// AVEncoder API Mapping (UE 5.1; unavailable on macOS)
	//----------------------------------------------------------

#if !PLATFORM_MAC
	/** Convert to AVEncoder H.264 profile (UE 5.1: FVideoEncoder::H264Profile) */
	AVEncoder::FVideoEncoder::H264Profile GetAVEncoderProfile() const;

	/** Convert to AVEncoder rate control mode (UE 5.1: FVideoEncoder::RateControlMode) */
	AVEncoder::FVideoEncoder::RateControlMode GetRateControlMode() const;

	/** Convert to AVEncoder multipass mode (UE 5.1: FVideoEncoder::MultipassMode) */
	AVEncoder::FVideoEncoder::MultipassMode GetMultipassMode() const;
#endif

	//----------------------------------------------------------
	// Validation
	//----------------------------------------------------------

	/** Validate configuration is suitable for ultra-low latency */
	bool Validate(FString& OutErrorMessage) const
	{
		// Check resolution bounds
		if (Width < 320 || Width > 3840)
		{
			OutErrorMessage = FString::Printf(TEXT("Invalid width %d (must be 320-3840)"), Width);
			return false;
		}

		if (Height < 240 || Height > 2160)
		{
			OutErrorMessage = FString::Printf(TEXT("Invalid height %d (must be 240-2160)"), Height);
			return false;
		}

		// Check frame rate
		if (FrameRate < 10 || FrameRate > 120)
		{
			OutErrorMessage = FString::Printf(TEXT("Invalid frame rate %d (must be 10-120)"), FrameRate);
			return false;
		}

		// Check bitrate
		const uint64 MinBitrate = 500000; // 500 kbps
		const uint64 MaxBitrate = 50000000; // 50 Mbps
		if (Bitrate < MinBitrate || Bitrate > MaxBitrate)
		{
			OutErrorMessage = FString::Printf(TEXT("Invalid bitrate %llu (must be 500kbps-50Mbps)"), Bitrate);
			return false;
		}

		// Check GOP size
		if (GOPSize < 5 || GOPSize > 120)
		{
			OutErrorMessage = FString::Printf(TEXT("Invalid GOP size %d (must be 5-120)"), GOPSize);
			return false;
		}

		// Validate ultra-low latency requirements
		if (!bZeroCopy)
		{
			OutErrorMessage = TEXT("Zero-copy must be enabled for ultra-low latency");
			return false;
		}

		if (NumBFrames != 0)
		{
			OutErrorMessage = TEXT("B-frames must be disabled (0) for ultra-low latency");
			return false;
		}

		if (NumRefFrames > 1)
		{
			OutErrorMessage = TEXT("Reference frames must be 1 for ultra-low latency");
			return false;
		}

		OutErrorMessage = TEXT("Validation successful");
		return true;
	}

	/** Get configuration summary for logging */
	FString ToString() const
	{
		return FString::Printf(
			TEXT("EncoderConfig: %dx%d @ %dfps, %.2f Mbps, GOP=%d, Preset=%s, Profile=%s, ZeroCopy=%s"),
			Width, Height, FrameRate,
			Bitrate / 1000000.0,
			GOPSize,
			*EncoderPresetToString(Preset),
			TEXT("Baseline"),
			bZeroCopy ? TEXT("Yes") : TEXT("No")
		);
	}
};