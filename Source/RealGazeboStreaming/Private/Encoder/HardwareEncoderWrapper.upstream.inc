// Copyright (c) 2024-2025 SUV Lab, Chungbuk National University
// Author    : Gonapinuwala Lahiru Sandaruwan
// Supervisor: Prof. SungTae Moon - Project lead & research supervision
// Licensed under the GNU General Public License v3.0.
// See LICENSE file in the project root for full license information.

#include "Encoder/HardwareEncoderWrapper.h"
#include "VideoEncoder.h"
#include "VideoEncoderFactory.h"
#include "VideoEncoderCommon.h"
#include "VideoEncoderInput.h"
#include "CodecPacket.h"
#include "HAL/PlatformTime.h"
#include "Modules/ModuleManager.h"
#include "RHI.h"
#include "RHIResources.h"
#include "CudaModule.h"

// Vulkan RHI interface for both Windows and Linux
#if PLATFORM_LINUX || PLATFORM_WINDOWS
#include "IVulkanDynamicRHI.h"
#endif

#if PLATFORM_LINUX
#include <unistd.h>
#endif

// CUDA support for NVENC on Linux is included above (line 17)

//----------------------------------------------------------
// Construction & Initialization
//----------------------------------------------------------

FHardwareEncoderWrapper::FHardwareEncoderWrapper()
{
	UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Created"));
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

	UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Initializing..."));
	UE_LOG(LogTemp, Log, TEXT("  Config: %s"), *InConfig.ToString());

	// Validate configuration
	if (!InConfig.Validate(OutErrorMessage))
	{
		UE_LOG(LogTemp, Error, TEXT("HardwareEncoderWrapper: Invalid config - %s"), *OutErrorMessage);
		return false;
	}

	Config = InConfig;

	// Create hardware encoder (NVENC or AMF)
	if (!CreateHardwareEncoder(OutErrorMessage))
	{
		UE_LOG(LogTemp, Error, TEXT("HardwareEncoderWrapper: Failed to create encoder - %s"), *OutErrorMessage);
		return false;
	}

	// Register callbacks
	RegisterEncoderCallbacks();

	bInitialized = true;
	UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Initialized successfully (%s)"),
		*EncoderTypeToString(EncoderType));

	return true;
}

void FHardwareEncoderWrapper::Shutdown()
{
	if (!bInitialized)
	{
		return;
	}

	const uint64 FramesEncoded = TotalFramesEncoded.load();
	UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Shutting down... (Encoded %llu frames)"), FramesEncoded);
	bShuttingDown = true;

	// Flush input first to complete pending operations
	if (VideoEncoderInput)
	{
		VideoEncoderInput->Flush();
	}

	// WORKAROUND: UE5 NVENC bug - Shutdown() crashes if no frames were encoded
	// because internal buffers were never allocated
	if (FramesEncoded > 0)
	{
		// Frames were encoded - safe to shutdown normally
		FPlatformProcess::Sleep(0.05f);

		if (VideoEncoder)
		{
			VideoEncoder->Shutdown();
			VideoEncoder.Reset();
		}
		if (VideoEncoderInput)
		{
			VideoEncoderInput.Reset();
		}
	}
	else
	{
		// No frames encoded - NVENC internal buffers not allocated
		// Must prevent destructor from running (it calls Shutdown which crashes)
		// Store in static array to keep alive (intentional leak for stability)
		UE_LOG(LogTemp, Warning, TEXT("HardwareEncoderWrapper: No frames encoded - leaking NVENC to prevent crash"));
		static TArray<TUniquePtr<AVEncoder::FVideoEncoder>> LeakedEncoders;
		static TArray<TSharedPtr<AVEncoder::FVideoEncoderInput>> LeakedInputs;
		if (VideoEncoder)
		{
			LeakedEncoders.Add(MoveTemp(VideoEncoder));
		}
		if (VideoEncoderInput)
		{
			LeakedInputs.Add(VideoEncoderInput);
			VideoEncoderInput.Reset();
		}
	}

	// Clear CUDA texture cache (releases cached CUarrays)
	ClearCUDATextureCache();

	bInitialized = false;
	bShuttingDown = false;

	UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Shutdown complete (Encoded: %llu frames, %llu keyframes, %.2f MB)"),
		FramesEncoded,
		TotalKeyframesEncoded.load(),
		TotalBytesOutput.load() / (1024.0 * 1024.0));
}

//----------------------------------------------------------
// Internal Initialization
//----------------------------------------------------------

