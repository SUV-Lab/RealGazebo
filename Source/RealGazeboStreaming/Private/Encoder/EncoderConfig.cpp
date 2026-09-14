// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#include "Encoder/EncoderConfig.h"

#if !PLATFORM_MAC
#include "VideoEncoder.h"

AVEncoder::FVideoEncoder::H264Profile FEncoderConfig::GetAVEncoderProfile() const
{
	return AVEncoder::FVideoEncoder::H264Profile::BASELINE;
}

AVEncoder::FVideoEncoder::RateControlMode FEncoderConfig::GetRateControlMode() const
{
	return bConstantBitrate
		? AVEncoder::FVideoEncoder::RateControlMode::CBR
		: AVEncoder::FVideoEncoder::RateControlMode::VBR;
}

AVEncoder::FVideoEncoder::MultipassMode FEncoderConfig::GetMultipassMode() const
{
	return bMultipass
		? AVEncoder::FVideoEncoder::MultipassMode::FULL
		: AVEncoder::FVideoEncoder::MultipassMode::DISABLED;
}
#endif
