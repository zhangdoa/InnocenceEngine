# E2E Rendering Test Harness — Design Spec

## Goal

Extend `RenderTest.exe` to validate rendered pixel content, not just absence-of-crash.
A new test case renders a deterministic scene, reads back pixels from the GPU via DX12
readback heap, samples specific regions, and asserts expected color ranges. Failure exits
with code 2.

## Background

`RenderTest.exe` currently only validates that the engine boots, renders N frames, and
exits cleanly (exit code 0 = no D3D12 validation error, non-zero = failure). There is no
check that anything was actually drawn.

The DX12 readback implementation (`ReadTextureBackToCPU`) exists in
`DX12RenderingServer_EngineComponent_Public.cpp` but is commented out. The interface
method on `IRenderingServer` is declared and returns an empty vector. `ReadRenderTargetSample`
also exists but returns an empty Vec4 stub.

## Design

### Step 1: Restore DX12 ReadTextureBackToCPU

Uncomment and complete `DX12RenderingServer::ReadTextureBackToCPU`. The infrastructure
is all there:
- `CreateReadBackHeapBuffer` method exists on DX12RenderingServer
- `DX12DeviceMemory` exists with `m_DefaultHeapBuffer`; add `m_ReadBackHeapBuffer` field
- `ExecuteCommandListAndWait`, `GetGlobalCommandAllocator`, `GetGlobalCommandQueue` all exist

The function copies a TextureComponent's default-heap buffer to a readback heap buffer,
maps it to CPU, converts raw bytes to `std::vector<Vec4>`, and returns them.

Only Sampler2D + RGBA Float32 path needs to work for the test. Float16 path can stay
commented until needed.

### Step 2: New test case `pixel_readback`

`TestRenderingClient` gets a new `TestCase::PixelReadback`. The test:

1. Creates a 64×64 RGBA Float32 render target with a simple solid-color shader
2. Renders one frame with clear color (0, 0, 0, 1) + a draw call that fills the entire
   target with a solid known color (1, 0, 0, 1) — red
3. Calls `g_Engine->getRenderingServer()->ReadTextureBackToCPU(renderPass, renderTarget)`
4. Samples the center region (pixels 24,24 to 40,40) and asserts all channels within
   tolerance (±0.05 of expected)
5. On failure: logs the mismatch, calls `g_Engine->getWindowSystem()->Terminate()` and
   stores a failure flag
6. `Terminate()` returns `false` on validation failure → `RenderTest.exe` exits with
   code 2 (crash code, distinct from D3D12 validation error code 1)

### Shader: `pixelReadback.vert` / `pixelReadback.frag`

- Vertex shader: emit a full-screen triangle from 3 invocations of `gl_VertexID`
- Fragment shader: output `float4(1, 0, 0, 1)` (solid red)
- HLSL, compiled to DXIL via the existing HLSL2DXIL pipeline

### Region sampling strategy

After readback:
```
pixels = ReadTextureBackToCPU(pass, renderTarget)
// Width = 64, index = y * 64 + x
for y in [24, 40):
  for x in [24, 40):
    pixel = pixels[y * 64 + x]
    assert abs(pixel.x - 1.0) < 0.05  // red channel
    assert pixel.y < 0.05             // green channel
    assert pixel.z < 0.05             // blue channel
```

This is robust to minor precision differences and avoids edge pixel sensitivity.

### Exit code contract

`TestRenderingClient::Terminate()` sets `m_ValidationPassed` flag.
`RenderTest`'s main (or the existing WinMain harness) returns `2` on `!m_ValidationPassed`.
The existing exit-code-0 D3D12 validation error check is separate (exit code 1 from GPU
debug layer).

### Invocation

```
RenderTest.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -test pixel_readback
```

## What does NOT change

- Existing `draw_instanced` and `bareboot` test cases — untouched
- DX12RenderingServer concrete rendering logic
- IRenderingServer public interface (ReadTextureBackToCPU already declared there)

## Testing

The test is self-validating: a passing pixel readback returns exit code 0, a broken
rendering path returns exit code 2. CI runs both `draw_instanced` (smoke test) and
`pixel_readback` (pixel validation).