bool FHardwareEncoderWrapper::CreateHardwareEncoder(FString& OutErrorMessage)
{
	// Step 1: Create VideoEncoderInput based on RHI type
	if (!GDynamicRHI)
	{
		OutErrorMessage = TEXT("RHI not initialized");
		return false;
	}

	const ERHIInterfaceType RHIType = RHIGetInterfaceType();

#if PLATFORM_WINDOWS
	if (RHIType == ERHIInterfaceType::D3D11)
	{
		VideoEncoderInput = AVEncoder::FVideoEncoderInput::CreateForD3D11(
			GDynamicRHI->RHIGetNativeDevice(),
			true,  // IsResizable
			IsRHIDeviceAMD()  // IsShared (for AMD compatibility)
		);
	}
	else if (RHIType == ERHIInterfaceType::D3D12)
	{
		// IMPORTANT: On Windows D3D12 with NVIDIA GPUs, NVENC requires a D3D11 device
		// for encoding. This means we MUST use IsShared=true to create a D3D11 intermediate
		// device that NVENC can use.
		//
		// UE5.1 BUG WORKAROUND:
		// When IsShared=true, there's a bug where pooled input frames don't properly clear
		// D3D12.Texture before reuse, causing check(D3D12.Texture == nullptr) to fail.
		//
		// Solution: Use IsShared=true (required for NVENC) and set MaxNumBuffers=1 to
		// prevent frame pooling/reuse. Each encoding will create a new frame, avoiding
		// the assertion failure.
		VideoEncoderInput = AVEncoder::FVideoEncoderInput::CreateForD3D12(
			GDynamicRHI->RHIGetNativeDevice(),
			true,  // IsResizable
			true   // IsShared - MUST be true for NVENC (needs D3D11 device)
		);

		// CRITICAL: Set MaxNumBuffers=1 to prevent frame reuse bug
		// This avoids the check(D3D12.Texture == nullptr) assertion failure
		if (VideoEncoderInput.IsValid())
		{
			VideoEncoderInput->SetMaxNumBuffers(1);
			UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: D3D12 shared mode with MaxNumBuffers=1 (NVENC workaround)"));
		}
	}
	else if (RHIType == ERHIInterfaceType::Vulkan)
	{
		// Windows Vulkan support
		IVulkanDynamicRHI* VulkanRHI = GetIVulkanDynamicRHI();
		if (!VulkanRHI)
		{
			OutErrorMessage = TEXT("Failed to get Vulkan RHI interface on Windows");
			return false;
		}

		AVEncoder::FVulkanDataStruct VulkanData;
		VulkanData.VulkanInstance = VulkanRHI->RHIGetVkInstance();
		VulkanData.VulkanPhysicalDevice = VulkanRHI->RHIGetVkPhysicalDevice();
		VulkanData.VulkanDevice = VulkanRHI->RHIGetVkDevice();

		UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Windows Vulkan - Instance=%p, PhysicalDevice=%p, Device=%p"),
			VulkanData.VulkanInstance, VulkanData.VulkanPhysicalDevice, VulkanData.VulkanDevice);

		VideoEncoderInput = AVEncoder::FVideoEncoderInput::CreateForVulkan(
			&VulkanData,
			true  // IsResizable
		);
	}
	else
#elif PLATFORM_LINUX
	if (RHIType == ERHIInterfaceType::Vulkan)
	{
		// On Linux with NVIDIA GPU, NVENC doesn't support Vulkan textures directly.
		// We must use CUDA instead. Check if CUDA module is available.
		UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Linux Vulkan detected, checking for CUDA..."));

		FCUDAModule* CudaModule = FModuleManager::GetModulePtr<FCUDAModule>("CUDA");
		UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: CUDA module ptr = %p"), CudaModule);

		if (CudaModule)
		{
			const bool bCudaAvailable = CudaModule->IsAvailable();
			UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: CUDA IsAvailable() = %s"), bCudaAvailable ? TEXT("true") : TEXT("false"));

			if (bCudaAvailable)
			{
				// NVIDIA GPU detected - use CUDA for NVENC (NVENC supports CUDA_R8G8B8A8_UNORM)
				CUcontext LocalCudaContext = CudaModule->GetCudaContext();
				UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: CUDA context = %p"), LocalCudaContext);

				if (LocalCudaContext)
				{
					UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Using CUDA for NVENC encoding (CudaContext=%p)"), LocalCudaContext);

					VideoEncoderInput = AVEncoder::FVideoEncoderInput::CreateForCUDA(
						LocalCudaContext,
						true  // IsResizable
					);

					if (VideoEncoderInput.IsValid())
					{
						bUseCUDAMode = true;
						CudaContext = LocalCudaContext;
						UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: CUDA encoder input created successfully"));
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("HardwareEncoderWrapper: CreateForCUDA failed - falling back to Vulkan"));
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("HardwareEncoderWrapper: CUDA available but no context - falling back to Vulkan"));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("HardwareEncoderWrapper: CUDA module not loaded - using Vulkan"));
		}

		// Fallback to Vulkan if CUDA failed (for AMD GPUs which support Vulkan via AMF)
		if (!VideoEncoderInput.IsValid())
		{
			IVulkanDynamicRHI* VulkanRHI = GetIVulkanDynamicRHI();
			if (!VulkanRHI)
			{
				OutErrorMessage = TEXT("Failed to get Vulkan RHI interface");
				return false;
			}

			AVEncoder::FVulkanDataStruct VulkanData;
			VulkanData.VulkanInstance = VulkanRHI->RHIGetVkInstance();
			VulkanData.VulkanPhysicalDevice = VulkanRHI->RHIGetVkPhysicalDevice();
			VulkanData.VulkanDevice = VulkanRHI->RHIGetVkDevice();

			UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Using Vulkan for AMF encoding - Instance=%p, PhysicalDevice=%p, Device=%p"),
				VulkanData.VulkanInstance, VulkanData.VulkanPhysicalDevice, VulkanData.VulkanDevice);

			VideoEncoderInput = AVEncoder::FVideoEncoderInput::CreateForVulkan(
				&VulkanData,
				true  // IsResizable
			);
		}
	}
	else
