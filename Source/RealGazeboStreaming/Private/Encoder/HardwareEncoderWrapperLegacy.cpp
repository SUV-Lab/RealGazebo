// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Licensed under the GNU General Public License v3.0.

#if !PLATFORM_MAC
// Keep the current upstream Windows/Linux implementation isolated in its own
// translation unit so the macOS AVCodecs backend does not compile legacy
// AVEncoder/CUDA includes on Apple platforms.
#include "HardwareEncoderWrapper.upstream.inc"
#endif
