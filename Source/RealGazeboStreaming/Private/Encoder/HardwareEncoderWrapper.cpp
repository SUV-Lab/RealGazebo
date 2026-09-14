// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Licensed under the GNU General Public License v3.0.

#if PLATFORM_MAC

#include "Encoder/HardwareEncoderWrapper.h"
#include "HAL/PlatformTime.h"
#include "RHI.h"
#include "RHIResources.h"
#include "UObject/UObjectGlobals.h"
#include "Video/Encoders/SimpleVideoEncoder.h"

FHardwareEncoderWrapper::FHardwareEncoderWrapper()
{
	UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Created (macOS VideoToolbox)"));
}

FHardwareEncoderWrapper::~FHardwareEncoderWrapper()
{
	Shutdown();
}

bool FHardwareEncoderWrapper::Initialize(const FEncoderConfig& InConfig, FString& OutErrorMessage)
{
	if (bInitialized)
	{
		OutErrorMessage = TEXT("Encoder already initialized");
		return true;
	}
	if (!InConfig.Validate(OutErrorMessage))
	{
		return false;
	}
	Config = InConfig;
	if (!CreateHardwareEncoder(OutErrorMessage))
	{
		return false;
	}
	RegisterEncoderCallbacks();
	bInitialized = true;
	return true;
}

void FHardwareEncoderWrapper::Shutdown()
{
	if (!bInitialized)
	{
		return;
	}
	bShuttingDown = true;
	if (AVCodecEncoder)
	{
		AVCodecEncoder->Close();
		AVCodecEncoder->RemoveFromRoot();
		AVCodecEncoder->ConditionalBeginDestroy();
		AVCodecEncoder = nullptr;
	}
	bInitialized = false;
	bShuttingDown = false;
}

bool FHardwareEncoderWrapper::CreateHardwareEncoder(FString& OutErrorMessage)
{
	if (!GDynamicRHI)
	{
		OutErrorMessage = TEXT("RHI not initialized");
		return false;
	}
	if (RHIGetInterfaceType() != ERHIInterfaceType::Metal)
	{
		OutErrorMessage = TEXT("macOS VideoToolbox encoder requires Metal RHI");
		return false;
	}

	AVCodecEncoder = NewObject<USimpleVideoEncoder>(GetTransientPackage());
	if (!AVCodecEncoder)
	{
		OutErrorMessage = TEXT("Failed to allocate USimpleVideoEncoder");
		return false;
	}
	AVCodecEncoder->AddToRoot();

	FSimpleVideoEncoderConfig CodecConfig;
	CodecConfig.Width = Config.Width;
	CodecConfig.Height = Config.Height;
	CodecConfig.TargetFramerate = Config.FrameRate;
	CodecConfig.TargetBitrate = static_cast<int32>(FMath::Min<uint64>(Config.Bitrate, MAX_int32));
	CodecConfig.MaxBitrate = static_cast<int32>(FMath::Min<uint64>(Config.Bitrate + Config.Bitrate / 5, MAX_int32));

	if (!AVCodecEncoder->Open(ESimpleVideoCodec::H264, CodecConfig, false))
	{
		AVCodecEncoder->RemoveFromRoot();
		AVCodecEncoder->ConditionalBeginDestroy();
		AVCodecEncoder = nullptr;
		OutErrorMessage = TEXT("Failed to open H.264 VideoToolbox encoder through AVCodecs/VTCodecs");
		return false;
	}

	EncoderType = EEncoderType::Unknown;
	return true;
}

void FHardwareEncoderWrapper::RegisterEncoderCallbacks()
{
	// USimpleVideoEncoder exposes a pull-based packet queue.
}

bool FHardwareEncoderWrapper::EncodeFrame(FRHITexture* Texture, uint64 FrameNumber, bool bForceKeyframe)
{
	if (!bInitialized || bShuttingDown || !AVCodecEncoder || !Texture || !Texture->IsValid())
	{
		EncodingFailureCount++;
		return false;
	}

	FTextureRHIRef TextureRef(Texture);
	const bool bForce = bForceKeyframe || bForceNextKeyframe.exchange(false);
	const double TimestampUs = FPlatformTime::Seconds() * 1000000.0;
	if (!AVCodecEncoder->SendFrame(TextureRef, TimestampUs, bForce))
	{
		EncodingFailureCount++;
		return false;
	}

	CurrentFrameNumber = FrameNumber;
	LastEncodeTime = FPlatformTime::Seconds();
	TotalFramesEncoded++;
	return true;
}