#endif
	{
		OutErrorMessage = FString::Printf(TEXT("Unsupported RHI type for encoding: %d"), (int32)RHIType);
		return false;
	}

	if (!VideoEncoderInput.IsValid())
	{
		OutErrorMessage = TEXT("Failed to create VideoEncoderInput");
		return false;
	}

	// Step 2: Build FLayerConfig with ALL configuration settings
	AVEncoder::FVideoEncoder::FLayerConfig LayerConfig;
	LayerConfig.Width = Config.Width;
	LayerConfig.Height = Config.Height;
	LayerConfig.MaxFramerate = Config.FrameRate;
	LayerConfig.TargetBitrate = Config.Bitrate;
	LayerConfig.MaxBitrate = Config.Bitrate * 1.2;  // Allow 20% overshoot

	// Ultra-low latency settings
	LayerConfig.H264Profile = Config.GetAVEncoderProfile();
	LayerConfig.RateControlMode = Config.GetRateControlMode();
	LayerConfig.MultipassMode = Config.GetMultipassMode();

	// GOP and frame structure (critical for low latency!)
	LayerConfig.QPMin = -1;  // Let encoder decide
	LayerConfig.QPMax = -1;  // Let encoder decide

	// Step 3: Get available encoders and create the best match
	const TArray<AVEncoder::FVideoEncoderInfo>& AvailableEncoders =
		AVEncoder::FVideoEncoderFactory::Get().GetAvailable();

	if (AvailableEncoders.Num() == 0)
	{
		OutErrorMessage = TEXT("No video encoders available. Check HardwareEncoders plugin.");
		return false;
	}

	// Try to find and create H.264 encoder (prefer NVENC, fallback to AMF)
	for (const AVEncoder::FVideoEncoderInfo& EncoderInfo : AvailableEncoders)
	{
		if (EncoderInfo.CodecType == AVEncoder::ECodecType::H264)
		{
			// Try to create encoder
			VideoEncoder = AVEncoder::FVideoEncoderFactory::Get().Create(
				EncoderInfo.ID,
				VideoEncoderInput,
				LayerConfig
			);

			if (VideoEncoder.IsValid())
			{
				// Detect encoder type from ID
				if (EncoderInfo.ID == 0)  // NVENC usually ID 0
				{
					EncoderType = EEncoderType::NVENC;
					UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Created NVENC encoder"));
				}
				else if (EncoderInfo.ID == 1)  // AMF usually ID 1
				{
					EncoderType = EEncoderType::AMF;
					UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Created AMF encoder"));
				}
				else
				{
					EncoderType = EEncoderType::Unknown;
					UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Created H264 encoder (ID=%d)"), EncoderInfo.ID);
				}

				// Successfully created encoder - break loop
				break;
			}
		}
	}

	if (!VideoEncoder.IsValid())
	{
		OutErrorMessage = TEXT("No H264 hardware encoder available (NVENC or AMF). Check GPU drivers and HardwareEncoders plugin.");
		EncoderType = EEncoderType::Unknown;
		return false;
	}

	return true;
}

void FHardwareEncoderWrapper::RegisterEncoderCallbacks()
{
	if (!VideoEncoder.IsValid())
	{
		return;
	}

	// Register output callback - called when encoder produces data
	// UE 5.1 signature: void(uint32 LayerIndex, const TSharedPtr<FVideoEncoderInputFrame> Frame, const FCodecPacket& Packet)
	VideoEncoder->SetOnEncodedPacket([this](uint32 LayerIndex, const TSharedPtr<AVEncoder::FVideoEncoderInputFrame> Frame, const AVEncoder::FCodecPacket& Packet)
	{
		OnEncodedFrame(LayerIndex, Frame, Packet);
	});
}

//----------------------------------------------------------
// CUDA/Vulkan Interop (Linux, NVIDIA) - WITH CACHING
//----------------------------------------------------------

void FHardwareEncoderWrapper::ClearCUDATextureCache()
{
#if PLATFORM_LINUX
	FScopeLock Lock(&CUDACacheMutex);

	if (CUDATextureCache.Num() == 0)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Clearing CUDA texture cache (%d entries)"), CUDATextureCache.Num());

	if (CudaContext)
	{
		FCUDAModule::CUDA().cuCtxPushCurrent(CudaContext);

		for (auto& Pair : CUDATextureCache)
		{
			FCachedCUDATexture& Cached = Pair.Value;
			if (Cached.bValid)
			{
				// Destroy linear staging array first (created via cuArrayCreate)
				if (Cached.LinearArray)
				{
					FCUDAModule::CUDA().cuArrayDestroy(Cached.LinearArray);
				}
				// Destroy external memory resources (tiled array from Vulkan)
				if (Cached.MipArray)
				{
					FCUDAModule::CUDA().cuMipmappedArrayDestroy(Cached.MipArray);
				}
				if (Cached.ExternalMemory)
				{
					FCUDAModule::CUDA().cuDestroyExternalMemory(Cached.ExternalMemory);
				}
			}
		}

		FCUDAModule::CUDA().cuCtxPopCurrent(nullptr);
	}

	CUDATextureCache.Empty();
#endif
}

