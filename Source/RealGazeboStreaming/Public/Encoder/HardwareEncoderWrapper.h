// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Licensed under the GNU General Public License v3.0.
#pragma once

#include "CoreMinimal.h"
#include "StreamingTypes.h"
#include "EncoderConfig.h"
#include "RHI.h"

#if PLATFORM_MAC
class USimpleVideoEncoder;
#endif

#if PLATFORM_LINUX
struct CUctx_st; using CUcontext = CUctx_st*;
struct CUarray_st; using CUarray = CUarray_st*;
struct CUmipmappedArray_st; using CUmipmappedArray = CUmipmappedArray_st*;
struct CUextMemory_st; using CUexternalMemory = CUextMemory_st*;

struct FCachedCUDATexture
{
	CUexternalMemory ExternalMemory = nullptr;
	CUmipmappedArray MipArray = nullptr;
	CUarray TiledArray = nullptr;
	CUarray LinearArray = nullptr;
	bool bValid = false;
	int32 Width = 0;
	int32 Height = 0;
};
#endif

#if !PLATFORM_MAC
namespace AVEncoder
{
	class FVideoEncoder;
	class FVideoEncoderInput;
	class FVideoEncoderInputFrame;
	class FCodecPacket;
	enum class EVideoFrameFormat;
}
#endif

struct FEncodedNALUnit
{
	TArray<uint8> Data;
	uint8 NALType = 0;
	bool bIsKeyframe = false;
	uint64 TimestampMs = 0;
	uint64 FrameNumber = 0;
	int32 GetSize() const { return Data.Num(); }
	bool IsValid() const { return Data.Num() > 0; }
	bool IsSPS() const { return NALType == 7; }
	bool IsPPS() const { return NALType == 8; }
	bool IsSlice() const { return NALType >= 1 && NALType <= 5; }
};

class FHardwareEncoderWrapper
{
public:
	FHardwareEncoderWrapper();
	~FHardwareEncoderWrapper();
	bool Initialize(const FEncoderConfig& Config, FString& OutErrorMessage);
	void Shutdown();
	void PrepareForShutdown() { bShuttingDown.store(true); }
	bool EncodeFrame(FRHITexture* Texture, uint64 FrameNumber, bool bForceKeyframe = false);
	bool GetEncodedData(TArray<FEncodedNALUnit>& OutNALUnits);
	void ForceKeyframe();
	bool IsReady() const { return bInitialized; }
	EEncoderType GetEncoderType() const { return EncoderType; }
	const FEncoderConfig& GetConfig() const { return Config; }
	FString GetStatsString() const;

private:
	bool CreateHardwareEncoder(FString& OutErrorMessage);
	void RegisterEncoderCallbacks();

	// Declared on all non-Mac platforms because upstream Shutdown() calls it
	// unconditionally; the implementation is a no-op outside Linux.
#if !PLATFORM_MAC
	void ClearCUDATextureCache();
#endif

#if PLATFORM_LINUX
	bool SetCUDATextureFromVulkan(FRHITexture* Texture,
		TSharedPtr<AVEncoder::FVideoEncoderInputFrame> InputFrame,
		FString& OutErrorMessage);
#endif

#if !PLATFORM_MAC
	void OnEncodedFrame(uint32 LayerIndex,
		const TSharedPtr<AVEncoder::FVideoEncoderInputFrame> Frame,
		const AVEncoder::FCodecPacket& Packet);
#endif

	void ParseNALUnits(const uint8* BitstreamData, uint32 BitstreamSize, TArray<FEncodedNALUnit>& OutNALUnits);
	uint8 ExtractNALType(const uint8* Data, uint32 Size) const;

	FEncoderConfig Config;
	EEncoderType EncoderType = EEncoderType::Unknown;

#if PLATFORM_LINUX
	CUcontext CudaContext = nullptr;
	TMap<FRHITexture*, FCachedCUDATexture> CUDATextureCache;
	FCriticalSection CUDACacheMutex;
	bool bUseCUDAMode = false;
	bool bLoggedUnsupportedTexture = false;
#endif

#if !PLATFORM_MAC
	TSharedPtr<AVEncoder::FVideoEncoderInput> VideoEncoderInput;
	TUniquePtr<AVEncoder::FVideoEncoder> VideoEncoder;
#endif

#if PLATFORM_MAC
	USimpleVideoEncoder* AVCodecEncoder = nullptr;
#endif

	std::atomic<bool> bInitialized{false};
	std::atomic<bool> bShuttingDown{false};
	std::atomic<bool> bForceNextKeyframe{false};
	std::atomic<uint64> CurrentFrameNumber{0};
	TQueue<FEncodedNALUnit> NALOutputQueue;
	FCriticalSection NALQueueMutex;
	std::atomic<uint64> TotalFramesEncoded{0};
	std::atomic<uint64> TotalKeyframesEncoded{0};
	std::atomic<uint64> TotalBytesOutput{0};
	std::atomic<uint64> EncodingFailureCount{0};
	double LastEncodeTime = 0.0;
};