bool FHardwareEncoderWrapper::GetEncodedData(TArray<FEncodedNALUnit>& OutNALUnits)
{
	OutNALUnits.Empty();
	if (!AVCodecEncoder || bShuttingDown)
	{
		return false;
	}

	TArray<FSimpleVideoPacket> Packets;
	AVCodecEncoder->ReceivePackets(Packets);
	for (FSimpleVideoPacket& Packet : Packets)
	{
		const uint8* Data = Packet.RawPacket.DataPtr.Get();
		const uint32 Size = Packet.RawPacket.DataSize;
		if (!Data || Size == 0)
		{
			continue;
		}

		TotalBytesOutput += Size;
		if (Packet.RawPacket.bIsKeyframe)
		{
			TotalKeyframesEncoded++;
		}

		TArray<FEncodedNALUnit> Parsed;
		ParseNALUnits(Data, Size, Parsed);
		if (Parsed.Num() == 0)
		{
			FEncodedNALUnit NAL;
			NAL.Data.Append(Data, Size);
			NAL.NALType = ExtractNALType(Data, Size);
			Parsed.Add(MoveTemp(NAL));
		}

		for (FEncodedNALUnit& NAL : Parsed)
		{
			NAL.bIsKeyframe = Packet.RawPacket.bIsKeyframe;
			NAL.TimestampMs = static_cast<uint64>(Packet.RawPacket.Timestamp / 1000);
			NAL.FrameNumber = CurrentFrameNumber.load();
			OutNALUnits.Add(MoveTemp(NAL));
		}
	}
	return OutNALUnits.Num() > 0;
}

void FHardwareEncoderWrapper::ForceKeyframe()
{
	bForceNextKeyframe = true;
}

void FHardwareEncoderWrapper::ParseNALUnits(const uint8* Data, uint32 Size, TArray<FEncodedNALUnit>& OutNALs)
{
	OutNALs.Empty();
	if (!Data || Size < 4)
	{
		return;
	}

	for (uint32 I = 0; I + 3 < Size; )
	{
		uint32 StartCode = 0;
		if (I + 4 <= Size && Data[I] == 0 && Data[I + 1] == 0 && Data[I + 2] == 0 && Data[I + 3] == 1)
		{
			StartCode = 4;
		}
		else if (Data[I] == 0 && Data[I + 1] == 0 && Data[I + 2] == 1)
		{
			StartCode = 3;
		}
		if (!StartCode)
		{
			++I;
			continue;
		}

		uint32 J = I + StartCode;
		while (J + 3 < Size)
		{
			const bool bStart4 = J + 4 <= Size && Data[J] == 0 && Data[J + 1] == 0 && Data[J + 2] == 0 && Data[J + 3] == 1;
			const bool bStart3 = Data[J] == 0 && Data[J + 1] == 0 && Data[J + 2] == 1;
			if (bStart4 || bStart3)
			{
				break;
			}
			++J;
		}
		if (J + 3 >= Size)
		{
			J = Size;
		}

		const uint32 NALSize = J - I;
		if (NALSize > StartCode)
		{
			FEncodedNALUnit NAL;
			NAL.Data.Append(Data + I, NALSize);
			NAL.NALType = ExtractNALType(Data + I + StartCode, NALSize - StartCode);
			OutNALs.Add(MoveTemp(NAL));
		}
		I = J;
	}
}

uint8 FHardwareEncoderWrapper::ExtractNALType(const uint8* Data, uint32 Size) const
{
	return Data && Size ? (Data[0] & 0x1F) : 0;
}

FString FHardwareEncoderWrapper::GetStatsString() const
{
	return FString::Printf(
		TEXT("HardwareEncoder (VideoToolbox): Encoded=%llu frames (%llu keyframes), Output=%.2f MB, Failures=%llu"),
		TotalFramesEncoded.load(),
		TotalKeyframesEncoded.load(),
		TotalBytesOutput.load() / (1024.0 * 1024.0),
		EncodingFailureCount.load());
}

#endif // PLATFORM_MAC