bool FHardwareEncoderWrapper::SetCUDATextureFromVulkan(FRHITexture* Texture,
	TSharedPtr<AVEncoder::FVideoEncoderInputFrame> InputFrame,
	FString& OutErrorMessage)
{
#if PLATFORM_LINUX
	if (!Texture || !InputFrame.IsValid())
	{
		OutErrorMessage = TEXT("Invalid texture or input frame");
		return false;
	}

	if (!CudaContext)
	{
		OutErrorMessage = TEXT("CUDA context not initialized");
		return false;
	}

	const int32 TexWidth = Texture->GetSizeX();
	const int32 TexHeight = Texture->GetSizeY();

	// Validate the layout this path actually depends on, before touching CUDA.
	// The import below hardcodes a single-mip, non-MSAA, 4-channel 8-bit 2D layout
	// (arrayDesc.NumChannels=4, CU_AD_FORMAT_UNSIGNED_INT8, numLevels=1, Depth=0) and a row
	// pitch of TexWidth*4. A mismatch would not fail loudly: cuMemcpy2D would copy the wrong
	// number of bytes per row and emit a silently corrupted frame instead of an error.
	// Note: FRHITextureDesc::IsTexture2D() also accepts Texture2DArray, which this path cannot
	// handle, so the dimension is compared directly.
	const FRHITextureDesc& TextureDesc = Texture->GetDesc();
	if (TextureDesc.Dimension != ETextureDimension::Texture2D
		|| TextureDesc.Format != PF_B8G8R8A8
		|| TextureDesc.NumMips != 1
		|| TextureDesc.NumSamples != 1)
	{
		OutErrorMessage = FString::Printf(
			TEXT("Unsupported texture for CUDA interop: %dx%d dimension=%d format=%s mips=%d samples=%d arraysize=%d (expected Texture2D / %s / 1 mip / no MSAA)"),
			TexWidth, TexHeight,
			(int32)TextureDesc.Dimension,
			GPixelFormats[TextureDesc.Format].Name,
			(int32)TextureDesc.NumMips,
			(int32)TextureDesc.NumSamples,
			(int32)TextureDesc.ArraySize,
			GPixelFormats[PF_B8G8R8A8].Name);

		// EncodeFrame runs per frame and its caller already logs OutErrorMessage on every
		// failure, so emit the detailed diagnostic only once to avoid flooding the log.
		if (!bLoggedUnsupportedTexture)
		{
			bLoggedUnsupportedTexture = true;
			UE_LOG(LogTemp, Error, TEXT("HardwareEncoderWrapper: %s"), *OutErrorMessage);
			UE_LOG(LogTemp, Error, TEXT("HardwareEncoderWrapper:   The CUDA/Vulkan zero-copy path requires the exact layout produced by FramePool (Create2D + PF_B8G8R8A8 + 1 mip + no MSAA)."));
			UE_LOG(LogTemp, Error, TEXT("HardwareEncoderWrapper:   Every frame using this texture will fail to encode. Repeat occurrences of this diagnostic are suppressed."));
		}
		return false;
	}

	FCUDAModule::CUDA().cuCtxPushCurrent(CudaContext);

	// Check cache first - if we have both TiledArray and LinearArray, just copy and return
	{
		FScopeLock Lock(&CUDACacheMutex);
		FCachedCUDATexture* Cached = CUDATextureCache.Find(Texture);
		if (Cached && Cached->bValid && Cached->TiledArray && Cached->LinearArray)
		{
			// CRITICAL FIX: Copy from tiled (Vulkan) array to linear (NVENC-compatible) array
			// This de-tiles the Vulkan optimal-tiled memory into linear layout
			CUDA_MEMCPY2D CopyParams = {};
			CopyParams.srcMemoryType = CU_MEMORYTYPE_ARRAY;
			CopyParams.srcArray = Cached->TiledArray;
			CopyParams.dstMemoryType = CU_MEMORYTYPE_ARRAY;
			CopyParams.dstArray = Cached->LinearArray;
			CopyParams.WidthInBytes = Cached->Width * 4;  // 4 bytes per pixel (BGRA)
			CopyParams.Height = Cached->Height;

			CUresult CuRes = FCUDAModule::CUDA().cuMemcpy2D(&CopyParams);
			FCUDAModule::CUDA().cuCtxPopCurrent(nullptr);

			if (CuRes != CUDA_SUCCESS)
			{
				OutErrorMessage = FString::Printf(TEXT("cuMemcpy2D (cached) failed (%d)"), (int32)CuRes);
				return false;
			}

			// Pass the LINEAR array to encoder (NOT the tiled one!)
			InputFrame->SetTexture(Cached->LinearArray, AVEncoder::FVideoEncoderInputFrame::EUnderlyingRHI::Vulkan, nullptr,
				[](CUarray) { /* No-op: Cache owns the resources */ });
			return true;
		}
	}

	// Not cached - need to import Vulkan texture AND create linear staging array
	IVulkanDynamicRHI* VulkanRHI = GetIVulkanDynamicRHI();
	if (!VulkanRHI)
	{
		FCUDAModule::CUDA().cuCtxPopCurrent(nullptr);
		OutErrorMessage = TEXT("Failed to get Vulkan RHI interface");
		return false;
	}

	const FVulkanRHIAllocationInfo AllocationInfo = VulkanRHI->RHIGetAllocationInfo(Texture);
	VkDevice Device = VulkanRHI->RHIGetVkDevice();

	PFN_vkGetMemoryFdKHR GetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)VulkanRHI->RHIGetVkDeviceProcAddr("vkGetMemoryFdKHR");
	if (!GetMemoryFdKHR)
	{
		FCUDAModule::CUDA().cuCtxPopCurrent(nullptr);
		OutErrorMessage = TEXT("vkGetMemoryFdKHR not available");
		return false;
	}

	int Fd = -1;
	VkMemoryGetFdInfoKHR MemoryGetFdInfoKHR{};
	MemoryGetFdInfoKHR.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
	MemoryGetFdInfoKHR.memory = AllocationInfo.Handle;
	MemoryGetFdInfoKHR.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
	MemoryGetFdInfoKHR.pNext = nullptr;

	VkResult VkRes = GetMemoryFdKHR(Device, &MemoryGetFdInfoKHR, &Fd);
	if (VkRes != VK_SUCCESS || Fd < 0)
	{
		FCUDAModule::CUDA().cuCtxPopCurrent(nullptr);
		OutErrorMessage = FString::Printf(TEXT("vkGetMemoryFdKHR failed (%d)"), (int32)VkRes);
		return false;
	}

	// === STEP 1: Import Vulkan texture as TILED CUarray (external memory) ===
	CUexternalMemory ExternalMemory = nullptr;
	CUmipmappedArray MipArray = nullptr;
	CUarray TiledArray = nullptr;

	CUDA_EXTERNAL_MEMORY_HANDLE_DESC ExtDesc{};
	ExtDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
	ExtDesc.handle.fd = Fd;
	ExtDesc.size = AllocationInfo.Offset + AllocationInfo.Size;

	CUresult CuRes = FCUDAModule::CUDA().cuImportExternalMemory(&ExternalMemory, &ExtDesc);
	if (CuRes != CUDA_SUCCESS)
	{
		FCUDAModule::CUDA().cuCtxPopCurrent(nullptr);
		close(Fd);
		OutErrorMessage = FString::Printf(TEXT("cuImportExternalMemory failed (%d)"), (int32)CuRes);
		return false;
	}
	// FD ownership transfers to CUDA - don't close it

	CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC MipDesc{};
	MipDesc.numLevels = 1;
	MipDesc.offset = AllocationInfo.Offset;
	MipDesc.arrayDesc.Width = TexWidth;
	MipDesc.arrayDesc.Height = TexHeight;
	MipDesc.arrayDesc.Depth = 0;
	MipDesc.arrayDesc.NumChannels = 4;
	MipDesc.arrayDesc.Format = CU_AD_FORMAT_UNSIGNED_INT8;
	MipDesc.arrayDesc.Flags = CUDA_ARRAY3D_SURFACE_LDST | CUDA_ARRAY3D_COLOR_ATTACHMENT;

	CuRes = FCUDAModule::CUDA().cuExternalMemoryGetMappedMipmappedArray(&MipArray, ExternalMemory, &MipDesc);
	if (CuRes != CUDA_SUCCESS)
	{
		FCUDAModule::CUDA().cuDestroyExternalMemory(ExternalMemory);
		FCUDAModule::CUDA().cuCtxPopCurrent(nullptr);
		OutErrorMessage = FString::Printf(TEXT("cuExternalMemoryGetMappedMipmappedArray failed (%d)"), (int32)CuRes);
		return false;
	}

	CuRes = FCUDAModule::CUDA().cuMipmappedArrayGetLevel(&TiledArray, MipArray, 0);
	if (CuRes != CUDA_SUCCESS)
	{
		FCUDAModule::CUDA().cuMipmappedArrayDestroy(MipArray);
		FCUDAModule::CUDA().cuDestroyExternalMemory(ExternalMemory);
		FCUDAModule::CUDA().cuCtxPopCurrent(nullptr);
		OutErrorMessage = FString::Printf(TEXT("cuMipmappedArrayGetLevel failed (%d)"), (int32)CuRes);
		return false;
	}

	// === STEP 2: Create LINEAR staging CUarray (NOT from external memory) ===
	// This array uses standard CUDA memory layout which NVENC can read correctly
	CUarray LinearArray = nullptr;
	CUDA_ARRAY_DESCRIPTOR LinearArrayDesc = {};
	LinearArrayDesc.Width = TexWidth;
	LinearArrayDesc.Height = TexHeight;
	LinearArrayDesc.Format = CU_AD_FORMAT_UNSIGNED_INT8;
	LinearArrayDesc.NumChannels = 4;

	CuRes = FCUDAModule::CUDA().cuArrayCreate(&LinearArray, &LinearArrayDesc);
	if (CuRes != CUDA_SUCCESS)
	{
		FCUDAModule::CUDA().cuMipmappedArrayDestroy(MipArray);
		FCUDAModule::CUDA().cuDestroyExternalMemory(ExternalMemory);
		FCUDAModule::CUDA().cuCtxPopCurrent(nullptr);
		OutErrorMessage = FString::Printf(TEXT("cuArrayCreate (linear staging) failed (%d)"), (int32)CuRes);
		return false;
	}

	// === STEP 3: Copy from tiled to linear (de-tiles the data) ===
	CUDA_MEMCPY2D CopyParams = {};
	CopyParams.srcMemoryType = CU_MEMORYTYPE_ARRAY;
	CopyParams.srcArray = TiledArray;
	CopyParams.dstMemoryType = CU_MEMORYTYPE_ARRAY;
	CopyParams.dstArray = LinearArray;
	CopyParams.WidthInBytes = TexWidth * 4;  // 4 bytes per pixel (BGRA)
	CopyParams.Height = TexHeight;

	CuRes = FCUDAModule::CUDA().cuMemcpy2D(&CopyParams);
	if (CuRes != CUDA_SUCCESS)
	{
		FCUDAModule::CUDA().cuArrayDestroy(LinearArray);
		FCUDAModule::CUDA().cuMipmappedArrayDestroy(MipArray);
		FCUDAModule::CUDA().cuDestroyExternalMemory(ExternalMemory);
		FCUDAModule::CUDA().cuCtxPopCurrent(nullptr);
		OutErrorMessage = FString::Printf(TEXT("cuMemcpy2D (initial) failed (%d)"), (int32)CuRes);
		return false;
	}

	FCUDAModule::CUDA().cuCtxPopCurrent(nullptr);

	// Cache both the tiled (external) and linear (staging) arrays
	{
		FScopeLock Lock(&CUDACacheMutex);
		FCachedCUDATexture& Cached = CUDATextureCache.Add(Texture);
		Cached.ExternalMemory = ExternalMemory;
		Cached.MipArray = MipArray;
		Cached.TiledArray = TiledArray;
		Cached.LinearArray = LinearArray;
		Cached.Width = TexWidth;
		Cached.Height = TexHeight;
		Cached.bValid = true;
	}

	UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Created CUDA linear staging array %dx%d format=%s (Total cached: %d)"),
		TexWidth, TexHeight, GPixelFormats[TextureDesc.Format].Name, CUDATextureCache.Num());

	// Pass the LINEAR array to encoder (NOT the tiled one!)
	InputFrame->SetTexture(LinearArray, AVEncoder::FVideoEncoderInputFrame::EUnderlyingRHI::Vulkan, nullptr,
		[](CUarray) { /* No-op: Cache owns the resources, will be cleaned up in ClearCUDATextureCache() */ });

	return true;
