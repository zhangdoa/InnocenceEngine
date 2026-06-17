---
id: TASK-251
title: >-
  GPUModelData m_BoundingBoxMin/Max: C++/HLSL std140 layout mismatch (offset 56 vs 64)
status: To Do
assignee:
  - code-impl
created_date: '2026-06-17'
labels:
  - rendering
  - bug
  - gpu-data
  - std140
  - follow-up
dependencies:
  - TASK-249
priority: medium
---

## Description

Surfaced by TASK-249 CL 4 (commit `75566da2`): the `static_assert`
on `sizeof(GPUModelData) == 160` forced an explicit investigation
of the actual C++ struct layout, which revealed a C++-HLSL
std140 mismatch that has been silently wrong since the struct
was authored.

### The defect

`Source/Engine/Common/GPUDataStructure.h:265-289` (C++):

```cpp
struct alignas(16) GPUModelData
{
    uint64_t m_VertexBufferAddress = 0;     // 0
    uint64_t m_IndexBufferAddress = 0;      // 8
    uint32_t m_VertexCount = 0;             // 16
    uint32_t m_IndexCount = 0;              // 20
    uint32_t m_VertexStride = 0;            // 24
    uint32_t m_IndexStride = 0;             // 28
    uint32_t m_MaterialIndex = 0;           // 32
    uint32_t m_ShaderProgramIndex = 0;      // 36
    float m_UUID = 0.0f;                    // 40
    uint32_t m_RenderPassIndex = 0;        // 44
    uint32_t m_VisibilityMask = 0;         // 48
    uint32_t m_MeshUsage = 0;              // 52
    Vec4 m_BoundingBoxMin;                  // 56   <-- C++ packs at 4B
    Vec4 m_BoundingBoxMax;                  // 72
    uint32_t m_InstanceCount = 1;           // 88
    uint32_t m_FirstInstance = 0;           // 92
    float padding[16];                      // 96
};
// sizeof == 160
```

`Source/Shaders/HLSL/common/common.hlsl:529-555` (HLSL):

```hlsl
struct GPUModelData
{
    ...
    uint m_VisibilityMask;                  // 48
    uint m_MeshUsage;                       // 52
    float4 m_BoundingBoxMin;                // 64  <-- HLSL lays out at 16B
    float4 m_BoundingBoxMax;                // 80
    ...
};
```

The HLSL cbuffer rule for `float4` inside a cbuffer is 16B aligned.
The C++ struct's `alignas(16)` forces the *struct's* alignment to
16, but does **not** propagate into the member layout — C++ packs
`Vec4` (alignof 4) at the next 4B boundary, which is 56 here.

### Consequence

The HLSL side reads bytes [56..72) as `m_BoundingBoxMin`, but it
expects to read bytes [64..80) per the HLSL layout. The C++ side
writes `m_BoundingBoxMin` to [56..72) and zeros the natural
4B-aligned `m_BoundingBoxMax` at [72..88) (which HLSL reads as the
upper 12B of `m_BoundingBoxMin` + first 4B of `m_BoundingBoxMax`).
The HLSL's true `m_BoundingBoxMin` field reads from the C++ bytes
[64..72) + [80..88) — the lower 8B of C++'s `m_BoundingBoxMin`
followed by the first 4B of C++'s `m_BoundingBoxMax`.

In practice: the C++ producer sets `ws.w = 1.0f` (line 201 of
`DrawCallService.cpp`), but the HLSL's `m_BoundingBoxMin.w` reads
the C++ bytes at offset 64-67, which is the SECOND float of the
C++ `m_BoundingBoxMin.y` (because [56,72) is 4 floats and the
HLSL offset 64 lands 8 bytes into that range). The `w=1.0f` is
silently dropped.

For a typical Sponza model: the C++ `m_BoundingBoxMin.y` is some
value in the scene's vertical range, the HLSL `m_BoundingBoxMin.w`
reads that, not 1.0f. Effects depend on how the shader uses
`.w` of the bounding box (perspective divide, normalization, etc).

### Reproduction

`offsetof(GPUModelData, m_BoundingBoxMin) == 56` in C++.
`GPUModelData::m_BoundingBoxMin` in HLSL is at cbuffer offset 64.
The 8-byte delta is what TASK-249 CL 4 surfaced via the
`static_assert` chain.

### Acceptance Criteria

- [ ] `GPUModelData::m_BoundingBoxMin` is at cbuffer offset 64 in
      both C++ and HLSL (matching `alignas(16)` semantics inside
      a cbuffer).
- [ ] Option A (preferred): declare `Vec4` (or a 16B-aligned
      equivalent) so C++ packs at 16B boundaries; HLSL unchanged.
- [ ] Option B: rearrange the C++ field order so the HLSL and
      C++ offsets agree.
- [ ] `static_assert` on `sizeof(GPUModelData) == 176` (the HLSL
      cbuffer size, post-fix).
- [ ] Live-engine smoke: HLSL `m_BoundingBoxMin.w` reads 1.0f
      in the producer's expected path; GPU test confirms
      shader uses the correct value.
- [ ] Test: a shader-side test that reads `m_BoundingBoxMin.w`
      from a known CB and asserts the expected value.

### Investigation hints

- `Source/Engine/Common/GraphicsPrimitive.h` defines `Vec4`; if
  it's `alignas(4)`, the migration requires either changing
  `Vec4` (touches many files) or introducing a 16B-aligned
  variant.
- The HLSL rule for cbuffer float4 alignment is per-cbuffer, not
  per-struct. The equivalent in C++ is to either (a) reorder so
  no 4B/8B fields precede the float4, or (b) force the float4
  to 16B alignment.
- A minimal-impact fix: swap `m_MeshUsage` (uint32) and
  `m_VisibilityMask` (uint32) to be 8B each (uint64 with high
  bits zero), pushing `m_BoundingBoxMin` to offset 64. But this
  bloats the struct to 16B+; consider a `alignas(16)` member
  decoration or a packed-then-aligned wrapper.

### Related

- TASK-249.A (m_ShaderProgramIndex / m_RenderPassIndex not
  populated; separate but same struct).
- TASK-243 (GBuffer-black regression; same class of silent-wrong
  CPU/GPU mismatch).

## Session log

- **2026-06-17**: filed by TASK-249 CL 4 (`75566da2`). The
  `static_assert` on `sizeof(GPUModelData) == 160` (the actual
  C++ size) flagged that the C++ struct's `m_BoundingBoxMin` is
  at offset 56, not the 64 the HLSL cbuffer expects. A
  temporary probe in `GPUUploadableTests_Standalone.exe`
  (since removed) confirmed the actual layout. Migration
  preserves the bug; the real fix is pending.
