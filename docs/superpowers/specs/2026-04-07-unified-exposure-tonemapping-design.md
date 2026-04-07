# Unified Exposure & Tone Mapping Design

## Goal

Eliminate the separate path tracer tone mapping pass and route both rasterized and path tracer HDR output through the same auto-exposure and tone mapping pipeline, so that visual comparison between the two modes is meaningful.

## Architecture

The post-processing tail is shared. The only variable is the HDR source texture:

```
Rasterized: LightPass -> PreTAA(+Sky) -> TAA -> HDR source
Path Tracer: AccumBuffer -----------------------> HDR source

HDR source -> LuminanceHistogram -> LuminanceAverage -> FinalBlend -> SwapChain
```

When the path tracer is active, the rasterized passes (opaque, light, radiance cache, sky, PreTAA, TAA) are skipped entirely. The post-processing tail (histogram, average, finalBlend) always runs.

## Changes

### 1. Remove GPUPathTracerToneMap pass

Delete the dedicated tone mapping pass and its resources:

- **Delete shader:** `Source/Shaders/HLSL/GPUPathTracerToneMap.comp`
- **Delete compiled shader:** `Bin/Shaders/DXIL/GPUPathTracerToneMap.comp.dxil`
- **Remove from GPUPathTracerPass:** Delete `m_ToneMapRenderPassComp`, `m_ToneMapSPC`, `m_ToneMapCommandList`, `m_ToneMapOutput` members. Remove their Setup/Initialize/Terminate/PrepareCommandList code. Remove the Graphics CL that transitions `m_ToneMapOutput`.
- **Update GetResult():** Return `m_AccumulationBuffer` (raw HDR) instead of `m_ToneMapOutput`.

### 2. Route HDR source in PrepareCommands

In `DefaultRenderingClientImpl::PrepareCommands()`, the path tracer branch currently returns early after setting `m_Canvas`. Change it to:

1. Prepare the path tracer ray tracing command list (unchanged).
2. Do NOT set `m_Canvas` or return early.
3. Fall through to the post-processing section.
4. Guard the rasterized passes (opaque through TAA) with `if (!m_GPUPathTracerActive)`.
5. Select the HDR source for histogram and finalBlend:
   - Rasterized: TAA result (unchanged)
   - Path tracer: `GPUPathTracerPass::Get().GetResult()` (now the raw accumulation buffer)

### 3. Route HDR source in ExecuteCommands

In `DefaultRenderingClientImpl::ExecuteCommands()`:

1. Guard the rasterized GPU execution blocks (opaque through TAA) with the path tracer check.
2. When path tracer is active, execute only the path tracer ray tracing CL, then the post-processing CLs (histogram, average, finalBlend).
3. The path tracer's Graphics CL for resource transitions still runs (barriers for AccumulationBuffer).
4. Remove the ToneMap CL execution block entirely.

### 4. Canvas assignment

`m_Canvas` and `m_CanvasOwner` always point to `FinalBlendPass::Get().GetResult()` and its render pass component, for both paths. Remove the path-tracer-specific canvas override.

### 5. Skip rasterized passes when path tracer is active

The following passes are skipped in both PrepareCommands and ExecuteCommands when `m_GPUPathTracerActive` is true:

- BRDFLUTPass, BRDFLUTMSPass (one-shot, already guarded)
- SunShadow passes
- OpaquePass
- RadianceCache passes (all 5)
- SSAOPass
- LightCullingPass, LightPass
- SkyPass
- PreTAAPass, TAAPass

The following always run regardless of mode:

- LuminanceHistogramPass (with routed input)
- LuminanceAveragePass
- FinalBlendPass (with routed input)

### 6. Billboard and debug overlays

FinalBlendPass composites billboard and debug overlays. These are rasterized-only features. When the path tracer is active, their textures will be black (no geometry rendered), so the compositing is a no-op. No code change needed.

## Tone mapping details

Both paths use the same pipeline in FinalBlendPass:

1. Read adapted average luminance from histogram buffer
2. Compute exposure: `1.0 / (avgLuminance * 9.6)`
3. Convert HDR to xyY, scale Y by exposure, convert back to RGB
4. Apply ACES filmic tone mapping
5. Apply accurate linear-to-sRGB gamma correction

The ACES curve in Tonemapping.hlsl (`TonemapACES`) and the one that was in GPUPathTracerToneMap.comp (`ACESFilmic`) use identical constants. No curve change.

## Testing

- **RenderTest:** Regression test, no path tracer involvement. Should pass unchanged.
- **Main.exe integration (10 frames):** Rasterized pipeline, verifies histogram/average/finalBlend still work.
- **Interactive toggle_pathtracer:** Pressing B should show the path tracer output with auto-exposure applied. No white blowout. Toggling back should show the rasterized output with the same exposure adaptation.
- **Interactive full:** Camera movement + path tracer toggle + scene reload. No crashes.

## Files touched

| File | Action |
|------|--------|
| `Source/Shaders/HLSL/GPUPathTracerToneMap.comp` | Delete |
| `Source/DefaultClient/RenderingClient/GPUPathTracerPass.h` | Remove ToneMap members |
| `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp` | Remove ToneMap setup/init/prepare/terminate, update GetResult |
| `Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp` | Route HDR source, guard rasterized passes, remove early return |