#else
	OutErrorMessage = TEXT("CUDA interop only supported on Linux");
	return false;
#endif
}

//----------------------------------------------------------
// Encoding Operations
//----------------------------------------------------------

bool FHardwareEncoderWrapper::EncodeFrame(FRHITexture* Texture, uint64 FrameNumber, bool bForceKeyframe)
{
	if (!bInitialized || bShuttingDown || !VideoEncoder.IsValid() || !VideoEncoderInput.IsValid())
	{
		EncodingFailureCount++;
		return false;
	}

	if (!Texture || !Texture->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("HardwareEncoderWrapper: Invalid texture"));
		EncodingFailureCount++;
		return false;
	}

	// Get RHI type once (used for D3D12 workaround and texture setting)
	const ERHIInterfaceType RHIType = RHIGetInterfaceType();

	// WINDOWS D3D12 WORKAROUND:
	// UE5.1 has a bug where pooled input frames don't clear D3D12.Texture on reuse,
	// causing check(D3D12.Texture == nullptr) assertion failure at VideoEncoderInput.cpp:879.
	// Workaround: Flush the input pool before obtaining a frame to force creation of new frames.
	// This is less efficient but necessary for correctness on Windows D3D12 + NVENC.
#if PLATFORM_WINDOWS
	if (RHIType == ERHIInterfaceType::D3D12)
	{
		// Flush to discard any pooled frames that have stale D3D12.Texture pointers
		VideoEncoderInput->Flush();
	}
