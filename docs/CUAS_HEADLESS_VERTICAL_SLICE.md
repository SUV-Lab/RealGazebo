# Headless UE 5.8 C-UAS vertical slice

## Goal

Run Unreal Engine 5.8 without a visible window while keeping full GPU rendering active. Vehicle motion is authoritative outside Unreal (Gazebo + ArduPilot SITL + MAVLink/bridge). Unreal mirrors vehicle/camera transforms and renders FPV cameras into GPU textures. Encoded video is produced once and fanned out to browser and STANAG 4609-compatible transports.

## Authority boundaries

- Gazebo / ArduPilot: vehicle dynamics, autopilot, swarm trajectory execution.
- RealGazeboBridge: transport authoritative pose/state into Unreal and manage vehicle lifecycle.
- Unreal Engine 5.8: photorealistic scene and sensor rendering only.
- RealGazeboStreaming: capture, GPU texture lifecycle, hardware encoding, encoded stream fan-out.
- C-Track / Furia: tactical tracks, identity, sensor/video association and operator UI.

## Initial trajectory source

For the first end-to-end demo, vehicles may use deterministic/random Lissajous references plus Boids-style separation/alignment/cohesion and collision avoidance. Unreal must not independently simulate those dynamics.

## Required video pipeline

```text
Gazebo + ArduPilot SITL
        |
      MAVLink / bridge pose
        v
Unreal actor + FPV SceneCaptureComponent2D
        |
        v
GPU render target / FRHITexture
        |
        | no ReadPixels(), no PNG/JPEG, no CPU frame staging
        v
hardware encoder
  macOS: Metal -> AVCodecs/VTCodecs -> VideoToolbox
  Linux: Vulkan/CUDA -> NVENC/AMF
  Windows: D3D/Vulkan -> NVENC/AMF
        |
        v
single encoded H.264 access-unit stream
        |
        +--> browser transport (prefer WebRTC for low latency)
        |
        +--> STANAG 4609 packaging (MPEG-TS + MISB KLV)
        |
        +--> optional RTSP compatibility / recording
```

The encoder output MUST be shared by transports. Browser and STANAG paths must not trigger independent renders or independent video encodes.

## Immediate architectural corrections

### 1. Rendering must not depend on RTSP viewers

Current `FStreamingPipeline::CaptureFrame()` skips capture when `FH264StreamSource` has no active RTSP client. That is valid as an RTSP optimization but invalid as the global render-demand policy: a STANAG recorder or WebRTC consumer may require frames while RTSP has no clients.

Replace the RTSP-specific gate with aggregate sink demand:

```text
ShouldCapture = AnyEncodedVideoSink.WantsFrames()
```

A continuous recorder/STANAG sink returns true. An RTSP sink may return false while idle.

### 2. Encoded video fan-out

`FStreamingPipeline::OnNALUnitsEncoded()` should publish to a collection of `IEncodedVideoSink` instances. `FH264StreamSource` becomes an RTSP sink adapter instead of being the only destination.

### 3. Metadata must travel beside encoded video

Each encoded access unit should have transport-neutral metadata available before packetization:

- monotonic frame number and timestamp
- WGS-84 latitude/longitude/altitude MSL
- platform roll/pitch/heading
- sensor-relative attitude
- horizontal/vertical FOV
- later: slant range, target location, track ID, sensor ID, security/classification fields as required

STANAG/MISB packetization maps this metadata to KLV. Browser transport can expose the same metadata over a synchronized data channel.

### 4. Headless means no visible window, not null rendering

Do not use Unreal modes that disable RHI/rendering. The process must keep Metal/Vulkan/D3D active and allow `SceneCaptureComponent2D::CaptureScene()` to render. Launch configuration must suppress the visible game/editor window while preserving GPU rendering.

## Vertical slice acceptance test

One vehicle is enough initially.

1. Start Gazebo + ArduPilot SITL.
2. Vehicle follows a scripted/Lissajous trajectory.
3. Bridge receives authoritative pose and updates exactly one Unreal vehicle actor.
4. FPV camera transform follows that actor.
5. UE 5.8 runs without a visible window and renders the FPV `SceneCaptureComponent2D`.
6. Frame remains GPU-resident through capture and hardware encode.
7. Browser receives live low-latency video.
8. The same encoded H.264 stream is packetized for a STANAG 4609-compatible output with synchronized metadata.
9. No RTSP client is required to keep the STANAG/browser path rendering.

## Scale-out after vertical slice

Only after the one-drone path is measured and stable:

- N vehicles/cameras
- render scheduling and per-camera FPS budgets
- encoder/session pooling limits
- demand-driven capture per sink
- C-Track track-to-video association
- CAT-129 / CAT-015 / CAT-016 / CAT-062 integration upstream of C-Track as appropriate
- end-to-end latency and GPU budget telemetry