#endif

	// Obtain input frame from VideoEncoderInput
	TSharedPtr<AVEncoder::FVideoEncoderInputFrame> InputFrame = VideoEncoderInput->ObtainInputFrame();
	if (!InputFrame.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("HardwareEncoderWrapper: Failed to obtain input frame"));
		EncodingFailureCount++;
		return false;
	}

	// Return frame to encoder input on failure paths before Encode()
	auto ReleaseInputFrame = [&InputFrame]()
	{
		if (InputFrame.IsValid())
		{
			InputFrame->Release();
			InputFrame.Reset();
		}
	};

	// Set frame dimensions
	InputFrame->SetWidth(Config.Width);
	InputFrame->SetHeight(Config.Height);

	// Set timestamp and frame ID
	InputFrame->SetTimestampUs(FPlatformTime::Seconds() * 1000000.0);
	InputFrame->SetFrameID(FrameNumber);

	// Set texture based on RHI type and encoding mode
	// Note: RHIType already declared above for D3D12 workaround
	bool bTextureSet = false;
#if PLATFORM_WINDOWS
	if (RHIType == ERHIInterfaceType::D3D11)
	{
		ID3D11Texture2D* D3D11Texture = static_cast<ID3D11Texture2D*>(Texture->GetNativeResource());
		InputFrame->SetTexture(D3D11Texture, [](ID3D11Texture2D*){ /* Texture release callback */ });
		bTextureSet = true;
	}
	else if (RHIType == ERHIInterfaceType::D3D12)
	{
		ID3D12Resource* D3D12Resource = static_cast<ID3D12Resource*>(Texture->GetNativeResource());
		InputFrame->SetTexture(D3D12Resource, [](ID3D12Resource*){ /* Texture release callback */ });
		bTextureSet = true;
	}
	else if (RHIType == ERHIInterfaceType::Vulkan)
	{
		// Windows Vulkan texture handling
		VkImage VulkanImage = static_cast<VkImage>(Texture->GetNativeResource());
		InputFrame->SetTexture(VulkanImage, [](VkImage){ /* Texture release callback */ });
		bTextureSet = true;
	}
#elif PLATFORM_LINUX
	if (RHIType == ERHIInterfaceType::Vulkan)
	{
		if (bUseCUDAMode)
		{
			FString InteropError;
			if (!SetCUDATextureFromVulkan(Texture, InputFrame, InteropError))
			{
				UE_LOG(LogTemp, Error, TEXT("HardwareEncoderWrapper: CUDA/Vulkan interop failed: %s"), *InteropError);
				EncodingFailureCount++;
				ReleaseInputFrame();
				return false;
			}
			bTextureSet = true;
		}
		else
		{
			// AMF mode: Direct Vulkan path
			VkImage VulkanImage = static_cast<VkImage>(Texture->GetNativeResource());
			InputFrame->SetTexture(VulkanImage, [](VkImage){ /* Texture release callback */ });
			bTextureSet = true;
		}
	}
#endif

	if (!bTextureSet)
	{
		UE_LOG(LogTemp, Warning, TEXT("HardwareEncoderWrapper: Unsupported RHI for SetTexture: %d"), (int32)RHIType);
		EncodingFailureCount++;
		ReleaseInputFrame();
		return false;
	}

	// Prepare encode options
	AVEncoder::FVideoEncoder::FEncodeOptions EncodeOptions;
	EncodeOptions.bForceKeyFrame = bForceKeyframe || bForceNextKeyframe.exchange(false);

	// CRITICAL: Final shutdown check right before Encode()
	// Shutdown may have started between the initial check and here
	if (bShuttingDown.load() || !VideoEncoder.IsValid())
	{
		ReleaseInputFrame();
		return false;
	}

	// Submit frame to encoder
	VideoEncoder->Encode(InputFrame, EncodeOptions);

	CurrentFrameNumber = FrameNumber;
	LastEncodeTime = FPlatformTime::Seconds();
	TotalFramesEncoded++;

	return true;
}

bool FHardwareEncoderWrapper::GetEncodedData(TArray<FEncodedNALUnit>& OutNALUnits)
{
	FScopeLock Lock(&NALQueueMutex);

	OutNALUnits.Empty();

	// Drain all available NAL units from queue
	FEncodedNALUnit NALUnit;
	while (NALOutputQueue.Dequeue(NALUnit))
	{
		// CRITICAL: Validate NAL data integrity after dequeue
		// Detects memory corruption from encoder callback issues
		const uint8* DataPtr = NALUnit.Data.GetData();
		const int32 DataSize = NALUnit.Data.Num();

		if (DataSize > 0 && (DataPtr == nullptr || reinterpret_cast<uintptr_t>(DataPtr) == ~static_cast<uintptr_t>(0)))
		{
			UE_LOG(LogTemp, Error, TEXT("HardwareEncoderWrapper: Corrupted NAL from queue (Ptr=0x%p, Size=%d) - encoder callback may have produced invalid data"),
				DataPtr, DataSize);
			continue; // Skip corrupted NAL
		}

		OutNALUnits.Add(MoveTemp(NALUnit));
	}

	return OutNALUnits.Num() > 0;
}

void FHardwareEncoderWrapper::ForceKeyframe()
{
	bForceNextKeyframe = true;
	UE_LOG(LogTemp, Log, TEXT("HardwareEncoderWrapper: Next frame will be keyframe"));
}

//----------------------------------------------------------
// Encoder Callbacks
//----------------------------------------------------------

void FHardwareEncoderWrapper::OnEncodedFrame(uint32 LayerIndex, const TSharedPtr<AVEncoder::FVideoEncoderInputFrame> Frame, const AVEncoder::FCodecPacket& Packet)
{
	// CRITICAL: Check shutdown flag first - encoder may call this callback during shutdown
	if (bShuttingDown.load())
	{
		return;
	}

	if (!Frame.IsValid())
	{
		return;
	}

	// Check if we have data
	if (Packet.DataSize == 0 || !Packet.Data.IsValid())
	{
		return;
	}

	// CRITICAL: Validate actual data pointer before use
	// During shutdown, the pointer may be invalid (0xFFFFFFFFFFFFFFFF)
	const uint8* PacketDataPtr = Packet.Data.Get();
	if (PacketDataPtr == nullptr || reinterpret_cast<uintptr_t>(PacketDataPtr) == ~static_cast<uintptr_t>(0))
	{
		UE_LOG(LogTemp, Warning, TEXT("HardwareEncoderWrapper: Invalid packet data pointer (0x%p) - skipping"), PacketDataPtr);
		return;
	}

	// Double-check shutdown flag after validation (may have changed)
	if (bShuttingDown.load())
	{
		return;
	}

	// Update statistics
	TotalBytesOutput += Packet.DataSize;
	if (Packet.IsKeyFrame)
	{
		TotalKeyframesEncoded++;
	}

	// Parse bitstream into NAL units
	TArray<FEncodedNALUnit> NALUnits;
	ParseNALUnits(PacketDataPtr, Packet.DataSize, NALUnits);

	// Check shutdown again after parsing (may have started during parse)
	if (bShuttingDown.load())
	{
		return;
	}

	// Enqueue NAL units for retrieval
	FScopeLock Lock(&NALQueueMutex);
	for (FEncodedNALUnit& NALUnit : NALUnits)
	{
		// CRITICAL: Validate NAL data before enqueue
		// If corrupted here, the problem is in ParseNALUnits or encoder output
		const uint8* DataPtr = NALUnit.Data.GetData();
		const int32 DataSize = NALUnit.Data.Num();

		if (DataSize > 0 && (DataPtr == nullptr || reinterpret_cast<uintptr_t>(DataPtr) == ~static_cast<uintptr_t>(0)))
		{
			UE_LOG(LogTemp, Error, TEXT("HardwareEncoderWrapper: ParseNALUnits produced corrupted NAL (Ptr=0x%p, Size=%d) - encoder output may be invalid"),
				DataPtr, DataSize);
			continue;
		}

		NALUnit.TimestampMs = Frame->GetTimestampUs() / 1000;
		NALUnit.FrameNumber = CurrentFrameNumber;
		NALUnit.bIsKeyframe = Packet.IsKeyFrame;

		NALOutputQueue.Enqueue(MoveTemp(NALUnit));
	}

	// Release the input frame
	Frame->Release();
}

//----------------------------------------------------------
// NAL Unit Processing
//----------------------------------------------------------

void FHardwareEncoderWrapper::ParseNALUnits(const uint8* BitstreamData, uint32 BitstreamSize, TArray<FEncodedNALUnit>& OutNALUnits)
{
	OutNALUnits.Empty();

	// H.264 NAL units are separated by start codes: 0x00 0x00 0x00 0x01 (4 bytes)
	// or 0x00 0x00 0x01 (3 bytes)

	uint32 Index = 0;

	while (Index < BitstreamSize - 4)
	{
		// Find start code
		bool bFoundStartCode = false;
		uint32 StartCodeSize = 0;

		// Check for 4-byte start code (0x00 0x00 0x00 0x01)
		if (BitstreamData[Index] == 0x00 && BitstreamData[Index + 1] == 0x00 &&
			BitstreamData[Index + 2] == 0x00 && BitstreamData[Index + 3] == 0x01)
		{
			bFoundStartCode = true;
			StartCodeSize = 4;
		}
		// Check for 3-byte start code (0x00 0x00 0x01)
		else if (BitstreamData[Index] == 0x00 && BitstreamData[Index + 1] == 0x00 &&
				 BitstreamData[Index + 2] == 0x01)
		{
			bFoundStartCode = true;
			StartCodeSize = 3;
		}

		if (!bFoundStartCode)
		{
			Index++;
			continue;
		}

		// Find next start code to determine NAL unit size
		uint32 NextStartCode = Index + StartCodeSize;
		while (NextStartCode < BitstreamSize - 4)
		{
			if ((BitstreamData[NextStartCode] == 0x00 && BitstreamData[NextStartCode + 1] == 0x00 &&
				 BitstreamData[NextStartCode + 2] == 0x00 && BitstreamData[NextStartCode + 3] == 0x01) ||
				(BitstreamData[NextStartCode] == 0x00 && BitstreamData[NextStartCode + 1] == 0x00 &&
				 BitstreamData[NextStartCode + 2] == 0x01))
			{
				break; // Found next start code
			}
			NextStartCode++;
		}

		// If we didn't find another start code, this is the last NAL unit
		if (NextStartCode >= BitstreamSize - 4)
		{
			NextStartCode = BitstreamSize;
		}

		// Extract NAL unit
		const uint32 NALSize = NextStartCode - Index;
		if (NALSize > StartCodeSize)
		{
			FEncodedNALUnit NALUnit;
			NALUnit.Data.Append(&BitstreamData[Index], NALSize);
			NALUnit.NALType = ExtractNALType(&BitstreamData[Index + StartCodeSize], NALSize - StartCodeSize);

			OutNALUnits.Add(MoveTemp(NALUnit));
		}

		Index = NextStartCode;
	}
}

uint8 FHardwareEncoderWrapper::ExtractNALType(const uint8* Data, uint32 Size) const
{
	if (!Data || Size < 1)
	{
		return 0;
	}

	// NAL unit type is in lower 5 bits of first byte
	// Byte format: forbidden_zero_bit(1) + nal_ref_idc(2) + nal_unit_type(5)
	return Data[0] & 0x1F;
}

//----------------------------------------------------------
// Statistics
//----------------------------------------------------------

FString FHardwareEncoderWrapper::GetStatsString() const
{
	const double CurrentTime = FPlatformTime::Seconds();
	const double TimeSinceLastEncode = CurrentTime - LastEncodeTime;

	return FString::Printf(
		TEXT("HardwareEncoder (%s): Encoded=%llu frames (%llu keyframes), Output=%.2f MB, Failures=%llu, LastEncode=%.3fs ago"),
		*EncoderTypeToString(EncoderType),
		TotalFramesEncoded.load(),
		TotalKeyframesEncoded.load(),
		TotalBytesOutput.load() / (1024.0 * 1024.0),
		EncodingFailureCount.load(),
		TimeSinceLastEncode
	);
}
